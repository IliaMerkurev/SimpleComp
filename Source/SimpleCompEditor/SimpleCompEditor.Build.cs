using UnrealBuildTool;

public class SimpleCompEditor : ModuleRules
{
	public SimpleCompEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"SimpleComp"
		});

		PrivateDependencyModuleNames.AddRange(new string[] {
			"BlueprintGraph",
			"UnrealEd",
			"KismetCompiler",
			"Slate",
			"SlateCore"
		});
	}
}
