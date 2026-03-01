// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class UnrealMCP : ModuleRules
{
    public UnrealMCP(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
        // Use IWYUSupport instead of the deprecated bEnforceIWYU in UE5.5
        IWYUSupport = IWYUSupport.Full;

        PublicIncludePaths.AddRange(
            new string[] {
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
                "CoreUObject",
                "Engine",
                "InputCore",
                "Networking",
                "Sockets",
                "HTTP",
                "Json",
                "JsonUtilities",
                "DeveloperSettings"
            }
        );

        // 런타임(패키징된 게임)에서도 필요한 모듈들만 남김
        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "Slate",
                "SlateCore",
                "UMG",
                "Projects",
                "AssetRegistry"
            }
        );

        // 에디터 빌드에서만 포함되어야 하는 모듈들을 이곳으로 이동
        if (Target.bBuildEditor == true)
        {
            PrivateDependencyModuleNames.AddRange(
                new string[]
                {
                    "UnrealEd",                 // 에러의 주범
                    "Blutility",
                    "EditorScriptingUtilities",
                    "EditorSubsystem",
                    "Kismet",
                    "KismetCompiler",
                    "BlueprintGraph",
                    "PropertyEditor",       // For widget property editing
					"ToolMenus",            // For editor UI
					"BlueprintEditorLibrary", // For Blueprint utilities
					"UMGEditor"             // For WidgetBlueprint.h and other UMG editor functionality
				}
            );
        }

        DynamicallyLoadedModuleNames.AddRange(
            new string[]
            {
				// ... add any modules that your module loads dynamically here ...
			}
        );
    }
}