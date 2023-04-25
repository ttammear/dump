using UnityEngine;
using UnityEngine.Playables;
using UnityEngine.Timeline;

[TrackColor(0.2660035f, 0.5659589f, 0.8823529f)]
[TrackClipType(typeof(ProgLine2DClip))]
public class ProgLine2DTrack : TrackAsset
{
    public override Playable CreateTrackMixer(PlayableGraph graph, GameObject go, int inputCount)
    {
        return ScriptPlayable<ProgLine2DMixerBehaviour>.Create (graph, inputCount);
    }
}
