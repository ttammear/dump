using System;

public struct AssetId {
    public string Package;
    public string Asset;
    internal IntPtr Handle;

    AssetId(string package, string asset) {
        this.Package = package;
        this.Asset = asset;
        // TODO: intern in engine
    }
}

public class ServerConfiguration {
    public AssetId MapAsset;
    public int MaxPlayers;
    public string ServerName;
    public string GamemodeName;
}

public class Sandbox
{
    public static void Hello() {
        System.Console.WriteLine("Hello from C#!");
    }

#if FALSE
    public static void Initialize(ServerConfiguration conf) {
        conf.MapAsset = new AssetId("Sponza", "Sponza");
        conf.MaxPlayers = 10;
        conf.ServerName = "Sandbox prealpha";
        conf.GamemodeName = "Deathmatch";
    }
#endif
}
