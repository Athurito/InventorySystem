// Fill out your copyright notice in the Description page of Project Settings.


#include "InventoryManagement/Utils/InventoryStatics.h"

#include "Engine/AssetManager.h"
#include "GameFramework/PlayerState.h"
#include "InventoryManagement/Components/Rpg_ContainerComponent.h"
#include "Items/Rpg_ItemDefinition.h"


URpg_ItemDefinition* UInventoryStatics::GetItemDefinitionById(const FPrimaryAssetId& ItemId)
{
	URpg_ItemDefinition* Def = nullptr;
	if (UAssetManager* AM = UAssetManager::GetIfInitialized())
	{
		const FSoftObjectPath Path = AM->GetPrimaryAssetPath(ItemId);
		if (Path.IsValid())
		{
			UObject* Obj = AM->GetStreamableManager().LoadSynchronous(Path, false);
			Def = Cast<URpg_ItemDefinition>(Obj);
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


