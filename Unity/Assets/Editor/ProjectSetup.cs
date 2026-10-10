using System.IO;
using UnityEditor;
using UnityEditor.SceneManagement;
using UnityEngine;
using UnityEngine.SceneManagement;

namespace AutoDc.EditorTools
{
    [InitializeOnLoad]
    public static class ProjectSetup
    {
        private const string ScenePath = "Assets/Scenes/AutoDC.unity";
        private const string LogPrefix = "[AutoDC]";
        private const string SceneOpenedKey = "AutoDC.SceneOpened";

        static ProjectSetup()
        {
            EditorApplication.delayCall += EnsureProjectReady;
        }

        [MenuItem("AutoDC/Create Scene")]
        public static void CreateScene()
        {
            if (!Application.isBatchMode && !EditorSceneManager.SaveCurrentModifiedScenesIfUserWantsTo())
            {
                return;
            }
            Scene scene = EditorSceneManager.NewScene(NewSceneSetup.EmptyScene, NewSceneMode.Single);
            GameObject root = new GameObject(nameof(AutoDcApp), typeof(AutoDcApp));
            Directory.CreateDirectory(Path.GetDirectoryName(ScenePath));
            if (!EditorSceneManager.SaveScene(scene, ScenePath))
            {
                Debug.LogError($"{LogPrefix} No se pudo guardar la escena en {ScenePath}.");
                return;
            }
            EditorBuildSettings.scenes = new[] { new EditorBuildSettingsScene(ScenePath, true) };
            Debug.Log($"{LogPrefix} Escena creada: {ScenePath} (objeto raiz '{root.name}'). Presiona Play.");
        }

        private static void EnsureProjectReady()
        {
            PlayerSettings.runInBackground = true;
            if (!File.Exists(ScenePath))
            {
                CreateScene();
            }
            else if (!Application.isBatchMode && !SessionState.GetBool(SceneOpenedKey, false)
                && SceneManager.GetActiveScene().path != ScenePath)
            {
                EditorSceneManager.OpenScene(ScenePath);
            }
            SessionState.SetBool(SceneOpenedKey, true);
        }
    }
}
