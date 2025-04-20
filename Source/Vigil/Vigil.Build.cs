// Copyright (c) Jared Taylor. All Rights Reserved

using UnrealBuildTool;

public class Vigil : ModuleRules
{
	public Vigil(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core", 
				"GameplayTags",
				"GameplayTasks",
				"GameplayAbilities",
				"TargetingSystem",
			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
			}
			);
	}
}
