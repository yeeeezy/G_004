// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class G_004 : ModuleRules
{
	public G_004(ReadOnlyTargetRules Target) : base(Target)
	{
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
			"Slate"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"G_004",
			"G_004/Variant_Platforming",
			"G_004/Variant_Platforming/Animation",
			"G_004/Variant_Combat",
			"G_004/Variant_Combat/AI",
			"G_004/Variant_Combat/Animation",
			"G_004/Variant_Combat/Gameplay",
			"G_004/Variant_Combat/Interfaces",
			"G_004/Variant_Combat/UI",
			"G_004/Variant_SideScrolling",
			"G_004/Variant_SideScrolling/AI",
			"G_004/Variant_SideScrolling/Gameplay",
			"G_004/Variant_SideScrolling/Interfaces",
			"G_004/Variant_SideScrolling/UI"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
