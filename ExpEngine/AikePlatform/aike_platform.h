#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifndef AIKE_NO_OPENGL

//////////////////// OPENGL //////////////////////////

#define GL_MAJOR_VERSION                  0x821B
#define GL_MINOR_VERSION                  0x821C
#define GL_ARRAY_BUFFER                   0x8892
#define GL_ELEMENT_ARRAY_BUFFER           0x8893
#define GL_STATIC_DRAW                    0x88E4
#define GL_STATIC_READ                    0x88E5
#define GL_STATIC_COPY                    0x88E6
#define GL_DYNAMIC_DRAW                   0x88E8
#define GL_DYNAMIC_READ                   0x88E9
#define GL_DYNAMIC_COPY                   0x88EA
#define GL_FRAGMENT_SHADER                0x8B30
#define GL_VERTEX_SHADER                  0x8B31
#define GL_COMPILE_STATUS                 0x8B81
#define GL_LINK_STATUS                    0x8B82

#define GL_DEPTH_BUFFER_BIT               0x00000100
#define GL_STENCIL_BUFFER_BIT             0x00000400
#define GL_COLOR_BUFFER_BIT               0x00004000
#define GL_FALSE                          0
#define GL_TRUE                           1
#define GL_POINTS                         0x0000
#define GL_LINES                          0x0001
#define GL_LINE_LOOP                      0x0002
#define GL_LINE_STRIP                     0x0003
#define GL_TRIANGLES                      0x0004
#define GL_TRIANGLE_STRIP                 0x0005
#define GL_TRIANGLE_FAN                   0x0006
#define GL_QUADS                          0x0007
#define GL_NEVER                          0x0200
#define GL_LESS                           0x0201
#define GL_EQUAL                          0x0202
#define GL_LEQUAL                         0x0203
#define GL_GREATER                        0x0204
#define GL_NOTEQUAL                       0x0205
#define GL_GEQUAL                         0x0206
#define GL_ALWAYS                         0x0207
#define GL_ZERO                           0
#define GL_ONE                            1
#define GL_SRC_COLOR                      0x0300
#define GL_ONE_MINUS_SRC_COLOR            0x0301
#define GL_SRC_ALPHA                      0x0302
#define GL_ONE_MINUS_SRC_ALPHA            0x0303
#define GL_DST_ALPHA                      0x0304
#define GL_ONE_MINUS_DST_ALPHA            0x0305
#define GL_DST_COLOR                      0x0306
#define GL_ONE_MINUS_DST_COLOR            0x0307
#define GL_SRC_ALPHA_SATURATE             0x0308
#define GL_NONE                           0
#define GL_FRONT_LEFT                     0x0400
#define GL_FRONT_RIGHT                    0x0401
#define GL_BACK_LEFT                      0x0402
#define GL_BACK_RIGHT                     0x0403
#define GL_FRONT                          0x0404
#define GL_BACK                           0x0405
#define GL_LEFT                           0x0406
#define GL_RIGHT                          0x0407
#define GL_FRONT_AND_BACK                 0x0408
#define GL_NO_ERROR                       0
#define GL_INVALID_ENUM                   0x0500
#define GL_INVALID_VALUE                  0x0501
#define GL_INVALID_OPERATION              0x0502
#define GL_OUT_OF_MEMORY                  0x0505
#define GL_CW                             0x0900
#define GL_CCW                            0x0901
#define GL_POINT_SIZE                     0x0B11
#define GL_POINT_SIZE_RANGE               0x0B12
#define GL_POINT_SIZE_GRANULARITY         0x0B13
#define GL_LINE_SMOOTH                    0x0B20
#define GL_LINE_WIDTH                     0x0B21
#define GL_LINE_WIDTH_RANGE               0x0B22
#define GL_LINE_WIDTH_GRANULARITY         0x0B23
#define GL_POLYGON_MODE                   0x0B40
#define GL_POLYGON_SMOOTH                 0x0B41
#define GL_CULL_FACE                      0x0B44
#define GL_CULL_FACE_MODE                 0x0B45
#define GL_FRONT_FACE                     0x0B46
#define GL_DEPTH_RANGE                    0x0B70
#define GL_DEPTH_TEST                     0x0B71
#define GL_DEPTH_WRITEMASK                0x0B72
#define GL_DEPTH_CLEAR_VALUE              0x0B73
#define GL_DEPTH_FUNC                     0x0B74
#define GL_STENCIL_TEST                   0x0B90
#define GL_STENCIL_CLEAR_VALUE            0x0B91
#define GL_STENCIL_FUNC                   0x0B92
#define GL_STENCIL_VALUE_MASK             0x0B93
#define GL_STENCIL_FAIL                   0x0B94
#define GL_STENCIL_PASS_DEPTH_FAIL        0x0B95
#define GL_STENCIL_PASS_DEPTH_PASS        0x0B96
#define GL_STENCIL_REF                    0x0B97
#define GL_STENCIL_WRITEMASK              0x0B98
#define GL_VIEWPORT                       0x0BA2
#define GL_DITHER                         0x0BD0
#define GL_BLEND_DST                      0x0BE0
#define GL_BLEND_SRC                      0x0BE1
#define GL_BLEND                          0x0BE2
#define GL_LOGIC_OP_MODE                  0x0BF0
#define GL_DRAW_BUFFER                    0x0C01
#define GL_READ_BUFFER                    0x0C02
#define GL_SCISSOR_BOX                    0x0C10
#define GL_SCISSOR_TEST                   0x0C11
#define GL_COLOR_CLEAR_VALUE              0x0C22
#define GL_COLOR_WRITEMASK                0x0C23
#define GL_DOUBLEBUFFER                   0x0C32
#define GL_STEREO                         0x0C33
#define GL_LINE_SMOOTH_HINT               0x0C52
#define GL_POLYGON_SMOOTH_HINT            0x0C53
#define GL_UNPACK_SWAP_BYTES              0x0CF0
#define GL_UNPACK_LSB_FIRST               0x0CF1
#define GL_UNPACK_ROW_LENGTH              0x0CF2
#define GL_UNPACK_SKIP_ROWS               0x0CF3
#define GL_UNPACK_SKIP_PIXELS             0x0CF4
#define GL_UNPACK_ALIGNMENT               0x0CF5
#define GL_PACK_SWAP_BYTES                0x0D00
#define GL_PACK_LSB_FIRST                 0x0D01
#define GL_PACK_ROW_LENGTH                0x0D02
#define GL_PACK_SKIP_ROWS                 0x0D03
#define GL_PACK_SKIP_PIXELS               0x0D04
#define GL_PACK_ALIGNMENT                 0x0D05
#define GL_MAX_TEXTURE_SIZE               0x0D33
#define GL_MAX_VIEWPORT_DIMS              0x0D3A
#define GL_SUBPIXEL_BITS                  0x0D50
#define GL_TEXTURE_1D                     0x0DE0
#define GL_TEXTURE_2D                     0x0DE1
#define GL_TEXTURE_WIDTH                  0x1000
#define GL_TEXTURE_HEIGHT                 0x1001
#define GL_TEXTURE_BORDER_COLOR           0x1004
#define GL_DONT_CARE                      0x1100
#define GL_FASTEST                        0x1101
#define GL_NICEST                         0x1102
#define GL_BYTE                           0x1400
#define GL_UNSIGNED_BYTE                  0x1401
#define GL_SHORT                          0x1402
#define GL_UNSIGNED_SHORT                 0x1403
#define GL_INT                            0x1404
#define GL_UNSIGNED_INT                   0x1405
#define GL_FLOAT                          0x1406
#define GL_STACK_OVERFLOW                 0x0503
#define GL_STACK_UNDERFLOW                0x0504
#define GL_CLEAR                          0x1500
#define GL_AND                            0x1501
#define GL_AND_REVERSE                    0x1502
#define GL_COPY                           0x1503
#define GL_AND_INVERTED                   0x1504
#define GL_NOOP                           0x1505
#define GL_XOR                            0x1506
#define GL_OR                             0x1507
#define GL_NOR                            0x1508
#define GL_EQUIV                          0x1509
#define GL_INVERT                         0x150A
#define GL_OR_REVERSE                     0x150B
#define GL_COPY_INVERTED                  0x150C
#define GL_OR_INVERTED                    0x150D
#define GL_NAND                           0x150E
#define GL_SET                            0x150F
#define GL_TEXTURE                        0x1702
#define GL_COLOR                          0x1800
#define GL_DEPTH                          0x1801
#define GL_STENCIL                        0x1802
#define GL_STENCIL_INDEX                  0x1901
#define GL_DEPTH_COMPONENT                0x1902
#define GL_RED                            0x1903
#define GL_GREEN                          0x1904
#define GL_BLUE                           0x1905
#define GL_ALPHA                          0x1906
#define GL_RGB                            0x1907
#define GL_RGBA                           0x1908
#define GL_POINT                          0x1B00
#define GL_LINE                           0x1B01
#define GL_FILL                           0x1B02
#define GL_KEEP                           0x1E00
#define GL_REPLACE                        0x1E01
#define GL_INCR                           0x1E02
#define GL_DECR                           0x1E03
#define GL_VENDOR                         0x1F00
#define GL_RENDERER                       0x1F01
#define GL_VERSION                        0x1F02
#define GL_EXTENSIONS                     0x1F03
#define GL_NEAREST                        0x2600
#define GL_LINEAR                         0x2601
#define GL_NEAREST_MIPMAP_NEAREST         0x2700
#define GL_LINEAR_MIPMAP_NEAREST          0x2701
#define GL_NEAREST_MIPMAP_LINEAR          0x2702
#define GL_LINEAR_MIPMAP_LINEAR           0x2703
#define GL_TEXTURE_MAG_FILTER             0x2800
#define GL_TEXTURE_MIN_FILTER             0x2801
#define GL_TEXTURE_WRAP_S                 0x2802
#define GL_TEXTURE_WRAP_T                 0x2803
#define GL_REPEAT                         0x2901

//OpenGL 1.1
#define GL_COLOR_LOGIC_OP                 0x0BF2
#define GL_POLYGON_OFFSET_UNITS           0x2A00
#define GL_POLYGON_OFFSET_POINT           0x2A01
#define GL_POLYGON_OFFSET_LINE            0x2A02
#define GL_POLYGON_OFFSET_FILL            0x8037
#define GL_POLYGON_OFFSET_FACTOR          0x8038
#define GL_TEXTURE_BINDING_1D             0x8068
#define GL_TEXTURE_BINDING_2D             0x8069
#define GL_TEXTURE_INTERNAL_FORMAT        0x1003
#define GL_TEXTURE_RED_SIZE               0x805C
#define GL_TEXTURE_GREEN_SIZE             0x805D
#define GL_TEXTURE_BLUE_SIZE              0x805E
#define GL_TEXTURE_ALPHA_SIZE             0x805F
#define GL_DOUBLE                         0x140A
#define GL_PROXY_TEXTURE_1D               0x8063
#define GL_PROXY_TEXTURE_2D               0x8064
#define GL_R3_G3_B2                       0x2A10
#define GL_RGB4                           0x804F
#define GL_RGB5                           0x8050
#define GL_RGB8                           0x8051
#define GL_RGB10                          0x8052
#define GL_RGB12                          0x8053
#define GL_RGB16                          0x8054
#define GL_RGBA2                          0x8055
#define GL_RGBA4                          0x8056
#define GL_RGB5_A1                        0x8057
#define GL_RGBA8                          0x8058
#define GL_RGB10_A2                       0x8059
#define GL_RGBA12                         0x805A
#define GL_RGBA16                         0x805B
#define GL_VERTEX_ARRAY                   0x8074

// OpenGL 1.2
#define GL_UNSIGNED_BYTE_3_3_2            0x8032
#define GL_UNSIGNED_SHORT_4_4_4_4         0x8033
#define GL_UNSIGNED_SHORT_5_5_5_1         0x8034
#define GL_UNSIGNED_INT_8_8_8_8           0x8035
#define GL_UNSIGNED_INT_10_10_10_2        0x8036
#define GL_TEXTURE_BINDING_3D             0x806A
#define GL_PACK_SKIP_IMAGES               0x806B
#define GL_PACK_IMAGE_HEIGHT              0x806C
#define GL_UNPACK_SKIP_IMAGES             0x806D
#define GL_UNPACK_IMAGE_HEIGHT            0x806E
#define GL_TEXTURE_3D                     0x806F
#define GL_PROXY_TEXTURE_3D               0x8070
#define GL_TEXTURE_DEPTH                  0x8071
#define GL_TEXTURE_WRAP_R                 0x8072
#define GL_MAX_3D_TEXTURE_SIZE            0x8073
#define GL_UNSIGNED_BYTE_2_3_3_REV        0x8362
#define GL_UNSIGNED_SHORT_5_6_5           0x8363
#define GL_UNSIGNED_SHORT_5_6_5_REV       0x8364
#define GL_UNSIGNED_SHORT_4_4_4_4_REV     0x8365
#define GL_UNSIGNED_SHORT_1_5_5_5_REV     0x8366
#define GL_UNSIGNED_INT_8_8_8_8_REV       0x8367
#define GL_UNSIGNED_INT_2_10_10_10_REV    0x8368
#define GL_BGR                            0x80E0
#define GL_BGRA                           0x80E1
#define GL_MAX_ELEMENTS_VERTICES          0x80E8
#define GL_MAX_ELEMENTS_INDICES           0x80E9
#define GL_CLAMP_TO_EDGE                  0x812F
#define GL_TEXTURE_MIN_LOD                0x813A
#define GL_TEXTURE_MAX_LOD                0x813B
#define GL_TEXTURE_BASE_LEVEL             0x813C
#define GL_TEXTURE_MAX_LEVEL              0x813D
#define GL_SMOOTH_POINT_SIZE_RANGE        0x0B12
#define GL_SMOOTH_POINT_SIZE_GRANULARITY  0x0B13
#define GL_SMOOTH_LINE_WIDTH_RANGE        0x0B22
#define GL_SMOOTH_LINE_WIDTH_GRANULARITY  0x0B23
#define GL_ALIASED_LINE_WIDTH_RANGE       0x846E

// OpenGL 1.3
#define GL_TEXTURE0                       0x84C0
#define GL_TEXTURE1                       0x84C1
#define GL_TEXTURE2                       0x84C2
#define GL_TEXTURE3                       0x84C3
#define GL_TEXTURE4                       0x84C4
#define GL_TEXTURE5                       0x84C5
#define GL_TEXTURE6                       0x84C6
#define GL_TEXTURE7                       0x84C7
#define GL_TEXTURE8                       0x84C8
#define GL_TEXTURE9                       0x84C9
#define GL_TEXTURE10                      0x84CA
#define GL_TEXTURE11                      0x84CB
#define GL_TEXTURE12                      0x84CC
#define GL_TEXTURE13                      0x84CD
#define GL_TEXTURE14                      0x84CE
#define GL_TEXTURE15                      0x84CF
#define GL_TEXTURE16                      0x84D0
#define GL_TEXTURE17                      0x84D1
#define GL_TEXTURE18                      0x84D2
#define GL_TEXTURE19                      0x84D3
#define GL_TEXTURE20                      0x84D4
#define GL_TEXTURE21                      0x84D5
#define GL_TEXTURE22                      0x84D6
#define GL_TEXTURE23                      0x84D7
#define GL_TEXTURE24                      0x84D8
#define GL_TEXTURE25                      0x84D9
#define GL_TEXTURE26                      0x84DA
#define GL_TEXTURE27                      0x84DB
#define GL_TEXTURE28                      0x84DC
#define GL_TEXTURE29                      0x84DD
#define GL_TEXTURE30                      0x84DE
#define GL_TEXTURE31                      0x84DF
#define GL_ACTIVE_TEXTURE                 0x84E0
#define GL_MULTISAMPLE                    0x809D
#define GL_SAMPLE_ALPHA_TO_COVERAGE       0x809E
#define GL_SAMPLE_ALPHA_TO_ONE            0x809F
#define GL_SAMPLE_COVERAGE                0x80A0
#define GL_SAMPLE_BUFFERS                 0x80A8
#define GL_SAMPLES                        0x80A9
#define GL_SAMPLE_COVERAGE_VALUE          0x80AA
#define GL_SAMPLE_COVERAGE_INVERT         0x80AB
#define GL_TEXTURE_CUBE_MAP               0x8513
#define GL_TEXTURE_BINDING_CUBE_MAP       0x8514
#define GL_TEXTURE_CUBE_MAP_POSITIVE_X    0x8515
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_X    0x8516
#define GL_TEXTURE_CUBE_MAP_POSITIVE_Y    0x8517
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_Y    0x8518
#define GL_TEXTURE_CUBE_MAP_POSITIVE_Z    0x8519
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_Z    0x851A
#define GL_PROXY_TEXTURE_CUBE_MAP         0x851B
#define GL_MAX_CUBE_MAP_TEXTURE_SIZE      0x851C
#define GL_COMPRESSED_RGB                 0x84ED
#define GL_COMPRESSED_RGBA                0x84EE
#define GL_TEXTURE_COMPRESSION_HINT       0x84EF
#define GL_TEXTURE_COMPRESSED_IMAGE_SIZE  0x86A0
#define GL_TEXTURE_COMPRESSED             0x86A1
#define GL_NUM_COMPRESSED_TEXTURE_FORMATS 0x86A2
#define GL_COMPRESSED_TEXTURE_FORMATS     0x86A3
#define GL_CLAMP_TO_BORDER                0x812D

// OpenGL 1.4
#define GL_BLEND_DST_RGB                  0x80C8
#define GL_BLEND_SRC_RGB                  0x80C9
#define GL_BLEND_DST_ALPHA                0x80CA
#define GL_BLEND_SRC_ALPHA                0x80CB
#define GL_POINT_FADE_THRESHOLD_SIZE      0x8128
#define GL_DEPTH_COMPONENT16              0x81A5
#define GL_DEPTH_COMPONENT24              0x81A6
#define GL_DEPTH_COMPONENT32              0x81A7
#define GL_MIRRORED_REPEAT                0x8370
#define GL_MAX_TEXTURE_LOD_BIAS           0x84FD
#define GL_TEXTURE_LOD_BIAS               0x8501
#define GL_INCR_WRAP                      0x8507
#define GL_DECR_WRAP                      0x8508
#define GL_TEXTURE_DEPTH_SIZE             0x884A
#define GL_TEXTURE_COMPARE_MODE           0x884C
#define GL_TEXTURE_COMPARE_FUNC           0x884D
#define GL_BLEND_COLOR                    0x8005
#define GL_BLEND_EQUATION                 0x8009
#define GL_CONSTANT_COLOR                 0x8001
#define GL_ONE_MINUS_CONSTANT_COLOR       0x8002
#define GL_CONSTANT_ALPHA                 0x8003
#define GL_ONE_MINUS_CONSTANT_ALPHA       0x8004
#define GL_FUNC_ADD                       0x8006
#define GL_FUNC_REVERSE_SUBTRACT          0x800B
#define GL_FUNC_SUBTRACT                  0x800A
#define GL_MIN                            0x8007
#define GL_MAX                            0x8008

// OpenGL 1.5
#define GL_BUFFER_SIZE                    0x8764
#define GL_BUFFER_USAGE                   0x8765
#define GL_QUERY_COUNTER_BITS             0x8864
#define GL_CURRENT_QUERY                  0x8865
#define GL_QUERY_RESULT                   0x8866
#define GL_QUERY_RESULT_AVAILABLE         0x8867
#define GL_ARRAY_BUFFER                   0x8892
#define GL_ELEMENT_ARRAY_BUFFER           0x8893
#define GL_ARRAY_BUFFER_BINDING           0x8894
#define GL_ELEMENT_ARRAY_BUFFER_BINDING   0x8895
#define GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING 0x889F
#define GL_READ_ONLY                      0x88B8
#define GL_WRITE_ONLY                     0x88B9
#define GL_READ_WRITE                     0x88BA
#define GL_BUFFER_ACCESS                  0x88BB
#define GL_BUFFER_MAPPED                  0x88BC
#define GL_BUFFER_MAP_POINTER             0x88BD
#define GL_STREAM_DRAW                    0x88E0
#define GL_STREAM_READ                    0x88E1
#define GL_STREAM_COPY                    0x88E2
#define GL_STATIC_DRAW                    0x88E4
#define GL_STATIC_READ                    0x88E5
#define GL_STATIC_COPY                    0x88E6
#define GL_DYNAMIC_DRAW                   0x88E8
#define GL_DYNAMIC_READ                   0x88E9
#define GL_DYNAMIC_COPY                   0x88EA
#define GL_SAMPLES_PASSED                 0x8914
#define GL_SRC1_ALPHA                     0x8589

// OpenGL 2.0
#define GL_BLEND_EQUATION_RGB             0x8009
#define GL_VERTEX_ATTRIB_ARRAY_ENABLED    0x8622
#define GL_VERTEX_ATTRIB_ARRAY_SIZE       0x8623
#define GL_VERTEX_ATTRIB_ARRAY_STRIDE     0x8624
#define GL_VERTEX_ATTRIB_ARRAY_TYPE       0x8625
#define GL_CURRENT_VERTEX_ATTRIB          0x8626
#define GL_VERTEX_PROGRAM_POINT_SIZE      0x8642
#define GL_VERTEX_ATTRIB_ARRAY_POINTER    0x8645
#define GL_STENCIL_BACK_FUNC              0x8800
#define GL_STENCIL_BACK_FAIL              0x8801
#define GL_STENCIL_BACK_PASS_DEPTH_FAIL   0x8802
#define GL_STENCIL_BACK_PASS_DEPTH_PASS   0x8803
#define GL_MAX_DRAW_BUFFERS               0x8824
#define GL_DRAW_BUFFER0                   0x8825
#define GL_DRAW_BUFFER1                   0x8826
#define GL_DRAW_BUFFER2                   0x8827
#define GL_DRAW_BUFFER3                   0x8828
#define GL_DRAW_BUFFER4                   0x8829
#define GL_DRAW_BUFFER5                   0x882A
#define GL_DRAW_BUFFER6                   0x882B
#define GL_DRAW_BUFFER7                   0x882C
#define GL_DRAW_BUFFER8                   0x882D
#define GL_DRAW_BUFFER9                   0x882E
#define GL_DRAW_BUFFER10                  0x882F
#define GL_DRAW_BUFFER11                  0x8830
#define GL_DRAW_BUFFER12                  0x8831
#define GL_DRAW_BUFFER13                  0x8832
#define GL_DRAW_BUFFER14                  0x8833
#define GL_DRAW_BUFFER15                  0x8834
#define GL_BLEND_EQUATION_ALPHA           0x883D
#define GL_MAX_VERTEX_ATTRIBS             0x8869
#define GL_VERTEX_ATTRIB_ARRAY_NORMALIZED 0x886A
#define GL_MAX_TEXTURE_IMAGE_UNITS        0x8872
#define GL_FRAGMENT_SHADER                0x8B30
#define GL_VERTEX_SHADER                  0x8B31
#define GL_MAX_FRAGMENT_UNIFORM_COMPONENTS 0x8B49
#define GL_MAX_VERTEX_UNIFORM_COMPONENTS  0x8B4A
#define GL_MAX_VARYING_FLOATS             0x8B4B
#define GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS 0x8B4C
#define GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS 0x8B4D
#define GL_SHADER_TYPE                    0x8B4F
#define GL_FLOAT_VEC2                     0x8B50
#define GL_FLOAT_VEC3                     0x8B51
#define GL_FLOAT_VEC4                     0x8B52
#define GL_INT_VEC2                       0x8B53
#define GL_INT_VEC3                       0x8B54
#define GL_INT_VEC4                       0x8B55
#define GL_BOOL                           0x8B56
#define GL_BOOL_VEC2                      0x8B57
#define GL_BOOL_VEC3                      0x8B58
#define GL_BOOL_VEC4                      0x8B59
#define GL_FLOAT_MAT2                     0x8B5A
#define GL_FLOAT_MAT3                     0x8B5B
#define GL_FLOAT_MAT4                     0x8B5C
#define GL_SAMPLER_1D                     0x8B5D
#define GL_SAMPLER_2D                     0x8B5E
#define GL_SAMPLER_3D                     0x8B5F
#define GL_SAMPLER_CUBE                   0x8B60
#define GL_SAMPLER_1D_SHADOW              0x8B61
#define GL_SAMPLER_2D_SHADOW              0x8B62
#define GL_DELETE_STATUS                  0x8B80
#define GL_COMPILE_STATUS                 0x8B81
#define GL_LINK_STATUS                    0x8B82
#define GL_VALIDATE_STATUS                0x8B83
#define GL_INFO_LOG_LENGTH                0x8B84
#define GL_ATTACHED_SHADERS               0x8B85
#define GL_ACTIVE_UNIFORMS                0x8B86
#define GL_ACTIVE_UNIFORM_MAX_LENGTH      0x8B87
#define GL_SHADER_SOURCE_LENGTH           0x8B88
#define GL_ACTIVE_ATTRIBUTES              0x8B89
#define GL_ACTIVE_ATTRIBUTE_MAX_LENGTH    0x8B8A
#define GL_FRAGMENT_SHADER_DERIVATIVE_HINT 0x8B8B
#define GL_SHADING_LANGUAGE_VERSION       0x8B8C
#define GL_CURRENT_PROGRAM                0x8B8D
#define GL_POINT_SPRITE_COORD_ORIGIN      0x8CA0
#define GL_LOWER_LEFT                     0x8CA1
#define GL_UPPER_LEFT                     0x8CA2
#define GL_STENCIL_BACK_REF               0x8CA3
#define GL_STENCIL_BACK_VALUE_MASK        0x8CA4
#define GL_STENCIL_BACK_WRITEMASK         0x8CA5

// OpenGL 2.1
#define GL_PIXEL_PACK_BUFFER              0x88EB
#define GL_PIXEL_UNPACK_BUFFER            0x88EC
#define GL_PIXEL_PACK_BUFFER_BINDING      0x88ED
#define GL_PIXEL_UNPACK_BUFFER_BINDING    0x88EF
#define GL_FLOAT_MAT2x3                   0x8B65
#define GL_FLOAT_MAT2x4                   0x8B66
#define GL_FLOAT_MAT3x2                   0x8B67
#define GL_FLOAT_MAT3x4                   0x8B68
#define GL_FLOAT_MAT4x2                   0x8B69
#define GL_FLOAT_MAT4x3                   0x8B6A
#define GL_SRGB                           0x8C40
#define GL_SRGB8                          0x8C41
#define GL_SRGB_ALPHA                     0x8C42
#define GL_SRGB8_ALPHA8                   0x8C43
#define GL_COMPRESSED_SRGB                0x8C48
#define GL_COMPRESSED_SRGB_ALPHA          0x8C49

// OpenGL 3.0
#define GL_COMPARE_REF_TO_TEXTURE         0x884E
#define GL_CLIP_DISTANCE0                 0x3000
#define GL_CLIP_DISTANCE1                 0x3001
#define GL_CLIP_DISTANCE2                 0x3002
#define GL_CLIP_DISTANCE3                 0x3003
#define GL_CLIP_DISTANCE4                 0x3004
#define GL_CLIP_DISTANCE5                 0x3005
#define GL_CLIP_DISTANCE6                 0x3006
#define GL_CLIP_DISTANCE7                 0x3007
#define GL_MAX_CLIP_DISTANCES             0x0D32
#define GL_MAJOR_VERSION                  0x821B
#define GL_MINOR_VERSION                  0x821C
#define GL_NUM_EXTENSIONS                 0x821D
#define GL_CONTEXT_FLAGS                  0x821E
#define GL_COMPRESSED_RED                 0x8225
#define GL_COMPRESSED_RG                  0x8226
#define GL_CONTEXT_FLAG_FORWARD_COMPATIBLE_BIT 0x00000001
#define GL_RGBA32F                        0x8814
#define GL_RGB32F                         0x8815
#define GL_RGBA16F                        0x881A
#define GL_RGB16F                         0x881B
#define GL_VERTEX_ATTRIB_ARRAY_INTEGER    0x88FD
#define GL_MAX_ARRAY_TEXTURE_LAYERS       0x88FF
#define GL_MIN_PROGRAM_TEXEL_OFFSET       0x8904
#define GL_MAX_PROGRAM_TEXEL_OFFSET       0x8905
#define GL_CLAMP_READ_COLOR               0x891C
#define GL_FIXED_ONLY                     0x891D
#define GL_MAX_VARYING_COMPONENTS         0x8B4B
#define GL_TEXTURE_1D_ARRAY               0x8C18
#define GL_PROXY_TEXTURE_1D_ARRAY         0x8C19
#define GL_TEXTURE_2D_ARRAY               0x8C1A
#define GL_PROXY_TEXTURE_2D_ARRAY         0x8C1B
#define GL_TEXTURE_BINDING_1D_ARRAY       0x8C1C
#define GL_TEXTURE_BINDING_2D_ARRAY       0x8C1D
#define GL_R11F_G11F_B10F                 0x8C3A
#define GL_UNSIGNED_INT_10F_11F_11F_REV   0x8C3B
#define GL_RGB9_E5                        0x8C3D
#define GL_UNSIGNED_INT_5_9_9_9_REV       0x8C3E
#define GL_TEXTURE_SHARED_SIZE            0x8C3F
#define GL_TRANSFORM_FEEDBACK_VARYING_MAX_LENGTH 0x8C76
#define GL_TRANSFORM_FEEDBACK_BUFFER_MODE 0x8C7F
#define GL_MAX_TRANSFORM_FEEDBACK_SEPARATE_COMPONENTS 0x8C80
#define GL_TRANSFORM_FEEDBACK_VARYINGS    0x8C83
#define GL_TRANSFORM_FEEDBACK_BUFFER_START 0x8C84
#define GL_TRANSFORM_FEEDBACK_BUFFER_SIZE 0x8C85
#define GL_PRIMITIVES_GENERATED           0x8C87
#define GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN 0x8C88
#define GL_RASTERIZER_DISCARD             0x8C89
#define GL_MAX_TRANSFORM_FEEDBACK_INTERLEAVED_COMPONENTS 0x8C8A
#define GL_MAX_TRANSFORM_FEEDBACK_SEPARATE_ATTRIBS 0x8C8B
#define GL_INTERLEAVED_ATTRIBS            0x8C8C
#define GL_SEPARATE_ATTRIBS               0x8C8D
#define GL_TRANSFORM_FEEDBACK_BUFFER      0x8C8E
#define GL_TRANSFORM_FEEDBACK_BUFFER_BINDING 0x8C8F
#define GL_RGBA32UI                       0x8D70
#define GL_RGB32UI                        0x8D71
#define GL_RGBA16UI                       0x8D76
#define GL_RGB16UI                        0x8D77
#define GL_RGBA8UI                        0x8D7C
#define GL_RGB8UI                         0x8D7D
#define GL_RGBA32I                        0x8D82
#define GL_RGB32I                         0x8D83
#define GL_RGBA16I                        0x8D88
#define GL_RGB16I                         0x8D89
#define GL_RGBA8I                         0x8D8E
#define GL_RGB8I                          0x8D8F
#define GL_RED_INTEGER                    0x8D94
#define GL_GREEN_INTEGER                  0x8D95
#define GL_BLUE_INTEGER                   0x8D96
#define GL_RGB_INTEGER                    0x8D98
#define GL_RGBA_INTEGER                   0x8D99
#define GL_BGR_INTEGER                    0x8D9A
#define GL_BGRA_INTEGER                   0x8D9B
#define GL_SAMPLER_1D_ARRAY               0x8DC0
#define GL_SAMPLER_2D_ARRAY               0x8DC1
#define GL_SAMPLER_1D_ARRAY_SHADOW        0x8DC3
#define GL_SAMPLER_2D_ARRAY_SHADOW        0x8DC4
#define GL_SAMPLER_CUBE_SHADOW            0x8DC5
#define GL_UNSIGNED_INT_VEC2              0x8DC6
#define GL_UNSIGNED_INT_VEC3              0x8DC7
#define GL_UNSIGNED_INT_VEC4              0x8DC8
#define GL_INT_SAMPLER_1D                 0x8DC9
#define GL_INT_SAMPLER_2D                 0x8DCA
#define GL_INT_SAMPLER_3D                 0x8DCB
#define GL_INT_SAMPLER_CUBE               0x8DCC
#define GL_INT_SAMPLER_1D_ARRAY           0x8DCE
#define GL_INT_SAMPLER_2D_ARRAY           0x8DCF
#define GL_UNSIGNED_INT_SAMPLER_1D        0x8DD1
#define GL_UNSIGNED_INT_SAMPLER_2D        0x8DD2
#define GL_UNSIGNED_INT_SAMPLER_3D        0x8DD3
#define GL_UNSIGNED_INT_SAMPLER_CUBE      0x8DD4
#define GL_UNSIGNED_INT_SAMPLER_1D_ARRAY  0x8DD6
#define GL_UNSIGNED_INT_SAMPLER_2D_ARRAY  0x8DD7
#define GL_QUERY_WAIT                     0x8E13
#define GL_QUERY_NO_WAIT                  0x8E14
#define GL_QUERY_BY_REGION_WAIT           0x8E15
#define GL_QUERY_BY_REGION_NO_WAIT        0x8E16
#define GL_BUFFER_ACCESS_FLAGS            0x911F
#define GL_BUFFER_MAP_LENGTH              0x9120
#define GL_BUFFER_MAP_OFFSET              0x9121
#define GL_DEPTH_COMPONENT32F             0x8CAC
#define GL_DEPTH32F_STENCIL8              0x8CAD
#define GL_FLOAT_32_UNSIGNED_INT_24_8_REV 0x8DAD
#define GL_INVALID_FRAMEBUFFER_OPERATION  0x0506
#define GL_FRAMEBUFFER_ATTACHMENT_COLOR_ENCODING 0x8210
#define GL_FRAMEBUFFER_ATTACHMENT_COMPONENT_TYPE 0x8211
#define GL_FRAMEBUFFER_ATTACHMENT_RED_SIZE 0x8212
#define GL_FRAMEBUFFER_ATTACHMENT_GREEN_SIZE 0x8213
#define GL_FRAMEBUFFER_ATTACHMENT_BLUE_SIZE 0x8214
#define GL_FRAMEBUFFER_ATTACHMENT_ALPHA_SIZE 0x8215
#define GL_FRAMEBUFFER_ATTACHMENT_DEPTH_SIZE 0x8216
#define GL_FRAMEBUFFER_ATTACHMENT_STENCIL_SIZE 0x8217
#define GL_FRAMEBUFFER_DEFAULT            0x8218
#define GL_FRAMEBUFFER_UNDEFINED          0x8219
#define GL_DEPTH_STENCIL_ATTACHMENT       0x821A
#define GL_MAX_RENDERBUFFER_SIZE          0x84E8
#define GL_DEPTH_STENCIL                  0x84F9
#define GL_UNSIGNED_INT_24_8              0x84FA
#define GL_DEPTH24_STENCIL8               0x88F0
#define GL_TEXTURE_STENCIL_SIZE           0x88F1
#define GL_TEXTURE_RED_TYPE               0x8C10
#define GL_TEXTURE_GREEN_TYPE             0x8C11
#define GL_TEXTURE_BLUE_TYPE              0x8C12
#define GL_TEXTURE_ALPHA_TYPE             0x8C13
#define GL_TEXTURE_DEPTH_TYPE             0x8C16
#define GL_UNSIGNED_NORMALIZED            0x8C17
#define GL_FRAMEBUFFER_BINDING            0x8CA6
#define GL_DRAW_FRAMEBUFFER_BINDING       0x8CA6
#define GL_RENDERBUFFER_BINDING           0x8CA7
#define GL_READ_FRAMEBUFFER               0x8CA8
#define GL_DRAW_FRAMEBUFFER               0x8CA9
#define GL_READ_FRAMEBUFFER_BINDING       0x8CAA
#define GL_RENDERBUFFER_SAMPLES           0x8CAB
#define GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE 0x8CD0
#define GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME 0x8CD1
#define GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL 0x8CD2
#define GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_CUBE_MAP_FACE 0x8CD3
#define GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LAYER 0x8CD4
#define GL_FRAMEBUFFER_COMPLETE           0x8CD5
#define GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT 0x8CD6
#define GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT 0x8CD7
#define GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER 0x8CDB
#define GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER 0x8CDC
#define GL_FRAMEBUFFER_UNSUPPORTED        0x8CDD
#define GL_MAX_COLOR_ATTACHMENTS          0x8CDF
#define GL_COLOR_ATTACHMENT0              0x8CE0
#define GL_COLOR_ATTACHMENT1              0x8CE1
#define GL_COLOR_ATTACHMENT2              0x8CE2
#define GL_COLOR_ATTACHMENT3              0x8CE3
#define GL_COLOR_ATTACHMENT4              0x8CE4
#define GL_COLOR_ATTACHMENT5              0x8CE5
#define GL_COLOR_ATTACHMENT6              0x8CE6
#define GL_COLOR_ATTACHMENT7              0x8CE7
#define GL_COLOR_ATTACHMENT8              0x8CE8
#define GL_COLOR_ATTACHMENT9              0x8CE9
#define GL_COLOR_ATTACHMENT10             0x8CEA
#define GL_COLOR_ATTACHMENT11             0x8CEB
#define GL_COLOR_ATTACHMENT12             0x8CEC
#define GL_COLOR_ATTACHMENT13             0x8CED
#define GL_COLOR_ATTACHMENT14             0x8CEE
#define GL_COLOR_ATTACHMENT15             0x8CEF
#define GL_COLOR_ATTACHMENT16             0x8CF0
#define GL_COLOR_ATTACHMENT17             0x8CF1
#define GL_COLOR_ATTACHMENT18             0x8CF2
#define GL_COLOR_ATTACHMENT19             0x8CF3
#define GL_COLOR_ATTACHMENT20             0x8CF4
#define GL_COLOR_ATTACHMENT21             0x8CF5
#define GL_COLOR_ATTACHMENT22             0x8CF6
#define GL_COLOR_ATTACHMENT23             0x8CF7
#define GL_COLOR_ATTACHMENT24             0x8CF8
#define GL_COLOR_ATTACHMENT25             0x8CF9
#define GL_COLOR_ATTACHMENT26             0x8CFA
#define GL_COLOR_ATTACHMENT27             0x8CFB
#define GL_COLOR_ATTACHMENT28             0x8CFC
#define GL_COLOR_ATTACHMENT29             0x8CFD
#define GL_COLOR_ATTACHMENT30             0x8CFE
#define GL_COLOR_ATTACHMENT31             0x8CFF
#define GL_DEPTH_ATTACHMENT               0x8D00
#define GL_STENCIL_ATTACHMENT             0x8D20
#define GL_FRAMEBUFFER                    0x8D40
#define GL_RENDERBUFFER                   0x8D41
#define GL_RENDERBUFFER_WIDTH             0x8D42
#define GL_RENDERBUFFER_HEIGHT            0x8D43
#define GL_RENDERBUFFER_INTERNAL_FORMAT   0x8D44
#define GL_STENCIL_INDEX1                 0x8D46
#define GL_STENCIL_INDEX4                 0x8D47
#define GL_STENCIL_INDEX8                 0x8D48
#define GL_STENCIL_INDEX16                0x8D49
#define GL_RENDERBUFFER_RED_SIZE          0x8D50
#define GL_RENDERBUFFER_GREEN_SIZE        0x8D51
#define GL_RENDERBUFFER_BLUE_SIZE         0x8D52
#define GL_RENDERBUFFER_ALPHA_SIZE        0x8D53
#define GL_RENDERBUFFER_DEPTH_SIZE        0x8D54
#define GL_RENDERBUFFER_STENCIL_SIZE      0x8D55
#define GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE 0x8D56
#define GL_MAX_SAMPLES                    0x8D57
#define GL_FRAMEBUFFER_SRGB               0x8DB9
#define GL_HALF_FLOAT                     0x140B
#define GL_MAP_READ_BIT                   0x0001
#define GL_MAP_WRITE_BIT                  0x0002
#define GL_MAP_INVALIDATE_RANGE_BIT       0x0004
#define GL_MAP_INVALIDATE_BUFFER_BIT      0x0008
#define GL_MAP_FLUSH_EXPLICIT_BIT         0x0010
#define GL_MAP_UNSYNCHRONIZED_BIT         0x0020
#define GL_COMPRESSED_RED_RGTC1           0x8DBB
#define GL_COMPRESSED_SIGNED_RED_RGTC1    0x8DBC
#define GL_COMPRESSED_RG_RGTC2            0x8DBD
#define GL_COMPRESSED_SIGNED_RG_RGTC2     0x8DBE
#define GL_RG                             0x8227
#define GL_RG_INTEGER                     0x8228
#define GL_R8                             0x8229
#define GL_R16                            0x822A
#define GL_RG8                            0x822B
#define GL_RG16                           0x822C
#define GL_R16F                           0x822D
#define GL_R32F                           0x822E
#define GL_RG16F                          0x822F
#define GL_RG32F                          0x8230
#define GL_R8I                            0x8231
#define GL_R8UI                           0x8232
#define GL_R16I                           0x8233
#define GL_R16UI                          0x8234
#define GL_R32I                           0x8235
#define GL_R32UI                          0x8236
#define GL_RG8I                           0x8237
#define GL_RG8UI                          0x8238
#define GL_RG16I                          0x8239
#define GL_RG16UI                         0x823A
#define GL_RG32I                          0x823B
#define GL_RG32UI                         0x823C
#define GL_VERTEX_ARRAY_BINDING           0x85B5

// OpenGL 3.1
#define GL_SAMPLER_2D_RECT                0x8B63
#define GL_SAMPLER_2D_RECT_SHADOW         0x8B64
#define GL_SAMPLER_BUFFER                 0x8DC2
#define GL_INT_SAMPLER_2D_RECT            0x8DCD
#define GL_INT_SAMPLER_BUFFER             0x8DD0
#define GL_UNSIGNED_INT_SAMPLER_2D_RECT   0x8DD5
#define GL_UNSIGNED_INT_SAMPLER_BUFFER    0x8DD8
#define GL_TEXTURE_BUFFER                 0x8C2A
#define GL_MAX_TEXTURE_BUFFER_SIZE        0x8C2B
#define GL_TEXTURE_BINDING_BUFFER         0x8C2C
#define GL_TEXTURE_BUFFER_DATA_STORE_BINDING 0x8C2D
#define GL_TEXTURE_RECTANGLE              0x84F5
#define GL_TEXTURE_BINDING_RECTANGLE      0x84F6
#define GL_PROXY_TEXTURE_RECTANGLE        0x84F7
#define GL_MAX_RECTANGLE_TEXTURE_SIZE     0x84F8
#define GL_R8_SNORM                       0x8F94
#define GL_RG8_SNORM                      0x8F95
#define GL_RGB8_SNORM                     0x8F96
#define GL_RGBA8_SNORM                    0x8F97
#define GL_R16_SNORM                      0x8F98
#define GL_RG16_SNORM                     0x8F99
#define GL_RGB16_SNORM                    0x8F9A
#define GL_RGBA16_SNORM                   0x8F9B
#define GL_SIGNED_NORMALIZED              0x8F9C
#define GL_PRIMITIVE_RESTART              0x8F9D
#define GL_PRIMITIVE_RESTART_INDEX        0x8F9E
#define GL_COPY_READ_BUFFER               0x8F36
#define GL_COPY_WRITE_BUFFER              0x8F37
#define GL_UNIFORM_BUFFER                 0x8A11
#define GL_UNIFORM_BUFFER_BINDING         0x8A28
#define GL_UNIFORM_BUFFER_START           0x8A29
#define GL_UNIFORM_BUFFER_SIZE            0x8A2A
#define GL_MAX_VERTEX_UNIFORM_BLOCKS      0x8A2B
#define GL_MAX_GEOMETRY_UNIFORM_BLOCKS    0x8A2C
#define GL_MAX_FRAGMENT_UNIFORM_BLOCKS    0x8A2D
#define GL_MAX_COMBINED_UNIFORM_BLOCKS    0x8A2E
#define GL_MAX_UNIFORM_BUFFER_BINDINGS    0x8A2F
#define GL_MAX_UNIFORM_BLOCK_SIZE         0x8A30
#define GL_MAX_COMBINED_VERTEX_UNIFORM_COMPONENTS 0x8A31
#define GL_MAX_COMBINED_GEOMETRY_UNIFORM_COMPONENTS 0x8A32
#define GL_MAX_COMBINED_FRAGMENT_UNIFORM_COMPONENTS 0x8A33
#define GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT 0x8A34
#define GL_ACTIVE_UNIFORM_BLOCK_MAX_NAME_LENGTH 0x8A35
#define GL_ACTIVE_UNIFORM_BLOCKS          0x8A36
#define GL_UNIFORM_TYPE                   0x8A37
#define GL_UNIFORM_SIZE                   0x8A38
#define GL_UNIFORM_NAME_LENGTH            0x8A39
#define GL_UNIFORM_BLOCK_INDEX            0x8A3A
#define GL_UNIFORM_OFFSET                 0x8A3B
#define GL_UNIFORM_ARRAY_STRIDE           0x8A3C
#define GL_UNIFORM_MATRIX_STRIDE          0x8A3D
#define GL_UNIFORM_IS_ROW_MAJOR           0x8A3E
#define GL_UNIFORM_BLOCK_BINDING          0x8A3F
#define GL_UNIFORM_BLOCK_DATA_SIZE        0x8A40
#define GL_UNIFORM_BLOCK_NAME_LENGTH      0x8A41
#define GL_UNIFORM_BLOCK_ACTIVE_UNIFORMS  0x8A42
#define GL_UNIFORM_BLOCK_ACTIVE_UNIFORM_INDICES 0x8A43
#define GL_UNIFORM_BLOCK_REFERENCED_BY_VERTEX_SHADER 0x8A44
#define GL_UNIFORM_BLOCK_REFERENCED_BY_GEOMETRY_SHADER 0x8A45
#define GL_UNIFORM_BLOCK_REFERENCED_BY_FRAGMENT_SHADER 0x8A46
#define GL_INVALID_INDEX                  0xFFFFFFFFu

// OpenGL 3.2

#define GL_CONTEXT_CORE_PROFILE_BIT       0x00000001
#define GL_CONTEXT_COMPATIBILITY_PROFILE_BIT 0x00000002
#define GL_LINES_ADJACENCY                0x000A
#define GL_LINE_STRIP_ADJACENCY           0x000B
#define GL_TRIANGLES_ADJACENCY            0x000C
#define GL_TRIANGLE_STRIP_ADJACENCY       0x000D
#define GL_PROGRAM_POINT_SIZE             0x8642
#define GL_MAX_GEOMETRY_TEXTURE_IMAGE_UNITS 0x8C29
#define GL_FRAMEBUFFER_ATTACHMENT_LAYERED 0x8DA7
#define GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS 0x8DA8
#define GL_GEOMETRY_SHADER                0x8DD9
#define GL_GEOMETRY_VERTICES_OUT          0x8916
#define GL_GEOMETRY_INPUT_TYPE            0x8917
#define GL_GEOMETRY_OUTPUT_TYPE           0x8918
#define GL_MAX_GEOMETRY_UNIFORM_COMPONENTS 0x8DDF
#define GL_MAX_GEOMETRY_OUTPUT_VERTICES   0x8DE0
#define GL_MAX_GEOMETRY_TOTAL_OUTPUT_COMPONENTS 0x8DE1
#define GL_MAX_VERTEX_OUTPUT_COMPONENTS   0x9122
#define GL_MAX_GEOMETRY_INPUT_COMPONENTS  0x9123
#define GL_MAX_GEOMETRY_OUTPUT_COMPONENTS 0x9124
#define GL_MAX_FRAGMENT_INPUT_COMPONENTS  0x9125
#define GL_CONTEXT_PROFILE_MASK           0x9126
#define GL_DEPTH_CLAMP                    0x864F
#define GL_QUADS_FOLLOW_PROVOKING_VERTEX_CONVENTION 0x8E4C
#define GL_FIRST_VERTEX_CONVENTION        0x8E4D
#define GL_LAST_VERTEX_CONVENTION         0x8E4E
#define GL_PROVOKING_VERTEX               0x8E4F
#define GL_TEXTURE_CUBE_MAP_SEAMLESS      0x884F
#define GL_MAX_SERVER_WAIT_TIMEOUT        0x9111
#define GL_OBJECT_TYPE                    0x9112
#define GL_SYNC_CONDITION                 0x9113
#define GL_SYNC_STATUS                    0x9114
#define GL_SYNC_FLAGS                     0x9115
#define GL_SYNC_FENCE                     0x9116
#define GL_SYNC_GPU_COMMANDS_COMPLETE     0x9117
#define GL_UNSIGNALED                     0x9118
#define GL_SIGNALED                       0x9119
#define GL_ALREADY_SIGNALED               0x911A
#define GL_TIMEOUT_EXPIRED                0x911B
#define GL_CONDITION_SATISFIED            0x911C
#define GL_WAIT_FAILED                    0x911D
#define GL_TIMEOUT_IGNORED                0xFFFFFFFFFFFFFFFFull
#define GL_SYNC_FLUSH_COMMANDS_BIT        0x00000001
#define GL_SAMPLE_POSITION                0x8E50
#define GL_SAMPLE_MASK                    0x8E51
#define GL_SAMPLE_MASK_VALUE              0x8E52
#define GL_MAX_SAMPLE_MASK_WORDS          0x8E59
#define GL_TEXTURE_2D_MULTISAMPLE         0x9100
#define GL_PROXY_TEXTURE_2D_MULTISAMPLE   0x9101
#define GL_TEXTURE_2D_MULTISAMPLE_ARRAY   0x9102
#define GL_PROXY_TEXTURE_2D_MULTISAMPLE_ARRAY 0x9103
#define GL_TEXTURE_BINDING_2D_MULTISAMPLE 0x9104
#define GL_TEXTURE_BINDING_2D_MULTISAMPLE_ARRAY 0x9105
#define GL_TEXTURE_SAMPLES                0x9106
#define GL_TEXTURE_FIXED_SAMPLE_LOCATIONS 0x9107
#define GL_SAMPLER_2D_MULTISAMPLE         0x9108
#define GL_INT_SAMPLER_2D_MULTISAMPLE     0x9109
#define GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE 0x910A
#define GL_SAMPLER_2D_MULTISAMPLE_ARRAY   0x910B
#define GL_INT_SAMPLER_2D_MULTISAMPLE_ARRAY 0x910C
#define GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE_ARRAY 0x910D
#define GL_MAX_COLOR_TEXTURE_SAMPLES      0x910E
#define GL_MAX_DEPTH_TEXTURE_SAMPLES      0x910F
#define GL_MAX_INTEGER_SAMPLES            0x9110

// OpenGL 3.3
#define GL_VERTEX_ATTRIB_ARRAY_DIVISOR    0x88FE
#define GL_SRC1_COLOR                     0x88F9
#define GL_ONE_MINUS_SRC1_COLOR           0x88FA
#define GL_ONE_MINUS_SRC1_ALPHA           0x88FB
#define GL_MAX_DUAL_SOURCE_DRAW_BUFFERS   0x88FC
#define GL_ANY_SAMPLES_PASSED             0x8C2F
#define GL_SAMPLER_BINDING                0x8919
#define GL_RGB10_A2UI                     0x906F
#define GL_TEXTURE_SWIZZLE_R              0x8E42
#define GL_TEXTURE_SWIZZLE_G              0x8E43
#define GL_TEXTURE_SWIZZLE_B              0x8E44
#define GL_TEXTURE_SWIZZLE_A              0x8E45
#define GL_TEXTURE_SWIZZLE_RGBA           0x8E46
#define GL_TIME_ELAPSED                   0x88BF
#define GL_TIMESTAMP                      0x8E28
#define GL_INT_2_10_10_10_REV             0x8D9F

// newer extensions (debug only?)
#define GL_DEBUG_OUTPUT                   0x92E0
#define GL_DEBUG_TYPE_ERROR               0x824C
#define GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR 0x824D
#define GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR  0x824E
#define GL_DEBUG_TYPE_PORTABILITY         0x824F
#define GL_DEBUG_TYPE_PERFORMANCE         0x8250
#define GL_DEBUG_TYPE_OTHER               0x8251
#define GL_DEBUG_SEVERITY_HIGH            0x9146
#define GL_DEBUG_SEVERITY_MEDIUM          0x9147
#define GL_DEBUG_SEVERITY_LOW             0x9148
#define GL_DEBUG_SEVERITY_NOTIFICATION    0x826B

#ifndef GLAPIENTRY
#if defined(_WIN32) && !defined(_WIN64)
#define GLAPIENTRY __stdcall
#else
#define GLAPIENTRY
#endif
#endif

#define GL_FUN_EXPORT extern

typedef char GLchar;
typedef intptr_t GLsizeiptr;
typedef void GLvoid;
typedef unsigned int GLenum;
typedef float GLfloat;
typedef int GLint;
typedef int GLsizei;
typedef unsigned int GLbitfield;
typedef double GLdouble;
typedef unsigned int GLuint;
typedef unsigned char GLboolean;
typedef unsigned char GLubyte;
//typedef intptr_t GLsizeiptr; // redefinition
typedef intptr_t GLintptr;
typedef unsigned short GLhalf;
typedef float GLclampf;
typedef double GLclampd;
typedef uint64_t GLuint64;
typedef int64_t GLint64;
typedef struct __GLsync *GLsync;

typedef void (GLAPIENTRY *GLDEBUGPROC)(GLenum source,GLenum type,GLuint id,GLenum severity,GLsizei length,const GLchar *message,const void *userParam);

#define GL_FUN_TYPEDEC(retType, funcName, signature) typedef retType (GLAPIENTRY *PFNGL ## funcName) signature;

GL_FUN_TYPEDEC(void, glGenVertexArrays, (GLsizei n, GLuint *arrays));
GL_FUN_TYPEDEC(void, glDeleteVertexArrays, (GLsizei m, GLuint *arrays));
GL_FUN_TYPEDEC(void, glGenBuffers, (GLsizei n, GLuint *buffers));
GL_FUN_TYPEDEC(void, glDeleteBuffers, (GLsizei n, GLuint *buffers));
GL_FUN_TYPEDEC(void, glBindBuffer, (GLenum target, GLuint buffer));
GL_FUN_TYPEDEC(void, glBufferData, (GLenum target, GLsizeiptr size, const void *data, GLenum usage));
GL_FUN_TYPEDEC(void, glBufferSubData, (GLenum target, GLintptr offset, GLsizeiptr size, const GLvoid* data));
//Gvoid, L_FUN_TYPEDEC(glDrawArrays, (GLenum mode, GLint first, GLsizei count));
GL_FUN_TYPEDEC(void, glEnableVertexAttribArray, (GLuint index));
GL_FUN_TYPEDEC(void, glDisableVertexAttribArray, (GLuint index));
GL_FUN_TYPEDEC(GLuint, glCreateShader, (GLenum type));
GL_FUN_TYPEDEC(void, glDeleteShader, (GLuint program));
GL_FUN_TYPEDEC(void, glShaderSource, (GLuint shader, GLsizei count, const GLchar *const*string, const GLint *length));
GL_FUN_TYPEDEC(void, glCompileShader, (GLuint shader));
GL_FUN_TYPEDEC(void, glGetShaderiv, (GLuint shader, GLenum pname, GLint *params));
GL_FUN_TYPEDEC(void, glGetShaderInfoLog, (GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *infoLog));
GL_FUN_TYPEDEC(void, glAttachShader, (GLuint program, GLuint shader));
GL_FUN_TYPEDEC(void, glGetProgramiv, (GLuint program, GLenum pname, GLint *params));
GL_FUN_TYPEDEC(void, glGetProgramInfoLog, (GLuint program, GLsizei bufSize, GLsizei *length, GLchar *infoLog));
GL_FUN_TYPEDEC(void, glDetachShader, (GLuint program, GLuint shader));
GL_FUN_TYPEDEC(GLuint, glCreateProgram, (void));
GL_FUN_TYPEDEC(void, glLinkProgram, (GLuint program));
GL_FUN_TYPEDEC(void, glVertexAttribPointer, (GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void *pointer));
GL_FUN_TYPEDEC(void, glVertexAttribIPointer, (GLuint index, GLint size, GLenum type, GLsizei stride, const void *pointer));
GL_FUN_TYPEDEC(void, glUseProgram, (GLuint program));
GL_FUN_TYPEDEC(void, glDeleteProgram, (GLuint program));
GL_FUN_TYPEDEC(void, glBindVertexArray, (GLuint array));

GL_FUN_TYPEDEC(void, glUniform1i, (GLint location, GLint v0));
GL_FUN_TYPEDEC(GLint, glGetUniformLocation, (GLuint program, const GLchar *name)); 
GL_FUN_TYPEDEC(void, glUniformMatrix3fv, (GLint location, GLsizei count, GLboolean transpose, const GLfloat *value));
GL_FUN_TYPEDEC(void, glUniformMatrix4fv, (GLint location, GLsizei count, GLboolean transpose, const GLfloat *value));

GL_FUN_TYPEDEC(void, glViewport,(GLint x, GLint y, GLsizei width, GLsizei height));
GL_FUN_TYPEDEC(void, glClearColor,(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha));
GL_FUN_TYPEDEC(void, glClear, (GLbitfield mask));
GL_FUN_TYPEDEC(GLenum, glGetError, (void));
GL_FUN_TYPEDEC(void, glGenTextures, (GLsizei n, GLuint *textures));
GL_FUN_TYPEDEC(void, glBindTexture, (GLenum target, GLuint texture));
GL_FUN_TYPEDEC(void, glTexImage3D, (GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const void *pixels));
GL_FUN_TYPEDEC(void, glTexParameteri, (GLenum target, GLenum pname, GLint param));
GL_FUN_TYPEDEC(void, glDeleteTextures, (GLsizei n, const GLuint *textures));
GL_FUN_TYPEDEC(void, glEnable, (GLenum cap));
GL_FUN_TYPEDEC(void, glBlendFunc, (GLenum sfactor, GLenum dfactor));
GL_FUN_TYPEDEC(void, glActiveTexture, (GLenum texture));
GL_FUN_TYPEDEC(void, glDrawArrays, (GLenum mode, GLint first, GLsizei count));
GL_FUN_TYPEDEC(void, glTexSubImage3D, (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void *pixels));
GL_FUN_TYPEDEC(const GLubyte*, glGetStringi, (GLenum name, GLuint index));
GL_FUN_TYPEDEC(void, glBindAttribLocation, (GLuint program, GLuint index, const GLchar *name));
GL_FUN_TYPEDEC(void, glGetActiveUniform, (GLuint program, GLuint index, GLsizei bufSize, GLsizei *length, GLint *size, GLenum *type, GLchar *name));
GL_FUN_TYPEDEC(GLboolean, glIsProgram, (GLuint program));
GL_FUN_TYPEDEC(GLboolean, glIsShader, (GLuint shader));
GL_FUN_TYPEDEC(void, glBindBufferBase, (GLenum target, GLuint index, GLuint buffer));
GL_FUN_TYPEDEC(void, glGetActiveUniformsiv, (GLuint program, GLsizei uniformCount, const GLuint *uniformIndices, GLenum pname, GLint *params));
GL_FUN_TYPEDEC(GLuint, glGetUniformBlockIndex, (GLuint program, const GLchar *name));
GL_FUN_TYPEDEC(void, glUniform4fv, (GLint location, GLsizei count, const GLfloat *value));
GL_FUN_TYPEDEC(void, glUniformBlockBinding, (GLuint program, GLuint blockIdx, GLuint blockBinding));
GL_FUN_TYPEDEC(void, glDrawElementsInstanced, (GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei primcount));
GL_FUN_TYPEDEC(void, glGenerateMipmap, (GLenum target));
GL_FUN_TYPEDEC(GLvoid*, glMapBuffer, (GLenum target, GLenum access));
GL_FUN_TYPEDEC(GLvoid*, glMapBufferRange, (GLenum target, GLintptr offset, GLsizeiptr length, GLbitfield access));
GL_FUN_TYPEDEC(void, glUnmapBuffer, (GLenum target));
GL_FUN_TYPEDEC(GLenum, glClientWaitSync, (GLsync sync, GLbitfield flags, GLuint64 timeout));
GL_FUN_TYPEDEC(GLenum, glWaitSync, (GLsync sync, GLbitfield flags, GLuint64 timeout));
GL_FUN_TYPEDEC(GLsync, glFenceSync, (GLenum condition, GLbitfield flags));
GL_FUN_TYPEDEC(void, glDeleteSync, (GLsync sync));
GL_FUN_TYPEDEC(void, glGenFramebuffers, (GLsizei n, GLuint *framebuffers));
GL_FUN_TYPEDEC(void, glBindFramebuffer, (GLenum target, GLuint framebuffer));
GL_FUN_TYPEDEC(void, glBindRenderbuffer, (GLenum target, GLuint renderbuffer));
GL_FUN_TYPEDEC(void, glFramebufferTexture2D, (GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level));
GL_FUN_TYPEDEC(void, glGenRenderbuffers, (GLsizei n, GLuint *renderbuffers));
GL_FUN_TYPEDEC(void, glRenderbufferStorage, (GLenum target, GLenum internalformat, GLsizei width, GLsizei height));
GL_FUN_TYPEDEC(void, glFramebufferRenderbuffer, (GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer));
GL_FUN_TYPEDEC(GLenum, glCheckFramebufferStatus, (GLenum target));
GL_FUN_TYPEDEC(void, glDeleteFramebuffers, (GLsizei n, GLuint *renderbuffers));
GL_FUN_TYPEDEC(void, glDeleteRenderbuffers, (GLsizei n, GLuint *renderbuffers));
GL_FUN_TYPEDEC(void, glBlitFramebuffer, (GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter));
GL_FUN_TYPEDEC(void, glGetBufferSubData, (GLenum target, GLintptr offset, GLsizeiptr size, GLvoid *data));
GL_FUN_TYPEDEC(void, glDrawBuffers, (GLsizei n, const GLenum *bufs));
GL_FUN_TYPEDEC(void, glClearBufferiv, (GLenum buffer, GLint drawBuffer, const GLint * value));
GL_FUN_TYPEDEC(void, glClearBufferfv, (GLenum buffer, GLint drawBuffer, const GLfloat * value));
GL_FUN_TYPEDEC(void, glActiveTexture, (GLenum texture));
GL_FUN_TYPEDEC(void, glBindTexture, (GLenum target, GLuint texture));
GL_FUN_TYPEDEC(void, glGenTextures, (GLsizei n, GLuint *textures));
GL_FUN_TYPEDEC(void, glTexImage2D, (GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void *pixels));
GL_FUN_TYPEDEC(void, glGetTexImage, (GLenum target, GLint level, GLenum format, GLenum type, void *pixels));
GL_FUN_TYPEDEC(void, glTexParameteri, (GLenum target, GLenum pname, GLint param));
GL_FUN_TYPEDEC(void, glEnable, (GLenum cap));
GL_FUN_TYPEDEC(void, glDisable, (GLenum cap));
GL_FUN_TYPEDEC(void, glBlendEquation, (GLenum mode));
GL_FUN_TYPEDEC(void, glBlendFunc, (GLenum sfactor, GLenum dfactor));
GL_FUN_TYPEDEC(void, glScissor, (GLint x, GLint y, GLsizei width, GLsizei height));
GL_FUN_TYPEDEC(void, glDrawElements, (GLenum mode, GLsizei count, GLenum type, const void *indices));
GL_FUN_TYPEDEC(void, glDeleteTextures, (GLsizei n, const GLuint *textures));
GL_FUN_TYPEDEC(void, glGetIntegerv, (GLenum pname, GLint *data));
GL_FUN_TYPEDEC(void, glClearColor, (GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha));
GL_FUN_TYPEDEC(void, glClear, (GLbitfield mask));
GL_FUN_TYPEDEC(void, glFlush, (void));
GL_FUN_TYPEDEC(void, glColorMask, (GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha));
GL_FUN_TYPEDEC(GLenum, glGetError, (void));
GL_FUN_TYPEDEC(const GLubyte *, glGetString, (GLenum name));



GL_FUN_TYPEDEC(void, glDebugMessageCallback, (GLDEBUGPROC callback, const void *userParam));

#undef GL_FUN_TYPEDEC

#define GL_FUNC_VAR(fname) extern PFNGL ## fname fname;
GL_FUNC_VAR(glGenVertexArrays);
GL_FUNC_VAR(glDeleteVertexArrays);
GL_FUNC_VAR(glGenBuffers);
GL_FUNC_VAR(glDeleteBuffers);
GL_FUNC_VAR(glBindBuffer);
GL_FUNC_VAR(glBufferData);
GL_FUNC_VAR(glBufferSubData);
GL_FUNC_VAR(glEnableVertexAttribArray);
GL_FUNC_VAR(glDisableVertexAttribArray);
GL_FUNC_VAR(glCreateShader);
GL_FUNC_VAR(glDeleteShader);
GL_FUNC_VAR(glShaderSource);
GL_FUNC_VAR(glCompileShader);
GL_FUNC_VAR(glGetShaderiv);
GL_FUNC_VAR(glGetShaderInfoLog);
GL_FUNC_VAR(glAttachShader);
GL_FUNC_VAR(glGetProgramiv);
GL_FUNC_VAR(glGetProgramInfoLog);
GL_FUNC_VAR(glDetachShader);
GL_FUNC_VAR(glCreateProgram);
GL_FUNC_VAR(glDeleteProgram);
GL_FUNC_VAR(glLinkProgram);
GL_FUNC_VAR(glVertexAttribPointer);
GL_FUNC_VAR(glVertexAttribIPointer);
GL_FUNC_VAR(glUseProgram);
GL_FUNC_VAR(glBindVertexArray);
GL_FUNC_VAR(glUniform1i);
GL_FUNC_VAR(glGetUniformLocation);
GL_FUNC_VAR(glUniformMatrix3fv);
GL_FUNC_VAR(glUniformMatrix4fv);
GL_FUNC_VAR(glGetStringi);
GL_FUNC_VAR(glBindAttribLocation);
GL_FUNC_VAR(glGetActiveUniform);
GL_FUNC_VAR(glIsProgram);
GL_FUNC_VAR(glIsShader);
GL_FUNC_VAR(glBindBufferBase);
GL_FUNC_VAR(glGetActiveUniformsiv);
GL_FUNC_VAR(glGetUniformBlockIndex);
GL_FUNC_VAR(glUniform4fv);
GL_FUNC_VAR(glUniformBlockBinding);
GL_FUNC_VAR(glDrawElementsInstanced);
GL_FUNC_VAR(glGenerateMipmap);
GL_FUNC_VAR(glMapBuffer);
GL_FUNC_VAR(glMapBufferRange);
GL_FUNC_VAR(glUnmapBuffer);
GL_FUNC_VAR(glClientWaitSync);
GL_FUNC_VAR(glWaitSync);
GL_FUNC_VAR(glFenceSync);
GL_FUNC_VAR(glDeleteSync);
GL_FUNC_VAR(glGenFramebuffers);
GL_FUNC_VAR(glBindFramebuffer);
GL_FUNC_VAR(glBindRenderbuffer);
GL_FUNC_VAR(glFramebufferTexture2D);
GL_FUNC_VAR(glGenRenderbuffers);
GL_FUNC_VAR(glRenderbufferStorage);
GL_FUNC_VAR(glFramebufferRenderbuffer);
GL_FUNC_VAR(glCheckFramebufferStatus);
GL_FUNC_VAR(glDeleteFramebuffers);
GL_FUNC_VAR(glDeleteRenderbuffers);
GL_FUNC_VAR(glBlitFramebuffer);
GL_FUNC_VAR(glGetBufferSubData);
GL_FUNC_VAR(glDrawBuffers);
GL_FUNC_VAR(glClearBufferiv);
GL_FUNC_VAR(glClearBufferfv);
GL_FUNC_VAR(glActiveTexture);
GL_FUNC_VAR(glBindTexture);
GL_FUNC_VAR(glGenTextures);
GL_FUNC_VAR(glTexImage2D);
GL_FUNC_VAR(glTexParameteri);
GL_FUNC_VAR(glEnable);
GL_FUNC_VAR(glDisable);
GL_FUNC_VAR(glBlendEquation);
GL_FUNC_VAR(glBlendFunc);
GL_FUNC_VAR(glScissor);
GL_FUNC_VAR(glDrawElements);
GL_FUNC_VAR(glDeleteTextures);
GL_FUNC_VAR(glGetIntegerv);
GL_FUNC_VAR(glClearColor);
GL_FUNC_VAR(glClear);
GL_FUNC_VAR(glGetTexImage);
GL_FUNC_VAR(glViewport);
GL_FUNC_VAR(glFlush);
GL_FUNC_VAR(glColorMask);
GL_FUNC_VAR(glGetError);
GL_FUNC_VAR(glGetString);


GL_FUNC_VAR(glDebugMessageCallback);
#undef GL_FUNC_VAR

#undef GL_FUNC_VAR

#endif

///////////////// OPENGL END ////////////////////////

#define CURSOR_NONE                 0
#define CURSOR_HORIZONTAL_ARROWS    1
#define CURSOR_VERTICAL_ARROWS      2

#define AIKE_MOUSEB1_BIT   1
#define AIKE_MOUSEB2_BIT   (1<<1)

#define AIKE_CTRL_MASK  0x8000
#define AIKE_ALT_MASK   0x4000
#define AIKE_SHIFT_MASK 0x2000
#define AIKE_KEYCODE_MASK 0x1FFF

// yes this is the same as linux keycodes
typedef enum AikeKeyCodes
{
    AIKE_KEY_RESERVED     =  0,
    AIKE_KEY_ESC		  =  1,
    AIKE_KEY_1			  =  2,
    AIKE_KEY_2			  =  3,
    AIKE_KEY_3			  =  4,
    AIKE_KEY_4			  =  5,
    AIKE_KEY_5			  =  6,
    AIKE_KEY_6			  =  7,
    AIKE_KEY_7			  =  8,
    AIKE_KEY_8			  =  9,
    AIKE_KEY_9			  =  10,
    AIKE_KEY_0			  =  11,
    AIKE_KEY_MINUS		  =  12,
    AIKE_KEY_EQUAL		  =  13,
    AIKE_KEY_BACKSPACE	  =  14,
    AIKE_KEY_TAB		  =  15,
    AIKE_KEY_Q			  =  16,
    AIKE_KEY_W			  =  17,
    AIKE_KEY_E			  =  18,
    AIKE_KEY_R			  =  19,
    AIKE_KEY_T			  =  20,
    AIKE_KEY_Y			  =  21,
    AIKE_KEY_U			  =  22,
    AIKE_KEY_I			  =  23,
    AIKE_KEY_O			  =  24,
    AIKE_KEY_P			  =  25,
    AIKE_KEY_LEFTBRACE	  =  26,
    AIKE_KEY_RIGHTBRACE	  =  27,
    AIKE_KEY_ENTER		  =  28,
    AIKE_KEY_LEFTCTRL	  =  29,
    AIKE_KEY_A			  =  30,
    AIKE_KEY_S			  =  31,
    AIKE_KEY_D			  =  32,
    AIKE_KEY_F			  =  33,
    AIKE_KEY_G			  =  34,
    AIKE_KEY_H			  =  35,
    AIKE_KEY_J			  =  36,
    AIKE_KEY_K			  =  37,
    AIKE_KEY_L			  =  38,
    AIKE_KEY_SEMICOLON	  =  39,
    AIKE_KEY_APOSTROPHE	  =  40,
    AIKE_KEY_GRAVE		  =  41,
    AIKE_KEY_LEFTSHIFT	  =  42,
    AIKE_KEY_BACKSLASH	  =  43,
    AIKE_KEY_Z			  =  44,
    AIKE_KEY_X			  =  45,
    AIKE_KEY_C			  =  46,
    AIKE_KEY_V			  =  47,
    AIKE_KEY_B			  =  48,
    AIKE_KEY_N			  =  49,
    AIKE_KEY_M			  =  50,
    AIKE_KEY_COMMA		  =  51,
    AIKE_KEY_DOT		  =  52,
    AIKE_KEY_SLASH		  =  53,
    AIKE_KEY_RIGHTSHIFT	  =  54,
    AIKE_KEY_KPASTERISK	  =  55,
    AIKE_KEY_LEFTALT	  =  56,
    AIKE_KEY_SPACE		  =  57,
    AIKE_KEY_CAPSLOCK	  =  58,
    AIKE_KEY_F1			  =  59,
    AIKE_KEY_F2			  =  60,
    AIKE_KEY_F3			  =  61,
    AIKE_KEY_F4			  =  62,
    AIKE_KEY_F5			  =  63,
    AIKE_KEY_F6			  =  64,
    AIKE_KEY_F7			  =  65,
    AIKE_KEY_F8			  =  66,
    AIKE_KEY_F9			  =  67,
    AIKE_KEY_F10	      =  68,
    AIKE_KEY_NUMLOCK	  =  69,
    AIKE_KEY_SCROLLLOCK	  =  70,
    AIKE_KEY_KP7		  =  71,
    AIKE_KEY_KP8		  =  72,
    AIKE_KEY_KP9		  =  73,
    AIKE_KEY_KPMINUS	  =  74,
    AIKE_KEY_KP4		  =  75,
    AIKE_KEY_KP5		  =  76,
    AIKE_KEY_KP6		  =  77,
    AIKE_KEY_KPPLUS		  =  78,
    AIKE_KEY_KP1		  =  79,
    AIKE_KEY_KP2		  =  80,
    AIKE_KEY_KP3		  =  81,
    AIKE_KEY_KP0		  =  82,
    AIKE_KEY_KPDOT		  =  83,

    AIKE_KEY_ZENKAKUHANKAKU = 85,
    AIKE_KEY_102ND		  =  86,
    AIKE_KEY_F11		  =  87,
    AIKE_KEY_F12		  =  88,
    AIKE_KEY_RO			  =  89,
    AIKE_KEY_KATAKANA	  =  90,
    AIKE_KEY_HIRAGANA	  =  91,
    AIKE_KEY_HENKAN		  =  92,
    AIKE_KEY_KATAKANAHIRAGANU = 93,
    AIKE_KEY_MUHENKAN	  =  94,
    AIKE_KEY_KPJPCOMMA	  =  95,
    AIKE_KEY_KPENTER	  =  96,
    AIKE_KEY_RIGHTCTRL	  =  97,
    AIKE_KEY_KPSLASH	  =  98,
    AIKE_KEY_SYSRQ		  =  99,
    AIKE_KEY_RIGHTALT	  =  100,
    AIKE_KEY_LINEFEED	  =  101,
    AIKE_KEY_HOME		  =  102,
    AIKE_KEY_UP			  =  103,
    AIKE_KEY_PAGEUP		  =  104,
    AIKE_KEY_LEFT		  =  105,
    AIKE_KEY_RIGHT		  =  106,
    AIKE_KEY_END		  =  107,
    AIKE_KEY_DOWN		  =  108,
    AIKE_KEY_PAGEDOWN	  =  109,
    AIKE_KEY_INSERT		  =  110,
    AIKE_KEY_DELETE		  =  111,
    AIKE_KEY_MACRO		  =  112,
    AIKE_KEY_MUTE		  =  113,
    AIKE_KEY_VOLUMEDOWN	  =  114,
    AIKE_KEY_VOLUMEUP	  =  115,
    AIKE_KEY_POWER		  =  116, /* SC System Power Down */
    AIKE_KEY_KPEQUAL	  =  117,
    AIKE_KEY_KPPLUSMINUS  =  118,
    AIKE_KEY_PAUSE		  =  119,
    AIKE_KEY_SCALE		  =  120, /* AL Compiz Scale (Expose) */

    AIKE_KEY_KPCOMMA	  =  121,
    AIKE_KEY_HANGEUL	  =  122,
    AIKE_KEY_HANGUEL	  =  AIKE_KEY_HANGEUL,
    AIKE_KEY_HANJA		  =  123,
    AIKE_KEY_YEN		  =  124,
    AIKE_KEY_LEFTMETA	  =  125,
    AIKE_KEY_RIGHTMETA	  =  126,
    AIKE_KEY_COMPOSE	  =  127,

    AIKE_KEY_STOP		  =  128, /* AC Stop */
    AIKE_KEY_AGAIN		  =  129,
    AIKE_KEY_PROPS		  =  130, /* AC Properties */
    AIKE_KEY_UNDO		  =  131, /* AC Undo */
    AIKE_KEY_FRONT		  =  132,
    AIKE_KEY_COPY		  =  133, /* AC Copy */
    AIKE_KEY_OPEN		  =  134, /* AC Open */
    AIKE_KEY_PASTE		  =  135, /* AC Paste */
    AIKE_KEY_FIND		  =  136, /* AC Search */
    AIKE_KEY_CUT		  =  137, /* AC Cut */
    AIKE_KEY_HELP		  =  138, /* AL Integrated Help Center */
    AIKE_KEY_MENU		  =  139, /* Menu (show menu) */
    AIKE_KEY_CALC		  =  140, /* AL Calculator */
    AIKE_KEY_SETUP		  =  141,
    AIKE_KEY_SLEEP		  =  142, /* SC System Sleep */
    AIKE_KEY_WAKEUP		  =  143, /* System Wake Up */
    AIKE_KEY_FILE		  =  144, /* AL Local Machine Browser */
    AIKE_KEY_SENDFILE	  =  145,
    AIKE_KEY_DELETEFILE	  =  146,
    AIKE_KEY_XFER		  =  147,
    AIKE_KEY_PROG1		  =  148,
    AIKE_KEY_PROG2		  =  149,
    AIKE_KEY_WWW		  =  150, /* AL Internet Browser */
    AIKE_KEY_MSDOS		  =  151,
    AIKE_KEY_COFFEE		  =  152, /* AL Terminal Lock/Screensaver */
    AIKE_KEY_SCREENLOCK	  =  AIKE_KEY_COFFEE,
    AIKE_KEY_ROTATE_DISPLAY	= 153, /* Display orientation for e.g. tablets */
    AIKE_KEY_DIRECTION	  =  AIKE_KEY_ROTATE_DISPLAY,
    AIKE_KEY_CYCLEWINDOWS =  154,
    AIKE_KEY_MAIL		  =  155,
    AIKE_KEY_BOOKMARKS	  =  156, /* AC Bookmarks */
    AIKE_KEY_COMPUTER	  =  157,
    AIKE_KEY_BACK		  =  158, /* AC Back */
    AIKE_KEY_FORWARD	  =  159, /* AC Forward */
    AIKE_KEY_CLOSECD	  =  160,
    AIKE_KEY_EJECTCD	  =  161,
    AIKE_KEY_EJECTCLOSECD =  162,
    AIKE_KEY_NEXTSONG	  =  163,
    AIKE_KEY_PLAYPAUSE	  =  164,
    AIKE_KEY_PREVIOUSSONG =  165,
    AIKE_KEY_STOPCD		  =  166,
    AIKE_KEY_RECORD		  =  167,
    AIKE_KEY_REWIND		  =  168,
    AIKE_KEY_PHONE		  =  169, /* Media Select Telephone */
    AIKE_KEY_ISO		  =  170,
    AIKE_KEY_CONFIG		  =  171, /* AL Consumer Control Configuration */
    AIKE_KEY_HOMEPAGE	  =  172, /* AC Home */
    AIKE_KEY_REFRESH	  =  173, /* AC Refresh */
    AIKE_KEY_EXIT		  =  174, /* AC Exit */
    AIKE_KEY_MOVE		  =  175,
    AIKE_KEY_EDIT		  =  176,
    AIKE_KEY_SCROLLUP	  =  177,
    AIKE_KEY_SCROLLDOWN	  =  178,
    AIKE_KEY_KPLEFTPAREN  =  179,
    AIKE_KEY_KPRIGHTPAREN =  180,
    AIKE_KEY_NEW		  =  181, /* AC New */
    AIKE_KEY_REDO		  =  182, /* AC Redo/Repeat */

    AIKE_KEY_F13		  =  183,
    AIKE_KEY_F14		  =  184,
    AIKE_KEY_F15		  =  185,
    AIKE_KEY_F16		  =  186,
    AIKE_KEY_F17		  =  187,
    AIKE_KEY_F18		  =  188,
    AIKE_KEY_F19		  =  189,
    AIKE_KEY_F20		  =  190,
    AIKE_KEY_F21		  =  191,
    AIKE_KEY_F22		  =  192,
    AIKE_KEY_F23		  =  193,
    AIKE_KEY_F24		  =  194,

    AIKE_KEY_PLAYCD		  =  200,
    AIKE_KEY_PAUSECD	  =  201,
    AIKE_KEY_PROG3		  =  202,
    AIKE_KEY_PROG4		  =  203,
    AIKE_KEY_DASHBOARD	  =  204, /* AL Dashboard */
    AIKE_KEY_SUSPEND	  =  205,
    AIKE_KEY_CLOSE		  =  206, /* AC Close */
    AIKE_KEY_PLAY		  =  207,
    AIKE_KEY_FASTFORWARD  =  208,
    AIKE_KEY_BASSBOOST	  =  209,
    AIKE_KEY_PRINT		  =  210,
    AIKE_KEY_HP			  =  211,
    AIKE_KEY_CAMERA		  =  212,
    AIKE_KEY_SOUND		  =  213,
    AIKE_KEY_QUESTION	  =  214,
    AIKE_KEY_EMAIL		  =  215,
    AIKE_KEY_CHAT		  =  216,
    AIKE_KEY_SEARCH		  =  217,
    AIKE_KEY_CONNECT	  =  218,
    AIKE_KEY_FINANCE	  =  219, /* AL Checkbook/Finance */
    AIKE_KEY_SPORT		  =  220,
    AIKE_KEY_SHOP		  =  221,
    AIKE_KEY_ALTERASE	  =  222,
    AIKE_KEY_CANCEL		  =  223, /* AC Cancel */
    AIKE_KEY_BRIGHTNESSDOWN	= 224,
    AIKE_KEY_BRIGHTNESSUP	= 225,
    AIKE_KEY_MEDIA		  =  226,

    AIKE_KEY_SWITCHVIDEOMODE = 227,	/* Cycle between available video outputs (Monitor/LCD/TV-out/etc) */
    AIKE_KEY_KBDILLUMTOGGLE	= 228,
    AIKE_KEY_KBDILLUMDOWN = 229,
    AIKE_KEY_KBDILLUMUP	  = 230,

    AIKE_KEY_SEND		  =  231, /* AC Send */
    AIKE_KEY_REPLY		  =  232, /* AC Reply */
    AIKE_KEY_FORWARDMAIL  =  233, /* AC Forward Msg */
    AIKE_KEY_SAVE		  =  234, /* AC Save */
    AIKE_KEY_DOCUMENTS	  =  235,

    AIKE_KEY_BATTERY	  =  236,

    AIKE_KEY_BLUETOOTH	  =  237,
    AIKE_KEY_WLAN		  =  238,
    AIKE_KEY_UWB		  =  239,

    AIKE_KEY_UNKNOWN	  =  240,

    AIKE_KEY_VIDEO_NEXT	  =  241, /* drive next video source */
    AIKE_KEY_VIDEO_PREV	  =  242, /* drive previous video source */
    AIKE_KEY_BRIGHTNESS_CYCLE = 243, /* brightness up, after max is min */
    AIKE_KEY_BRIGHTNESS_AUTO  = 244, /* Set Auto Brightness: manual brightness control is off, rely on ambient */
    AIKE_KEY_BRIGHTNESS_ZERO = AIKE_KEY_BRIGHTNESS_AUTO,
    AIKE_KEY_DISPLAY_OFF  =  245, /* display device to off state */

    AIKE_KEY_WWAN		  =  246, /* Wireless WAN (LTE, UMTS, GSM, etc.) */
    AIKE_KEY_WIMAX		  =  AIKE_KEY_WWAN,
    AIKE_KEY_RFKILL		  =  247, /* Key that controls all radios */

    AIKE_KEY_MICMUTE	  =  248, /* Mute / unmute the microphone */

    AIKE_BTN_MISC		  =  0x100,
    AIKE_BTN_0			  =  0x100,
    AIKE_BTN_1			  =  0x101,
    AIKE_BTN_2			  =  0x102,
    AIKE_BTN_3			  =  0x103,
    AIKE_BTN_4			  =  0x104,
    AIKE_BTN_5			  =  0x105,
    AIKE_BTN_6			  =  0x106,
    AIKE_BTN_7			  =  0x107,
    AIKE_BTN_8			  =  0x108,
    AIKE_BTN_9			  =  0x109,

    AIKE_BTN_MOUSE		  =  0x110,
    AIKE_BTN_LEFT		  =  0x110,
    AIKE_BTN_RIGHT		  =  0x111,
    AIKE_BTN_MIDDLE		  =  0x112,
    AIKE_BTN_SIDE		  =  0x113,
    AIKE_BTN_EXTRA		  =  0x114,
    AIKE_BTN_FORWARD	  =  0x115,
    AIKE_BTN_BACK		  =  0x116,
    AIKE_BTN_TASK		  =  0x117,

    AIKE_BTN_JOYSTICK	  =  0x120,
    AIKE_BTN_TRIGGER	  =  0x120,
    AIKE_BTN_THUMB		  =  0x121,
    AIKE_BTN_THUMB2		  =  0x122,
    AIKE_BTN_TOP		  =  0x123,
    AIKE_BTN_TOP2		  =  0x124,
    AIKE_BTN_PINKIE		  =  0x125,
    AIKE_BTN_BASE		  =  0x126,
    AIKE_BTN_BASE2		  =  0x127,
    AIKE_BTN_BASE3		  =  0x128,
    AIKE_BTN_BASE4		  =  0x129,
    AIKE_BTN_BASE5		  =  0x12a,
    AIKE_BTN_BASE6		  =  0x12b,
    AIKE_BTN_DEAD		  =  0x12f,

    AIKE_BTN_GAMEPAD	  =  0x130,
    AIKE_BTN_SOUTH		  =  0x130,
    AIKE_BTN_A			  =  AIKE_BTN_SOUTH,
    AIKE_BTN_EAST		  =  0x131,
    AIKE_BTN_B			  =  AIKE_BTN_EAST,
    AIKE_BTN_C			  =  0x132,
    AIKE_BTN_NORTH		  =  0x133,
    AIKE_BTN_X			  =  AIKE_BTN_NORTH,
    AIKE_BTN_WEST		  =  0x134,
    AIKE_BTN_Y			  =  AIKE_BTN_WEST,
    AIKE_BTN_Z			  =  0x135,
    AIKE_BTN_TL			  =  0x136,
    AIKE_BTN_TR			  =  0x137,
    AIKE_BTN_TL2		  =  0x138,
    AIKE_BTN_TR2		  =  0x139,
    AIKE_BTN_SELECT		  =  0x13a,
    AIKE_BTN_START		  =  0x13b,
    AIKE_BTN_MODE		  =  0x13c,
    AIKE_BTN_THUMBL		  =  0x13d,
    AIKE_BTN_THUMBR		  =  0x13e,

    AIKE_BTN_DIGI		  =  0x140,
    AIKE_BTN_TOOL_PEN	  =  0x140,
    AIKE_BTN_TOOL_RUBBER  =  0x141,
    AIKE_BTN_TOOL_BRUSH	  =  0x142,
    AIKE_BTN_TOOL_PENCIL  =  0x143,
    AIKE_BTN_TOOL_AIRBRUSH=  0x144,
    AIKE_BTN_TOOL_FINGER  =  0x145,
    AIKE_BTN_TOOL_MOUSE	  =  0x146,
    AIKE_BTN_TOOL_LENS	  =  0x147,
    AIKE_BTN_TOOL_QUINTTAP = 0x148,	/* Five fingers on trackpad */
    AIKE_BTN_STYLUS3	  =  0x149,
    AIKE_BTN_TOUCH		  =  0x14a,
    AIKE_BTN_STYLUS		  =  0x14b,
    AIKE_BTN_STYLUS2	  =  0x14c,
    AIKE_BTN_TOOL_DOUBLETAP	= 0x14d,
    AIKE_BTN_TOOL_TRIPLETAP	= 0x14e,
    AIKE_BTN_TOOL_QUADTAP	= 0x14f, /* Four fingers on trackpad */

    AIKE_BTN_WHEEL		  =  0x150,
    AIKE_BTN_GEAR_DOWN	  =  0x150,
    AIKE_BTN_GEAR_UP	  =  0x151,

    AIKE_KEY_OK			  =  0x160,
    AIKE_KEY_SELECT		  =  0x161,
    AIKE_KEY_GOTO		  =  0x162,
    AIKE_KEY_CLEAR		  =  0x163,
    AIKE_KEY_POWER2		  =  0x164,
    AIKE_KEY_OPTION		  =  0x165,
    AIKE_KEY_INFO		  =  0x166, /* AL OEM Features/Tips/Tutorial */
    AIKE_KEY_TIME		  =  0x167,
    AIKE_KEY_VENDOR		  =  0x168,
    AIKE_KEY_ARCHIVE	  =  0x169,
    AIKE_KEY_PROGRAM	  =  0x16a, /* Media Select Program Guide */
    AIKE_KEY_CHANNEL	  =  0x16b,
    AIKE_KEY_FAVORITES	  =  0x16c,
    AIKE_KEY_EPG		  =  0x16d,
    AIKE_KEY_PVR		  =  0x16e, /* Media Select Home */
    AIKE_KEY_MHP		  =  0x16f,
    AIKE_KEY_LANGUAGE	  =  0x170,
    AIKE_KEY_TITLE		  =  0x171,
    AIKE_KEY_SUBTITLE	  =  0x172,
    AIKE_KEY_ANGLE		  =  0x173,
    AIKE_KEY_ZOOM		  =  0x174,
    AIKE_KEY_MODE		  =  0x175,
    AIKE_KEY_KEYBOARD	  =  0x176,
    AIKE_KEY_SCREEN		  =  0x177,
    AIKE_KEY_PC			  =  0x178, /* Media Select Computer */
    AIKE_KEY_TV			  =  0x179, /* Media Select TV */
    AIKE_KEY_TV2		  =  0x17a, /* Media Select Cable */
    AIKE_KEY_VCR		  =  0x17b, /* Media Select VCR */
    AIKE_KEY_VCR2		  =  0x17c, /* VCR Plus */
    AIKE_KEY_SAT		  =  0x17d, /* Media Select Satellite */
    AIKE_KEY_SAT2		  =  0x17e,
    AIKE_KEY_CD			  =  0x17f, /* Media Select CD */
    AIKE_KEY_TAPE		  =  0x180, /* Media Select Tape */
    AIKE_KEY_RADIO		  =  0x181,
    AIKE_KEY_TUNER		  =  0x182, /* Media Select Tuner */
    AIKE_KEY_PLAYER		  =  0x183,
    AIKE_KEY_TEXT		  =  0x184,
    AIKE_KEY_DVD		  =  0x185, /* Media Select DVD */
    AIKE_KEY_AUX		  =  0x186,
    AIKE_KEY_MP3		  =  0x187,
    AIKE_KEY_AUDIO		  =  0x188, /* AL Audio Browser */
    AIKE_KEY_VIDEO		  =  0x189, /* AL Movie Browser */
    AIKE_KEY_DIRECTORY	  =  0x18a,
    AIKE_KEY_LIST		  =  0x18b,
    AIKE_KEY_MEMO		  =  0x18c, /* Media Select Messages */
    AIKE_KEY_CALENDAR	  =  0x18d,
    AIKE_KEY_RED		  =  0x18e,
    AIKE_KEY_GREEN		  =  0x18f,
    AIKE_KEY_YELLOW		  =  0x190,
    AIKE_KEY_BLUE		  =  0x191,
    AIKE_KEY_CHANNELUP	  =  0x192, /* Channel Increment */
    AIKE_KEY_CHANNELDOWN  =  0x193, /* Channel Decrement */
    AIKE_KEY_FIRST		  =  0x194,
    AIKE_KEY_LAST		  =  0x195, /* Recall Last */
    AIKE_KEY_AB			  =  0x196,
    AIKE_KEY_NEXT		  =  0x197,
    AIKE_KEY_RESTART  	  =  0x198,
    AIKE_KEY_SLOW	      =  0x199,
    AIKE_KEY_SHUFFLE	  =  0x19a,
    AIKE_KEY_BREAK		  =  0x19b,
    AIKE_KEY_PREVIOUS	  =  0x19c,
    AIKE_KEY_DIGITS		  =  0x19d,
    AIKE_KEY_TEEN		  =  0x19e,
    AIKE_KEY_TWEN		  =  0x19f,
    AIKE_KEY_VIDEOPHONE	  =  0x1a0, /* Media Select Video Phone */
    AIKE_KEY_GAMES	      =  0x1a1, /* Media Select Games */
    AIKE_KEY_ZOOMIN		  =  0x1a2, /* AC Zoom In */
    AIKE_KEY_ZOOMOUT	  =  0x1a3, /* AC Zoom Out */
    AIKE_KEY_ZOOMRESET	  =  0x1a4, /* AC Zoom */
    AIKE_KEY_WORDPROCESSOR	=0x1a5, /* AL Word Processor */
    AIKE_KEY_EDITOR		  =  0x1a6, /* AL Text Editor */
    AIKE_KEY_SPREADSHEET  =  0x1a7, /* AL Spreadsheet */
    AIKE_KEY_GRAPHICSEDITOR	= 0x1a8, /* AL Graphics Editor */
    AIKE_KEY_PRESENTATION =  0x1a9, /* AL Presentation App */
    AIKE_KEY_DATABASE	  =  0x1aa, /* AL Database App */
    AIKE_KEY_NEWS		  =  0x1ab, /* AL Newsreader */
    AIKE_KEY_VOICEMAIL	  =  0x1ac, /* AL Voicemail */
    AIKE_KEY_ADDRESSBOOK  =  0x1ad, /* AL Contacts/Address Book */
    AIKE_KEY_MESSENGER	  =  0x1ae, /* AL Instant Messaging */
    AIKE_KEY_DISPLAYTOGGLE=  0x1af, /* Turn display (LCD) on and off */
    AIKE_KEY_BRIGHTNESS_TOGGLE = AIKE_KEY_DISPLAYTOGGLE,
    AIKE_KEY_SPELLCHECK	  =  0x1b0, /* AL Spell Check */
    AIKE_KEY_LOGOFF	      =  0x1b1, /* AL Logoff */
    
    AIKE_KEY_COUNT
} AikeKeyCodes;

typedef struct AikeWindow
{
    float screenX;
    float screenY;
    float width;
    float height;
    bool doubleBuffered;
    void *nativePtr;
} AikeWindow;

typedef void* (*thread_proc_t)(void *userData);
typedef struct AikeThread
{
    void *nativePtr;
    bool exited;
} AikeThread;

typedef struct AikeTime
{
    uint64_t sec; // seconds since start
    uint64_t nsec; // nano seconds
} AikeTime;

// TODO: this number is random, use something that's not random
#define AIKE_MAX_PATH 260

enum AikeFileEntryType
{
    Aike_File_Entry_None,
    Aike_File_Entry_File,
    Aike_File_Entry_Directory,
};

typedef struct AikeFileEntry
{
    char name[AIKE_MAX_PATH];
    uint32_t type;
} AikeFileEntry;

// NOTE: struct AikeDirectory is platform specific
// NOTE: sturct AikeIORequest is platform specific

typedef struct AikeFile
{
    int64_t fd;
    uint64_t size;
    const char *filePath;
} AikeFile;

struct AikePlatform;

typedef AikeFile* (*open_file_t)(struct AikePlatform *platform, const char* filePath, uint32_t mode);
typedef void (*close_file_t)(struct AikePlatform *platform, AikeFile *file);


#ifdef AIKE_AIO
#include "aike_aio.h"
#endif

#define AIKE_EXPECTED_CACHE_LINE_SIZE 64

typedef enum AikeAllocateFlags
{
    Aike_Memory_Flag_Zero     =     1<<0, // should the memory be zeroed (might be zeroed anyways)
    Aike_Memory_Flag_Commit   =     1<<1, // should the memory be commited to physical memory?
    Aike_Memory_Flag_AllowExec=     1<<2, // if this flag is not set and platform supports the feature then code execution will not be allowed from the allocated memory region. There is no guarantee that such feature exists
    
} AikeAllocateFlags;

typedef struct AikeMemoryBlock
{
    uint32_t flags;
    size_t size; // size allocated as
    size_t realSize; // size actually allocated, not for user!
    uint8_t *memory;
} AikeMemoryBlock;

typedef struct AikePlatform
{
    AikeWindow mainWin;

    struct ContextMenu *menus[16];
    // root menu of right click
    struct ContextMenu *rootMenu;

    uint32_t numMenus;

    double dt;
    uint64_t frameCounter;

    // mouse
    float mouseX;
    float mouseY;
    float mouseScreenX;
    float mouseScreenY;
    float mouseVerAxis;
    float mouseHorAxis;
    uint32_t mouseButtons;
    // keyboard
    uint16_t keyStates[AIKE_KEY_COUNT];

    AikeTime startTime;

    void *userData; // THIS IS FREE FOR USE

    void (*exit)(struct AikePlatform *platform);

    void (*create_opengl_context)(struct AikePlatform *platform, AikeWindow *win);
    void (*destroy_opengl_context)(AikeWindow *win);

    bool (*create_borderless_window)(struct AikeWindow *win, uint32_t width, uint32_t height);
    void (*destroy_window)(struct AikePlatform *platform, AikeWindow *window);
    void (*resize_window)(struct AikePlatform *platform, AikeWindow *window, uint32_t width, uint32_t height);
    void (*move_window)(struct AikePlatform *platform, AikeWindow *window, float x, float y);
    void (*window_to_screen_coord)(struct AikePlatform *platform, AikeWindow *win, float x, float y, float *outx, float *outy);
    void (*screen_to_window_coord)(struct AikePlatform *platform, AikeWindow *win, float x, float y, float *outx, float *outy);
    bool (*mouse_coord_valid)(struct AikePlatform *platform, AikeWindow *win);
    void (*set_cursor)(struct AikePlatform *platform, uint32_t cursor);
    void (*present_frame)(struct AikePlatform *platform, AikeWindow *window);
    void (*make_window_current)(struct AikePlatform *platform, AikeWindow *window);
    void (*sleep)(uint32_t usecs);
    AikeTime (*get_monotonic_time)(struct AikePlatform *platform);

    AikeThread* (*create_thread)(void *userData, thread_proc_t procedure);
    bool (*join_thread)(AikeThread *thread, void **result);
    bool (*detach_thread)(AikeThread *thread);

    bool (*swap_interval)(struct AikeWindow *window, int interval);

    AikeMemoryBlock* (*allocate_memory)(struct AikePlatform *platform, size_t size, uint32_t flags);
    void (*free_memory)(struct AikePlatform *platform, AikeMemoryBlock *block);

    AikeDirectory* (*open_directory)(struct AikePlatform *platform, const char *dir);
    // TODO: is there even any point having the single file version?
    uint32_t (*next_file)(struct AikePlatform *platform, AikeDirectory *dir, AikeFileEntry *buf);
    uint32_t (*next_files)(struct AikePlatform *platform, AikeDirectory *dir, AikeFileEntry buf[], uint32_t bufLen);
    void (*close_directory)(struct AikePlatform *platform, AikeDirectory *dir);

    open_file_t open_file;
    close_file_t close_file;

    uint32_t (*next_character)(struct AikePlatform *platform); // read next character from text input stream (keyboard)

    AikeMemoryBlock* (*map_file)(struct AikePlatform *platform, const char *filePath, uint64_t offset, uint64_t size); // size 0 means whole file
    void (*unmap_file)(struct AikePlatform *platform, AikeMemoryBlock *block);


#ifdef AIKE_AIO
    init_async_io_t init_async_io;
    submit_io_request_t submit_io_request;
    get_next_io_event_t get_next_io_event;
    destroy_async_io_t destroy_async_io;

    tcp_listen_t tcp_listen;
    tcp_close_server_t tcp_close_server;
    tcp_accept_t tcp_accept;
    tcp_connect_t tcp_connect;
    tcp_close_connection_t tcp_close_connection;
    tcp_recv_t tcp_recv;
    tcp_send_t tcp_send;
#endif
} AikePlatform;

static inline double aike_timedif_sec(AikeTime from, AikeTime to)
{
    double ret = (to.sec - from.sec) + (((int64_t)to.nsec - (int64_t)from.nsec) / 1000000000.0);
    return ret;
}

// TODO: do all android sdks lack aligned_alloc?
#if defined AIKE_ANDROID
void* aligned_alloc(size_t alignment, uint32_t size);
#endif


#if defined _WIN32 || defined __CYGWIN__
  #ifdef BUILDING_DLL
    #ifdef __GNUC__
      #define DLL_PUBLIC __attribute__ ((dllexport))
    #else
      #define DLL_PUBLIC __declspec(dllexport) // Note: actually gcc seems to also supports this syntax.
    #endif
  #else
    #ifdef __GNUC__
      #define DLL_PUBLIC __attribute__ ((dllimport))
    #else
      #define DLL_PUBLIC __declspec(dllimport) // Note: actually gcc seems to also supports this syntax.
    #endif
  #endif
  #define DLL_LOCAL
#else
  #if __GNUC__ >= 4
    #define DLL_PUBLIC __attribute__ ((visibility ("default")))
    #define DLL_LOCAL  __attribute__ ((visibility ("hidden")))
  #else
    #define DLL_PUBLIC
    #define DLL_LOCAL
  #endif
#endif

#ifndef aike_thread_local
# if __STDC_VERSION__ >= 201112 && !defined __STDC_NO_THREADS__
#  define aike_thread_local _Thread_local
# elif defined _WIN32 && ( \
       defined _MSC_VER || \
       defined __ICL || \
       defined __DMC__ || \
       defined __BORLANDC__ )
#  define aike_thread_local __declspec(thread) 
/* note that ICC (linux) and Clang are covered by __GNUC__ */
# elif defined __GNUC__ || \
       defined __SUNPRO_C || \
       defined __xlC__
#  define aike_thread_local __thread
# else
#  error "Cannot define thread_local"
# endif
#endif

