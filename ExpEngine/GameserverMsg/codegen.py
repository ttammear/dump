import xml.etree.ElementTree as ET
import sys
import shutil
import re
from io import StringIO as sio

xmlRoot = ET.parse("game_server_msg.xml").getroot()

serverMessages = {}
serverMessages["msgs"] = []

msgIds = set([])
msgNames = set([])

msgs = []

type_max_value = {
    "uint8_t": 255,
    "uint16_t": 65535,
    "uint32_t": 4294967295,
}

type_parse_func_name = {
    "uint8_t": "stream_read_uint8",
    "uint16_t": "stream_read_uint16",
    "uint32_t": "stream_read_uint32",
    "V3": "stream_read_v3",
    "V2": "stream_read_v2",
    "Quat": "stream_read_quat",
}

type_write_func_name = {
    "uint8_t": "stream_write_uint8",
    "uint16_t": "stream_write_uint16",
    "uint32_t": "stream_write_uint32",
    "V3": "stream_write_v3",
    "V2": "stream_write_v2",
    "Quat": "stream_write_quat",
}


def parse_data_fields(rootEl, fieldList):
    for childEl in rootEl:
        field = {}
        if childEl.tag == "DataArray":
            field["Type"] = 'array' 
            nameEl = childEl.find("Name")
            if nameEl is None:
                sys.exit("DataArray must have Name")
            field["Name"] = nameEl.text
            maxLenEl = childEl.find("MaxLen")
            countEl = childEl.find("CountField")
            if countEl is None:
                sys.exit("DataArray must have CountField")
            field["CountField"] = {}
            if countEl.find("Type") is None:
                sys.exit("DataArray.CountField must have Type")
            field["CountField"]["Type"] = countEl.find("Type").text
            if countEl.find("Name") is None:
                sys.exit("DataArray.CountField must have Name")
            field["CountField"]["Name"] = countEl.find("Name").text
            if maxLenEl is None:
                maxLen = type_max_value[field["CountField"]["Type"]]
                print("Warning: DataArray max length not specified, assuming " + str(maxLen) + " (from count type)")
                field["MaxLen"] = maxLen
            else:
                field["MaxLen"] = int(maxLenEl.text)

            field["fields"] = []
            parse_data_fields(childEl, field["fields"])
            fieldList.append(field)
        elif childEl.tag == "DataField":
            typeEl = childEl.find("Type")
            if typeEl is None:
                sys.exit("DataField must have Type")
            nameEl = childEl.find("Name")
            if nameEl is None:
                sys.exit("DataField must have Name")
            field["Type"] = typeEl.text
            # TODO: check that name is unique
            field["Name"] = nameEl.text
            fieldList.append(field)

for msgEl in xmlRoot.findall('Msg'):
    msg = {}
    nameEl = msgEl.find('Name')
    if nameEl is None:
        sys.exit("Msg element missing a name!")
    else:
        msg["Name"] = nameEl.text
        if nameEl.text in msgNames:
            sys.exit("Msg with duplicate name not allowed")
        msgNames.add(nameEl.text)
    msgIdEl = msgEl.find('MsgId')
    if msgIdEl is None:
        sys.exit("Msg must have MsgId")
    else:
        msgId = int(msgIdEl.text)
        if msgId in msgIds:
            sys.exit("Msg with duplicate ids not allowd")
        if msgId > 255:
            sys.exit("Msg id must be in range 0-255")
        msgIds.add(msgId)
        msg["MsgId"] = msgId
    toClientEl = msgEl.find('ToClient')
    toServerEl = msgEl.find('ToServer')
    if toClientEl is None:
        msg["ToClient"] = False
    else:
        msg["ToClient"] = toClientEl.text == "1"
    if toServerEl is None:
        msg["ToServer"] = False
    else:
        msg["ToServer"] = toServerEl.text == "1"

    if msg["ToServer"] == False and msg["ToClient"] == False:
        sys.exit("Msg element must have at least 'ToClient' or 'ToServer' set to 1")
    fields = []
    parse_data_fields(msgEl, fields)
    msg["fields"] = fields
    msgs.append(msg)

def gen_struct_field(outs, field, level):
    if field["Type"] == "array":
        outs.write("    "*level)
        outs.write(field["CountField"]["Type"])
        outs.write(" ")
        outs.write(field["CountField"]["Name"])
        outs.write(";\r\n")
        outs.write("    "*level)
        outs.write("struct {\r\n")

        for subfield in field["fields"]:
            gen_struct_field(outs, subfield, level+1)

        outs.write("    "*level)
        outs.write("} ")
        outs.write(field["Name"])
        outs.write("[")
        outs.write(str(field["MaxLen"]))
        outs.write("];\r\n")
    else:
        outs.write("    "*level)
        outs.write(field["Type"])
        outs.write(" ");
        outs.write(field["Name"])
        outs.write(";\r\n")

def gen_msg_structs(msgs, output):
    for msg in msgs:
        name = "Msg" + msg["Name"]
        output.write("typedef struct ")
        output.write(name + " ")
        output.write("{\r\n")
        output.write("    uint8_t type;\r\n");
        for field in msg["fields"]:
            gen_struct_field(output, field, 1)

        output.write("} ")
        output.write(name)
        output.write(";\r\n\r\n")

def msg_func_name(msgName):
    name = re.sub(r'(?<!^)(?=[A-Z])', '_', msgName).lower()
    name = "message_" + name
    return name

def msg_type_name(msgName):
    return "Msg" + msgName

def gen_field_parser(field, output, structPrefix, level):
    if field["Type"] == "array":
        countVarName = structPrefix + field["CountField"]["Name"]
        output.write("    "*level + "success &= ")
        output.write(type_parse_func_name[field["CountField"]["Type"]])
        output.write("(s, &")    
        output.write(countVarName)
        output.write(");\r\n")
        # TODO: no need for this check if it's guaranteed by type
        output.write("    "*level + "success &= ")    
        output.write(countVarName)
        output.write(" <= " + str(field["MaxLen"]));
        output.write(";\r\n")
        output.write("    "*level + "for(int i = 0; success && i < ")
        output.write(countVarName)
        output.write("; i++) {\r\n")

        for subfield in field["fields"]:
            gen_field_parser(subfield, output, structPrefix+field["Name"]+"[i].", level+1)
        output.write("    "*level + "}\r\n")
    else:
        output.write("    "*level + "success &= ")
        output.write(type_parse_func_name[field["Type"]])
        output.write("(s, &")
        output.write(structPrefix)
        output.write(field["Name"])
        output.write(");\r\n")

def gen_msg_parser_prototype(msgs, output):
    output.write("void parse_client_msg(struct ByteStream *s, void *structBuf, uint32_t bufSize, void *usrData);\r\n")
    output.write("void parse_server(struct ByteStream *s, void *structBuf, uint32_t bufSize, void *usrData);\r\n")

# gen code to parse data from byte stream and trigger event
def gen_msg_parser(msgs, output, genServer, genClient):
    if genServer:
        output.write("void parse_server_msg(struct ByteStream *s, void *structBuf, uint32_t bufSize, void *usrData) {\r\n")
    elif genClient:
        output.write("void parse_client_msg(struct ByteStream *s, void *structBuf, uint32_t bufSize, void *usrData) {\r\n")
    else: # unhandled case, parser that can deal with both
        raise
    output.write("    uint8_t type;\r\n")
    output.write("    stream_read_uint8(s, &type);\r\n")
    output.write("    uint32_t success = 1;\r\n")
    output.write("    switch(type) {\r\n")
    for msg in msgs:
        if genServer and (not msg["ToServer"]):
            continue
        if genClient and (not msg["ToClient"]):
            continue
        structVarName = str.lower(msg_type_name(msg["Name"]))
        structTypeName = msg_type_name(msg["Name"])
        output.write("    case ")
        output.write(str(msg["MsgId"]))
        output.write(": // ")
        output.write(structTypeName);
        output.write("\r\n    {\r\n")
        output.write("        ")
        output.write(""+structTypeName+"* ")
        output.write(""+structVarName+" = ("+structTypeName+"*)structBuf;\r\n")

        for field in msg["fields"]:
            gen_field_parser(field, output, structVarName+"->", 2)

        output.write(" "*8)
        # TODO: also require all data to be used for message to be valid?
        output.write("if(success) {\r\n")
        output.write(" "*12)
        output.write(msg_func_name(msg["Name"]))
        output.write("(("+msg_type_name(msg["Name"])+"*)")
        output.write("structBuf")
        output.write(", usrData);")
        output.write("\r\n" + " "*8 + "}\r\n")
        output.write("    } break;\r\n")
    output.write("    }\r\n")
    output.write("}\r\n")


def camel_case_split(str): 
    start_idx = [i for i, e in enumerate(str) 
                 if e.isupper()] + [len(str)] 
    start_idx = [0] + start_idx 
    return [str[x: y] for x, y in zip(start_idx, start_idx[1:])]

# example: void message_update_entities(MsgUpdateEntities *msg, void *usrPtr);
def gen_msg_events(msgs, output):
    for msg in msgs:
        name = msg_func_name(msg["Name"])
        output.write("static void ")
        output.write(name)
        output.write("(")
        output.write(msg_type_name(msg["Name"])+" *msg, void *usrPtr);\r\n")
    output.write("\r\n")


def msg_write_func_name(msgName):
    name = re.sub(r'(?<!^)(?=[A-Z])', '_', msgName).lower()
    name = "message_write_" + name
    return name

# example bool message_write_update_entitied(struct ByteStream *s, const MsgUpdateEntities *msg)
def gen_msg_writer_prototypes(msgs, output):
    for msg in msgs:
        output.write("bool ")
        output.write(msg_write_func_name(msg["Name"]))
        output.write("(TessStack *stack, const ")
        output.write(msg_type_name(msg["Name"]))
        output.write("* msg, uint8_t isserver, void *usrData);\r\n\r\n")
    output.write("static void server_send_data(uint8_t *data, uint32_t len, void *usrPtr);\r\n");
    output.write("static void client_send_data(uint8_t *data, uint32_t len, void *usrPtr);\r\n\r\n");

def gen_msg_field_writer(field, output, prefix, level):
    if field["Type"] == "array":
        arrCountVarName = prefix + field["CountField"]["Name"]
        output.write("    success &= "*level)
        output.write(type_write_func_name[field["CountField"]["Type"]])
        # TODO: check bounds
        output.write("(&s, ")
        output.write(arrCountVarName)
        output.write(");\r\n")
        output.write("    "*level + "success &= " + arrCountVarName + " <= " + str(field["MaxLen"]))
        output.write(";\r\n")
        output.write("    "*level + "for(int i = 0; success && i < ")
        output.write(arrCountVarName)
        output.write("; i++) {\r\n")
        for subfield in field["fields"]:
            gen_msg_field_writer(subfield, output, prefix+field["Name"]+"[i].", level+1)
        output.write("    "*level + "}\r\n")
    else:
        output.write("    "*level + "success &= ") 
        output.write(type_write_func_name[field["Type"]])
        output.write("(&s, ")
        output.write(prefix+field["Name"])
        output.write(");\r\n")

def gen_msg_writer_bodies(msgs, output, genServer, genClient):
    for msg in msgs:
        if genServer and (not msg["ToClient"]):
            continue
        if genClient and (not msg["ToServer"]):
            continue
        output.write("bool ")
        output.write(msg_write_func_name(msg["Name"]))
        output.write("(TessStack *stack, const ")
        output.write(msg_type_name(msg["Name"]))
        output.write("* msg, uint8_t isserver, void *usrData) {\r\n")
        output.write("    uint8_t *mem = stack_push(stack, 65536, 4);\r\n")
        output.write("    ByteStream s;\r\ninit_byte_stream(&s, mem, 65536);\r\n")
        output.write("    uint8_t success = 1;\r\n")
        output.write("    success &= stream_write_uint8(&s, ")
        output.write(str(msg["MsgId"]))
        output.write(");\r\n")
        for field in msg["fields"]:
            gen_msg_field_writer(field, output, "msg->", 1)
        if genServer:
            output.write("    server_send_data(s.start, stream_get_offset(&s), usrData);\r\n")
        elif genClient:

            output.write("    client_send_data(s.start, stream_get_offset(&s), usrData);\r\n")
        else:
            print("Unhandled case: parser for both client and server at same time!\r\n")
            raise
        output.write("    stack_pop(stack);\r\n")
        output.write("    return success;\r\n}\r\n")

def gen_msg_writer_generic_macros(msgs, output):
    i = 0
    lastServerCount = 0
    lastClientCount = 0
    for msg in msgs:
        if msg["ToClient"]:
            lastServerCount = i
        if msg["ToServer"]:
            lastClientCount = i
        i += 1

    output.write("//NOTE: this is basically function overloading for c11\r\n")
    output.write("#define server_message_send(Stream, X, UsrData) _Generic((X), \\\r\n")
    i = 0
    for msg in msgs:
        if msg["ToClient"]:
            output.write(msg_type_name(msg["Name"]))
            output.write("*: ")
            output.write(msg_write_func_name(msg["Name"]))
            if i != lastServerCount:
                output.write(",")
            output.write(" \\\r\n")
        i += 1
    output.write(")(Stream, X, 1, UsrData)\r\n")

    output.write("#define client_message_send(Stream, X, UsrData) _Generic((X), \\\r\n")
    i = 0
    for msg in msgs:
        if msg["ToServer"]:
            output.write(msg_type_name(msg["Name"]))
            output.write("*: ")
            output.write(msg_write_func_name(msg["Name"]))
            if i != lastClientCount:
                output.write(",")
            output.write(" \\\r\n")
        i += 1
    output.write(")(Stream, X, 0,UsrData)\r\n")


output = sio()
output.write("#pragma once\r\n\r\n")
output.write("// THIS CODE IS AUTOMATICALLY GENERATED\r\n// IF IT NEEDS TO BE MODIEFIED, DO SO IN THE GENERATOR\r\n\r\n")
gen_msg_structs(msgs, output)
gen_msg_parser_prototype(msgs, output)
gen_msg_events(msgs, output)
gen_msg_writer_prototypes(msgs, output)
gen_msg_writer_generic_macros(msgs, output);
with open('game_server_structs.h', 'w') as fd:
    output.seek(0)
    shutil.copyfileobj(output, fd)

output = sio()
output.write("#include \"game_server_structs.h\"\r\n\r\n")
output.write("// THIS CODE IS AUTOMATICALLY GENERATED\r\n// IF IT NEEDS TO BE MODIEFIED, DO SO IN THE GENERATOR\r\n\r\n")
gen_msg_parser(msgs, output, False, True)
gen_msg_writer_bodies(msgs, output, False, True)
with open('game_client_structs.c', 'w') as fd:
    output.seek(0)
    shutil.copyfileobj(output, fd)

output = sio()
output.write("#include \"game_server_structs.h\"\r\n\r\n")
output.write("// THIS CODE IS AUTOMATICALLY GENERATED\r\n// IF IT NEEDS TO BE MODIEFIED, DO SO IN THE GENERATOR\r\n\r\n")
gen_msg_parser(msgs, output, True, False)
gen_msg_writer_bodies(msgs, output, True, False)
with open('game_server_structs.c', 'w') as fd:
    output.seek(0)
    shutil.copyfileobj(output, fd)
