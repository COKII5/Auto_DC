using System;
using System.Diagnostics;
using AutoDc.Control;
using AutoDc.Protocol;

namespace AutoDc.Communication
{
    public sealed class CarSession
    {
        public const string DefaultHost = "192.168.4.1";
        public const int DefaultPort = 4210;
        public const float MinThrottleLimit = 0.2f;
        public const int MaxSteeringTrimDeg = 15;

        private const float SendPeriodSeconds = 0.02f;
        private const int StopRepeatCount = 3;

        private readonly UdpCarLink _link = new UdpCarLink();
        private readonly LinkMonitor _monitor = new LinkMonitor();
        private ushort _sequence;
        private float _secondsSinceSend;
        private float _throttleLimit = 1f;
        private int _steeringTrimDeg;

        public LinkMonitor Monitor => _monitor;
        public LinkState State { get; private set; } = LinkState.Disconnected;
        public bool IsOpen => _link.IsOpen;
        public int CorruptPackets => _link.CorruptPackets;
        public string LastError { get; private set; } = "";
        public DriveCommand CurrentCommand { get; private set; }

        public float ThrottleLimit
        {
            get => _throttleLimit;
            set => _throttleLimit = Math.Max(MinThrottleLimit, Math.Min(1f, value));
        }

        public int SteeringTrimDeg
        {
            get => _steeringTrimDeg;
            set => _steeringTrimDeg = Math.Max(-MaxSteeringTrimDeg, Math.Min(MaxSteeringTrimDeg, value));
        }

        public bool Connect(string host, int port)
        {
            _monitor.Reset();
            if (!_link.TryOpen(host, port, out string failure))
            {
                LastError = failure;
                return false;
            }
            LastError = "";
            _secondsSinceSend = SendPeriodSeconds;
            return true;
        }

        public void Disconnect()
        {
            if (_link.IsOpen)
            {
                for (int attempt = 0; attempt < StopRepeatCount; attempt++)
                {
                    SendCommand(new DriveIntent(0f, 0f, false));
                }
            }
            _link.Close();
            State = LinkState.Disconnected;
        }

        public void Tick(float deltaTime, DriveIntent intent)
        {
            CurrentCommand = BuildCommand(intent, _sequence);
            ConsumeTelemetry();
            if (_link.Error != null)
            {
                LastError = _link.Error;
            }
            State = _monitor.Evaluate(_link.IsOpen, Stopwatch.GetTimestamp());
            if (!_link.IsOpen)
            {
                return;
            }
            _secondsSinceSend += deltaTime;
            if (_secondsSinceSend >= SendPeriodSeconds)
            {
                _secondsSinceSend = Math.Min(_secondsSinceSend - SendPeriodSeconds, SendPeriodSeconds);
                SendCommand(intent);
            }
        }

        private void ConsumeTelemetry()
        {
            while (_link.TryDequeue(out CarTelemetry telemetry, out long arrivalTimestamp))
            {
                _monitor.RecordTelemetry(telemetry, arrivalTimestamp);
            }
        }

        private void SendCommand(DriveIntent intent)
        {
            _sequence++;
            DriveCommand command = BuildCommand(intent, _sequence);
            if (_link.TrySend(command.Encode()))
            {
                _monitor.RecordSent(command.Sequence, Stopwatch.GetTimestamp());
            }
        }

        private DriveCommand BuildCommand(DriveIntent intent, ushort sequence)
        {
            return new DriveCommand(
                sequence,
                ToPercent(intent.Throttle * _throttleLimit),
                ToPercent(intent.Steering),
                (sbyte)_steeringTrimDeg,
                intent.Brake);
        }

        private static sbyte ToPercent(float normalized)
        {
            float clamped = Math.Max(-1f, Math.Min(1f, normalized));
            return (sbyte)Math.Round(clamped * PacketFormat.CommandLimit);
        }
    }
}
