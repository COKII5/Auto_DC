namespace AutoDc.Twin
{
    public readonly struct VehicleGeometry
    {
        public VehicleGeometry(float maxSpeedMetersPerSecond, float wheelBaseMeters, float trackWidthMeters, float maxSteerDeg)
        {
            MaxSpeedMetersPerSecond = maxSpeedMetersPerSecond;
            WheelBaseMeters = wheelBaseMeters;
            TrackWidthMeters = trackWidthMeters;
            MaxSteerDeg = maxSteerDeg;
        }

        public float MaxSpeedMetersPerSecond { get; }
        public float WheelBaseMeters { get; }
        public float TrackWidthMeters { get; }
        public float MaxSteerDeg { get; }
    }
}
