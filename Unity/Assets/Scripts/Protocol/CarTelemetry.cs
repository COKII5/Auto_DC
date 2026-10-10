using System;

namespace AutoDc.Protocol
{
    public readonly struct CarTelemetry
    {
        private CarTelemetry(ushort ackSequence, sbyte appliedThrottle, sbyte appliedSteering, SteeringMode steeringMode,
            byte maxSteerDeg, byte ackAgeMs, uint uptimeMs, ushort distanceMm, bool failsafe)
        {
            AckSequence = ackSequence;
            AppliedThrottle = appliedThrottle;
            AppliedSteering = appliedSteering;
            SteeringMode = steeringMode;
            MaxSteerDeg = maxSteerDeg;
            AckAgeMs = ackAgeMs;
            UptimeMs = uptimeMs;
            DistanceMm = distanceMm;
            Failsafe = failsafe;
        }

        public ushort AckSequence { get; }
        public sbyte AppliedThrottle { get; }
        public sbyte AppliedSteering { get; }
        public SteeringMode SteeringMode { get; }
        public byte MaxSteerDeg { get; }
        public byte AckAgeMs { get; }
        public uint UptimeMs { get; }
        public ushort DistanceMm { get; }
        public bool HasDistance => DistanceMm != PacketFormat.DistanceUnknown;
        public bool Failsafe { get; }

        public static bool TryDecode(byte[] bytes, int count, out CarTelemetry telemetry)
        {
            telemetry = default;
            if (bytes == null || count != PacketFormat.TelemetrySize
                || bytes[0] != PacketFormat.Magic || bytes[1] != PacketFormat.TelemetryType)
            {
                return false;
            }
            if (Crc8.Compute(bytes, count - 1) != bytes[count - 1])
            {
                return false;
            }
            sbyte throttle = (sbyte)bytes[4];
            sbyte steering = (sbyte)bytes[5];
            SteeringMode mode = (SteeringMode)bytes[6];
            if (!IsWithinLimit(throttle) || !IsWithinLimit(steering) || !Enum.IsDefined(typeof(SteeringMode), mode))
            {
                return false;
            }
            telemetry = new CarTelemetry(
                (ushort)(bytes[2] | (bytes[3] << 8)),
                throttle,
                steering,
                mode,
                bytes[7],
                bytes[8],
                ReadUInt32LittleEndian(bytes, 9),
                (ushort)(bytes[13] | (bytes[14] << 8)),
                (bytes[15] & PacketFormat.FailsafeFlag) != 0);
            return true;
        }

        private static bool IsWithinLimit(sbyte value)
        {
            return value >= -PacketFormat.CommandLimit && value <= PacketFormat.CommandLimit;
        }

        private static uint ReadUInt32LittleEndian(byte[] bytes, int offset)
        {
            return (uint)(bytes[offset] | (bytes[offset + 1] << 8) | (bytes[offset + 2] << 16) | (bytes[offset + 3] << 24));
        }
    }
}
