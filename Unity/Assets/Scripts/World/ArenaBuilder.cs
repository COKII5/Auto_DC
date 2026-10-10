using AutoDc.Twin;
using UnityEngine;

namespace AutoDc.World
{
    public static class ArenaBuilder
    {
        private const float ArenaSizeMeters = 20f;
        private const float UnityPlaneSizeMeters = 10f;
        private const int GridTextureSize = 200;
        private const int MinorLineSpacingPixels = 20;
        private const int MajorLinePixels = 2;
        private const float OriginMarkerSize = 0.05f;

        private static readonly Color FloorColor = new Color(0.32f, 0.34f, 0.36f);
        private static readonly Color MajorLineColor = new Color(0.5f, 0.53f, 0.56f);
        private static readonly Color MinorLineColor = new Color(0.38f, 0.4f, 0.42f);
        private static readonly Color OriginColor = new Color(0.9f, 0.35f, 0.1f);
        private static readonly Color SkyColor = new Color(0.16f, 0.18f, 0.22f);

        public static Color BackgroundColor => SkyColor;

        public static void Build(Transform parent)
        {
            GameObject floor = PrimitiveFactory.CreatePart(PrimitiveType.Plane, "Floor", parent, Vector3.zero,
                Vector3.one * (ArenaSizeMeters / UnityPlaneSizeMeters), Color.white);
            Material floorMaterial = floor.GetComponent<Renderer>().material;
            floorMaterial.mainTexture = CreateGridTexture();
            floorMaterial.mainTextureScale = Vector2.one * ArenaSizeMeters;

            PrimitiveFactory.CreatePart(PrimitiveType.Cube, "Origin", parent, new Vector3(0f, 0.001f, 0f),
                new Vector3(OriginMarkerSize, 0.002f, OriginMarkerSize), OriginColor);

            Light sun = new GameObject("Sun").AddComponent<Light>();
            sun.transform.SetParent(parent, false);
            sun.type = LightType.Directional;
            sun.shadows = LightShadows.Soft;
            sun.transform.rotation = Quaternion.Euler(50f, -30f, 0f);
        }

        private static Texture2D CreateGridTexture()
        {
            Texture2D texture = new Texture2D(GridTextureSize, GridTextureSize, TextureFormat.RGBA32, true)
            {
                name = "Grid",
                wrapMode = TextureWrapMode.Repeat,
                filterMode = FilterMode.Trilinear,
                anisoLevel = 8,
            };
            Color[] pixels = new Color[GridTextureSize * GridTextureSize];
            for (int row = 0; row < GridTextureSize; row++)
            {
                for (int column = 0; column < GridTextureSize; column++)
                {
                    pixels[row * GridTextureSize + column] = GridPixelColor(row, column);
                }
            }
            texture.SetPixels(pixels);
            texture.Apply(true);
            return texture;
        }

        private static Color GridPixelColor(int row, int column)
        {
            if (row < MajorLinePixels || column < MajorLinePixels)
            {
                return MajorLineColor;
            }
            if (row % MinorLineSpacingPixels == 0 || column % MinorLineSpacingPixels == 0)
            {
                return MinorLineColor;
            }
            return FloorColor;
        }
    }
}
