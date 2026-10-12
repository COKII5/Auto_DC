using System.Globalization;
using AutoDc.Communication;
using UnityEngine;

namespace AutoDc.UI
{
    public sealed class ConnectionPanel
    {
        private const float Margin = 10f;
        private const float Width = 300f;
        private const float FieldCaptionWidth = 60f;
        private const float PercentScale = 100f;
        private const float SectionSpacing = 8f;
        private const float CenterButtonWidth = 70f;
        private const float MillisecondsPerSecond = 1000f;

        private readonly CarSession _session;
        private string _host = CarSession.DefaultHost;
        private string _portText = CarSession.DefaultPort.ToString(CultureInfo.InvariantCulture);
        private string _inputError = "";

        public ConnectionPanel(CarSession session)
        {
            _session = session;
        }

        public void Draw()
        {
            GUILayout.BeginArea(new Rect(Margin, Margin, Width, Screen.height - 2f * Margin));
            GUILayout.BeginVertical(HudTheme.Panel);
            GUILayout.Label("CONEXIÓN (Wi-Fi UDP)", HudTheme.Title);
            DrawAddressFields();
            if (GUILayout.Button(_session.IsOpen ? "Desconectar" : "Conectar"))
            {
                ToggleConnection();
            }
            DrawLinkStatus();
            GUILayout.Space(SectionSpacing);
            GUILayout.Label("AJUSTES", HudTheme.Title);
            DrawSettings();
            GUILayout.EndVertical();
            GUILayout.EndArea();
        }

        private void DrawAddressFields()
        {
            bool previousEnabled = GUI.enabled;
            GUI.enabled = !_session.IsOpen;
            GUILayout.BeginHorizontal();
            GUILayout.Label("IP", HudTheme.Label, GUILayout.Width(FieldCaptionWidth));
            _host = GUILayout.TextField(_host);
            GUILayout.EndHorizontal();
            GUILayout.BeginHorizontal();
            GUILayout.Label("Puerto", HudTheme.Label, GUILayout.Width(FieldCaptionWidth));
            _portText = GUILayout.TextField(_portText);
            GUILayout.EndHorizontal();
            GUI.enabled = previousEnabled;
        }

        private void ToggleConnection()
        {
            if (_session.IsOpen)
            {
                _session.Disconnect();
                return;
            }
            if (!int.TryParse(_portText.Trim(), NumberStyles.Integer, CultureInfo.InvariantCulture, out int port))
            {
                _inputError = $"Puerto no numérico: '{_portText}'";
                return;
            }
            _inputError = "";
            _session.Connect(_host.Trim(), port);
        }

        private void DrawLinkStatus()
        {
            HudTheme.ColoredLabel(StateText(_session.State), StateColor(_session.State));
            LinkMonitor monitor = _session.Monitor;
            if (_session.IsOpen && monitor.HasTelemetry)
            {
                GUILayout.Label($"RTT: {monitor.LastRttMs:0.0} ms  (prom. {monitor.SmoothedRttMs:0.0} ms)", HudTheme.Label);
                GUILayout.Label($"Telemetría: {monitor.TelemetryRateHz} Hz   Paquetes corruptos: {_session.CorruptPackets}", HudTheme.Label);
                GUILayout.Label($"Carro encendido hace {monitor.LastTelemetry.UptimeMs / MillisecondsPerSecond:0} s", HudTheme.Label);
            }
            string error = _inputError.Length > 0 ? _inputError : _session.LastError;
            if (error.Length > 0)
            {
                HudTheme.ColoredLabel($"Error: {error}", HudTheme.Bad);
            }
        }

        private void DrawSettings()
        {
            GUILayout.Label($"Potencia máxima: {_session.ThrottleLimit * PercentScale:0} %", HudTheme.Label);
            _session.ThrottleLimit = GUILayout.HorizontalSlider(_session.ThrottleLimit, CarSession.MinThrottleLimit, CarSession.MaxThrottleLimit);
            GUILayout.Label($"Trim del servo: {_session.SteeringTrimDeg:+0;-0;0}°", HudTheme.Label);
            GUILayout.BeginHorizontal();
            float trim = GUILayout.HorizontalSlider(_session.SteeringTrimDeg, -CarSession.MaxSteeringTrimDeg, CarSession.MaxSteeringTrimDeg);
            _session.SteeringTrimDeg = Mathf.RoundToInt(trim);
            if (GUILayout.Button("Centrar", GUILayout.Width(CenterButtonWidth)))
            {
                _session.SteeringTrimDeg = 0;
            }
            GUILayout.EndHorizontal();
        }

        private static string StateText(LinkState state)
        {
            switch (state)
            {
                case LinkState.WaitingForCar:
                    return "Esperando telemetría del carro...";
                case LinkState.Connected:
                    return "Conectado";
                case LinkState.NoResponse:
                    return "Sin respuesta: el carro se detuvo (failsafe)";
                default:
                    return "Desconectado: el gemelo simula localmente";
            }
        }

        private static Color StateColor(LinkState state)
        {
            switch (state)
            {
                case LinkState.Connected:
                    return HudTheme.Good;
                case LinkState.WaitingForCar:
                    return HudTheme.Warning;
                case LinkState.NoResponse:
                    return HudTheme.Bad;
                default:
                    return HudTheme.Muted;
            }
        }
    }
}
