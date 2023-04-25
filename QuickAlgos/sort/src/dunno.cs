using System;
using System.Linq;
using AnimLib;
using System.Collections.Generic;
using System.Threading.Tasks;
using System.Threading;

public class Dunno : AnimationBehaviour
{
    public void Init(AnimationSettings settings) {
        settings.Name = "My animation";
        // animation length must be bounded
        // (it gets "baked" to allow seeking whole animation in editor)
        settings.MaxLength = 600.0f; 
    }

    public async Task Swap(Rectangle[] arr, int idx1, int idx2, double duration = 1.0) {
        var pos1 = arr[idx1].Transform.Pos;
        var pos2 = arr[idx2].Transform.Pos;
        await Animate.InterpF(x => {
                arr[idx1].Transform.Pos = Vector2.Lerp(pos1, pos2, x);
                arr[idx2].Transform.Pos = Vector2.Lerp(pos2, pos1, x);
            }, 0.0f, 1.0f, duration, Animate.InterpCurve.EaseInOut);
        var temp = arr[idx1];
        arr[idx1] = arr[idx2];
        arr[idx2] = temp;
    }

    public Text2D[] CreateText(string[] lines, Vector2 pos) {
        var ret = new Text2D[lines.Length];
        for(int i = 0; i < lines.Length; i++) {
            float size = 22.0f;
            var line = new Text2D();
            line.Transform.Pos = pos + (float)i*new Vector2(0.0f, 1.2f*size);
            line.Size = size;
            line.Text = lines[i];
            ret[i] = line;
        }
        return ret;
    }

    public async Task Animation(World world, Animator animator) {
        var hw = new Text2D();
        hw.Transform.Pos = new Vector2(100.0f, 50.0f);
        hw.Size = 32.0f;
        hw.Color = Color.BLACK;
        hw.Anchor = new Vector2(0.5f, 0.5f);
        hw.HAlign = TextHorizontalAlignment.Center;
        hw.VAlign = TextVerticalAlignment.Center;
        hw.Text = "Preparation";
        world.CreateInstantly(hw);
        var hw2 = world.CreateClone(hw);
        hw2.Transform.Pos = (100.0f, 100.0f);
        hw2.Color = new Color(200, 0, 0, 255);
        hw2.Size = 24.0f;
        hw2.Text = "    Sorted array";

        await Time.WaitSeconds(1.0);

        var startColor = Color.RED;
        var endColor = Color.VIOLET;

        int barCount = 40;
        Rectangle[] bars = new Rectangle[40];

        var values = new Dictionary<Rectangle, float>();

        var rect = new Rectangle(20.0f, 100.0f);
        rect.Transform.Pos = (0.0f, -350.0f);
        rect.Anchor = (-0.4f, 0.0f);
        rect.Pivot = (0.0f, -0.5f);
        rect.Height = 0.0f;
        rect.Color = startColor;
        Task last = null;
        for(int i = 0; i < barCount; i++) {
            var temp = world.Clone(rect);
            bars[i] = temp;
            values.Add(temp, (float)i);
            temp.Transform.Pos = rect.Transform.Pos + (i*25.0f, 0.0f);
            temp.Height = rect.Height + i*10.0f;
            temp.Color = Color.LerpHSV(startColor, endColor, (float)i/40.0f);

            last = Animate.InterpF(x => {
                    temp.Height = x;
                }, 0.0f, 300.0f + i*10.0f, 1.5, Animate.InterpCurve.EaseOutElastic);
            _ = world.CreateFadeIn(temp, 0.5f);
            await Time.WaitSeconds(0.05);
        }
        await last;

        var positions = bars.Select(x => x.Transform.Pos).ToArray();
        var rnd = new Random();
        var newBars = bars.ToArray().OrderBy(x => rnd.Next()).ToArray();
        var newPositions = newBars.Select(x => x.Transform.Pos).ToArray();

        var list = new List<int>();

        hw2.Text = "    Shuffle";
        await Time.WaitSeconds(1.0);

        await Animate.InterpF(x => {
                for(int i = 0; i < barCount; i++) {
                    bars[i].Transform.Pos = Vector3.Lerp(positions[i], newPositions[i], x);
                }
            }, 0.0f, 1.0f, 0.5, Animate.InterpCurve.EaseInOut);

        await Time.WaitSeconds(1.0);

        newBars = newBars.OrderBy(x => x.Transform.Pos.x).ToArray();

        var pointer = new Rectangle(10.0f, 40.0f);
        pointer.Color = Color.RED;
        pointer.Anchor = (-0.4f, 0.0f);
        pointer.Pivot = (0.0f, 0.5f);
        pointer.Transform.Pos = (12.5f, -355.0f);
        _ = world.CreateFadeIn(pointer, 0.5f);

        hw.Text = "Optimized bubble sort";
        hw2.Text = "    Sort";

        // program text
        var program = new string[] {
            "n = count-1",
            "while (n > 0)",
            "   newn = 0",
            "   for (i = 0; i < n; i++)",
            "       if (A[i] > A[i+1])",
            "           swap(A[i], A[i+1])",
            "           newn = i",
            "   n = newn",
        };
        var variables = new string[] {
            "n = 0",
            "i = 0",
            "newn = 0",
        };
        var texts = CreateText(program, (1300.0f, 400.0f));
        var vtexts = CreateText(variables, (1300.0f, 700.0f));

        var text = new Text2D();
        text.Size = 28.0f;
        text.Text = "Program";
        text.Transform.Pos = (1270.0f, 350.0f);

        var text2 = world.Clone(text);
        text2.Text = "Variables";
        text2.Transform.Pos = (1270.0f, 650.0f);

        var speedTxt = world.Clone(hw2);
        speedTxt.Color = Color.BLACK;
        speedTxt.Text = "Speeds up after first loop";
        speedTxt.Transform.Pos = (1300.0f, 200.0f);

        _ = world.CreateFadeIn(text, 0.5f);
        _ = world.CreateFadeIn(text2, 0.5f);
        texts.ToList().ForEach(x => world.CreateFadeIn(x, 0.5f));
        vtexts.ToList().ForEach(x => world.CreateFadeIn(x, 0.5f));
        _ = world.CreateFadeIn(speedTxt, 0.5f);

        await Time.WaitSeconds(2.0);


        Vector2 pos = (1260.0f, -410.0f);
        var pc = new Rectangle(40.0f, 10.0f);
        pc.Anchor = (-0.5f, 0.5f);
        pc.Transform.Pos = pos;
        _ = world.CreateFadeIn(pc, 0.5f);
        await Time.WaitSeconds(2.0);

        var lineT = texts.Select(x => 0.0f).ToArray();
        var varT = vtexts.Select(x => 0.0f).ToArray();

        _ = Animate.Time(x => {
            for(int i = 0; i < texts.Length; i++) {
                var val = lineT[i] - Time.T*0.1;
                lineT[i] = (float)val;
                var clamped = val > 0.0f ? val : 0.0f;
                if(val > -20.0f) {
                    texts[i].Color = new Color((byte)clamped, 0, 0, 255);
                }
            }
            for(int i = 0; i < vtexts.Length; i++) {
                var val = varT[i] - Time.T*0.1;
                varT[i] = (float)val;
                var clamped = val > 0.0f ? val : 0.0f;
                if(val > -20.0f) {
                    vtexts[i].Color = new Color((byte)clamped, (byte)clamped, 0, 255);
                }
            }
        }, 999999.0);

        var setLine = async (int x, bool wait) => {
            pc.Transform.Pos = pos + (0.0f, -x*1.2f*22.0f);
            var tasks = new List<Task>();
            for(int i = 0; i < texts.Length; i++) {
                if(i == x) {
                    lineT[i] = 200.0f;
                }
            }
            if(wait) await Time.WaitSeconds(0.3);
            else await Time.WaitFrame();
        };

        var sorting = async (CancellationToken token) => {
            while(!token.IsCancellationRequested) {
                double waitTime = 0.5;
                hw2.Text = "    Sorting";
                await Time.WaitSeconds(waitTime);
                if(token.IsCancellationRequested) break;
                hw2.Text = "    Sorting.";
                await Time.WaitSeconds(waitTime);
                if(token.IsCancellationRequested) break;
                hw2.Text = "    Sorting..";
                await Time.WaitSeconds(waitTime);
                if(token.IsCancellationRequested) break;
                hw2.Text = "    Sorting...";
                await Time.WaitSeconds(waitTime);
            }
        };
        var tsrc = new CancellationTokenSource();
        var textTask = sorting(tsrc.Token);

        // bubble sort
        int n = barCount-1;
        vtexts[0].Text = $"n = {n}";
        varT[0] = 200;
        bool speed = false;
        while(n > 0) {
            await setLine(0, !speed);
            await setLine(1, !speed);
            await setLine(2, !speed);
            int newn = 0;
            vtexts[2].Text = $"newn = {0}";
            varT[2] = 200;
            Task movet = Task.CompletedTask;
            for(int i = 0; i < n; i++) {
                await setLine(3, !speed);
                vtexts[1].Text = $"i = {i}";
                varT[1] = 200;
                await movet;
                movet = Animate.Move(pointer.Transform, (12.5f + i * 25.0f, -355.0f), speed ? 0.05 : 0.2);
                if(!speed)
                    await Time.WaitSeconds(0.1);
                var val1 = values[newBars[i]];
                var val2 = values[newBars[i+1]];
                await setLine(4, !speed);
                if(val1 > val2) {
                    await setLine(5, !speed);
                    var swapt = Swap(newBars, i, i+1, speed ? 0.02 : 0.1);
                    await setLine(6, !speed);
                    newn = i;
                    vtexts[2].Text = $"newn = {i}";
                    varT[2] = 200;
                    await swapt;
                }
            }
            speed = true;
            await setLine(7, !speed);
            n = newn;
            vtexts[0].Text = $"n = {newn}";
            varT[0] = 200;
        }

        tsrc.Cancel();

        _ = Animate.Color(hw2, hw2.Color, new Color(0, 200, 0, 255), 0.9);
        hw2.Text = "    Sorted!";

        await Time.WaitSeconds(3.0);
    }
}
