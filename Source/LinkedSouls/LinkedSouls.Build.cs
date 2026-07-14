// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class LinkedSouls : ModuleRules
{
	public LinkedSouls(ReadOnlyTargetRules Target) : base(Target)
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
			"Slate",
			"SlateCore",
			// Gameplay Ability System: attributes and abilities
			"GameplayAbilities",
			"GameplayTags",
			"GameplayTasks"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"LinkedSouls",
			"LinkedSouls/DualWorld",
			"LinkedSouls/Player",
			"LinkedSouls/SoulEnergy",
			"LinkedSouls/Puzzles",
			"LinkedSouls/Elements",
			"LinkedSouls/Enemies",
			"LinkedSouls/WorldPortal",
			"LinkedSouls/HUD",
			"LinkedSouls/GameState",
			"LinkedSouls/Abilities",
			"LinkedSouls/Input",
			"LinkedSouls/Variant_Platforming",
			"LinkedSouls/Variant_Platforming/Animation",
			"LinkedSouls/Variant_Combat",
			"LinkedSouls/Variant_Combat/AI",
			"LinkedSouls/Variant_Combat/Animation",
			"LinkedSouls/Variant_Combat/Gameplay",
			"LinkedSouls/Variant_Combat/Interfaces",
			"LinkedSouls/Variant_Combat/UI",
			"LinkedSouls/Variant_SideScrolling",
			"LinkedSouls/Variant_SideScrolling/AI",
			"LinkedSouls/Variant_SideScrolling/Gameplay",
			"LinkedSouls/Variant_SideScrolling/Interfaces",
			"LinkedSouls/Variant_SideScrolling/UI"
		});

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");
	}
}
