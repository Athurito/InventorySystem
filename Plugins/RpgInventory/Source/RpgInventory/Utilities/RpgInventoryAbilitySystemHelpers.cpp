// Fill out your copyright notice in the Description page of Project Settings.

#include "RpgInventoryAbilitySystemHelpers.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"

namespace RpgInventory
{
	UAbilitySystemComponent* GetASCFromPlayerStateOwner(const UObject* WorldContextObject, const AActor* Actor)
	{
		if (!Actor)
		{
			return nullptr;
		}

		// If the actor itself is the PS (common in Lyra), grab ASC directly.
		if (const APlayerState* PS = Cast<APlayerState>(Actor))
		{
			if (const IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(PS))
			{
				return ASI->GetAbilitySystemComponent();
			}
			return nullptr;
		}

		// If it's a pawn/character, the ASC is on its PlayerState.
		if (const APawn* Pawn = Cast<APawn>(Actor))
		{
			if (const APlayerState* PS = Pawn->GetPlayerState())
			{
				if (const IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(PS))
				{
					return ASI->GetAbilitySystemComponent();
				}
			}
		}

		// Fallback: nothing found.
		return nullptr;
	}
}
