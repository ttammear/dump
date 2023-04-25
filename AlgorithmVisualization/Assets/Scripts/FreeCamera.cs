using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using UnityEngine;

public class FreeCamera : MonoBehaviour
{
  public static FreeCamera Instance;

  public const float MoveSpeed = 10.0f;
  public float CurrentMoveSpeed = MoveSpeed;
  float _rotX = 0.0f;
  float _rotY = 0.0f;

  public bool Disabled
  {
    get
    {
      return _disabled;
    }
    set
    {
      if(value == false)
      {
        _rotY = -Camera.main.transform.eulerAngles.x;
        _rotX = Camera.main.transform.eulerAngles.y;
      }
      _disabled = value;
    }
  }

  public bool _disabled = false;

  private void Start()
  {
    Instance = this;
  }

  void LateUpdate()
  {
    if (Disabled)
      return;

    Transform camtrans = Camera.main.transform;
    bool moving = false;
    if (Input.GetKey(KeyCode.W))
    {
      moving = true;
      camtrans.position += camtrans.forward * Time.deltaTime * CurrentMoveSpeed;
    }
    if (Input.GetKey(KeyCode.S))
    {
      moving = true;
      camtrans.position -= camtrans.forward * Time.deltaTime * CurrentMoveSpeed;
    }
    if (Input.GetKey(KeyCode.A))
    {
      moving = true;
      camtrans.position -= camtrans.right * Time.deltaTime * CurrentMoveSpeed;
    }
    if (Input.GetKey(KeyCode.D))
    {
      moving = true;
      camtrans.position += camtrans.right * Time.deltaTime * CurrentMoveSpeed;
    }

    if (!moving)
      CurrentMoveSpeed -= CurrentMoveSpeed * Time.deltaTime;

    CurrentMoveSpeed = Mathf.Max(CurrentMoveSpeed, MoveSpeed);
    if(Input.GetKey(KeyCode.LeftShift))
      CurrentMoveSpeed += CurrentMoveSpeed * Time.deltaTime;

    _rotX += Input.GetAxis("Mouse X");
    _rotY += Input.GetAxis("Mouse Y");

    camtrans.rotation = Quaternion.AngleAxis(_rotX, Vector3.up) * Quaternion.AngleAxis(_rotY, -Vector3.right);
  }
}