namespace AutoDc.Control
{
    public readonly struct DriveIntent
    {
        public DriveIntent(float throttle, float steering, bool brake)
        {
            Throttle = throttle;
            Steering = steering;
            Brake = brake;
        }

        public float Throttle { get; }
        public float Steering { get; }
        public bool Brake { get; }
    }
}
