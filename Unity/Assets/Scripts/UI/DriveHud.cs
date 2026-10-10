using AutoDc.Communication;
using AutoDc.Protocol;
using AutoDc.Twin;
using UnityEngine;

namespace AutoDc.UI
{
    public sealed class DriveHud
    {
        private const float Margin = 10f;
        private const float Width = 340f;
        private const float Height = 275f;
        private const float MillimetersPerMeter = 1000f;
        private const int ObstacleWarningMm = 200;
        private const float MetersPerSecondToKmh = 3.6f;
        private const float CommandScale = PacketFormat.CommandLimit;
        private const string ControlsHelp =
            "W/S o flechas: acelerar / reversa   A/D: girar   Espacio: frenar\n" +
            "Gamepad: gatillos o stick izq., A frena   R: reiniciar   C: cámara";

        public void Draw(CarSession session, VehicleTwin twin, TwinSource source)
        {
            GUILayout.BeginArea(new Rect(Margin, Screen.height - Height - Margin, Width, Height));
            GUILayout.BeginVertical(HudTheme.Panel);
            GUILayout.Label("CONDUCCIÓN", HudTheme.Title);
            GUILayout.Label($"Gemelo: {SourceText(source)}   Dirección: {ModeText(twin.ActiveSteeringMode)}", HudTheme.Label);

            DriveCommand command = session.CurrentCommand;
            HudTheme.SignedBar("Acelerador", command.Throttle / CommandScale, HudTheme.CommandedBar, $"{command.Throttle} %");
            HudTheme.SignedBar("Giro", command.Steering / CommandScale, HudTheme.CommandedBar, $"{command.Steering} %");
            if (source == TwinSource.Telemetry)
            {
                CarTelemetry telemetry = session.Monitor.LastTelemetry;
                HudTheme.SignedBar("Acel. real", telemetry.AppliedThrottle / CommandScale, HudTheme.AppliedBar, $"{telemetry.AppliedThrottle} %");
                HudTheme.SignedBar("Giro real", telemetry.AppliedSteering / CommandScale, HudTheme.AppliedBar, $"{telemetry.AppliedSteering} %");
            }

            float speed = twin.ForwardSpeed;
            GUILayout.Label($"Velocidad: {speed:0.00} m/s ({speed * MetersPerSecondToKmh:0.0} km/h)   Ruedas: {twin.FrontWheelAngleDeg:+0.0;-0.0;0.0}°", HudTheme.Label);
            if (source == TwinSource.Telemetry)
            {
                DrawDistance(session.Monitor.LastTelemetry);
            }
            if (command.Brake)
            {
                HudTheme.ColoredLabel("FRENO", HudTheme.Warning);
            }
            if (source == TwinSource.Halted || (source == TwinSource.Telemetry && session.Monitor.LastTelemetry.Failsafe))
            {
                HudTheme.ColoredLabel("FAILSAFE: motores detenidos y servo centrado", HudTheme.Bad);
            }
            GUILayout.FlexibleSpace();
            HudTheme.ColoredLabel(ControlsHelp, HudTheme.Muted);
            GUILayout.EndVertical();
            GUILayout.EndArea();
        }

        private static void DrawDistance(CarTelemetry telemetry)
        {
            if (!telemetry.HasDistance)
            {
                HudTheme.ColoredLabel("Distancia: sin lectura", HudTheme.Muted);
                return;
            }
            string text = $"Distancia al obstáculo: {telemetry.DistanceMm / MillimetersPerMeter:0.00} m";
            HudTheme.ColoredLabel(text, telemetry.DistanceMm <= ObstacleWarningMm ? HudTheme.Warning : HudTheme.Good);
        }

        private static string SourceText(TwinSource source)
        {
            switch (source)
            {
                case TwinSource.Telemetry:
                    return "telemetría";
                case TwinSource.Halted:
                    return "detenido";
                default:
                    return "simulación local";
            }
        }

        private static string ModeText(SteeringMode mode)
        {
            return mode == SteeringMode.Servo ? "servo" : "diferencial";
        }
    }
}
