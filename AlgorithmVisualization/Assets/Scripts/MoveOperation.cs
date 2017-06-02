using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using UnityEngine;

  public class CustomTransform
  {
    public Vector3 position;
    public Quaternion rotation;
  }

  public class MoveOperation : CustomYieldInstruction
  {
    public static List<MoveOperation> mOps = new List<MoveOperation>();

    public Vector3 curPosition;
    public Vector3 targetPositon;
    public float t;
    public float time;
    public CustomTransform transform;
    bool _done = false;
    public override bool keepWaiting
    {
      get
      {
        return !_done;
      }
    }

    public MoveOperation()
    {
      mOps.Add(this);

    }

    public void Update(float dt)
    {
      if (Vector3.Distance(curPosition, targetPositon) < 0.05f)
        _done = true;
      if (_done)
      {
        return;
      }
      transform.position = Vector3.Lerp(curPosition, targetPositon, t/time);
      t += dt;
      if (t / time >= 1.0f)
      {
        _done = true;
        transform.position = targetPositon;
      }
    }

  public static void UpdateOperations()
  {
    List<MoveOperation> rmops = new List<MoveOperation>();
		foreach(var mo in mOps)
    {
      mo.Update(Time.deltaTime);
      if (!mo.keepWaiting)
        rmops.Add(mo);
    }
    foreach (var mo in rmops)
      mOps.Remove(mo);
  }

  public static MoveOperation MoveTransform(CustomTransform transform, Vector3 start, Vector3 target, float time)
  {
    var ret = new MoveOperation();
    ret.curPosition = start;
    ret.targetPositon = target;
    ret.t = 0.0f;
    ret.time = time;
    ret.transform = transform;
    return ret;
  }
  }
