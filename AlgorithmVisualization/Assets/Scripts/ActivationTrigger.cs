using System;
using UnityEngine;
using UnityEngine.Events;

[System.Serializable]
public class ActivationTriggerEvent : UnityEvent{}

public class ActivationTrigger : MonoBehaviour
{
    public ActivationTriggerEvent enableEvent;
    public ActivationTriggerEvent disableEvent;

    void OnEnable()
    {
        if(enableEvent != null)
            enableEvent.Invoke();
    }

    void OnDisable()
    {
        if(disableEvent != null)
            disableEvent.Invoke();
    }
}
