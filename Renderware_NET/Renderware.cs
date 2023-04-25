using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;

namespace RenderwareNet {
    public enum RWChunkId
    {
        Struct = 0x1,
        String = 0x2,
        Extension = 0x3,
        Camera = 0x5,
        Texture = 0x6,
        Material = 0x7,
        Material_List = 0x8,
        Frame_List = 0xE,
        Geometry = 0xF,
        Clump = 0x10,
        Light = 0x12,
        Atomic = 0x14,
        Texture_Native = 0x15,
        Texture_Dictionary = 0x16,
        Geometry_List = 0x1A,
    }
    public enum RWFilterMode
    {
        FILTER_NONE = 0x0,
        FILTER_NEAREST = 0x01,
        FILTER_LINEAR  = 0x02,
        FILTER_MIP_NEAREST = 0x03,
        FILTER_MIP_LINEAR = 0x04,
        FILTER_LINEAR_MIP_NEAREST = 0x05,
        FILTER_LINEAR_MIP_LINEAR = 0x06,
    }

    public enum RWTextureAddressingMode
    {
        TEXTURE_ADDRESSING_MODE_NONE = 0x0,
        TEXTURE_ADDRESSING_MODE_WRAP = 0x1,
        TEXTURE_ADDRESSING_MODE_MIRROR = 0x2,
        TEXTURE_ADDRESSING_MODE_CLAMP = 0x3,
        TEXTURE_ADDRESSING_MODE_BORDER = 0x4,
    }

    public enum RWRasterFormat
    {
        FORMAT_Default          = 0x0000,
        FORMAT_1555             = 0x0100, // (1 bit alpha, RGB 5 bits each; also used for DXT1 with alpha)
        FORMAT_565              = 0x0200, // (5 bits red, 6 bits green, 5 bits blue; also used for DXT1 without alpha)
        FORMAT_4444             = 0x0300, // (RGBA 4 bits each; also used for DXT3)
        FORMAT_LUM8             = 0x0400, // (gray scale, D3DFMT_L8)
        FORMAT_8888             = 0x0500, // (RGBA 8 bits each)
        FORMAT_888              = 0x0600, // (RGB 8 bits each, D3DFMT_X8R8G8B8)
        FORMAT_555              = 0x0A00, // (RGB 5 bits each - rare, use 565 instead, D3DFMT_X1R5G5B5)

        // for internal use
        FORMAT_DXT1_ALPHA = 0x10000,
        FORMAT_DXT3 = 0x20000,
        FORMAT_DXT1_NO_ALPHA = 0x30000,
    }
    
    public enum RWRasterFormatExtension
    {
        FORMAT_EXT_NONE         = 0x0000,
        FORMAT_EXT_AUTO_MIPMAP  = 0x1000, // (RW generates mipmaps, see special section below)
        FORMAT_EXT_PAL8         = 0x2000, // (2^8 = 256 palette colors)
        FORMAT_EXT_PAL4         = 0x4000, // (2^4 = 16 palette colors)
        FORMAT_EXT_MIPMAP       = 0x8000, // (mipmaps included)
    }

    public enum RWGeometryFormatFlag {
        TRIANGLE_STRIP = 0x1,
        POSITION = 0x2,
        TEXTURE_COORD1 = 0x4,
        PRELIT = 0x8,
        NORMALS = 0x10,
        LIGHT = 0x20,
        MODULATE_MATERIAL_COLOR = 0x40,
        TEXTURE_COORD2 = 0x80,
        NATIVE = 0x1000000,
        MASK_NUM_TEXTURE_SETS = 0xFF0000,
    };
    public struct RWVector4 {
        public float X, Y, Z, W;
    }
    public struct RWVector3 {
        public float X, Y, Z;
        public static RWVector3 operator+(RWVector3 l, RWVector3 r) {
            return new RWVector3 {
                X = l.X+r.X,
                Y = l.Y+r.Y,
                Z = l.Z+r.Z
            };
        }
    }
    public struct RWVector2 {
        public float X,Y;
    }
    public struct RWMatrix3x3 {
        // damn C# for not having fixed size arrays
        public RWVector3 M1; // matrix row 1
        public RWVector3 M2;
        public RWVector3 M3;
        public static RWVector3 operator*(RWMatrix3x3 m, RWVector3 v) {
            return new RWVector3 {
                X = m.M1.X*v.X+m.M1.Y*v.Y+m.M1.Z*v.Z,
                Y = m.M2.X*v.X+m.M2.Y*v.Y+m.M2.Z*v.Z,
                Z = m.M3.X*v.X+m.M3.Y*v.Y+m.M3.Z*v.Z,
            };
        }
    }
    public struct RWTriangle {
        // NOTE: actual order in files is 2, 1, materialId, 3
        public ushort V1;
        public ushort V2;
        public ushort V3;
        public ushort MaterialId;
    }
    public struct RWSphere {
        public RWVector3 Position;
        public float Radius;
    }
    public struct RWColor {
        public byte r, g, b, a;

        public RWColor(byte r, byte g, byte b, byte a) {
            this.r = r;
            this.g = g;
            this.b = b;
            this.a = a;
        }
        
        public static RWColor FromRGB565(ushort value) {
            RWColor ret;
            ret.r = (byte)((value>>11)&0x1F);
            ret.g = (byte)((value>>5)&0x3F);
            ret.b = (byte)(value&0x1F);
            ret.r = (byte)((ret.r << 3) | (ret.r >> 2)); // 11111 -> 11111000 -> 11111111
            ret.g = (byte)((ret.g << 2) | (ret.g >> 4)); //111111 -> 11111100 -> 11111111
            ret.b = (byte)((ret.b << 3) | (ret.b >> 2));
            ret.a = 255;
            return ret;
        }
        public static RWColor FromRGB4444(ushort value)
        {
            RWColor ret;
            ret.r = (byte)((value >> 12) & 0xF);
            ret.g = (byte)((value >> 8) & 0xF);
            ret.b = (byte)((value >> 4) & 0xF);
            ret.a = (byte)(value & 0xF);
            ret.r = (byte)((ret.r << 4) | ret.r);
            ret.g = (byte)((ret.g << 4) | ret.g);
            ret.b = (byte)((ret.b << 4) | ret.b);
            ret.a = (byte)((ret.a << 4) | ret.a);
            return ret;
        }
        public static RWColor operator/ (RWColor a, int b) {
            int sr = a.r/b;
            int sg = a.g/b;
            int sb = a.b/b;
            int sa = a.a/b;
            return new RWColor((byte)(sr<255?sr:255), (byte)(sg<255?sg:255), (byte)(sb<255?sb:255), (byte)(sa<255?sa:255));
        }
        public static RWColor operator>> (RWColor a, int b) {
            return new RWColor((byte)(a.r>>b), (byte)(a.g>>b), (byte)(a.b>>b), (byte)(a.a>>b));
        }
        public static RWColor operator<< (RWColor a, int b) {
            int sr = a.r<<b;
            int sg = a.g<<b;
            int sb = a.b<<b;
            int sa = a.a<<b;
            return new RWColor((byte)(sr<255?sr:255), (byte)(sg<255?sg:255), (byte)(sb<255?sb:255), (byte)(sa<255?sa:255));
        }
        public static RWColor operator* (RWColor a, int b) {
            int sr = a.r*b;
            int sg = a.g*b;
            int sb = a.b*b;
            int sa = a.a*b;
            return new RWColor((byte)(sr<255?sr:255), (byte)(sg<255?sg:255), (byte)(sb<255?sb:255), (byte)(sa<255?sa:255));
        }
        public static RWColor operator+ (RWColor a, RWColor b) {
            int sr = a.r+b.r;
            int sg = a.g+b.g;
            int sb = a.b+b.b;
            int sa = a.a+b.a;
            return new RWColor((byte)(sr<255?sr:255), (byte)(sg<255?sg:255), (byte)(sb<255?sb:255), (byte)(sa<255?sa:255));
        }

        public static byte[] ConvertToBGRA(RWColor[,] img) {
            int width = img.GetLength(0);
            int height = img.GetLength(1);
            byte[] ret = new byte[width*height*4];
            for(int i = 0; i < width; i++) {
                for(int j = 0; j < height; j++) {
                    int idx = j*4*width + i*4;
                    ret[idx+0] = img[i,j].b;
                    ret[idx+1] = img[i,j].g;
                    ret[idx+2] = img[i,j].r;
                    ret[idx+3] = img[i,j].a;
                    /*if(i == j) { // blue
                        ret[idx+0] = 0x00; // blue
                        ret[idx+1] = 0xFF; // green
                        ret[idx+2] = 0x00; // red
                        ret[idx+3] = 0xFF; // alpha
                    }*/
                }
            }
            return ret;
        }

        public static byte[] ConvertToRGBA(RWColor[,] img) {
            int width = img.GetLength(0);
            int height = img.GetLength(1);
            byte[] ret = new byte[width*height*4];
            for(int i = 0; i < width; i++) {
                for(int j = 0; j < height; j++) {
                    int idx = j*4*width + i*4;
                    ret[idx+0] = img[i,j].r;
                    ret[idx+1] = img[i,j].g;
                    ret[idx+2] = img[i,j].b;
                    ret[idx+3] = img[i,j].a;
                }
            }
            return ret;
        }

        public static RWColor[,] From888(byte[] data, int width, int height) {
            var ret = new RWColor[width, height];
            var stream = new MemoryStream(data);
            var br = new BinaryReader(stream);
            Debug.Assert(data.Length == width*height*4);
            for(int y = 0; y < height; y++) {
                for(int x = 0; x < width; x++) {
                    var pixel = br.ReadUInt32();
                    byte r = (byte)((pixel>>24)&0xFF);
                    byte g = (byte)((pixel>>16)&0xFF);
                    byte b = (byte)((pixel>>8)&0xFF);
                    byte a = 255; // because its 888 meaning alpha channel should not be used?
                    ret[x, y] = new RWColor() {r = r, g = g, b = b, a = a};
                }
            }
            return ret;
        }
    }
    public class RWFrame {
        public RWMatrix3x3 RotationMatrix;
        public RWVector3 Position;
        public int FrameIndex;
        public uint MatrixFlags;
    }
    public class RWMorphTarget{
        public RWSphere BoundingSphere;
        public bool HasVertices;
        public bool HasNormals;
        public RWVector3[] Vertices;
        public RWVector3[] Normals;
    }
    public class RWGeometry {
        public uint Format;
        // NOTE: these 3 are only RW < 0x34000
        public float SurfaceAmbient;
        public float SurfaceSpecular;
        public float SurfaceDiffuse;
        public int NumVertices {
            get {
                if(MorphTargets == null || MorphTargets.Length == 0) {
                    return 0;
                }
                return MorphTargets[0].Vertices.Length;
            }
        }
        public uint[] PrelitColors; // RGBA format
        public RWVector2[,] TextureCoords = new RWVector2[0,0]; // there can be multiple texture sets
        public RWTriangle[] Triangles = new RWTriangle[0];
        public RWMorphTarget[] MorphTargets = new RWMorphTarget[0];

    }
    public class RWMaterialTexture {
        public RWFilterMode Filtering;
        public RWTextureAddressingMode UAddressing;
        public RWTextureAddressingMode VAddressing;
        public bool MipmapsUsed;
        public string TextureName;
        public string AlphaTextureName;

    }
    public class RWMaterial {
        public uint Flags;
        public uint Color;
        //public int Unused;
        public float SurfaceAmbient;
        public float SurfaceSpecular;
        public float SurfaceDiffuse;
        public RWMaterialTexture Texture;
    }
    public class RWAtomic {
        public uint FrameIndex;
        public uint GeometryIndex;
        public uint Flags;
    }
    public class RWClump {
        public int NumAtomics;
        public int NumLights;
        public int NumCameras;
        public List<RWFrame> FrameList;
        public List<RWGeometry> GeometryList;
        public RWMaterial[] Materials = new RWMaterial[0]; // multiple elements can reference one material
        public RWAtomic[] Atomics = new RWAtomic[0];
        // TODO: atomics
        // TODO: lights
        // TODO: cameras
    }
    public class RWTextureNativePC {
        public uint PlatformId;
        public RWFilterMode FilterMode;
        public RWTextureAddressingMode UAddressing;
        public RWTextureAddressingMode VAddressing;
        public string Name;
        public string MaskName;

        public RWRasterFormat RasterFormat;
        public uint HasAlpha; // TODO: not on GTA:SA (it uses D3DFORMAT here)
        public int Width;
        public int Height;
        public int Depth;
        public int NumLevels;
        public int RasterType;
        public int Compression; // TODO: GTA:SA uses more complex structure here
        public RWColor[,] TextureData;
    }
    public class RWTextureDictionary {
        public RWTextureNativePC[] TexturesPC;
    }
    public class RWIMGDirectoryEntry {
        public int Offset; // in 2048 byte sectors
        public int Size; // in 2048 byte sectors
        public string Name; // max 24 bytes (including null terminator)
    }
    public class RWIMGDirectory {
        public RWIMGDirectoryEntry[] Entries;
    }

    public class RWViceCityIdeObjEntry {
        public int ID;
        public string ModelName;
        public string TextureName;
        public float[] DrawDistances = new float[0];
        public uint Flags;

    }
    public class RWViceCityIde {
        public List<RWViceCityIdeObjEntry> Objs = new List<RWViceCityIdeObjEntry>();
    }

    public class RWViceCityIplInstEntry {
        public int ID;
        public string ModelName;
        public int Interior;
        public RWVector3 Position;
        public RWVector3 Scale;
        public RWVector4 Rotation; // quaternion!
    }

    public class RWViceCityIpl {
        public List<RWViceCityIplInstEntry> Inst = new List<RWViceCityIplInstEntry>();
    }
}