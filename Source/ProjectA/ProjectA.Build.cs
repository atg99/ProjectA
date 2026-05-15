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
            // RenderData ���ٿ�
            "RenderCore",
            "RHI",

            "MeshDescription",
			"SkeletalMeshDescription",
			// http
			"HTTP", "Json", "JsonUtilities",
            "Sockets", "Networking",

            "OnlineSubsystem",
            "OnlineSubsystemSteam",
            "OnlineSubsystemUtils",

            // Plugins
            "ATGGridInventory"
        });

        // ThirdParty ������ ��Ŭ��� ��ο� �߰�
        PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "ThirdParty"));

        PrivateDependencyModuleNames.AddRange(new string[] { "NetCore" });
		
		

		PublicIncludePaths.AddRange(new string[] {
			"ProjectA"
		});

        // Uncomment if you are using Slate UI
        PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
        
        // Uncomment if you are using online features
        // PrivateDependencyModuleNames.Add("OnlineSubsystem");

        // To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
    }
}
