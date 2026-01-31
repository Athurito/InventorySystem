// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

class UAbilitySystemComponent;

namespace RpgInventory
{
	/**
	 * Returns the ASC that lives on the owning PlayerState (Lyra-style).
	 * 
	 * Supported input:
	 * - PlayerState -> its ASC
	 * - Pawn/Character -> Pawn->PlayerState -> ASC
	 * - any Actor -> if it is a Pawn, same as above
	 */
	UAbilitySystemComponent* GetASCFromPlayerStateOwner(const UObject* WorldContextObject, const AActor* Actor);
}
