using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.Playables;
using UnityEngine.Timeline;

[Serializable]
public class ProgLine2DClip : PlayableAsset, ITimelineClipAsset
{
    public ProgLine2DBehaviour template = new ProgLine2DBehaviour ();
    public List<Vector3> nodes = new List<Vector3>();

    public ClipCaps clipCaps
    {
        get { return ClipCaps.None; }
    }

    public override Playable CreatePlayable (PlayableGraph graph, GameObject owner)
    {
        var playable = ScriptPlayable<ProgLine2DBehaviour>.Create (graph, template);
        ProgLine2DBehaviour clone = playable.GetBehaviour ();
        return playable;
    }
}
