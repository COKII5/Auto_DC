using AutoDc.Communication;
using AutoDc.Control;
using AutoDc.Protocol;
using AutoDc.Twin;
using AutoDc.UI;
using AutoDc.World;
using UnityEngine;

namespace AutoDc
{
    public sealed class AutoDcApp : MonoBehaviour
    {
        private const int TargetFrameRate = 60;
        private const float CameraNearClip = 0.01f;
        private const float CameraFieldOfView = 55f;

        private readonly DriveInput _input = new DriveInput();
        private readonly CarSession _session = new CarSession();
        private readonly DriveHud _driveHud = new DriveHud();
        private ConnectionPanel _connectionPanel;
        private VehicleTwin _twin;
        private TwinSource _twinSource = TwinSource.LocalSimulation;

        private void Awake()
        {
            Application.targetFrameRate = TargetFrameRate;
            ArenaBuilder.Build(transform);
            _twin = CreateCar();
            CreateCamera(_twin.transform);
            _connectionPanel = new ConnectionPanel(_session);
        }

        private void Update()
        {
            float deltaTime = Time.deltaTime;
            DriveIntent intent = _input.Read(deltaTime);
            if (_input.ResetRequested)
            {
                _twin.ResetPose();
            }
            _session.Tick(deltaTime, intent);
            DriveTwin(deltaTime);
        }

        private void OnGUI()
        {
            _connectionPanel.Draw();
            _driveHud.Draw(_session, _twin, _twinSource);
        }

        private void OnDestroy()
        {
            _session.Disconnect();
        }

        private void DriveTwin(float deltaTime)
        {
            _twinSource = SelectTwinSource(_session.State);
            DriveCommand command = _session.CurrentCommand;
            switch (_twinSource)
            {
                case TwinSource.Telemetry:
                    CarTelemetry telemetry = _session.Monitor.LastTelemetry;
                    _twin.Drive(PacketFormat.ToNormalized(telemetry.AppliedThrottle), PacketFormat.ToNormalized(telemetry.AppliedSteering),
                        command.Brake, telemetry.SteeringMode, telemetry.MaxSteerDeg, deltaTime);
                    break;
                case TwinSource.LocalSimulation:
                    _twin.Drive(PacketFormat.ToNormalized(command.Throttle), PacketFormat.ToNormalized(command.Steering),
                        command.Brake, _twin.OfflineSteeringMode, _twin.OfflineMaxSteerDeg, deltaTime);
                    break;
                default:
                    _twin.Drive(0f, 0f, true, _twin.ActiveSteeringMode, _twin.OfflineMaxSteerDeg, deltaTime);
                    break;
            }
        }

        private static TwinSource SelectTwinSource(LinkState state)
        {
            switch (state)
            {
                case LinkState.Connected:
                    return TwinSource.Telemetry;
                case LinkState.Disconnected:
                    return TwinSource.LocalSimulation;
                default:
                    return TwinSource.Halted;
            }
        }

        private VehicleTwin CreateCar()
        {
            GameObject car = new GameObject("CarTwin");
            car.transform.SetParent(transform, false);
            VehicleTwin twin = car.AddComponent<VehicleTwin>();
            CarView view = car.AddComponent<CarView>();
            view.Build(twin.WheelBaseMeters, twin.TrackWidthMeters, twin.WheelRadiusMeters);
            twin.Bind(view);
            return twin;
        }

        private void CreateCamera(Transform target)
        {
            Camera sceneCamera = new GameObject("MainCamera").AddComponent<Camera>();
            sceneCamera.tag = "MainCamera";
            sceneCamera.transform.SetParent(transform, false);
            sceneCamera.nearClipPlane = CameraNearClip;
            sceneCamera.fieldOfView = CameraFieldOfView;
            sceneCamera.clearFlags = CameraClearFlags.SolidColor;
            sceneCamera.backgroundColor = ArenaBuilder.BackgroundColor;
            sceneCamera.gameObject.AddComponent<AudioListener>();
            sceneCamera.gameObject.AddComponent<FollowCamera>().Follow(target);
        }
    }
}
