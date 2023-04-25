using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using UnityEngine;
using UnityEngine.UI;
using TMPro;

[DefaultExecutionOrder(1000)]
public class QuickLabels : MonoBehaviour
{
    struct Label
    {
        public string text;
        public int size;
        public Color color;
        public Vector2 pos;
        public float? width;
        public TextAnchor anchor;
    }

    struct Label3D
    {
        public string text;
        public Vector3 position;
        public Color color;
        public int size;
    }

    struct BGLabel
    {
        public Text text;
        public GameObject bg;
    }

    static int _lastId = 0;
    static Dictionary<int, Label> _labels = new Dictionary<int, Label>();

    static Dictionary<int, BGLabel> _bgLabels = new Dictionary<int, BGLabel>();
    static Dictionary<int, Image> _images = new Dictionary<int, Image>();

    static Dictionary<int, TextMeshProUGUI> _tmTextLabels = new Dictionary<int, TextMeshProUGUI>();

    static Canvas canvas;

    void Awake()
    {
        canvas = GetComponent<Canvas>();
        if(canvas == null)
            canvas = FindObjectOfType<Canvas>();
    }

    /*public static int ShowLabel(Vector2 pos, string text, int size, Color color, float? width = null, TextAnchor anchor = TextAnchor.UpperLeft)
    {
        var ret = _lastId++;
        var label = new Label()
        {
            text = text,
            size = size,
            color = color,
            pos = pos,
            width = width,
            anchor = anchor
        };
        _labels.Add(ret, label);
        return ret;
    }*/

    /*public static int ShowLabel(Vector2 pos, string text, int size, Color color, float? width = null, TextAnchor anchor = TextAnchor.UpperLeft)
    {
        RectTransform canvasTrans = canvas.transform as RectTransform;
        var newText = new GameObject("Text");
        newText.transform.SetParent(canvas.gameObject.transform);
        var textcomp = newText.AddComponent<Text>();
        textcomp.text = text;
        textcomp.font = Resources.GetBuiltinResource<Font>("Arial.ttf");
        textcomp.fontSize = size;
        var textTrans = newText.transform as RectTransform;
        textTrans.anchoredPosition3D = new Vector3(pos.x, canvasTrans.sizeDelta.y - pos.y, 0.0f);
        textTrans.sizeDelta = new Vector2(width == null ? 9000.0f : width.Value, 9000.0f);
        textTrans.localScale = new Vector3(1.0f, 1.0f, 1.0f);
        textTrans.localRotation = Quaternion.identity;
        textTrans.anchorMin = new Vector2(0.0f, 0.0f);
        textTrans.anchorMax = new Vector2(0.0f, 0.0f);
        textTrans.pivot = new Vector2(0.0f, 1.0f);
        textcomp.color = color;
        var ret = _lastId++;
        _textLabels.Add(ret, textcomp);
         return ret;
    }*/

    public static int ShowLabel(Vector2 pos, string text, int size, Color color, float? width = null, TextAnchor anchor = TextAnchor.UpperLeft)
    {
        RectTransform canvasTrans = canvas.transform as RectTransform;
        var newText = new GameObject("ShowLabel() Text");
        newText.transform.SetParent(canvas.transform);
        var textcomp = newText.AddComponent<TextMeshProUGUI>();
        textcomp.text = text;
        textcomp.fontSize = size;
        var textTrans = newText.transform as RectTransform;
        textTrans.anchoredPosition3D = new Vector3(pos.x, canvasTrans.sizeDelta.y - pos.y, 0.0f);
        textTrans.sizeDelta = new Vector2(width == null ? 9000.0f : width.Value, 9000.0f);
        textTrans.localScale = new Vector3(1.0f, 1.0f, 1.0f);
        textTrans.localRotation = Quaternion.identity;
        textTrans.anchorMin = new Vector2(0.0f, 0.0f);
        textTrans.anchorMax = new Vector2(0.0f, 0.0f);
        textTrans.pivot = new Vector2(0.0f, 1.0f);
        textcomp.color = color;
        var ret = _lastId++;
        _tmTextLabels.Add(ret, textcomp);
        newText.SetActive(true);
        return ret;
    }

    public static void ChangeLabel(int label, Vector2 pos, string text, Color color)
    {
        RectTransform canvasTrans = canvas.transform as RectTransform;
        var textcomp = _tmTextLabels[label];
        textcomp.text = text;
        var textTrans = textcomp.transform as RectTransform;
        textTrans.anchoredPosition3D = new Vector3(pos.x, canvasTrans.sizeDelta.y - pos.y, 0.0f);
        textcomp.color = color;
    }

    public static int ShowLabelWithBackground(Rect rect, string text, int size, Color color, Color background)
    {
        RectTransform canvasTrans = canvas.transform as RectTransform;
        var newText = new GameObject("Text");
        newText.transform.SetParent(canvas.gameObject.transform);
        var textcomp = newText.AddComponent<Text>();
        textcomp.text = text;
        textcomp.font = Resources.GetBuiltinResource<Font>("Arial.ttf");
        textcomp.fontSize = size;
        var textTrans = newText.transform as RectTransform;
        textTrans.anchoredPosition3D = new Vector3(rect.xMin, canvasTrans.sizeDelta.y - rect.yMin, 0.0f);
        textTrans.sizeDelta = new Vector2(rect.width, rect.height);
        textTrans.localScale = new Vector3(1.0f, 1.0f, 1.0f);
        textTrans.localRotation = Quaternion.identity;
        textTrans.anchorMin = new Vector2(0.0f, 0.0f);
        textTrans.anchorMax = new Vector2(0.0f, 0.0f);
        textTrans.pivot = new Vector2(0.0f, 1.0f);
        textTrans.SetSiblingIndex(0);
        textcomp.color = color;

        var newBG = new GameObject("Background");
        newBG.transform.SetParent(canvas.gameObject.transform);
        var bgComp = newBG.AddComponent<Image>();
        var bgTrans = newBG.transform as RectTransform;
        float halfSize = size * 0.5f;
        bgTrans.anchoredPosition3D = new Vector3(rect.xMin - size, canvasTrans.sizeDelta.y - rect.yMin + halfSize, 0.0f);
        bgComp.color = background;
        bgTrans.sizeDelta = new Vector2(rect.width + size*2.0f, rect.height + size);
        bgTrans.localScale = Vector3.one;
        bgTrans.localRotation = Quaternion.identity;
        bgTrans.anchorMin = new Vector2(0.0f, 0.0f);
        bgTrans.anchorMax = new Vector2(0.0f, 0.0f);
        bgTrans.pivot = new Vector2(0.0f, 1.0f);
        bgTrans.SetSiblingIndex(0);

        var ret = _lastId++;
        _bgLabels.Add(ret, new BGLabel() {text = textcomp, bg = newBG} );
        return ret;
    }

    public static int ShowImage(Vector2 pos, Sprite image, float width)
    {
        RectTransform canvasTrans = canvas.transform as RectTransform;
        var newBG = new GameObject("Background");
        newBG.transform.SetParent(canvas.gameObject.transform);
        var bgComp = newBG.AddComponent<Image>();
        var bgTrans = newBG.transform as RectTransform;
        bgTrans.anchoredPosition3D = new Vector3(pos.x, canvasTrans.sizeDelta.y - pos.y, 0.0f);
        bgTrans.sizeDelta = new Vector2(width, 99999.0f);
        bgTrans.localScale = Vector3.one;
        bgTrans.localRotation = Quaternion.identity;
        bgTrans.anchorMin = new Vector2(0.0f, 0.0f);
        bgTrans.anchorMax = new Vector2(0.0f, 0.0f);
        bgTrans.pivot = new Vector2(0.0f, 1.0f);
        bgComp.sprite = image;
        bgComp.preserveAspect = true;
        var ret = _lastId++;
        _images.Add(ret, bgComp);
        return ret;
    }

    public static int Show3DLabel(Vector3 pos, string text, int size, Color color, Vector2 screenOffset = new Vector2())
    {
        Vector3 pos2 = Camera.main.WorldToScreenPoint(pos);
        float swidth = Camera.main.targetTexture == null ? Screen.width : Camera.main.targetTexture.width;
        float sheight = Camera.main.targetTexture == null ? Screen.height : Camera.main.targetTexture.height;
        var gpos = new Rect(pos2.x, pos2.y, 1.0f, 1.0f);

        int ret = ShowLabel(new Vector2(gpos.x + screenOffset.x, gpos.y + screenOffset.y), text, size, color);

        return ret;
    }

    public static void Change3DLabel(int label, Vector3 pos, string text, Color color, Vector2 screenOffset = new Vector2())
    {
        Vector3 pos2 = Camera.main.WorldToScreenPoint(pos);
        float swidth = Camera.main.targetTexture == null ? Screen.width : Camera.main.targetTexture.width;
        float sheight = Camera.main.targetTexture == null ? Screen.height : Camera.main.targetTexture.height;
        var gpos = new Rect(pos2.x, pos2.y, 1.0f, 1.0f);
        ChangeLabel(label, new Vector2(gpos.x + screenOffset.x, gpos.y + screenOffset.y), text, color);
    }

    public static void HideLabel(int label)
    {
        TextMeshProUGUI text;
        _tmTextLabels.TryGetValue(label, out text);
        Destroy(text.gameObject);
        _tmTextLabels.Remove(label);
    }

    public static void HideLabelWithBackground(int label)
    {
        BGLabel olabel;
        _bgLabels.TryGetValue(label, out olabel);
        Destroy(olabel.text.gameObject);
        Destroy(olabel.bg);
    }

    public static void Hide3DLabel(int label)
    {
        HideLabel(label);
    }

    public static void HideImage(int img)
    {
        Image oimg;
        _images.TryGetValue(img, out oimg);
        Destroy(oimg.gameObject);
    }

    static GUIStyle style = new GUIStyle();
    static FontStyle fstyle = new FontStyle();

    private void OnGUI()
    {
        var rt = RenderTexture.active;
        RenderTexture.active = Camera.main.targetTexture;

        style.richText = true;
        foreach (var lbl in _labels)
        {
            var label = lbl.Value;
            style.fontSize = label.size;
            style.normal.textColor = label.color;
            style.wordWrap = true;
            style.alignment = label.anchor;
            GUI.Label(new Rect(label.pos.x, label.pos.y, label.width == null ? 1000.0f : label.width.Value, 100.0f * label.size), label.text, style);
        }

        RenderTexture.active = rt;
    }


}
