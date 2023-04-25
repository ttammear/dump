using UnityEngine;
using UnityEngine.Playables;

public class MyTime : MonoBehaviour
{
    public bool offlineRender = false;
    public int fps = 60;
    public static MyTime Instance;

    double secondsSinceStart = 0.0;
    double timeDebt = 0.0;
    int framesSinceStart = 0;

    public PlayableDirector playableDirector = null;
    public static bool animate = true;
    public double animationProgress;

    public void Start()
    {
        animate = true;
        Instance = this;
        secondsSinceStart = 0.0;
        if(playableDirector != null)
        {
            playableDirector.time = 0.0;
            animationProgress = 0.0;
        }
    }

    public static double ssStart
    {
        get {

            return Instance.secondsSinceStart;
        }
    }

    public static int fsStart
    {
        get {
            return Instance.framesSinceStart;
        }
    }

    public static double dt
    {
        get
        {
            if(Instance.offlineRender)
            {
                return 1.0 / (double)Instance.fps;
            }
            else
            {
                return (double)Time.deltaTime;
            }
        }
    }

    public static double debt
    {
        get
        {
            return Instance.timeDebt;
        }
        set
        {
            Instance.timeDebt = System.Math.Max(0.0, value);
        }
    }

    public static double ConsumeDebt(double max)
    {
        Debug.Assert(max >= 0.0);
        double ret = System.Math.Max(0.0, max - debt);
        debt -= max;
        return ret;
    }


    public void Update()
    {
        if(offlineRender)
        {
            secondsSinceStart += 1.0 / (double)fps;
        }
        else
        {
            secondsSinceStart += (double)Time.deltaTime;
        }
        //timeDebt -= dt;
        timeDebt = System.Math.Max(0.0, timeDebt);

        if(playableDirector != null && animate)
        {
            playableDirector.time = animationProgress;
            playableDirector.Evaluate();
            if(offlineRender)
                animationProgress += 1.0 / (double)fps;
            else
                animationProgress += (double)Time.deltaTime;
        }

        framesSinceStart++;
    }
}
