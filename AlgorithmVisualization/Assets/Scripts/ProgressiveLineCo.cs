using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using UnityEngine;

public class ProgressiveLineCo : CustomYieldInstruction
{
    List<Vector3> path;
    double animLength;
    double progressTime;
    double waitTime;
    Color color;

    double totalDistance;

    public ProgressiveLineCo(List<Vector3> path, double length, Color color, double waitTime = 0.0)
    {
        this.path = path;
        this.animLength = length;
        this.progressTime = 0.0;
        this.waitTime = waitTime;
        this.color = color;

        this.totalDistance = 0.0;
        for(int i = 0; i < path.Count-1; i++)
        {
            totalDistance += Vector3.Distance(path[i], path[i+1]);
        }
    }

    public ProgressiveLineCo(List<Vector2> path, double length)
    {
        List<Vector3> path3 = path.Select(x => new Vector3(x.x, x.y, 0.0f)).ToList();
        this.path = path3;
        this.animLength = length;
        this.progressTime = 0.0;
    }

    public override bool keepWaiting
    {
        get 
        {
            progressTime += MyTime.dt;
            double progress = progressTime / animLength;
            progress = System.Math.Max(0.0, progress);
            progress = System.Math.Min(1.0, progress);

            double progressDist = totalDistance*progress;
            double curDist = 0.0;
            double curDistNext = 0.0f;
            int pn;
            for(pn = 0; pn < path.Count-1; pn++)
            {
                double d = Vector3.Distance(path[pn], path[pn+1]);
                curDistNext = curDist + d;
                if(curDistNext < progressDist)
                {
                    curDist += d;
                    QuickShapes2D.DrawLine(path[pn], path[pn+1], 3.0f, this.color);
                    continue;
                }
                else
                    break;
            }

            if(pn != path.Count-1)
            {
                double nodeDist = Vector3.Distance(path[pn], path[pn+1]);
                double progDist = curDistNext - progressDist;
                float localProgress = 1.0f - (float)(progDist / nodeDist);
                Vector3 end = Vector3.Lerp(path[pn], path[pn+1], localProgress);
                QuickShapes2D.DrawLine(path[pn], end, 3.0f, this.color);
            }

            bool ret = progressTime < (animLength+waitTime);
            return ret;
        }
    }
}
