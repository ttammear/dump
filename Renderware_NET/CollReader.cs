using System;
using System.Collections.Generic;
using System.IO;

namespace RenderwareNet {

public class TVector {
    public float x, y, z;
    public static TVector Parse(BinaryReader br) {
        var ret = new TVector();
        ret.x = br.ReadSingle();
        ret.y = br.ReadSingle();
        ret.z = br.ReadSingle();
        return ret;
    }
}

public class TBounds {
    public float radius;
    public TVector center;
    public TVector min;
    public TVector max;

    public static TBounds Parse(BinaryReader br, int version) {
        if(version == 1) {
            TBounds ret =  new TBounds();
            ret.radius = br.ReadSingle();
            ret.center = TVector.Parse(br);
            ret.min = TVector.Parse(br);
            ret.max = TVector.Parse(br);
            return ret;
        } else {
            throw new NotImplementedException();
        }
    }
}

public class TSurface {
    public byte material;
    public byte flag;
    public byte brightness;
    public byte light;
    public static TSurface Parse(BinaryReader br) {
        TSurface ret = new TSurface();
        ret.material = br.ReadByte();
        ret.flag = br.ReadByte();
        ret.brightness = br.ReadByte();
        ret.light = br.ReadByte();
        return ret;
    }
}

public class TSphere {
    public float radius;
    public TVector center;
    public TSurface surface;

    public static TSphere Parse(BinaryReader br, int version) {
        if(version == 1) {
            TSphere ret = new TSphere();
            ret.radius = br.ReadSingle();
            ret.center = TVector.Parse(br);
            ret.surface = TSurface.Parse(br);
            return ret;
        } else {
            throw new NotImplementedException();
        }
    }
}

public class TBox {
    public TVector min;
    public TVector max;
    public TSurface surface;

    public static TBox Parse(BinaryReader br, int version) {
        if(version == 1) {
            var ret = new TBox();
            ret.min = TVector.Parse(br);
            ret.max = TVector.Parse(br);
            ret.surface = TSurface.Parse(br);
            return ret;
        } else {
            throw new NotImplementedException();
        }
    }
}

public class TFace1 {
    public uint a; // 32 bit
    public uint b;
    public uint c;
    TSurface surface;
    public static TFace1 Parse(BinaryReader br) {
        var ret = new TFace1();
        ret.a = br.ReadUInt32();
        ret.b = br.ReadUInt32();
        ret.c = br.ReadUInt32();
        ret.surface = TSurface.Parse(br);
        return ret;
    }
}

public class TFace2 {
    public int a; // 16 bit
    public int b;
    public int c;
    public int material; // 8 bit
    public int light; // 8 bit
}

public class TVertex2 {
    public int x, y, z; // 16 bit
}

public class CollHeaderExtra2 {
    public ushort numSpheres;
    public ushort numBoxes;
    public ushort numFaces;
    public byte numLines;
    public uint flags;
    public uint offsetSpheres;
    public uint offsetBoxes;
    public uint offsetLines;
    public uint offsetMeshVertices;
    public uint offsetMeshFaces;
    public uint offsetTrianglePlanes;

    public static CollHeaderExtra2 Parse(BinaryReader br, int version) {
        CollHeaderExtra2 ret = new CollHeaderExtra2();
        ret.numSpheres = br.ReadUInt16();
        ret.numBoxes = br.ReadUInt16();
        ret.numFaces = br.ReadUInt16();
        ret.numLines = br.ReadByte();
        br.ReadByte(); // unused
        ret.flags = br.ReadUInt32();
        ret.offsetSpheres = br.ReadUInt32();
        ret.offsetBoxes = br.ReadUInt32();
        ret.offsetLines = br.ReadUInt32();
        ret.offsetMeshVertices = br.ReadUInt32();
        ret.offsetMeshFaces = br.ReadUInt32();
        ret.offsetTrianglePlanes = br.ReadUInt32();
        return ret;
    }
}

public class CollHeaderExtra3 {
    public uint numShadowMeshFaces;
    public uint offsetShadowMeshVertices;
    public uint offsetShadowMeshFaces;

    public static CollHeaderExtra3 Parse(BinaryReader br, int version) {
        CollHeaderExtra3 ret = new CollHeaderExtra3();
        ret.numShadowMeshFaces = br.ReadUInt32();
        ret.offsetShadowMeshVertices = br.ReadUInt32();
        ret.offsetShadowMeshVertices = br.ReadUInt32();
        return ret;
    }
}

public class CollHeaderExtra4 {
    public uint unused;
    public static CollHeaderExtra4 Parse(BinaryReader br, int version) {
        CollHeaderExtra4 ret = new CollHeaderExtra4();
        ret.unused = br.ReadUInt32();
        return ret;
    }
}

public class CollHeader {
    public int version; // not part of file format
    public string fourCC;
    public uint fileSize;
    public string collisionModelName;
    public ushort modelId;
    public TBounds bounds;

    public CollHeaderExtra2 extra2;
    public CollHeaderExtra3 extra3;
    public CollHeaderExtra4 extra4;

    public static CollHeader Parse(BinaryReader br) {
        CollHeader ret = new CollHeader();
        var fourcc = br.ReadChars(4);
        var foourccs = new string(fourcc);
        switch(foourccs) {
            case "COLL":
            ret.version = 1;
            break;
            case "COL2":
            ret.version = 2;
            break;
            case "COL3":
            ret.version = 3;
            break;
            default: // unknown FOURCC, probably just padding
            return null;
        }
        ret.fileSize = br.ReadUInt32();
        ret.collisionModelName = System.Text.Encoding.ASCII.GetString(br.ReadBytes(22), 0, 22);
        ret.collisionModelName = ret.collisionModelName.Substring(0, ret.collisionModelName.IndexOf('\0'));
        ret.modelId = br.ReadUInt16();
        ret.bounds = TBounds.Parse(br, ret.version);

        if(ret.version >= 2) {
            ret.extra2 = CollHeaderExtra2.Parse(br, ret.version);
            if(ret.version >= 3) {
                ret.extra3 = CollHeaderExtra3.Parse(br, ret.version);
                if(ret.version >= 4) {
                    ret.extra4 = CollHeaderExtra4.Parse(br, ret.version);
                }
            }
        }

        return ret;
    }
}

public enum CollFlags {
    UseCones = 1, // over lines
    NotEmpty = 2,
    HasFaceGroups = 8,
    HasShadowMesh = 16,
}

public class CollData {
    public TSphere[] spheres;
    public TBox[] boxes;
    public TVector[] vertices1; // ver 1
    public TVertex2[] vertices2; // ver 2+

    public TFace1[] facesVer1; // ver 1
    public TFace2[] facesVer2; // ver 2+

}

public static class CollReader {
    public static CollHeader parseHeader(BinaryReader br) {

        CollHeader ret = CollHeader.Parse(br);
        return ret;
    }

    private static CollData parseV1Data(BinaryReader binaryReader, CollHeader header) {
        var ret = new CollData();
        
        uint sphereCount = binaryReader.ReadUInt32();
        ret.spheres = new TSphere[sphereCount];
        for(int i = 0; i < sphereCount; i++) {
            ret.spheres[i] = TSphere.Parse(binaryReader, header.version);
        }
        uint unusedCount = binaryReader.ReadUInt32();
        if(unusedCount != 0) {
            throw new Exception("0 elements expected in unknown section");
        }
        uint boxCount = binaryReader.ReadUInt32();
        ret.boxes = new TBox[boxCount];
        for(int i = 0; i < boxCount; i++) {
            ret.boxes[i] = TBox.Parse(binaryReader, header.version);
        }
        uint vertexCount = binaryReader.ReadUInt32();
        ret.vertices1 = new TVector[vertexCount];
        for(int i = 0; i < vertexCount; i++) {
            ret.vertices1[i] = TVector.Parse(binaryReader);
        }
        uint faceCount = binaryReader.ReadUInt32();
        ret.facesVer1 = new TFace1[faceCount];
        for(int i = 0; i < faceCount; i++) {
            ret.facesVer1[i] = TFace1.Parse(binaryReader);
        }

        return ret;
    }

    public static CollData parseData(BinaryReader binaryReader, CollHeader header) {
        if(header.version == 1) {
            return parseV1Data(binaryReader, header);
        } else {
            throw new NotImplementedException();
        }
    }
}

}