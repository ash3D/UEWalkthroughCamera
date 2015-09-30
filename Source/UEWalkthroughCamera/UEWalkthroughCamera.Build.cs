namespace UnrealBuildTool.Rules
{
	using System.IO;

	public class UEWalkthroughCamera : ModuleRules
	{
		// not available from ModuleRules in 4.7 for some reason
		public string ModuleDirectory
		{
			get
			{
				return Path.GetDirectoryName(RulesCompiler.GetModuleFilename(GetType().Name));
			}
		}

		public UEWalkthroughCamera(TargetInfo Target)
		{
			bEnableExceptions = true;

			string DGLE = Path.Combine(ModuleDirectory, "..", "DWinDirect");

			PublicIncludePaths.AddRange(
				new string[] {
					Path.Combine(DGLE, "General"),
					Path.Combine(DGLE, "render prototype"),
					// ... add public include paths required here ...
				}
				);

			PrivateIncludePaths.AddRange(
				new string[] {
					// ... add other private include paths required here ...
				}
				);

			PublicDependencyModuleNames.AddRange(
				new string[]
				{
					"Core",
					"Engine",
					// ... add other public dependencies that you statically link with here ...
				}
				);

			PrivateDependencyModuleNames.AddRange(
				new string[]
				{
					"boost",
					"CoreUObject",
					// ... add private dependencies that you statically link with here ...
				}
				);

			DynamicallyLoadedModuleNames.AddRange(
				new string[]
				{
					// ... add any modules that your module loads dynamically here ...
				}
				);
		}
	}
}