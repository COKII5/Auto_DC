namespace AutoDc.Twin
{
    public interface IVehicleKinematics
    {
        BodyVelocity Compute(float throttle, float steering, VehicleGeometry geometry);
    }
}
