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
			"SimpleComp",
			"BlueprintGraph"
		});

		PrivateDependencyModuleNames.AddRange(new string[] {
			"UnrealEd",
			"KismetCompiler",
			"Slate",
			"SlateCore"
		});
	}
}
