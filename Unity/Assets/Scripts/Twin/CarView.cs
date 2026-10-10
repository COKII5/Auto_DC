using UnityEngine;

namespace AutoDc.Twin
{
    public sealed class CarView : MonoBehaviour
    {
        private const float WheelWidth = 0.012f;
        private const float ChassisThickness = 0.004f;
        private const float ChassisOverhang = 0.025f;
        private const float WheelMarkerScale = 0.45f;
        private const float TrailSeconds = 20f;
        private const float TrailWidth = 0.01f;
        private const float TrailHeight = 0.002f;
        private const string TrailShaderName = "Sprites/Default";

        private static readonly Color ChassisColor = new Color(0.85f, 0.85f, 0.82f);
        private static readonly Color WheelColor = new Color(0.12f, 0.12f, 0.12f);
        private static readonly Color WheelMarkerColor = new Color(0.98f, 0.78f, 0.15f);
        private static readonly Color BridgeColor = new Color(0.75f, 0.12f, 0.12f);
        private static readonly Color HeatsinkColor = new Color(0.08f, 0.08f, 0.08f);
        private static readonly Color BoardColor = new Color(0.1f, 0.12f, 0.15f);
        private static readonly Color BatteryColor = new Color(0.2f, 0.45f, 0.75f);
        private static readonly Color ServoColor = new Color(0.15f, 0.3f, 0.85f);
        private static readonly Color TrailColor = new Color(0.1f, 0.6f, 1f, 0.7f);

        private Transform _frontLeftPivot;
        private Transform _frontRightPivot;
        private Transform _frontLeftSpinner;
        private Transform _frontRightSpinner;
        private Transform _rearLeftSpinner;
        private Transform _rearRightSpinner;
        private TrailRenderer _trail;

        public void Build(float wheelBase, float trackWidth, float wheelRadius)
        {
            float halfTrack = trackWidth * 0.5f;
            float chassisLength = wheelBase + 2f * ChassisOverhang;
            float chassisWidth = trackWidth - 2f * WheelWidth;
            float deckHeight = wheelRadius + ChassisThickness * 0.5f;

            PrimitiveFactory.CreatePart(PrimitiveType.Cube, "Chassis", transform, new Vector3(0f, wheelRadius, wheelBase * 0.5f),
                new Vector3(chassisWidth, ChassisThickness, chassisLength), ChassisColor);
            BuildElectronics(wheelBase, deckHeight);

            _rearLeftSpinner = CreateWheel("RearLeftWheel", transform, new Vector3(-halfTrack, wheelRadius, 0f), wheelRadius);
            _rearRightSpinner = CreateWheel("RearRightWheel", transform, new Vector3(halfTrack, wheelRadius, 0f), wheelRadius);
            _frontLeftPivot = CreatePivot("FrontLeftPivot", new Vector3(-halfTrack, wheelRadius, wheelBase));
            _frontRightPivot = CreatePivot("FrontRightPivot", new Vector3(halfTrack, wheelRadius, wheelBase));
            _frontLeftSpinner = CreateWheel("FrontLeftWheel", _frontLeftPivot, Vector3.zero, wheelRadius);
            _frontRightSpinner = CreateWheel("FrontRightWheel", _frontRightPivot, Vector3.zero, wheelRadius);
            _trail = CreateTrail();
        }

        public void SetWheelState(float frontAngleDeg, float leftSpinDeg, float rightSpinDeg)
        {
            Quaternion steer = Quaternion.Euler(0f, frontAngleDeg, 0f);
            _frontLeftPivot.localRotation = steer;
            _frontRightPivot.localRotation = steer;
            _rearLeftSpinner.localRotation = Quaternion.Euler(leftSpinDeg, 0f, 0f);
            _rearRightSpinner.localRotation = Quaternion.Euler(rightSpinDeg, 0f, 0f);
            _frontLeftSpinner.localRotation = Quaternion.Euler(leftSpinDeg, 0f, 0f);
            _frontRightSpinner.localRotation = Quaternion.Euler(rightSpinDeg, 0f, 0f);
        }

        public void ClearTrail()
        {
            if (_trail != null)
            {
                _trail.Clear();
            }
        }

        private void BuildElectronics(float wheelBase, float deckHeight)
        {
            PrimitiveFactory.CreatePart(PrimitiveType.Cube, "Batteries", transform, new Vector3(0f, deckHeight + 0.01f, 0.005f),
                new Vector3(0.04f, 0.02f, 0.065f), BatteryColor);
            PrimitiveFactory.CreatePart(PrimitiveType.Cube, "L298N", transform, new Vector3(0f, deckHeight + 0.022f, 0.045f),
                new Vector3(0.043f, 0.004f, 0.043f), BridgeColor);
            PrimitiveFactory.CreatePart(PrimitiveType.Cube, "L298NHeatsink", transform, new Vector3(0f, deckHeight + 0.034f, 0.045f),
                new Vector3(0.023f, 0.02f, 0.016f), HeatsinkColor);
            PrimitiveFactory.CreatePart(PrimitiveType.Cube, "ESP32", transform, new Vector3(0f, deckHeight + 0.004f, 0.09f),
                new Vector3(0.028f, 0.006f, 0.05f), BoardColor);
            PrimitiveFactory.CreatePart(PrimitiveType.Cube, "SteeringServo", transform, new Vector3(0f, deckHeight + 0.011f, wheelBase + 0.005f),
                new Vector3(0.023f, 0.022f, 0.012f), ServoColor);
        }

        private Transform CreatePivot(string objectName, Vector3 localPosition)
        {
            Transform pivot = new GameObject(objectName).transform;
            pivot.SetParent(transform, false);
            pivot.localPosition = localPosition;
            return pivot;
        }

        private static Transform CreateWheel(string objectName, Transform parent, Vector3 localPosition, float radius)
        {
            Transform spinner = new GameObject(objectName).transform;
            spinner.SetParent(parent, false);
            spinner.localPosition = localPosition;
            float diameter = radius * 2f;
            GameObject tire = PrimitiveFactory.CreatePart(PrimitiveType.Cylinder, "Tire", spinner, Vector3.zero,
                new Vector3(diameter, WheelWidth * 0.5f, diameter), WheelColor);
            tire.transform.localRotation = Quaternion.Euler(0f, 0f, 90f);
            PrimitiveFactory.CreatePart(PrimitiveType.Cube, "Marker", spinner, new Vector3(0f, radius * 0.55f, 0f),
                new Vector3(WheelWidth * 1.1f, radius * WheelMarkerScale, radius * 0.25f), WheelMarkerColor);
            return spinner;
        }

        private TrailRenderer CreateTrail()
        {
            Shader trailShader = Shader.Find(TrailShaderName);
            if (trailShader == null)
            {
                Debug.LogWarning($"[AutoDC] Shader '{TrailShaderName}' no disponible: el gemelo se dibuja sin rastro.");
                return null;
            }
            Transform anchor = CreatePivot("TrailAnchor", new Vector3(0f, TrailHeight, 0f));
            TrailRenderer trail = anchor.gameObject.AddComponent<TrailRenderer>();
            trail.time = TrailSeconds;
            trail.widthMultiplier = TrailWidth;
            trail.minVertexDistance = 0.01f;
            trail.alignment = LineAlignment.TransformZ;
            anchor.localRotation = Quaternion.Euler(90f, 0f, 0f);
            trail.material = new Material(trailShader);
            trail.startColor = TrailColor;
            trail.endColor = new Color(TrailColor.r, TrailColor.g, TrailColor.b, 0f);
            trail.shadowCastingMode = UnityEngine.Rendering.ShadowCastingMode.Off;
            return trail;
        }
    }
}
