using UnityEngine;

namespace AutoDc.Twin
{
    public sealed class ServoSteeringKinematics : IVehicleKinematics
    {
        public BodyVelocity Compute(float throttle, float steering, VehicleGeometry geometry)
        {
            float speed = throttle * geometry.MaxSpeedMetersPerSecond;
            float steerDeg = steering * geometry.MaxSteerDeg;
            float yawRateRad = speed * Mathf.Tan(steerDeg * Mathf.Deg2Rad) / geometry.WheelBaseMeters;
            return new BodyVelocity(speed, yawRateRad * Mathf.Rad2Deg, steerDeg, speed, speed);
        }
    }
}
