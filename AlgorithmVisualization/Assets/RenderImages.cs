using System.Collections;
using System;
using System.Collections.Generic;
using UnityEngine;
using System.IO;
using System.Diagnostics;

[RequireComponent(typeof(Camera))]
public class RenderImages : MonoBehaviour 
{
    RenderTexture renderTex;
    RenderTexture renderTex2;

    RenderTexture currentRenderTarget;

    Texture2D buffer;
    Camera cam;
    int frameCounter = 0;

    //3840 × 2160
    int resX = 1920;
    int resY = 1080;

    BinaryWriter ffmpeg_stdin;
    TextReader ffmpeg_stdout;
    TextReader ffmpeg_stderror;
    Process ffmpeg_process;

    bool initialized = false;

    private void ffmpegExited(object sender, System.EventArgs e)
    {
        UnityEngine.Debug.Log("ffmpeg exited!!!!!");
    }

    public void OpenPipe(int width, int height, int framerate)
    {
        string name = DateTime.Now.ToString("yyyy_MMdd_HHmmss") + ".mp4";

        var opt = "-y -f rawvideo -vcodec rawvideo -pixel_format rgb24";
        opt += " -video_size " + width + "x" + height;
        opt += " -r " + framerate;
        opt += " -i - -vf vflip -pix_fmt yuv420p -crf 15";
        opt += " " + name;

        var info = new ProcessStartInfo("ffmpeg", opt);
        info.UseShellExecute = false;
        info.CreateNoWindow = true;
        info.RedirectStandardInput = true;
        info.RedirectStandardOutput = true;
        info.RedirectStandardError = true;

        ffmpeg_process = Process.Start(info);
        ffmpeg_process.Exited += ffmpegExited;
        ffmpeg_stdin = new BinaryWriter(ffmpeg_process.StandardInput.BaseStream);
        //ffmpeg_stdout = new StreamReader(ffmpeg_process.StandardOutput.BaseStream);
        //ffmpeg_stderror = new StreamReader(ffmpeg_process.StandardError.BaseStream);

        ffmpeg_process.EnableRaisingEvents = true;
        ffmpeg_process.OutputDataReceived += (object sendingProcess, DataReceivedEventArgs e) =>
        {
            if (e.Data != null)
            {
                UnityEngine.Debug.Log("ffmpeg STDOUT: " + e.Data);
            }
        };
        ffmpeg_process.ErrorDataReceived += (object sendingProcess, DataReceivedEventArgs e) =>
        {
            if (e.Data != null)
            {
                UnityEngine.Debug.Log("ffmpeg STDERR: " + e.Data);
            }
        };

        ffmpeg_process.BeginOutputReadLine();
        ffmpeg_process.BeginErrorReadLine();

        UnityEngine.Debug.Log(ffmpeg_process);

    }

    public void ClosePipe()
    {
        ffmpeg_process.StandardInput.Close();
        ffmpeg_process.WaitForExit();
        ffmpeg_process.CancelOutputRead();
        ffmpeg_process.CancelErrorRead();
        UnityEngine.Debug.Log("DONE!");
        ffmpeg_process.Close();
        ffmpeg_process.Dispose();
        ffmpeg_stdin = null;
    }

    void SwapBuffers()
    {
        if(currentRenderTarget == renderTex)
        {
            cam.targetTexture = renderTex2;
            currentRenderTarget = renderTex2;
        }
        else
        {
            cam.targetTexture = renderTex;
            currentRenderTarget = renderTex;
        }
    }

    RenderTexture GetPrevRenderTarget()
    {
        if(currentRenderTarget == renderTex)
            return renderTex2;
        else
            return renderTex;
    }

	// Use this for initialization
	void Start () 
    {
        cam = GetComponent<Camera>();
        renderTex = new RenderTexture(resX, resY, 24);
        currentRenderTarget = renderTex;
        renderTex2 = new RenderTexture(resX, resY, 24);
        buffer = new Texture2D(resX, resY, TextureFormat.RGB24, false);
        cam.targetTexture = renderTex;

        OpenPipe(resX, resY, MyTime.Instance.fps);
        initialized = true;
	}
	
	void Update () 
    {
        if(ffmpeg_process.HasExited)
        {
            UnityEngine.Debug.Log("ffmpeg EXITED!!!!");
            UnityEngine.Debug.Break();
        }
	}

    void OnDestroy()
    {
        if(!initialized)
            return;
        renderTex = null;
        buffer = null;
        UnityEngine.Debug.Log("Destroyed");
        ClosePipe();
        initialized = false;
    }

    IEnumerator OnPostRender()
    {
        yield return new WaitForEndOfFrame();

        Stopwatch sw = new Stopwatch();

        if(frameCounter % 100 == 0)
            UnityEngine.Debug.Log("Frame " + frameCounter);


        if(frameCounter > 0)
        {
            sw.Start();
            RenderTexture.active = GetPrevRenderTarget();
            buffer.ReadPixels(new Rect(0, 0, resX, resY), 0, 0);
            buffer.Apply();

            ffmpeg_stdin.Write(buffer.GetRawTextureData());
            ffmpeg_stdin.Flush();
            sw.Stop();
        }
        else
        {
            buffer.ReadPixels(new Rect(0, 0, resX, resY), 0, 0);
            buffer.Apply();
            var pngData = buffer.EncodeToPNG();
            
            string name = "render/image"+frameCounter+".png";
            using(FileStream fs = File.Open(name, FileMode.OpenOrCreate))
            {
                var bw = new BinaryWriter(fs);
                bw.Write(pngData);
            }
        }
    
        SwapBuffers();

        frameCounter++;
    }
}
