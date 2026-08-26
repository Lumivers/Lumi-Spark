// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Lumi_Spark : ModuleRules
{
	public Lumi_Spark(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "GameplayTags"});
	}
}
