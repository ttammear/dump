using System;
using System.Collections.Generic;
using System.Linq;
using UnityEngine;
using System.Text;

public class TestScript : MonoBehaviour
{
  private void Update()
  {
    var fadeGreen = new Color(0.0f, 1.0f, 0.0f, 0.5f);

    QuickShapes.DrawCube(Vector3.forward * 3.0f, 1.0f, Color.red);
    QuickShapes.DrawCube(Vector3.right*2.0f + Vector3.forward * 3.0f, 1.0f, Color.blue);
    QuickShapes.DrawCube(Vector3.left*2.0f + Vector3.forward * 3.0f, 1.0f, fadeGreen);
    //var fadeRed = new Color(1.0f, 0.0f, 0.0f, 0.5f);
    //QuickShapes.DrawGrid2D(new Vector3(-50.0f, 0.0f, -50.0f), 1.0f, 100.0f, 0.01f, Color.red);
    QuickShapes.DrawGrid3D(new Vector3(-1.5f, -1.5f, -1.5f), 1.0f, 3, 0.01f, Color.red);
  }

  private void OnPostRender()
  {

  }
}
