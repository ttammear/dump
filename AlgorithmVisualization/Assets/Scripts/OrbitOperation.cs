using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using UnityEngine;

public class CustomWait : CustomYieldInstruction
{
    bool waitFrames;
    double startTime;
    int startFrame;

    double timeToWait;
    int framesToWait;

    bool addDebt = false;

    public CustomWait(double time, bool accDebt = false)
    {
        waitFrames = false;
        startTime = MyTime.ssStart;
        timeToWait = time;
        addDebt = accDebt;
    }

    public CustomWait(int frames)
    {
        waitFrames = true;
        startFrame = MyTime.fsStart;
        framesToWait = frames;
    }

    public override bool keepWaiting
    {
        get
        {
            if(waitFrames)
            {
                return MyTime.fsStart - startFrame < framesToWait;
            }
            else
            {
                bool ret = MyTime.ssStart - startTime < timeToWait;
                if(!ret && addDebt)
                {
                    MyTime.debt += MyTime.ssStart - startTime - timeToWait;
                }
                return ret;
            }
        }
    }

}

public class MoveRotateOperation : CustomYieldInstruction
{
    public Vector3 startPos;
    public Quaternion startRot;

    public Vector3 endPos;
    public Quaternion endRot;
    public float time;
    float curTime;

    public Transform transform;
    public bool done;

    public static List<MoveRotateOperation> operations = new List<MoveRotateOperation>();

    public static MoveRotateOperation MoveRotate(Transform transform, Vector3 endpos, Quaternion endrot, float time)
    {
        MoveRotateOperation ret = new MoveRotateOperation();
        ret.startPos = transform.position;
        ret.startRot = transform.rotation;
        ret.endPos = endpos;
        ret.endRot = endrot;
        ret.time = time;
        ret.transform = transform;

        ret.done = false;
        operations.Add(ret);
        return ret;
    }

    public override bool keepWaiting
    {
        get 
        {
            return !done;
        }
    }

    public void Update(float dt)
    {
        float t = Mathf.Clamp01(curTime/time);
        transform.position = Vector3.Lerp(startPos, endPos, t);
        transform.rotation = Quaternion.Slerp(startRot, endRot, t);
        done = (curTime/time) >= 1.0f;
        curTime += dt;
    }
}

public class OrbitOperation : CustomYieldInstruction
{
  public Vector3 startPosition;
  public Vector3 lookTarget;
  public float t;
  public float time;
  public Transform transform;
  public float degrees;
  public float yAngle;
  public float xAngle;
  public float distance;
  public bool loop = false;

  public bool _done = false;

  public static List<OrbitOperation> _currentOperations = new List<OrbitOperation>();

  public OrbitOperation()
  {
    _currentOperations.Add(this);
  }

  public override bool keepWaiting
  {
    get
    {
      return !_done;
    }
  }

  public void Update(float dt)
  {
    transform.position = lookTarget + 
      (Quaternion.AngleAxis(Mathf.Lerp(yAngle, degrees+yAngle, t/time), Vector3.up)
        * Quaternion.AngleAxis(xAngle,Vector3.right)
        * (-Vector3.forward*distance));
    transform.LookAt(lookTarget);
    t += dt;
    if (t / time >= 1.0f && !loop)
      _done = true;
  }

  public void End()
  {
    _done = true;
  }

  public static OrbitOperation OrbitTarget(Transform transform, Vector3 target, float distance, float degrees, float time, bool loop = false)
  {
    var ret = new OrbitOperation();
    ret.lookTarget = target;
    ret.t = 0.0f;
    ret.time = time;
    ret.degrees = degrees;
    ret.distance = Vector3.Distance(transform.position, target);
    ret.transform = transform;
    transform.LookAt(target);
    ret.yAngle = transform.eulerAngles.y;
    ret.xAngle = transform.eulerAngles.x;
    
    return ret;
  }

  static List<OrbitOperation> rlist = new List<OrbitOperation>();
  static List<MoveRotateOperation> rlist2 = new List<MoveRotateOperation>();

  public static void UpdateOperations(float dt)
  {
    foreach(var opr in _currentOperations)
    {
      opr.Update(dt);
      if (!opr.keepWaiting)
        rlist.Add(opr);
    }

    foreach(var opr in MoveRotateOperation.operations)
    {
        opr.Update(dt);
        if(!opr.keepWaiting)
            rlist2.Add(opr);
    }

    foreach (var opr in rlist)
      _currentOperations.Remove(opr);
    foreach (var opr in rlist2)
        MoveRotateOperation.operations.Remove(opr);

    rlist.Clear();
    rlist2.Clear();
  }
}

public static class CustomCoroutines
{
    public static void UpdateRoutines()
    {
        OrbitOperation.UpdateOperations((float)MyTime.dt);
    }
}
