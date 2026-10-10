using UnityEngine;

namespace AutoDc.Twin
{
    public static class PrimitiveFactory
    {
        public static GameObject CreatePart(PrimitiveType type, string objectName, Transform parent,
            Vector3 localPosition, Vector3 localScale, Color color)
        {
            GameObject part = GameObject.CreatePrimitive(type);
            part.name = objectName;
            Object.Destroy(part.GetComponent<Collider>());
            part.transform.SetParent(parent, false);
            part.transform.localPosition = localPosition;
            part.transform.localScale = localScale;
            part.GetComponent<Renderer>().material.color = color;
            return part;
        }
    }
}
