// Copyright Epic Games, Inc. All Rights Reserved.

using System.IO;
using UnrealBuildTool;

public class ProjectA : ModuleRules
{
	public ProjectA(ReadOnlyTargetRules Target) : base(Target)
	{
        //Iris
        SetupIrisSupport(Target);

        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"UMG",
			"Slate",
            "Niagara",
            "ProceduralMeshComponent",
            "RenderCore",
            "RHI",
            "GameplayAbilities", "GameplayTags", "GameplayTasks", // gameplay Ability System
			"GeometryFramework",
            "GeometryScriptingCore",
            "DynamicMesh",
            "MeshConversion",
            "GeometryCore",
            "MeshConversion",
            "MeshConversionEngineTypes", 
			// Iris
            "NetCore",
            "IrisCore",
            // RenderData 접근용
            "RenderCore",
            "RHI",

            "MeshDescription",
			"SkeletalMeshDescription",
			// http
			"HTTP", "Json", "JsonUtilities",
            "Sockets", "Networking",

            "OnlineSubsystem",
            "OnlineSubsystemSteam",
            "OnlineSubsystemUtils"
        });

        // ThirdParty 폴더를 인클루드 경로에 추가
        PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "ThirdParty"));

        PrivateDependencyModuleNames.AddRange(new string[] { "NetCore" });
		
		

		PublicIncludePaths.AddRange(new string[] {
			"ProjectA",
			"ProjectA/Variant_Platforming",
			"ProjectA/Variant_Platforming/Animation",
			"ProjectA/Variant_Combat",
			"ProjectA/Variant_Combat/AI",
			"ProjectA/Variant_Combat/Animation",
			"ProjectA/Variant_Combat/Gameplay",
			"ProjectA/Variant_Combat/Interfaces",
			"ProjectA/Variant_Combat/UI",
			"ProjectA/Variant_SideScrolling",
			"ProjectA/Variant_SideScrolling/AI",
			"ProjectA/Variant_SideScrolling/Gameplay",
			"ProjectA/Variant_SideScrolling/Interfaces",
			"ProjectA/Variant_SideScrolling/UI"
		});

        // Uncomment if you are using Slate UI
        PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
        
        // Uncomment if you are using online features
        // PrivateDependencyModuleNames.Add("OnlineSubsystem");

        // To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
    }
}
