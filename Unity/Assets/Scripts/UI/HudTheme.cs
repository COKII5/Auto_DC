using UnityEngine;

namespace AutoDc.UI
{
    internal static class HudTheme
    {
        public static readonly Color Good = new Color(0.35f, 0.85f, 0.45f);
        public static readonly Color Warning = new Color(1f, 0.75f, 0.2f);
        public static readonly Color Bad = new Color(1f, 0.35f, 0.3f);
        public static readonly Color Muted = new Color(0.7f, 0.72f, 0.75f);
        public static readonly Color CommandedBar = new Color(0.3f, 0.6f, 1f);
        public static readonly Color AppliedBar = new Color(0.35f, 0.85f, 0.45f);

        private const int FontSize = 13;
        private const int TitleFontSize = 14;
        private const float CaptionWidth = 92f;
        private const float BarHeight = 10f;
        private const float ValueWidth = 48f;
        private const float MinBarWidth = 10f;
        private const float BarVerticalOffset = 2f;
        private const float CenterLineWidth = 1f;
        private const float CenterLineOverhang = 2f;
        private const int PanelHorizontalPadding = 12;
        private const int PanelVerticalPadding = 10;
        private static readonly Color PanelColor = new Color(0.06f, 0.07f, 0.09f, 0.82f);
        private static readonly Color BarBackground = new Color(1f, 1f, 1f, 0.12f);

        private static GUIStyle _panel;
        private static GUIStyle _title;
        private static GUIStyle _label;

        public static GUIStyle Panel
        {
            get
            {
                EnsureStyles();
                return _panel;
            }
        }

        public static GUIStyle Title
        {
            get
            {
                EnsureStyles();
                return _title;
            }
        }

        public static GUIStyle Label
        {
            get
            {
                EnsureStyles();
                return _label;
            }
        }

        public static void ColoredLabel(string text, Color color)
        {
            Color previous = GUI.contentColor;
            GUI.contentColor = color;
            GUILayout.Label(text, Label);
            GUI.contentColor = previous;
        }

        public static void SignedBar(string caption, float value, Color color, string valueText)
        {
            GUILayout.BeginHorizontal();
            GUILayout.Label(caption, Label, GUILayout.Width(CaptionWidth));
            Rect area = GUILayoutUtility.GetRect(MinBarWidth, BarHeight, GUILayout.ExpandWidth(true));
            area.y += (Label.lineHeight - BarHeight) * 0.5f + BarVerticalOffset;
            area.height = BarHeight;
            FillRect(area, BarBackground);
            float center = area.x + area.width * 0.5f;
            float extent = Mathf.Clamp(value, -1f, 1f) * area.width * 0.5f;
            FillRect(new Rect(Mathf.Min(center, center + extent), area.y, Mathf.Abs(extent), area.height), color);
            FillRect(new Rect(center - CenterLineWidth * 0.5f, area.y - CenterLineOverhang, CenterLineWidth,
                area.height + 2f * CenterLineOverhang), Muted);
            GUILayout.Label(valueText, Label, GUILayout.Width(ValueWidth));
            GUILayout.EndHorizontal();
        }

        private static void FillRect(Rect rect, Color color)
        {
            Color previous = GUI.color;
            GUI.color = color;
            GUI.DrawTexture(rect, Texture2D.whiteTexture);
            GUI.color = previous;
        }

        private static void EnsureStyles()
        {
            if (_panel != null)
            {
                return;
            }
            Texture2D background = new Texture2D(1, 1);
            background.SetPixel(0, 0, PanelColor);
            background.Apply();
            _panel = new GUIStyle(GUI.skin.box)
            {
                padding = new RectOffset(PanelHorizontalPadding, PanelHorizontalPadding, PanelVerticalPadding, PanelVerticalPadding),
                alignment = TextAnchor.UpperLeft,
            };
            _panel.normal.background = background;
            _title = new GUIStyle(GUI.skin.label) { fontSize = TitleFontSize, fontStyle = FontStyle.Bold };
            _title.normal.textColor = Color.white;
            _label = new GUIStyle(GUI.skin.label) { fontSize = FontSize, wordWrap = true };
            _label.normal.textColor = Color.white;
        }
    }
}
