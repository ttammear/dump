using UnityEngine;
using System;
using System.Linq;
using System.Collections;
using System.Collections.Generic;
using UnityEngine.Playables;
using TMPro;

[ExecuteInEditMode]
[RequireComponent(typeof(PlayableDirector))]
public class AstarAnimation : MonoBehaviour
{
    Astar astar;
    Camera cam;

    public Vector2 start1Pos;
    public Vector2 dest1Pos;

    public Color notVisitedColor;
    public Color closedColor;
    public Color openColor;
    public Color blockedColor;

    public TextMeshProUGUI openSetText; 

    List<GraphNode> path = new List<GraphNode>();
    PlayableDirector pd;

    void OnEnable()
    {
        pd = GetComponent<PlayableDirector>();
        astar = new Astar();
        astar.notVisitedColor = notVisitedColor;
        astar.closedColor = closedColor;
        astar.openColor = openColor;
        astar.blockedColor = blockedColor;
        astar.ResetGrid();
        cam = GetComponent<Camera>();

        //QuickLabels.ShowLabel(new Vector2(0.0f, 300.0f), "Testing this thing", 100, Color.red);
        //path = astar.dostuff(getNode(start1.position), getNode(dest1.position));
    }

    IEnumerator FirstAnimation()
    {
        yield return StartCoroutine(astar.dostuff(start1Pos, dest1Pos, path));
        pd.Resume();
        MyTime.animate = true;
        yield return null;
    }

    void SetOpenSetText(string text)
    {
        if(openSetText != null)
        {
            openSetText.text = text;
        }
    }

    IEnumerator DetailedAnimation()
    {
        yield return StartCoroutine(astar.doDetailedStuff(start1Pos, dest1Pos, path, SetOpenSetText));
        pd.Resume();
        MyTime.animate = true;
        yield return null;
    }

    IEnumerator ScoresAnimation()
    {
        Vector3 offset = new Vector3(-25.0f, -25.0f, 0.0f);
        
        GraphNode startNode = astar.GetTileNode(new Vector2(10.0f, 10.0f), 1.0f, 50);
        startNode.color = Color.green;

        GraphNode curNode = astar.GetTileNode(new Vector2(35.0f, 20.0f), 1.0f, 50);
        curNode.color = Color.yellow;

        var gScorePath = new List<Vector3>();
        gScorePath.Add(new Vector3(10.0f, 10.0f, 0.0f));
        gScorePath.Add(new Vector3(25.0f, 10.0f, 0.0f));
        gScorePath.Add(new Vector3(35.0f, 20.0f, 0.0f));
        for(int i = 0; i < gScorePath.Count; i++)
        {
            gScorePath[i] = (gScorePath[i] - new Vector3(25.0f, 25.0f, 0.0f)) * 10.0f;
        }
        yield return StartCoroutine(new ProgressiveLineCo(gScorePath, 3.0f, Color.green, 3.0f));
        var fScorePath = new List<Vector3>();
        fScorePath.Add(new Vector3(35.0f, 20.0f, 0.0f));
        fScorePath.Add(new Vector3(30.0f, 40.0f, 0.0f));
        for(int i = 0; i < fScorePath.Count; i++)
            fScorePath[i] = (fScorePath[i] - new Vector3(25.0f, 25.0f, 0.0f)) * 10.0f;
        yield return StartCoroutine(new ProgressiveLineCo(fScorePath, 3.0f, Color.red, 3.0f));
        yield return null;
    }

    void OnDisable()
    {
        astar = null;
    }

    void Update()
    {
        if(astar != null)
        {
            astar.Render();
            /*var node = astar.GetTileNode(cam.transform.position, 1.0f, 50);
            if(node != null)
                node.color = Color.red;*/
        }

        if(path != null && path.Count > 1)
        {
            for(int i = 1; i < path.Count; i++)
            {
                Vector2 start = new Vector2(path[i].position.x - 25.0f, path[i].position.z-25.0f);
                Vector2 end = new Vector2(path[i-1].position.x - 25.0f, path[i-1].position.z-25.0f);
                //QuickShapes2D.DrawLine(start*10.0f, end*10.0f, 1.0f, Color.blue);
                //QuickShapes.DrawLine(path[i].position, path[i-1].position, 0.1f, Color.yellow);
            }
            StartCoroutine(new ProgressiveLineCo(path.Select(x => new Vector3((x.position.x-25.0f)*10.0f, (x.position.z-25.0f)*10.0f, x.position.z)).ToList(), 2.0f, Color.blue, 10.0f));
            path = null;
        }

    }

    public void StartAnimation()
    {
        Debug.Log("Start Animation!!!!");
        if(Application.isPlaying)
        {
            pd.Pause();
            MyTime.animate = false;
            StartCoroutine(FirstAnimation());
        }
    }

    public void Stop()
    {
        Debug.Break();
    }

    public void DoScoresAnimation()
    {
        if(Application.isPlaying)
        {
            StartCoroutine(ScoresAnimation());
        }
    }

    public void DoDetailedAnimation()
    {
        if(Application.isPlaying)
        {
            MyTime.animate = false;
            StartCoroutine(DetailedAnimation());
        }
    }

    public void ClearGrid()
    {
       astar?.ResetGrid(); 
       path?.Clear();
    }
}
