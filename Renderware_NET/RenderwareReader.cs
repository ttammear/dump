using System;
using System.IO;
using System.Diagnostics;
using System.Collections.Generic;
using System.Text;

namespace RenderwareNet {
    class DXT1Decompressor {

        public enum ColorFormat
        {
            COLOR_RGB565,
            COLOR_RGB4444,
        }

        public static RWColor[,] DecompressDXT3Block(BinaryReader br) {
            RWColor[,] ret = new RWColor[4,4];
            ulong alphaData = br.ReadUInt64();
            ushort c0 = br.ReadUInt16();
            ushort c1 = br.ReadUInt16();
            uint lookup = br.ReadUInt32();
            RWColor[] c = new RWColor[4];
            c[0] = RWColor.FromRGB565(c0);
            c[1] = RWColor.FromRGB565(c1);
            c[2] = ((c[0]/3)*2+c[1]/3);
            c[3] = (c[0]/3+(c[1]/3)*2);
            int offset = 0;
            for(int y = 0; y < 4; y++) {
                for(int x = 0; x < 4; x++, offset+=4) {
                    ret[y,x] = c[(lookup>>(offset>>1))&0x3];
                    byte alpha = (byte)((alphaData>>offset)&0xF);
                    alpha = (byte)((alpha<<4)|alpha);
                    ret[y,x].a = alpha;
                }
            }
            return ret;
        }

        public static RWColor[,] DecompressDXT1Block(BinaryReader br, ColorFormat f) {
            RWColor[,] ret = new RWColor[4,4];
            ushort c0 = br.ReadUInt16();
            ushort c1 = br.ReadUInt16();
            uint lookup = br.ReadUInt32();
            RWColor[] c = new RWColor[4];

            c[0] = RWColor.FromRGB565(c0);
            c[1] = RWColor.FromRGB565(c1);

            if(c0 > c1) {
                c[2] = ((c[0]/3)*2+c[1]/3);
                c[2].a = 255;
                c[3] = (c[0]/3+(c[1]/3)*2);
                c[3].a = 255;
            } else {
                c[2] = (c[0]/2+c[1]/2);
                c[3] = new RWColor(0, 0, 0, 0); 
            }
            if(f == ColorFormat.COLOR_RGB565)
            {
                for(int i = 0; i < 3; i++)
                {
                    c[i].a = 255;
                }
            }
            ret[0,0] = c[lookup&0x3];
            ret[0,1] = c[(lookup>> 2)&0x3];
            ret[0,2] = c[(lookup>> 4)&0x3];
            ret[0,3] = c[(lookup>> 6)&0x3];
            ret[1,0] = c[(lookup>> 8)&0x3];
            ret[1,1] = c[(lookup>>10)&0x3];
            ret[1,2] = c[(lookup>>12)&0x3];
            ret[1,3] = c[(lookup>>14)&0x3];
            ret[2,0] = c[(lookup>>16)&0x3];
            ret[2,1] = c[(lookup>>18)&0x3];
            ret[2,2] = c[(lookup>>20)&0x3];
            ret[2,3] = c[(lookup>>22)&0x3];
            ret[3,0] = c[(lookup>>24)&0x3];
            ret[3,1] = c[(lookup>>26)&0x3];
            ret[3,2] = c[(lookup>>28)&0x3];
            ret[3,3] = c[(lookup>>30)&0x3];
            return ret;
        }

        public static RWColor[,] DecompressDXT1(int width, int height, byte[] data, DXT1Decompressor.ColorFormat f) {
            BinaryReader br = new BinaryReader(new MemoryStream(data));
            RWColor[,] ret = new RWColor[width, height];
            for(int iy = 0; iy < height/4; iy++) {
                for(int ix = 0; ix < width/4; ix++) {
                    int x = ix*4;
                    int y = iy*4;
                    var block = DecompressDXT1Block(br, f);
                    for(int by = 0; by < 4; by++) 
                    for(int bx = 0; bx < 4; bx++) 
                    {
                        ret[x+bx,y+by] = block[by,bx];
                    }
                }
            }
            return ret;
        }

        public static RWColor[,] DecompressDXT3(int width, int height, byte[] data) {
            BinaryReader br = new BinaryReader(new MemoryStream(data));
            RWColor[,] ret = new RWColor[width, height];
            for(int iy = 0; iy < height/4; iy++) {
                for(int ix = 0; ix < width/4; ix++) {
                    int x = ix*4;
                    int y = iy*4;
                    var block = DecompressDXT3Block(br);
                    for(int by = 0; by < 4; by++) 
                    for(int bx = 0; bx < 4; bx++) 
                    {
                        ret[x+bx,y+by] = block[by,bx];
                    }
                }
            }
            return ret;
        }
    }

    public static class RenderwareReader {
        public const int RENDERWARE_SECTOR_SIZE = 2048;
        class ChunkHeader
        {
            private BinaryReader _br;
            private long _startPos;
            public uint type;
            public uint size;
            public uint libraryId;
            public string versionString;
            public uint version;

            static uint renderwareVersionUnpack(uint value)
            {
                return ((value >> 15) & 0x3FF00) + 0x30000 | (value >> 16 & 0x3F);
            }
            static string renderwareVersionToString(UInt32 upper)
            {
                return $"{(upper >> 16) & 0xF}.{(upper >> 12) & 0xF}.{(upper >> 4) & 0xFF}.{upper & 0xF}";
            }

            public ChunkHeader(BinaryReader br)
            {
                _br = br;
                if(br.BaseStream.Length - br.BaseStream.Position < 12) {
                    type = 0;
                    size = 0;
                    libraryId = 0xFFFFFFFF;
                    version = 0xFFFFFFFF;
                    versionString = "N/A";
                    _startPos = br.BaseStream.Position;
                    Console.WriteLine("ChunkHeader but stream ended!");
                } else {
                    type = br.ReadUInt32();
                    size = br.ReadUInt32();
                    libraryId = br.ReadUInt32();
                    version = renderwareVersionUnpack(libraryId);
                    versionString = renderwareVersionToString(version);
                    _startPos = br.BaseStream.Position;

                    Console.WriteLine($"type {type:X} size {size} ver {version:X}");
                }
            }

            public void Skip() {
                //binaryReader.ReadBytes((int)size);
                _br.BaseStream.Position += size - (_br.BaseStream.Position-_startPos);
            }

            public string ReadString() {
                Debug.Assert(type == (int)RenderwareNet.RWChunkId.String);
                var ret = System.Text.Encoding.Default.GetString(_br.ReadBytes((int)size));
                ret = ret.Substring(0, ret.IndexOf('\0'));
                return ret;
            }
        }

        static RWTextureNativePC ParseTexDictionaryStructOldPC(BinaryReader br) // 3.0.X.X - 3.5.0.0 PC
        {
            var texn = new RWTextureNativePC();

            var texDicChunk = new ChunkHeader(br);
            Debug.Assert(texDicChunk.type == 0x15);
            var chunk = new ChunkHeader(br);
            Debug.Assert(chunk.type == 0x1);
            // texture format
            texn.PlatformId = br.ReadUInt32(); // 8 for GTA3 and VC, 9 for SA, "PS2\0" on PS2, 5 for XBOX
            var filterData = br.ReadUInt32();
            var filterMode = (RWFilterMode)((filterData >> 24)&0xFF);
            texn.UAddressing = (RWTextureAddressingMode)((filterData >> 20) & 0xF);
            texn.VAddressing = (RWTextureAddressingMode)((filterData >> 16) & 0xF);
            var name = System.Text.Encoding.Default.GetString(br.ReadBytes(32), 0, 32);
            texn.Name = name.Substring(0, name.IndexOf('\0'));
            var maskName = System.Text.Encoding.Default.GetString(br.ReadBytes(32), 0, 32);
            texn.MaskName = maskName.Substring(0, maskName.IndexOf('\0'));
            var rasterFormatData = br.ReadUInt32();
            var rasterExt = (RWRasterFormatExtension)(rasterFormatData & 0xF000);
            var rasterFormat = (RWRasterFormat)((rasterFormatData)&0x0FFF);
            
            // NOTE: for GTA:SA this is D3DFORMAT
            var hasAlpha = br.ReadUInt32();
            texn.Width = br.ReadUInt16();
            texn.Height = br.ReadUInt16();
            texn.Depth = br.ReadByte();
            texn.NumLevels = br.ReadByte();
            texn.RasterType = br.ReadByte();
            // NOTE: this is for GTA3 & VC, for GTA:SA [alpha:1, cubeTexture:1, autoMipMaps:1, compressed:1, pad:4]
            var compression = br.ReadByte();
            texn.Compression = compression;
            if(compression == 1 && rasterFormat == RWRasterFormat.FORMAT_1555) {
                rasterFormat = RWRasterFormat.FORMAT_DXT1_ALPHA;
            } else if(compression == 1 && rasterFormat == RWRasterFormat.FORMAT_565) {
                rasterFormat = RWRasterFormat.FORMAT_DXT1_NO_ALPHA;
            } else if(compression == 3 && rasterFormat == RWRasterFormat.FORMAT_4444) {
                rasterFormat = RWRasterFormat.FORMAT_DXT3;
            }
            texn.RasterFormat = rasterFormat;
            Console.WriteLine($"{texn.Name} format {rasterFormat} ext {rasterExt} {texn.Width}x{texn.Height} depth {texn.Depth} levels {texn.NumLevels} rasterType {texn.RasterType} compression {compression}");

            var size = br.ReadUInt32();
            // has palette?
            if((rasterExt & (RWRasterFormatExtension.FORMAT_EXT_PAL8 | RWRasterFormatExtension.FORMAT_EXT_PAL4)) == 0) {
                var textureData = br.ReadBytes((int)size);
                Console.WriteLine($"Size ? {size}");

                RWColor[,] imageData = null;
                if(rasterFormat == RWRasterFormat.FORMAT_DXT1_ALPHA) {
                    imageData = DXT1Decompressor.DecompressDXT1(texn.Width, texn.Height, textureData, DXT1Decompressor.ColorFormat.COLOR_RGB4444);
                } else if(rasterFormat == RWRasterFormat.FORMAT_DXT1_NO_ALPHA)
                {
                    imageData = DXT1Decompressor.DecompressDXT1(texn.Width, texn.Height, textureData, DXT1Decompressor.ColorFormat.COLOR_RGB565);
                } else if(rasterFormat == RWRasterFormat.FORMAT_DXT3) {
                    imageData = DXT1Decompressor.DecompressDXT3(texn.Width, texn.Height, textureData);
                } else if(rasterFormat == RWRasterFormat.FORMAT_888) {
                    imageData = RWColor.From888(textureData, texn.Width, texn.Height);
                }
                texn.TextureData = imageData;
            } else if((rasterExt & RWRasterFormatExtension.FORMAT_EXT_PAL8) != 0) {
                RWColor[] palette = new RWColor[256];
                for(int pcol = 0; pcol < 256; pcol++) {
                    palette[pcol] = new RWColor(){r = br.ReadByte(), g = br.ReadByte(), b = br.ReadByte(), a = br.ReadByte()};
                }
                RWColor[,] imageData = new RWColor[texn.Width, texn.Height];
                for(int imgY = 0; imgY < texn.Height; imgY ++) {
                    for(int imgX = 0; imgX < texn.Width; imgX ++) {
                        byte idx = br.ReadByte();
                        imageData[imgX, imgY] = palette[idx];
                    }
                }
                texn.TextureData = imageData;
            } else {
                throw new NotImplementedException();
            }
            
            
            chunk = new ChunkHeader(br);
            //Debug.Assert(chunk.type == 0x3); // extension
            texDicChunk.Skip();
            
            return texn;
        }

        public static RWTextureDictionary ParseTexDictionaryOld(BinaryReader br) // 3.0.X.X - 3.5.0.0
        {
            var chunk = new ChunkHeader(br);
            Debug.Assert(chunk.type == (int)RWChunkId.Texture_Dictionary);
            var dict = new RWTextureDictionary();
            chunk = new ChunkHeader(br);
            Debug.Assert(chunk.type == 0x1);
            var texCount = br.ReadUInt32();
            //var deviceId = br.ReadUInt16();
            Console.WriteLine($"texture Count {texCount:X}");
            var tlist = new List<RWTextureNativePC>();
            for(int i = 0; i < texCount; i++)
            {
                var tpc = ParseTexDictionaryStructOldPC(br);
                tlist.Add(tpc);
            }
            dict.TexturesPC = tlist.ToArray();
            return dict;
        }

        private static void skipExtension(BinaryReader binaryReader) {
            ChunkHeader chunk;
            do {
                chunk = new ChunkHeader(binaryReader);
                if(chunk.type == (int)RWChunkId.Extension) {
                    ChunkHeader schunk;
                    do {
                        schunk = new ChunkHeader(binaryReader);
                        if(schunk.type == (int)RWChunkId.Struct) {
                            schunk.Skip();
                        } else {
                            binaryReader.BaseStream.Seek(-12, SeekOrigin.Current);
                        }
                    } while(binaryReader.BaseStream.Position != binaryReader.BaseStream.Length 
                        && schunk.type == (int)RWChunkId.Struct);
                } else {
                    binaryReader.BaseStream.Seek(-12, SeekOrigin.Current);
                }
            } while(chunk.type == (int)RWChunkId.Extension);
        }

        public static RWClump ParseClump(BinaryReader br) {
            var ret = new RWClump();

            var chunk = new ChunkHeader(br);
            Debug.Assert(chunk.type == 0x10);
            chunk = new ChunkHeader(br);
            Debug.Assert(chunk.type == 0x1); //struct
            ret.NumAtomics = br.ReadInt32();
            Console.WriteLine($"numAtomics {ret.NumAtomics}");
            if(chunk.version > 0x31000) { // TODO: wiki says it should be 33000, but build 31802 in vice city seems to have these (wheels.TXD)
                ret.NumLights = br.ReadInt32();
                ret.NumCameras = br.ReadInt32();
                Console.WriteLine($"numLights {ret.NumLights} numCameras {ret.NumCameras}");
            }
            chunk = new ChunkHeader(br);
            Debug.Assert(chunk.type == (int)RWChunkId.Frame_List);
            chunk = new ChunkHeader(br);
            Debug.Assert(chunk.type == (int)RWChunkId.Struct);
            ret.FrameList = new List<RWFrame>();
            uint numFrames = br.ReadUInt32();
            Console.WriteLine($"numFrames {numFrames}");
            for(int i = 0; i < numFrames; i++) {
                var frame = new RWFrame();
                frame.RotationMatrix.M1.X = br.ReadSingle();
                frame.RotationMatrix.M1.Y = br.ReadSingle();
                frame.RotationMatrix.M1.Z = br.ReadSingle();
                frame.RotationMatrix.M2.X = br.ReadSingle();
                frame.RotationMatrix.M2.Y = br.ReadSingle();
                frame.RotationMatrix.M2.Z = br.ReadSingle();
                frame.RotationMatrix.M3.X = br.ReadSingle();
                frame.RotationMatrix.M3.Y = br.ReadSingle();
                frame.RotationMatrix.M3.Z = br.ReadSingle();
                frame.Position.X = br.ReadSingle();
                frame.Position.Y = br.ReadSingle();
                frame.Position.Z = br.ReadSingle();
                //Console.WriteLine($"pos {frame.Position.X} {frame.Position.Y} {frame.Position.Z}");
                frame.FrameIndex = br.ReadInt32();
                frame.MatrixFlags = br.ReadUInt32();
                ret.FrameList.Add(frame);
            }
            do {
                // TODO: any useful data here?
                chunk = new ChunkHeader(br);
                if(chunk.type == (int)RWChunkId.Extension) {
                    chunk.Skip();
                }
            } while(chunk.type == (int)RWChunkId.Extension);
            Debug.Assert(chunk.type == (int)RWChunkId.Geometry_List);
            chunk = new ChunkHeader(br);
            Debug.Assert(chunk.type == (int)RWChunkId.Struct);
            uint numGeometries = br.ReadUInt32();
            Console.WriteLine($"numGeometries {numGeometries}");
            ret.GeometryList = new List<RWGeometry>();
            for(int i = 0; i < numGeometries; i++) {
                var geometry = new RWGeometry();

                var geometryChunk = new ChunkHeader(br);
                Debug.Assert(geometryChunk.type == (int)RWChunkId.Geometry);
                var ofst1 = br.BaseStream.Position;
                
                chunk = new ChunkHeader(br);
                Debug.Assert(chunk.type == (int)RWChunkId.Struct);
                
                geometry.Format = br.ReadUInt32();
                int numTriangles = br.ReadInt32();
                int numVertices = br.ReadInt32();
                int numMorphTargets = br.ReadInt32();
                int numTextureSets = (int)(geometry.Format>>16)&0xFF;
                Console.WriteLine($"format {geometry.Format:X} numTriangles {numTriangles} numVertices {numVertices} numMorphTargets {numMorphTargets} numTextureSets {numTextureSets}");
                 if(geometryChunk.version < 0x32000) {
                    geometry.SurfaceAmbient = br.ReadSingle();
                    geometry.SurfaceSpecular = br.ReadSingle();
                    geometry.SurfaceDiffuse = br.ReadSingle();
                    Console.WriteLine($"ambient {geometry.SurfaceAmbient} specular {geometry.SurfaceSpecular} diffuse {geometry.SurfaceDiffuse}");
                }
                if((geometry.Format & (uint)RWGeometryFormatFlag.NATIVE) == 0) {
                    if((geometry.Format & (uint)RWGeometryFormatFlag.PRELIT) != 0) {
                        geometry.PrelitColors = new uint[numVertices];
                        for(int cidx = 0; cidx < numVertices; cidx++) {
                            geometry.PrelitColors[i] = br.ReadUInt32();
                        }
                    }
                    if(numTextureSets == 0) {
                        // use flags
                        bool tex1 = (geometry.Format & (uint)RWGeometryFormatFlag.TEXTURE_COORD1) != 0;
                        bool tex2 = (geometry.Format & (uint)RWGeometryFormatFlag.TEXTURE_COORD2) != 0;
                        if(tex1 && !tex2) {
                            geometry.TextureCoords = new RWVector2[1, numVertices];
                        } else if(tex2) {
                            geometry.TextureCoords = new RWVector2[2, numVertices];
                        }
                        if(tex1 || tex2) { // TODO: should texture1 exist when tex1 == 0?
                            // read texture coord 1
                            for(int vIdx = 0; vIdx < numVertices; vIdx++) {
                                geometry.TextureCoords[0, vIdx] = new RWVector2{X = br.ReadSingle(), Y = br.ReadSingle()};
                                //Console.WriteLine($"t1 {vIdx} {texBuf1[vIdx, 0]} {texBuf1[vIdx, 1]}");
                            }
                        }
                        if(tex2) {
                            // read texture coord 2
                            for(int vIdx = 0; vIdx < numVertices; vIdx++) {
                                geometry.TextureCoords[1, vIdx] = new RWVector2{X = br.ReadSingle(), Y = br.ReadSingle()};
                                //Console.WriteLine($"t2 {vIdx} {texBuf2[vIdx, 0]} {texBuf2[vIdx, 1]}");
                            }
                        }
                    } else {
                        geometry.TextureCoords = new RWVector2[numTextureSets, numVertices];
                        for(int tIdx = 0; tIdx < numTextureSets; tIdx++) {
                            // read texture coord set tIdx
                            for(int vIdx = 0; vIdx < numVertices; vIdx++) {
                                geometry.TextureCoords[tIdx, vIdx] = new RWVector2{X = br.ReadSingle(), Y = br.ReadSingle()};
                                //Console.WriteLine($"t{tIdx+1} {vIdx} {texBuf[tIdx, vIdx, 0]} {texBuf[tIdx, vIdx, 1]}");
                            }
                        }
                    }
                    geometry.Triangles = new RWTriangle[numTriangles];
                    for(int triIdx = 0; triIdx < numTriangles; triIdx++) {
                        ushort v2 = br.ReadUInt16();
                        ushort v1 = br.ReadUInt16();
                        ushort materialId = br.ReadUInt16();
                        ushort v3 = br.ReadUInt16();
                        geometry.Triangles[triIdx] = new RWTriangle{V1 = v1, V2 = v2, V3 = v3, MaterialId = materialId};
                        //Console.WriteLine($"i {triIdx} {v1} {v2} {v3} {materialId}");
                    }
                    geometry.MorphTargets = new RWMorphTarget[numMorphTargets];
                    for(int morphIdx = 0; morphIdx < numMorphTargets; morphIdx++) {
                        float boundX = br.ReadSingle();
                        float boundY = br.ReadSingle();
                        float boundZ = br.ReadSingle();
                        float boundRadius = br.ReadSingle();
                        bool hasVertices = br.ReadUInt32() != 0;
                        bool hasNormals = br.ReadUInt32() != 0;
                        RWVector3[] vertices = null;
                        if(hasVertices) {
                            vertices = new RWVector3[numVertices];
                            for(int vidx = 0; vidx < numVertices; vidx++) {
                                vertices[vidx].X = br.ReadSingle();
                                vertices[vidx].Y = br.ReadSingle();
                                vertices[vidx].Z = br.ReadSingle();
                            }
                        }
                        RWVector3[] normals = null;
                        if(hasNormals) {
                            normals = new RWVector3[numVertices];
                            for(int vidx = 0; vidx < numVertices; vidx++) {
                                normals[vidx].X = br.ReadSingle();
                                normals[vidx].Y = br.ReadSingle();
                                normals[vidx].Z = br.ReadSingle();
                            }
                        }
                        Console.WriteLine($"morph target {morphIdx} boundSphere ({boundX}, {boundY}, {boundZ}, {boundRadius}) hasVertices {hasVertices} hasNormals {hasNormals}");
                        geometry.MorphTargets[morphIdx] = new RWMorphTarget{
                            BoundingSphere = new RWSphere{Position = new RWVector3{X = boundX, Y = boundY, Z = boundZ}, Radius = boundRadius},
                            HasVertices = hasVertices,
                            HasNormals = hasNormals,
                            Vertices = vertices,
                            Normals = normals,
                        };
                    }
                }

                // skip extensions
                /*do {
                    // TODO: any useful data here?
                    chunk = new ChunkHeader(br);
                    if(chunk.type == (int)RWChunkId.Extension) {
                        chunk.Skip();
                    }
                } while(chunk.type == (int)RWChunkId.Extension);*/

                // Read material list
                chunk = new ChunkHeader(br);
                Debug.Assert(chunk.type == (int)RWChunkId.Material_List);
                chunk = new ChunkHeader(br);
                Debug.Assert(chunk.type == (int)RWChunkId.Struct);
                uint numMaterials = br.ReadUInt32();
                int realMatCount = 0;
                var mats = new RWMaterial[numMaterials];
                var realMats = new List<RWMaterial>();
                for(int j = 0; j < numMaterials; j++) {
                    int matIdx = br.ReadInt32();
                    if(matIdx == -1) {
                        var newMat = new RWMaterial();
                        realMats.Add(newMat);
                        mats[j] = newMat;
                        realMatCount++;
                    } else {
                        mats[j] = realMats[matIdx];
                    }
                }
                ret.Materials = mats;
                for(int j = 0; j < realMatCount; j++) {
                    var mat = realMats[j];
                    var matChunk = new ChunkHeader(br);
                    Debug.Assert(matChunk.type == (int)RWChunkId.Material);
                    chunk = new ChunkHeader(br);
                    Debug.Assert(chunk.type == (int)RWChunkId.Struct);
                    mat.Flags = br.ReadUInt32();
                    mat.Color = br.ReadUInt32();
                    /*uint pad = */br.ReadUInt32();
                    bool isTextured = br.ReadUInt32() != 0;
                    if(chunk.version > 0x30400) {
                        mat.SurfaceAmbient = br.ReadSingle();
                        mat.SurfaceSpecular = br.ReadSingle();
                        mat.SurfaceDiffuse = br.ReadSingle();
                    }
                    if(isTextured) {
                        var mtex = new RWMaterialTexture();
                        
                        var texChunk = new ChunkHeader(br);
                        Debug.Assert(texChunk.type == (int)RWChunkId.Texture);
                        texChunk = new ChunkHeader(br);
                        Debug.Assert(texChunk.type == (int)RWChunkId.Struct);
                        
                        mtex.Filtering = (RWFilterMode)br.ReadByte();
                        byte addressing = br.ReadByte(); // U and V 4bit
                        mtex.UAddressing = (RWTextureAddressingMode)((addressing >> 4)&0xF);
                        mtex.VAddressing = (RWTextureAddressingMode)(addressing&0xF);
                        // TODO: is this right?
                        ushort mipUsed = br.ReadUInt16();
                        mtex.MipmapsUsed = (mipUsed&1) != 0;
                        var schunk = new ChunkHeader(br);
                        mtex.TextureName = schunk.ReadString();
                        schunk = new ChunkHeader(br);
                        mtex.AlphaTextureName = schunk.ReadString();
                        Console.WriteLine($"material tex {mtex.TextureName} atex {mtex.AlphaTextureName}");
                        mat.Texture = mtex;
                    }
                    matChunk.Skip();
                }
                Console.WriteLine($"numMaterials {numMaterials} present {realMatCount}");
                geometryChunk.Skip();
                ret.GeometryList.Add(geometry);  
            }
            ret.Atomics = new RWAtomic[ret.NumAtomics];
            for(int i = 0; i < ret.NumAtomics; i++) {
                chunk = new ChunkHeader(br);
                ret.Atomics[i] = new RWAtomic();
                Debug.Assert(chunk.type == (int)RWChunkId.Atomic);
                var struc = new ChunkHeader(br);
                Debug.Assert(struc.type == (int)RWChunkId.Struct);
                ret.Atomics[i].FrameIndex = br.ReadUInt32();
                ret.Atomics[i].GeometryIndex = br.ReadUInt32();
                ret.Atomics[i].Flags = br.ReadUInt32();
                br.ReadUInt32(); // unused
                chunk.Skip();
            }
            do { // TODO: what are these?
                chunk = new ChunkHeader(br);
                if(chunk.type == (int)RWChunkId.Struct) {
                    chunk.Skip();
                } else { // rewind
                    br.BaseStream.Seek(-12, SeekOrigin.Current);
                }
            } while(chunk.type == (int)RWChunkId.Struct);
            for(int i = 0; i < ret.NumLights; i++) {
                chunk = new ChunkHeader(br);
                Debug.Assert(chunk.type == (int)RWChunkId.Light);
                chunk = new ChunkHeader(br);
                Debug.Assert(chunk.type == (int)RWChunkId.Struct);
                chunk.Skip();
                skipExtension(br);
            }
            for(int i = 0; i < ret.NumCameras; i++) {
                chunk = new ChunkHeader(br);
                Debug.Assert(chunk.type == (int)RWChunkId.Camera);
                chunk = new ChunkHeader(br);
                Debug.Assert(chunk.type == (int)RWChunkId.Struct);
                chunk.Skip();
                skipExtension(br);
            }
            skipExtension(br);
            return ret;
        }

        private static RWIMGDirectoryEntry[] readIMGDirectoryFile(string directoryFilePath) {
            try {
                using(var fs = new FileStream(directoryFilePath, FileMode.Open)) {
                    var br = new BinaryReader(fs);
                    int count = (int)fs.Length/32;
                    var ret = new RWIMGDirectoryEntry[fs.Length/32];
                    for(int i = 0; i < count; i++) {
                        ret[i] = new RWIMGDirectoryEntry();
                        ret[i].Offset = br.ReadInt32();
                        ret[i].Size = br.ReadInt32();
                        var name = System.Text.Encoding.Default.GetString(br.ReadBytes(24), 0, 24);
                        ret[i].Name = name.Substring(0, name.IndexOf('\0'));
                        Console.WriteLine($"Entry {i} {ret[i].Offset} {ret[i].Size} {ret[i].Name}");
                    }
                    return ret;
                }
            } catch {
                return null;
            }
        }

        public static RWIMGDirectory ParseIMGDirectory(string filePath) {
            try {
                using(var fs = new FileStream(filePath, FileMode.Open)) {
                    string s = "VER2";
                    byte[] bytes = Encoding.ASCII.GetBytes(s);  /* Use the correct encoding here. */
                    int ver2Int = BitConverter.ToInt32(bytes, 0);
                    var br = new BinaryReader(fs);
                    var fileMagicValue = br.ReadUInt32();
                    if(fileMagicValue == ver2Int) {
                        throw new NotImplementedException("Version 2 .img not implemented!");
                    } else if(fileMagicValue == 0xA94E2A52) {
                        throw new NotImplementedException("GTA IV .img not implemented!");
                    } else {
                        var fileName = Path.GetFileName(filePath);
                        var fileDirectory = Path.GetDirectoryName(filePath);
                        var dirFilePath = Path.Combine(fileDirectory, Path.GetFileNameWithoutExtension(fileName)+".dir");
                        Console.WriteLine($"File {fileName} Directory {fileDirectory} DIR {dirFilePath}");
                        return new RWIMGDirectory() { Entries = readIMGDirectoryFile(dirFilePath)};
                    }
                }
            } catch (Exception e) {
                Console.WriteLine($"Failed to open {filePath} Exception: {e.ToString()}");
                return null;
            }
        }

        public static RWViceCityIdeObjEntry ParseIdeObjEntry(string line) {
            var ret = new RWViceCityIdeObjEntry();
            var split = line.Split(',');
            ret.ID = int.Parse(split[0]);
            ret.ModelName = split[1].Trim();
            ret.TextureName = split[2].Trim();
            int modelCount = int.Parse(split[3]);
            Debug.Assert(modelCount <= 3); // probably variant 4 (GTA SA) which isn't dealt with
            ret.DrawDistances = new float[modelCount];
            for(int i = 0; i < modelCount; i++) {
                ret.DrawDistances[i] = float.Parse(split[4+i]);
            }
            ret.Flags = uint.Parse(split[4+modelCount]);
            Console.WriteLine($"Ide: {ret.ID}, {ret.ModelName}, {ret.TextureName}, {ret.DrawDistances.Length}, {ret.Flags}");
            return ret;
        }

        public static RWViceCityIde ParseItemDefinitions(string filePath) {
            var ret = new RWViceCityIde();
            int mode = 0;
            try {
                using(var fs = new FileStream(filePath, FileMode.Open)) {
                    var tr = new StreamReader(fs);
                    string line;
                    do {
                        line = tr.ReadLine();
                        if(line == null) {
                            continue;
                        }
                        switch(line.Trim()) {
                            case "objs":
                            mode = 1;
                            break;
                            case "end":
                            mode = 0;
                            break;
                            default:
                            if(mode == 1) {
                                var obj = ParseIdeObjEntry(line);
                                if(obj != null) {
                                    ret.Objs.Add(obj);
                                }
                            }
                            break;
                        }
                    } while(line != null);
                }
            }  catch (FileNotFoundException){
                Console.WriteLine("IDE file not found " + filePath);
            } catch (IOException) {
                Console.WriteLine("IDE IO failure " + filePath);
            }
            return ret;
        }

        public static RWViceCityIplInstEntry ParseIplInstEntry(string line) {
            var ret = new RWViceCityIplInstEntry();
            var split = line.Split(',');
            if(split.Length < 13) {
                Console.WriteLine("Invalid ipl inst entry, 13 columns expected!");
                return null;
            }
            ret.ID = int.Parse(split[0]);
            ret.ModelName = split[1].Trim();
            ret.Interior = int.Parse(split[2]);
            ret.Position.X = float.Parse(split[3]);
            ret.Position.Y = float.Parse(split[4]);
            ret.Position.Z = float.Parse(split[5]);
            ret.Scale.X = float.Parse(split[6]);
            ret.Scale.Y = float.Parse(split[7]);
            ret.Scale.Z = float.Parse(split[8]);
            ret.Rotation.X = float.Parse(split[9]);
            ret.Rotation.Y = float.Parse(split[10]);
            ret.Rotation.Z = float.Parse(split[11]);
            ret.Rotation.W = float.Parse(split[12]);
            Console.WriteLine($"inst {ret.ID}, {ret.ModelName}, {ret.Interior}, {ret.Position.X}, {ret.Position.Y}, {ret.Position.Z}");
            return ret;
        }

        public static RWViceCityIpl ParseItemPlacements(string filePath) {
            var ret = new RWViceCityIpl();
            int mode = 0;
            try {
                using(var fs = new FileStream(filePath, FileMode.Open)) {
                    var tr = new StreamReader(fs);
                    string line;
                    do {
                        line = tr.ReadLine();
                        if(line == null) {
                            continue;
                        }
                        switch(line.Trim()) {
                            case "inst":
                            mode = 1;
                            break;
                            case "end":
                            mode = 0;
                            break;
                            default:
                            if(mode == 1) {
                                var inst = ParseIplInstEntry(line);
                                if(inst != null) {
                                    ret.Inst.Add(inst);
                                }
                            }
                            break;
                        }
                    } while(line != null);
                }
            } catch (FileNotFoundException){
                Console.WriteLine("IPL file not found " + filePath);
            } catch (IOException) {
                Console.WriteLine("IPL IO failure " + filePath);
            }
            return ret;
        }

        public static object ParseEntry(RWIMGDirectoryEntry entry, FileStream fs) {
            var extension = Path.GetExtension(entry.Name).ToLower();
            if(extension == ".dff") {
                fs.Seek(entry.Offset*RENDERWARE_SECTOR_SIZE, SeekOrigin.Begin);
                var br = new BinaryReader(fs);
                return ParseClump(br);  
            } else if(extension == ".txd") {
                fs.Seek(entry.Offset*RENDERWARE_SECTOR_SIZE, SeekOrigin.Begin);
                var br = new BinaryReader(fs);
                return ParseTexDictionaryOld(br);
            } else {
                throw new NotImplementedException($"Extension {extension} not implemented!");
            }
        }
    }
}