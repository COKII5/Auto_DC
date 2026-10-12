namespace AutoDc.Protocol
{
    public static class PacketFormat
    {
        public const byte Magic = 0xAD;
        public const byte DriveType = 0x01;
        public const byte TelemetryType = 0x81;
        public const int DriveSize = 9;
        public const int TelemetrySize = 17;
        public const ushort DistanceUnknown = 0xFFFF;
        public const byte BrakeFlag = 0x01;
        public const byte FailsafeFlag = 0x01;
        public const int CommandLimit = 100;

        public static float ToNormalized(int percent)
        {
            return (float)percent / CommandLimit;
        }
    }
}
