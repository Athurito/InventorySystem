// Fill out your copyright notice in the Description page of Project Settings.


#include "InventoryStatics.h"
#include "AbilitySystemInterface.h"
#include "Engine/AssetManager.h"
#include "GameFramework/PlayerState.h"
#include "RpgInventory/InventoryManagement/Components/Rpg_ContainerComponent.h"
#include "RpgInventory/Items/InventoryItemDefinition.h"


UInventoryItemDefinition* UInventoryStatics::GetItemDefinitionById(const FPrimaryAssetId& ItemId)
{
	UInventoryItemDefinition* Def = nullptr;
	if (UAssetManager* AM = UAssetManager::GetIfInitialized())
	{
		const FSoftObjectPath Path = AM->GetPrimaryAssetPath(ItemId);
		if (Path.IsValid())
		{
			UObject* Obj = AM->GetStreamableManager().LoadSynchronous(Path, false);
			Def = Cast<UInventoryItemDefinition>(Obj);
		}
	}
	return Def;
}

URpg_ContainerComponent* UInventoryStatics::GetContainerComponent(AActor* Owner)
{
	if (!IsValid(Owner))
	{
		return nullptr;
	}

	return Owner->FindComponentByClass<URpg_ContainerComponent>();
}

URpg_ContainerComponent* UInventoryStatics::ResolveInventoryFromInstigator(APawn* Instigator)
{
	if (!IsValid(Instigator))
		return nullptr;
	
	if (APlayerState* PS = Instigator->GetPlayerState())
	{
		if (URpg_ContainerComponent* C = PS->FindComponentByClass<URpg_ContainerComponent>())
			return C;
	}
	
	if (AController* Controller = Instigator->GetController())
	{
		if (URpg_ContainerComponent* C = Controller->FindComponentByClass<URpg_ContainerComponent>())
			return C;
	}

	if (URpg_ContainerComponent* C = Instigator->FindComponentByClass<URpg_ContainerComponent>())
	{
		return C;
	}
	
	return nullptr;
}

UAbilitySystemComponent* UInventoryStatics::ResolveASCFromPawn(APawn* InstigatorPawn)
{
	if (!InstigatorPawn) return nullptr;
	// Pawn implements ASI
	if (const IAbilitySystemInterface* ASIInst = Cast<IAbilitySystemInterface>(InstigatorPawn))
	{
		if (UAbilitySystemComponent* C = ASIInst->GetAbilitySystemComponent()) return C;
	}
	// PlayerState usually holds ASC
	if (APlayerState* PS = InstigatorPawn->GetPlayerState())
	{
		if (const IAbilitySystemInterface* ASIPS = Cast<IAbilitySystemInterface>(PS))
		{
			if (UAbilitySystemComponent* C = ASIPS->GetAbilitySystemComponent()) return C;
		}
	}
	// Controller may also implement
	if (AController* Cntr = InstigatorPawn->GetController())
	{
		if (const IAbilitySystemInterface* ASIC = Cast<IAbilitySystemInterface>(Cntr))
		{
			if (UAbilitySystemComponent* C = ASIC->GetAbilitySystemComponent()) return C;
		}
	}
	return nullptr;
}