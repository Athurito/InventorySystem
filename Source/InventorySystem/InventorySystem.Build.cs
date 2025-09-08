// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class InventorySystem : ModuleRules
{
	public InventorySystem(ReadOnlyTargetRules Target) : base(Target)
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
			"InventorySystem",
			"InventorySystem/Variant_Platforming",
			"InventorySystem/Variant_Platforming/Animation",
			"InventorySystem/Variant_Combat",
			"InventorySystem/Variant_Combat/AI",
			"InventorySystem/Variant_Combat/Animation",
			"InventorySystem/Variant_Combat/Gameplay",
			"InventorySystem/Variant_Combat/Interfaces",
			"InventorySystem/Variant_Combat/UI",
			"InventorySystem/Variant_SideScrolling",
			"InventorySystem/Variant_SideScrolling/AI",
			"InventorySystem/Variant_SideScrolling/Gameplay",
			"InventorySystem/Variant_SideScrolling/Interfaces",
			"InventorySystem/Variant_SideScrolling/UI"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
