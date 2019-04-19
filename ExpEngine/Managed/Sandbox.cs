using System;
using System.Runtime.InteropServices;
using System.IO;
using Newtonsoft.Json;

public enum NativeApiProc {
    Unknown,
    InternAssetId,
    SetMap,
    SetServerTitle,
    SetGamemodeTitle,
    SetMaxPlayers,
}

public class AssetIdDTO {
    public string Package { get; set; }
    public string Asset { get; set; }
}

public struct AssetId {
    public string Package;
    public string Asset;
    internal IntPtr Handle;

    public AssetId(string package, string asset) {
        this.Package = package;
        this.Asset = asset;
        // TODO: intern in engine
        Handle = Sandbox.nativeInternAssetId(Sandbox.NativeGameServerContext, package, asset);
    }
}

public class ServerConfiguration {
    public AssetId MapAsset { get; set; }
    public int MaxPlayers { get; set; }
    public string ServerTitle { get; set; }
    public string GamemodeTitle { get; set; }
}

public class Sandbox
{
    [UnmanagedFunctionPointer(CallingConvention.ThisCall)]
    public delegate IntPtr nativeInternAssetIdDelegate(IntPtr thisPtr, [MarshalAs(UnmanagedType.LPStr)]String packageCstr, [MarshalAs(UnmanagedType.LPStr)]String assetCstr);
    public delegate void nativeSetMapDelegate(IntPtr thisPtr, IntPtr assetId);
    public delegate void nativeSetServerTitleDelegate(IntPtr thisPtr, [MarshalAs(UnmanagedType.LPStr)]String title);
    public delegate void nativeSetGamemodeTitleDelegate(IntPtr thisPtr, [MarshalAs(UnmanagedType.LPStr)]String title);
    public delegate void nativeSetMaxPlayersDelegate(IntPtr thisPtr, int maxPlayers);

    public static IntPtr NativeGameServerContext;

    public static nativeInternAssetIdDelegate nativeInternAssetId;
    public static nativeSetMapDelegate NativeSetMap;
    public static nativeSetServerTitleDelegate NativeSetServerTitle;
    public static nativeSetGamemodeTitleDelegate NativeSetGamemodeTitle;
    public static nativeSetMaxPlayersDelegate NativeSetMaxPlayers;

    public static void Hello() {
        System.Console.WriteLine("Hello from C#!");
        Start();
    }

    public static void Start() {
        var configString = System.IO.File.ReadAllText(@"config.json");
        if(configString == null) {
            throw new FileNotFoundException("config.json file not found!");
        }
        var config = JsonConvert.DeserializeObject<ServerConfiguration>(configString);
        if(config == null) {
            throw new FormatException("config.json could not be read!");
        }
        System.Console.WriteLine(config.ServerTitle);
        AssetId sponza = new AssetId("ViceCity", "Airport");
        NativeSetMap(NativeGameServerContext, sponza.Handle);
    }

    public static void SetNativeApiFunc(NativeApiProc nativeApiFunc, IntPtr procPtr) {
        switch(nativeApiFunc) {
            case NativeApiProc.InternAssetId:
                nativeInternAssetId = (nativeInternAssetIdDelegate)Marshal.GetDelegateForFunctionPointer(procPtr, typeof(nativeInternAssetIdDelegate));
                break;
            case NativeApiProc.SetMap:
                NativeSetMap = (nativeSetMapDelegate)Marshal.GetDelegateForFunctionPointer(procPtr, typeof(nativeSetMapDelegate));
                break;
            case NativeApiProc.SetServerTitle:
                NativeSetServerTitle = (nativeSetServerTitleDelegate)Marshal.GetDelegateForFunctionPointer(procPtr, typeof(nativeSetServerTitleDelegate));
                break;
            case NativeApiProc.SetGamemodeTitle:
                NativeSetGamemodeTitle = (nativeSetGamemodeTitleDelegate)Marshal.GetDelegateForFunctionPointer(procPtr, typeof(nativeSetGamemodeTitleDelegate));
                break;
            case NativeApiProc.SetMaxPlayers:
                NativeSetMaxPlayers = (nativeSetMaxPlayersDelegate)Marshal.GetDelegateForFunctionPointer(procPtr, typeof(nativeSetMaxPlayersDelegate));
                break;
        }
    }

    public static void SetContext(IntPtr ctx) {
        NativeGameServerContext = ctx;
    }

#if FALSE
    public static void Initialize(ServerConfiguration conf) {
        //conf.MapAsset = new AssetId("Sponza", "Sponza");
        //conf.MaxPlayers = 10;
        //conf.ServerTitle = "Sandbox prealpha";
        //conf.GamemodeTitle = "Deathmatch";
    }
#endif
}
