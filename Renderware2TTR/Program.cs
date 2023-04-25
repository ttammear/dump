using System;
using RenderwareNet;
using TTRNet;
using System.IO;
using System.Diagnostics;
using ImageMagick;
using System.Collections.Generic;

namespace Renderware2TTR
{
    class TempSection {
        public List<int> indices = new List<int>();
    };
    struct TempVertex {
        public RWVector3 Vertex;
        public RWVector2 TexCoord;
        public RWVector3 Normal;
    }
    class Program
    {
        private static TTRCollider colliderFromCollData(CollData data, TTRHeader ttr) {
            TTRCollider coll = new TTRCollider();

            var stream = new MemoryStream();
            var vbuf = new BinaryWriter(stream);
            for(int i = 0; i < data.vertices1.Length; i++) {
                var v = data.vertices1[i];
                vbuf.Write(v.x);
                vbuf.Write(v.y);
                vbuf.Write(v.z);
            }
            coll.VertexBuffer = new TTRBuffer() {
                Data = stream.ToArray()
            };

            vbuf.Seek(0, SeekOrigin.Begin);
            for(int i = 0; i < data.facesVer1.Length; i++) {
                var f = data.facesVer1[i];
                vbuf.Write(f.a);
                vbuf.Write(f.b);
                vbuf.Write(f.c);
            }
            coll.IndexBuffer = new TTRBuffer() {
                Data = stream.ToArray()
            };

            vbuf.Seek(0, SeekOrigin.Begin);
            for(int i = 0; i < data.spheres.Length; i++) {
                var s = data.spheres[i];
                vbuf.Write(s.center.x);
                vbuf.Write(s.center.y);
                vbuf.Write(s.center.z);
                vbuf.Write(s.radius);
            }
            coll.SphereBuffer = new TTRBuffer() {
                Data = stream.ToArray()
            };

            vbuf.Seek(0, SeekOrigin.Begin);
            for(int i = 0; i < data.boxes.Length; i++) {
                var b = data.boxes[i];
                vbuf.Write(b.min.x);
                vbuf.Write(b.min.y);
                vbuf.Write(b.min.z);
                vbuf.Write(b.max.x);
                vbuf.Write(b.max.y);
                vbuf.Write(b.max.z);
            }
            coll.BoxBuffer = new TTRBuffer() {
                Data = stream.ToArray()
            };

            coll.numVertices = (uint)data.vertices1.Length;
            coll.numIndices = (uint)data.facesVer1.Length;
            coll.numSpheres = (uint)data.spheres.Length;
            coll.numBoxes = (uint)data.boxes.Length;

            stream.Close();
            stream.Dispose();

            return coll;
        }
        private static TTRMesh meshFromRWClump(RWClump clump, TTRHeader ttr) {
            List<TempVertex> combinedVertices = new List<TempVertex>();
            List<int> combinedIndices = new List<int>();
            var sections = new Dictionary<int, TempSection>(); // key is material
            int vid = 0;
            int iid = 0;

            foreach(var atomic in clump.Atomics) {
                var frame = clump.FrameList[(int)atomic.FrameIndex];
                var geo = clump.GeometryList[(int)atomic.GeometryIndex];
                int voffset = vid;

                for(int i = 0; i < geo.NumVertices; i++, vid++) {
                    combinedVertices.Add(new TempVertex {
                            Vertex = (frame.RotationMatrix*geo.MorphTargets[0].Vertices[i])+frame.Position,
                            TexCoord = geo.TextureCoords.GetLength(0) < 1 ? new RWVector2{X = 0.0f, Y = 0.0f} : geo.TextureCoords[0, i],
                            Normal = geo.MorphTargets[0].HasNormals ? geo.MorphTargets[0].Normals[i] : new RWVector3{X = 0.0f, Y = 0.0f, Z = 0.0f}
                        });
                }
                for(int i = 0; i < geo.Triangles.Length; i++, iid+=3) {
                    var tri = geo.Triangles[i];
                    TempSection sec;
                    if(!sections.TryGetValue(tri.MaterialId, out sec)) {
                        sec = new TempSection();
                        sections.Add(tri.MaterialId, sec);
                        Console.WriteLine("Section " + tri.MaterialId + " " + clump.GeometryList.IndexOf(geo)+ " "+i);
                    }
                    int t1 = tri.V1+voffset;
                    int t2 = tri.V2+voffset;
                    int t3 = tri.V3+voffset;
                    Debug.Assert(t1 <= 65536 && t2 <= 65536 && t3 <= 65536);
                    sec.indices.Add(t1);
                    sec.indices.Add(t2);
                    sec.indices.Add(t3);
                }
                Debug.Assert(vid <= 65536); // 16 bit overflow
            }

            TTRMesh ret = new TTRMesh();
            var meshConf = new TTRMesh.TTRMeshConfiguration();
            meshConf.IndexSize = 2;
            meshConf.Attributes.Add(TTRVertexAttributeType.Vec2); // texture coord
            meshConf.Attributes.Add(TTRVertexAttributeType.Vec4); // color
            meshConf.Attributes.Add(TTRVertexAttributeType.Vec3); // normals
            ret.MeshDescriptor = meshConf;

            ret.NumVertices = (uint)combinedVertices.Count;
            // numindices is set below
            ret.MeshSections = new List<TTRMesh.TTRMeshSection>();

            foreach(var sec in sections) {
                TTRMaterial mat;
                RWMaterial rwmat;
                if(sec.Key < clump.Materials.Length) {
                    rwmat = clump.Materials[sec.Key];
                } else {
                    rwmat = clump.Materials[0];
                    Console.WriteLine("Mesh section material out of range!! Using material 0");
                }
                RWMaterialTexture rwmtex = rwmat.Texture;
                var alpha = rwmat.Texture?.AlphaTextureName;
                if(rwmtex != null && !string.IsNullOrEmpty(alpha?.Trim()))
                {
                    var texName = (rwmtex.TextureName+"_t").ToLower();
                    mat = new TTRMaterial() {
                        ShaderType = TTRShaderType.Unlit_Textured_Cutout,
                        // TODO: use alphatexturename (might not use same texture!)
                        AlbedoTexture = ttr.GetOrCreateImport(true, TTRAssetType.Texture, "ViceCity", texName),
                        TintColor = new TTRVector4() {X = 1, Y = 1, Z = 1, W = 1}
                    };
                } else if(rwmtex != null) {
                    var texName = (rwmtex.TextureName+"_t").ToLower();
                    mat = new TTRMaterial() {
                        ShaderType = TTRShaderType.Unlit_Textured,
                        AlbedoTexture = ttr.GetOrCreateImport(true, TTRAssetType.Texture, "ViceCity", texName),
                        TintColor = new TTRVector4() {X = 1, Y = 1, Z = 1, W = 1}
                    };
                } else {
                   mat = new TTRMaterial() {
                        ShaderType = TTRShaderType.Unlit_Color,
                        TintColor = new TTRVector4() {X = 1, Y = 1, Z = 1, W = 1}
                    };
                }

                int idOffset = combinedIndices.Count*2; // each index 2 bytes!
                int indexCount = sec.Value.indices.Count;
                combinedIndices.AddRange(sec.Value.indices);

                ret.MeshSections.Add(new TTRMesh.TTRMeshSection() {
                    StartIndex = (uint)idOffset,
                    IndexCount = (uint)indexCount,
                    Material = mat
                });
            }
            ret.NumIndices = (uint)combinedIndices.Count;

            // create index buffer
            var ibuf = new TTRBuffer();
            var ms = new MemoryStream();
            var bw = new BinaryWriter(ms);
            /*for(int i = 0; i < geom.Triangles.Length; i++) {
                bw.Write((ushort)geom.Triangles[i].V1);
                bw.Write((ushort)geom.Triangles[i].V2);
                bw.Write((ushort)geom.Triangles[i].V3);
            }*/
            for(int i = 0; i < combinedIndices.Count; i++) {
                bw.Write((ushort)combinedIndices[i]);
            }
            ibuf.Data = ms.ToArray();
            Debug.Assert(ibuf.Data.Length == combinedIndices.Count*2);
            ret.IndexBuffer = ibuf;
            // create vertex buffer
            var vbuf = new TTRBuffer();
            ms.Seek(0, SeekOrigin.Begin);
            bw = new BinaryWriter(ms);
            for(int i = 0; i < combinedVertices.Count; i++) {
                bw.Write(combinedVertices[i].Vertex.X);
                bw.Write(combinedVertices[i].Vertex.Z);
                bw.Write(combinedVertices[i].Vertex.Y);
                bw.Write(1.0f); // TODO: should get rid of this!
                bw.Write(combinedVertices[i].TexCoord.X);
                bw.Write(1.0f-combinedVertices[i].TexCoord.Y);
                bw.Write(1.0f); bw.Write(1.0f); bw.Write(1.0f); bw.Write(1.0f); // TODO: use color from renderware data!
                bw.Write(combinedVertices[i].Normal.X);
                bw.Write(combinedVertices[i].Normal.Z);
                bw.Write(combinedVertices[i].Normal.Y);
            }
            /*for(int i = 0; i < geom.NumVertices; i++) {
                // write position
                var vert = geom.MorphTargets[0].Vertices[i];
                bw.Write(vert.X); bw.Write(vert.Z); bw.Write(vert.Y); bw.Write(1.0f); // gta has Z up, we have y up
                // write texture coord
                RWVector2 texCoord = new RWVector2{X = 0.0f, Y = 0.0f};
                if(geom.TextureCoords.GetLength(0) >= 1) {
                    texCoord = geom.TextureCoords[0, i];
                }
                bw.Write(texCoord.X); bw.Write(texCoord.Y);
                // write color
                bw.Write(1.0f); bw.Write(1.0f); bw.Write(1.0f); bw.Write(1.0f); // TODO: use color from renderware data!
                // write normals
                RWVector3 normal = new RWVector3() {X = 0.0f, Y = 0.0f, Z = 1.0f};
                if(geom.MorphTargets[0].HasNormals) {
                    normal = geom.MorphTargets[0].Normals[i];
                }
                bw.Write(normal.X); bw.Write(normal.Y); bw.Write(normal.Z);
            }*/
            vbuf.Data = ms.ToArray();
            Debug.Assert(vbuf.Data.Length == combinedVertices.Count*52);
            ret.VertexBuffer = vbuf;

            return ret;
        }

        private static TTRTexture textureFromRWTextureNativePC(RWTextureNativePC tex) {
            TTRTexture ret = new TTRTexture();
            ret.Format = TTRTextureFormat.RGBA;
            ret.Width = (uint)tex.Width;
            ret.Height = (uint)tex.Height;
            // TODO: RGBA not BGRA
            ret.Buffer = new TTRBuffer() { Data = RWColor.ConvertToRGBA(tex.TextureData)};
            return ret;
        }

        private static void AddRWTexture(TTRHeader ttr, RWTextureNativePC tex, string name) {
            var ttrTex = textureFromRWTextureNativePC(tex);
            ttr.AddAsset(ttrTex, (name+"_t").ToLower());
        }

        private static void WriteTTR(object asset, string path) {
            if(asset is RWTextureDictionary) {
                TTRHeader ttr = new TTRHeader();
                ttr.MajorVersion = 0;
                ttr.MinorVersion = 1;
                var dic = asset as RWTextureDictionary;
                /*for(int i = 0; i < dic.TexturesPC.Length; i++) {
                    var imageData = dic.TexturesPC[i].TextureData;
                    if (imageData != null)
                    {
                        var imageDataBytes = RWColor.ConvertToBGRA(imageData);
                        var readSettings = new MagickReadSettings();
                        readSettings.Width = imageData.GetLength(0);
                        readSettings.Height = imageData.GetLength(1);
                        readSettings.Format = MagickFormat.Bgra;
                        readSettings.Verbose = true;
                        System.IO.Directory.CreateDirectory("textures");
                        string fileName = "/keep/GameAssets/ViceCityRaw/"+dic.TexturesPC[i].Name+".png";
                        using (MagickImage test = new MagickImage(imageDataBytes, readSettings))
                        {
                            test.Format = MagickFormat.Png;
                            test.Write(fileName);
                            Console.WriteLine($"Wrote {fileName}"); 
                        }
                    }
                }*/
                
                foreach(var tex in dic.TexturesPC) {
                    AddRWTexture(ttr, tex, tex.Name);
                }
                if(ttr.DescriptorTable.Count > 0) {
                    using(var fs = new FileStream(path, FileMode.OpenOrCreate)) {
                        TTRWriter.CreateTTR(ttr, fs);
                        Console.WriteLine($"Wrote texture {path}");
                    }
                }
            } else if(asset is RWClump) {
                var clump = asset as RWClump;
                TTRHeader ttr = new TTRHeader();
                ttr.MajorVersion = 0;
                ttr.MinorVersion = 1;
                // TODO: read ALL geometry
                if(clump.GeometryList.Count > 0) { // TODO: ignores ones without any geometry for now
                    var mesh = meshFromRWClump(clump, ttr);
                    ttr.AddAsset(mesh, Path.GetFileNameWithoutExtension(path).ToLower());
                    if(ttr.DescriptorTable.Count > 0) {
                        using(var fs = new FileStream(path, FileMode.OpenOrCreate)) {
                            TTRWriter.CreateTTR(ttr, fs);
                            Console.WriteLine($"Wrote mesh {path}");
                        }
                    }
                }
            } else if(asset is CollData) {
                var coll = asset as CollData;
                TTRHeader ttr = new TTRHeader();
                ttr.MajorVersion = 0;
                ttr.MinorVersion = 1;
                var tcol = colliderFromCollData(coll, ttr);
                ttr.AddAsset(tcol, Path.GetFileNameWithoutExtension(path).ToLower());
                if(ttr.DescriptorTable.Count > 0) {
                    using(var fs = new FileStream(path, FileMode.OpenOrCreate)) {
                        TTRWriter.CreateTTR(ttr, fs);
                        Console.WriteLine($"Wrote collider {path}");
                    }
                }
            } 
            else {
                Console.WriteLine($"Unknown asset {asset.GetType().ToString()}");
            }
        }

        static TTRHeader ParseMap(Dictionary<string, object> assets, string[] idePaths, string iplPath, string assetName) {
            var ttr = new TTRHeader();

            var map = new TTRMap();

            //var idePath = @"/keep/GameAssets/ViceCity/data/maps/airport/airport.ide";
            //var iplPath = @"/keep/GameAssets/ViceCity/data/maps/airport/airport.ipl";
            //var genericIdePath = @"/keep/GameAssets/ViceCity/data/maps/generic.ide";

            var objs = new List<RWViceCityIdeObjEntry>();
            var ides = new RWViceCityIde[idePaths.Length];
            for(int i = 0; i < idePaths.Length; i++) {
                ides[i] = RenderwareReader.ParseItemDefinitions(idePaths[i]);
                objs.AddRange(ides[i].Objs);
            }
            //var ids = RenderwareReader.ParseItemDefinitions(idePath);
            //var genIds = RenderwareReader.ParseItemDefinitions(genericIdePath);
            var ipls = RenderwareReader.ParseItemPlacements(iplPath);
            var objMap = new Dictionary<int, RWViceCityIdeObjEntry>();
            
            
            //objs.AddRange(ids.Objs);
            //objs.AddRange(genIds.Objs);

            foreach(var obj in objs) {
                objMap.Add(obj.ID, obj);
                var ttrobj = new TTRObject();
                var modName = (obj.ModelName+"_m").ToLower();
                var colName = (obj.ModelName+"_c").ToLower();
                object col = null;
                assets.TryGetValue(colName, out col);
                ttrobj.Mesh = ttr.GetOrCreateImport(true, TTRAssetType.Mesh, "ViceCity", modName);
                if(col != null) {
                    ttrobj.Collider = ttr.GetOrCreateImport(true, TTRAssetType.Collider, "ViceCity", colName);
                } else {
                    ttrobj.Collider = null;
                }
                //var texName = (obj.TextureName+"_t").ToLower();
                RWMaterialTexture rwmtex = (assets[modName] as RWClump).Materials[0].Texture;
                if(rwmtex != null) {
                    var texName = (rwmtex.TextureName+"_t").ToLower();
                    ttrobj.Material = new TTRMaterial() {
                        ShaderType = TTRShaderType.Unlit_Textured,
                        AlbedoTexture = ttr.GetOrCreateImport(true, TTRAssetType.Texture, "ViceCity", texName),
                        TintColor = new TTRVector4() {X = 1, Y = 1, Z = 1, W = 1}
                    };
                } else {
                    ttrobj.Material = new TTRMaterial() {
                        ShaderType = TTRShaderType.Unlit_Color,
                        TintColor = new TTRVector4() {X = 1, Y = 1, Z = 1, W = 1}
                    };
                }
                string objName = (obj.ModelName+"_o").ToLower();
                ttr.AddAsset(ttrobj, objName);
                map.ObjectTable.Add(new TTRMap.TTRMapObjectEntry() {
                    ObjectId = (uint)obj.ID,
                    AssetId = "ViceCity/"+objName,
                });
            }
            foreach(var itm in ipls.Inst) {
                map.EntityTable.Add( new TTRMap.TTRMapEntityEntry {
                    Position = new TTRVector3{X = itm.Position.X, Y = itm.Position.Z, Z = itm.Position.Y},
                    Rotation = new TTRVector4{X = itm.Rotation.X, Y = itm.Rotation.Z, Z = itm.Rotation.Y, W = itm.Rotation.W},
                    //Rotation = new TTRVector4{X = 0.0f, Y = 0.0f, Z = 0.0f, W = 1.0f},
                    Scale = new TTRVector3{X = itm.Scale.X, Y = itm.Scale.Y, Z = itm.Scale.Z},
                    ObjectId = (uint)itm.ID,
                });
            }
            ttr.AddAsset(map, assetName);
            return ttr;
        }

        static void Main(string[] args)
        {
            List<string> assetnames = new List<string>();
            Dictionary<string, object> assets = new Dictionary<string, object>();
            Dictionary<string, string> alphaTexMap = new Dictionary<string, string>();

            var imgPath = @"/keep/GameAssets/ViceCity/models/gta3.img";
            var dir = RenderwareReader.ParseIMGDirectory(imgPath);
            HashSet<string> names = new HashSet<string>();
            int colcount = 0;
            using(var fs = new FileStream(imgPath, FileMode.Open)) {
                for(int j = 0; j < dir.Entries.Length; j++) {
                    object asset = null;
                    var ext = Path.GetExtension(dir.Entries[j].Name).ToLower();        
                    bool isTexture = ext == ".txd";
                    bool isModel = ext == ".dff";
                    bool isCol = ext == ".col";
                    var name = Path.GetFileNameWithoutExtension(dir.Entries[j].Name).ToLower();
                    if(isTexture) {
                        name += "_t";
                    }
                    else if(isModel) {
                        name += "_m";
                    }
                    else if(isCol) {
                        name += "_c";
                    }
                    if((isTexture || isModel)) {
                        asset = RenderwareReader.ParseEntry(dir.Entries[j], fs);
                        if(isTexture) {
                            var rwtex = asset as RWTextureDictionary;
                            foreach(var rwtexx in rwtex.TexturesPC) {
                                alphaTexMap.TryAdd(rwtexx.MaskName, rwtexx.Name);
                            }
                        }
                        Console.WriteLine("asset " + name);
                        assets.TryAdd(name, asset);
                        assetnames.Add(name);
                        var fname = @"/keep/Projects/ExpEngineBuild/Packages/ViceCity/"+name+".ttr";
                        bool doesntExist = names.Add(fname);
                        //Debug.Assert(doesntExist, $"File {fname} already exists");
                        WriteTTR(asset, fname);
                    } else if(isCol){ 
                        var entry = dir.Entries[j];
                        var br = new BinaryReader(fs);
                        long totalSectionBytes = 0;
                        while(totalSectionBytes < entry.Size*RenderwareReader.RENDERWARE_SECTOR_SIZE) {
                            colcount++;
                            fs.Seek(entry.Offset*RenderwareReader.RENDERWARE_SECTOR_SIZE + totalSectionBytes, SeekOrigin.Begin);
                            var chdr = CollReader.parseHeader(br);
                            if(chdr != null) {
                                // TODO: why not just put them in same file?
                                var cname = chdr.collisionModelName+"_c";
                                totalSectionBytes += chdr.fileSize+8;
                                var cbody = CollReader.parseData(br, chdr);
                                assets.TryAdd(cname, cbody);
                                assetnames.Add(cname);
                                var fname = @"/keep/Projects/ExpEngineBuild/Packages/ViceCity/"+cname+".ttr";
                                names.Add(fname);
                                WriteTTR(cbody, fname);
                            }
                            else {
                                break;
                            }
                        }
                    }
                }
            }
            FileStream ffss = new FileStream("/keep/files.txt", FileMode.OpenOrCreate);
            foreach(var fname in assetnames) {
                ffss.Write(System.Text.Encoding.ASCII.GetBytes(fname));
            }
            ffss.Dispose();

            var apIde = @"/keep/GameAssets/ViceCity/data/maps/airport/airport.ide";
            var apIpl = @"/keep/GameAssets/ViceCity/data/maps/airport/airport.ipl";
            var mallIde = @"/keep/GameAssets/ViceCity/data/maps/mall/mall.ide";
            var mallIpl = @"/keep/GameAssets/ViceCity/data/maps/mall/mall.ipl";
            var hotelIde = @"/keep/GameAssets/ViceCity/data/maps/hotel/hotel.ide";
            var hotelIpl = @"/keep/GameAssets/ViceCity/data/maps/hotel/hotel.ipl";
            var cislandIde = @"/keep/GameAssets/ViceCity/data/maps/cisland/cisland.ide";
            var cislandIpl = @"/keep/GameAssets/ViceCity/data/maps/cisland/cisland.ipl";
            var docksIde = @"/keep/GameAssets/ViceCity/data/maps/docks/docks.ide";
            var docksIpl = @"/keep/GameAssets/ViceCity/data/maps/docks/docks.ipl";
            var genericIde = @"/keep/GameAssets/ViceCity/data/maps/generic.ide";

            List<TTRHeader> maps = new List<TTRHeader>();
            string[] mapnames = new string[]{"airport", "mall", "hotel", "cisland", "docks"};
            maps.Add(ParseMap(assets, new string[]{apIde, genericIde}, apIpl, "Airport"));
            maps.Add(ParseMap(assets, new string[]{mallIde, genericIde}, mallIpl, "Mall"));
            maps.Add(ParseMap(assets, new string[]{hotelIde, genericIde}, hotelIpl, "Hotel"));
            maps.Add(ParseMap(assets, new string[]{cislandIde, genericIde}, cislandIpl, "CenterIsland"));
            maps.Add(ParseMap(assets, new string[]{docksIde, genericIde}, docksIpl, "Docks"));
            for(int i = 0; i < maps.Count; i++) {
                using(var fs = new FileStream($"/keep/Projects/ExpEngineBuild/Packages/ViceCity/{mapnames[i]}.ttr", FileMode.OpenOrCreate)) {
                    TTRWriter.CreateTTR(maps[i], fs);
                    Console.WriteLine($"Wrote map");
                }
            }

            /*RWClump clump = null;
            using (BinaryReader f = new BinaryReader(File.Open(@"/keep/GameAssets/ViceCity/models/generic/wheels.DFF", FileMode.Open)))
            {
                clump = RenderwareReader.ParseClump(f);
            }
            if(clump == null)
                return;
            
            using (Stream fs = new FileStream(@"/keep/Projects/ExpEngineBuild/Packages/ViceCity/wheels.ttr", FileMode.OpenOrCreate)) {
                
                TTRHeader ttr = new TTRHeader();
                ttr.MajorVersion = 0;
                ttr.MinorVersion = 1;

                var mesh = meshFromRWGeometry(clump.GeometryList[0]);
                ttr.AddAsset(mesh, "wheels");

                var obj = new TTRObject();
                obj.Mesh = ttr.GetOrCreateImport(true, TTRAssetType.Mesh, "ViceCity", "wheels");
                var mat = new TTRMaterial();
                mat.ShaderType = TTRShaderType.Unlit_Textured;
                mat.AlbedoTexture = ttr.GetOrCreateImport(true, TTRAssetType.Texture, "ViceCity", "Was_meshfence");
                mat.TintColor = new TTRVector4{X = 1.0f, Y = 1.0f, Z = 1.0f, W = 1.0f};
                obj.Material = mat;
                ttr.AddAsset(obj, "wheelsO");

                var map = new TTRMap();
                map.ObjectTable.Add(new TTRMap.TTRMapObjectEntry() {
                    ObjectId = 1,
                    AssetId = "ViceCity/wheelsO",
                });
                map.EntityTable.Add(new TTRMap.TTRMapEntityEntry() {
                    Position = new TTRVector3() {X = 0.0f, Y = 0.0f, Z = 0.0f},
                    Rotation = new TTRVector4() {X = 0.0f, Y = 0.0f, Z = 0.0f, W = 1.0f},
                    Scale = new TTRVector3() {X = 1.0f, Y = 1.0f, Z = 1.0f},
                    ObjectId = 1,
                });
                ttr.AddAsset(map, "ViceCity");
                
                TTRWriter.CreateTTR(ttr, fs);
            }*/
        }
    }
}
