// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class SimpleComp : ModuleRules
{
    public SimpleComp(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine" });
    }
}
