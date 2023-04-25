package main
import "os/exec"
import "sync"
import "io"
import "fmt"

type FfmpegCtx struct {
    outFileName string
    width, height int32
    dataChannel chan[]byte
    wg sync.WaitGroup
}

func ffmpegOutputRoutine(pipe io.ReadCloser, out chan string) {
    readBuf := make([]byte, 1024)
    for ;; {
        bytesRead, err := pipe.Read(readBuf)
        if bytesRead > 0 {
            out <- string(readBuf)
            // zero the buffer
            for i := 0; i < bytesRead; i++ {
                readBuf[i] = 0
            }
        }
        if err != nil {
            println("Output done")
            break
        }
    }
}

func ffmpegRoutine(from *FfmpegCtx) {
    from.wg.Add(1);
    videoSizeString := fmt.Sprintf("%dx%d", from.width, from.height)
    // -pix_fmt yuv420p
    ffmpegCmd := exec.Command("ffmpeg", "-f", "rawvideo", "-vcodec", "rawvideo", "-pixel_format", "rgb24", "-video_size", videoSizeString, "-framerate", "60", "-i", "-", "-y", "-vf", "vflip", "-preset", "ultrafast", "-crf", "0", "-tune", "animation", from.outFileName)
    println("Getting Input")
    ffmpegIn, err := ffmpegCmd.StdinPipe()
    if err != nil {
        panic("ffmpeg stdIn error")
    }
    ffmpegOut, err := ffmpegCmd.StderrPipe()
    if err != nil {
        panic("ffmpeg stdOut error")
    }
    outputChannel := make(chan string, 64)
    go ffmpegOutputRoutine(ffmpegOut, outputChannel)
    println("Starting ffmpeg")
    err = ffmpegCmd.Start()
    if err != nil {
        panic(err)
    }

    count := 0;
    for ;; {
        frameData := <-from.dataChannel
        if frameData == nil {
            select {
            case msg := <-outputChannel:
                println(msg)
            default:
                break
            }
            break;
        }
        println("ffmpeg frame ", count)
        count++;
        ffmpegIn.Write(frameData)
        select {
        case msg := <-outputChannel:
            println(msg)
        default:
            break
        }
    }
    ffmpegIn.Close()
    //ffmpegOut.Close()
    println("ffmpeg waiting")
    err = ffmpegCmd.Wait()
    if err != nil {
        println("ffmpeg Wait() failed")
    }
    println("ffmpeg done ")
    from.wg.Done()
}

func (ffmpeg *FfmpegCtx) start(outFile string, width int32, height int32) {
    ffmpeg.dataChannel = make(chan []byte, 50)
    ffmpeg.outFileName = outFile
    ffmpeg.width = width
    ffmpeg.height = height
    go ffmpegRoutine(ffmpeg)
}

func (ffmpeg *FfmpegCtx) sendFrame(fd []byte) {
    ffmpeg.dataChannel <- fd
}

func (ffmpeg *FfmpegCtx) stop() {
    ffmpeg.dataChannel <- nil
}

func (ffmpeg *FfmpegCtx) wait() {
    ffmpeg.wg.Wait()
}
