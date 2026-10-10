using UnityEngine;
using UnityEngine.InputSystem;

namespace AutoDc.World
{
    public sealed class FollowCamera : MonoBehaviour
    {
        [SerializeField] private Vector3 _chaseOffset = new Vector3(0f, 0.22f, -0.42f);
        [SerializeField] private float _chaseLookAhead = 0.08f;
        [SerializeField] private float _topDownHeight = 2.2f;
        [SerializeField] private float _smoothingPerSecond = 6f;

        private Transform _target;
        private bool _topDown;

        public void Follow(Transform target)
        {
            _target = target;
            SnapToTarget();
        }

        private void LateUpdate()
        {
            if (_target == null)
            {
                return;
            }
            Keyboard keyboard = Keyboard.current;
            if (keyboard != null && keyboard.cKey.wasPressedThisFrame)
            {
                _topDown = !_topDown;
            }
            float blend = 1f - Mathf.Exp(-_smoothingPerSecond * Time.deltaTime);
            transform.position = Vector3.Lerp(transform.position, DesiredPosition(), blend);
            transform.rotation = Quaternion.Slerp(transform.rotation, DesiredRotation(), blend);
        }

        private void SnapToTarget()
        {
            if (_target != null)
            {
                transform.SetPositionAndRotation(DesiredPosition(), DesiredRotation());
            }
        }

        private Vector3 DesiredPosition()
        {
            if (_topDown)
            {
                return _target.position + Vector3.up * _topDownHeight;
            }
            Quaternion yaw = Quaternion.Euler(0f, _target.eulerAngles.y, 0f);
            return _target.position + yaw * _chaseOffset;
        }

        private Quaternion DesiredRotation()
        {
            if (_topDown)
            {
                return Quaternion.Euler(90f, 0f, 0f);
            }
            Vector3 focus = _target.position + _target.forward * _chaseLookAhead;
            return Quaternion.LookRotation(focus - DesiredPosition(), Vector3.up);
        }
    }
}
