using UnityEngine;

namespace AutoDc.Twin
{
    public sealed class DifferentialKinematics : IVehicleKinematics
    {
        public BodyVelocity Compute(float throttle, float steering, VehicleGeometry geometry)
        {
            float left = throttle + steering;
            float right = throttle - steering;
            float largest = Mathf.Max(Mathf.Abs(left), Mathf.Abs(right));
            if (largest > 1f)
            {
                left /= largest;
                right /= largest;
            }
            float leftSpeed = left * geometry.MaxSpeedMetersPerSecond;
            float rightSpeed = right * geometry.MaxSpeedMetersPerSecond;
            float yawRateRad = (leftSpeed - rightSpeed) / geometry.TrackWidthMeters;
            return new BodyVelocity((leftSpeed + rightSpeed) * 0.5f, yawRateRad * Mathf.Rad2Deg, 0f, leftSpeed, rightSpeed);
        }
    }
}
