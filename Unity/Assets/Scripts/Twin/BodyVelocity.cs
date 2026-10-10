namespace AutoDc.Twin
{
    public readonly struct BodyVelocity
    {
        public BodyVelocity(float forwardSpeed, float yawRateDegPerSecond, float frontWheelAngleDeg,
            float leftWheelSpeed, float rightWheelSpeed)
        {
            ForwardSpeed = forwardSpeed;
            YawRateDegPerSecond = yawRateDegPerSecond;
            FrontWheelAngleDeg = frontWheelAngleDeg;
            LeftWheelSpeed = leftWheelSpeed;
            RightWheelSpeed = rightWheelSpeed;
        }

        public float ForwardSpeed { get; }
        public float YawRateDegPerSecond { get; }
        public float FrontWheelAngleDeg { get; }
        public float LeftWheelSpeed { get; }
        public float RightWheelSpeed { get; }
    }
}
