using AutoDc.Protocol;
using UnityEngine;

namespace AutoDc.Twin
{
    public sealed class VehicleTwin : MonoBehaviour
    {
        private const float MinDimensionMeters = 0.01f;
        private const float MinWheelRadiusMeters = 0.001f;
        private const float MaxSteerLimitDeg = 60f;
        private const float DegreesPerTurn = 360f;

        [Header("Geometria (medir en el carro real)")]
        [SerializeField] private float _wheelBaseMeters = 0.12f;
        [SerializeField] private float _trackWidthMeters = 0.10f;
        [Tooltip("Radio de rueda medido en el carro: 2,4 cm.")]
        [SerializeField] private float _wheelRadiusMeters = 0.024f;

        [Header("Respuesta")]
        [Tooltip("Velocidad lineal sin carga a 6 V del informe, Ec. (19).")]
        [SerializeField] private float _maxSpeedMetersPerSecond = 1.257f;
        [SerializeField] private float _motorTimeConstantSeconds = 0.15f;
        [SerializeField] private float _brakeTimeConstantSeconds = 0.03f;
        [SerializeField] private float _servoTimeConstantSeconds = 0.08f;

        [Header("Simulacion sin conexion")]
        [SerializeField] private SteeringMode _offlineSteeringMode = SteeringMode.Servo;
        [SerializeField] private float _offlineMaxSteerDeg = 30f;

        private readonly IVehicleKinematics _servoKinematics = new ServoSteeringKinematics();
        private readonly IVehicleKinematics _differentialKinematics = new DifferentialKinematics();
        private CarView _view;
        private Vector3 _startPosition;
        private Quaternion _startRotation;
        private float _throttle;
        private float _steering;
        private float _leftWheelSpinDeg;
        private float _rightWheelSpinDeg;

        public float WheelBaseMeters => _wheelBaseMeters;
        public float TrackWidthMeters => _trackWidthMeters;
        public float WheelRadiusMeters => _wheelRadiusMeters;
        public SteeringMode OfflineSteeringMode => _offlineSteeringMode;
        public float OfflineMaxSteerDeg => _offlineMaxSteerDeg;
        public SteeringMode ActiveSteeringMode { get; private set; }
        public float ForwardSpeed { get; private set; }
        public float FrontWheelAngleDeg { get; private set; }

        public void Bind(CarView view)
        {
            _view = view;
            _startPosition = transform.position;
            _startRotation = transform.rotation;
        }

        public void Drive(float throttle, float steering, bool brake, SteeringMode mode, float maxSteerDeg, float deltaTime)
        {
            float throttleTimeConstant = brake ? _brakeTimeConstantSeconds : _motorTimeConstantSeconds;
            float steeringTimeConstant = mode == SteeringMode.Servo ? _servoTimeConstantSeconds : _motorTimeConstantSeconds;
            _throttle = FirstOrderLag(_throttle, brake ? 0f : throttle, throttleTimeConstant, deltaTime);
            _steering = FirstOrderLag(_steering, steering, steeringTimeConstant, deltaTime);

            ActiveSteeringMode = mode;
            IVehicleKinematics kinematics = mode == SteeringMode.Servo ? _servoKinematics : _differentialKinematics;
            VehicleGeometry geometry = new VehicleGeometry(_maxSpeedMetersPerSecond, _wheelBaseMeters, _trackWidthMeters, maxSteerDeg);
            BodyVelocity velocity = kinematics.Compute(_throttle, _steering, geometry);
            Integrate(velocity, deltaTime);
        }

        public void ResetPose()
        {
            transform.SetPositionAndRotation(_startPosition, _startRotation);
            _throttle = 0f;
            _steering = 0f;
            ForwardSpeed = 0f;
            if (_view != null)
            {
                _view.ClearTrail();
            }
        }

        private void Integrate(BodyVelocity velocity, float deltaTime)
        {
            float halfYaw = velocity.YawRateDegPerSecond * deltaTime * 0.5f;
            transform.Rotate(0f, halfYaw, 0f, Space.Self);
            transform.position += transform.forward * (velocity.ForwardSpeed * deltaTime);
            transform.Rotate(0f, halfYaw, 0f, Space.Self);

            ForwardSpeed = velocity.ForwardSpeed;
            FrontWheelAngleDeg = velocity.FrontWheelAngleDeg;
            _leftWheelSpinDeg = AdvanceSpin(_leftWheelSpinDeg, velocity.LeftWheelSpeed, deltaTime);
            _rightWheelSpinDeg = AdvanceSpin(_rightWheelSpinDeg, velocity.RightWheelSpeed, deltaTime);
            if (_view != null)
            {
                _view.SetWheelState(FrontWheelAngleDeg, _leftWheelSpinDeg, _rightWheelSpinDeg);
            }
        }

        private float AdvanceSpin(float spinDeg, float wheelSpeed, float deltaTime)
        {
            return Mathf.Repeat(spinDeg + wheelSpeed * deltaTime / _wheelRadiusMeters * Mathf.Rad2Deg, DegreesPerTurn);
        }

        private static float FirstOrderLag(float current, float target, float timeConstant, float deltaTime)
        {
            if (timeConstant <= 0f)
            {
                return target;
            }
            return current + (target - current) * (1f - Mathf.Exp(-deltaTime / timeConstant));
        }

        private void OnValidate()
        {
            _wheelBaseMeters = Mathf.Max(MinDimensionMeters, _wheelBaseMeters);
            _trackWidthMeters = Mathf.Max(MinDimensionMeters, _trackWidthMeters);
            _wheelRadiusMeters = Mathf.Max(MinWheelRadiusMeters, _wheelRadiusMeters);
            _offlineMaxSteerDeg = Mathf.Clamp(_offlineMaxSteerDeg, 0f, MaxSteerLimitDeg);
        }
    }
}
