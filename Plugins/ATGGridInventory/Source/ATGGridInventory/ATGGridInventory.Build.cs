// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class ATGGridInventory : ModuleRules
{
	public ATGGridInventory(ReadOnlyTargetRules Target) : base(Target)
	{
		SetupIrisSupport(Target);

		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicIncludePaths.AddRange(
			new string[] {
				System.IO.Path.Combine(ModuleDirectory, "Public/Data"),
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
				"CoreUObject",
				"Engine",
				"InputCore",
				"UMG",
				"Slate",
				"SlateCore",
				"NetCore",
				"GameplayTags",
				"GameplayAbilities",
				"GameplayTasks",
			}
			);


		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				// ...
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
