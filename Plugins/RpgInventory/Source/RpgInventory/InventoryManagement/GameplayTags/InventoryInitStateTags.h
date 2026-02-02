// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "NativeGameplayTags.h"

/**
 * Init-state tags used with `ModularGameplay` / `UGameFrameworkComponentManager`.
 *
 * Mirrors the common Lyra chain:
 * `InitState.Spawned` -> `InitState.DataAvailable` -> `InitState.DataInitialized` -> `InitState.GameplayReady`
 */
namespace RpgTags
{
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(InitState_Spawned);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(InitState_DataAvailable);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(InitState_DataInitialized);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(InitState_GameplayReady);
}
