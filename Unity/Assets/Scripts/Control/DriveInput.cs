using UnityEngine;
using UnityEngine.InputSystem;

namespace AutoDc.Control
{
    public sealed class DriveInput
    {
        private const float KeyboardRampPerSecond = 3f;
        private const float KeyboardReleasePerSecond = 6f;
        private const float StickDeadZone = 0.12f;

        private float _keyboardThrottle;
        private float _keyboardSteering;

        public bool ResetRequested { get; private set; }

        public DriveIntent Read(float deltaTime)
        {
            Keyboard keyboard = Keyboard.current;
            Gamepad gamepad = Gamepad.current;
            ReadKeyboardAxes(keyboard, deltaTime);

            float padThrottle = 0f;
            float padSteering = 0f;
            bool padBrake = false;
            if (gamepad != null)
            {
                float triggers = gamepad.rightTrigger.ReadValue() - gamepad.leftTrigger.ReadValue();
                Vector2 stick = gamepad.leftStick.ReadValue();
                padThrottle = ApplyDeadZone(LargestMagnitude(triggers, stick.y));
                padSteering = ApplyDeadZone(stick.x);
                padBrake = gamepad.buttonSouth.isPressed;
            }

            bool keyboardBrake = keyboard != null && keyboard.spaceKey.isPressed;
            ResetRequested = (keyboard != null && keyboard.rKey.wasPressedThisFrame)
                || (gamepad != null && gamepad.selectButton.wasPressedThisFrame);

            return new DriveIntent(
                LargestMagnitude(_keyboardThrottle, padThrottle),
                LargestMagnitude(_keyboardSteering, padSteering),
                keyboardBrake || padBrake);
        }

        private void ReadKeyboardAxes(Keyboard keyboard, float deltaTime)
        {
            float throttleTarget = 0f;
            float steeringTarget = 0f;
            if (keyboard != null)
            {
                throttleTarget = Axis(keyboard.wKey.isPressed || keyboard.upArrowKey.isPressed,
                    keyboard.sKey.isPressed || keyboard.downArrowKey.isPressed);
                steeringTarget = Axis(keyboard.dKey.isPressed || keyboard.rightArrowKey.isPressed,
                    keyboard.aKey.isPressed || keyboard.leftArrowKey.isPressed);
            }
            _keyboardThrottle = Ramp(_keyboardThrottle, throttleTarget, deltaTime);
            _keyboardSteering = Ramp(_keyboardSteering, steeringTarget, deltaTime);
        }

        private static float Axis(bool positive, bool negative)
        {
            return (positive ? 1f : 0f) - (negative ? 1f : 0f);
        }

        private static float Ramp(float current, float target, float deltaTime)
        {
            float rate = Mathf.Approximately(target, 0f) ? KeyboardReleasePerSecond : KeyboardRampPerSecond;
            return Mathf.MoveTowards(current, target, rate * deltaTime);
        }

        private static float ApplyDeadZone(float value)
        {
            float magnitude = Mathf.Abs(value);
            if (magnitude < StickDeadZone)
            {
                return 0f;
            }
            return Mathf.Sign(value) * (magnitude - StickDeadZone) / (1f - StickDeadZone);
        }

        private static float LargestMagnitude(float first, float second)
        {
            return Mathf.Abs(first) >= Mathf.Abs(second) ? first : second;
        }
    }
}
