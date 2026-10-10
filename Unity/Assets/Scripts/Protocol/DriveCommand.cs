namespace AutoDc.Protocol
{
    public readonly struct DriveCommand
    {
        public DriveCommand(ushort sequence, sbyte throttle, sbyte steering, sbyte steeringTrimDeg, bool brake)
        {
            Sequence = sequence;
            Throttle = throttle;
            Steering = steering;
            SteeringTrimDeg = steeringTrimDeg;
            Brake = brake;
        }

        public ushort Sequence { get; }
        public sbyte Throttle { get; }
        public sbyte Steering { get; }
        public sbyte SteeringTrimDeg { get; }
        public bool Brake { get; }

        public byte[] Encode()
        {
            byte[] bytes = new byte[PacketFormat.DriveSize];
            bytes[0] = PacketFormat.Magic;
            bytes[1] = PacketFormat.DriveType;
            bytes[2] = (byte)(Sequence & 0xFF);
            bytes[3] = (byte)(Sequence >> 8);
            bytes[4] = (byte)Throttle;
            bytes[5] = (byte)Steering;
            bytes[6] = (byte)SteeringTrimDeg;
            bytes[7] = Brake ? PacketFormat.BrakeFlag : (byte)0x00;
            bytes[8] = Crc8.Compute(bytes, PacketFormat.DriveSize - 1);
            return bytes;
        }
    }
}
