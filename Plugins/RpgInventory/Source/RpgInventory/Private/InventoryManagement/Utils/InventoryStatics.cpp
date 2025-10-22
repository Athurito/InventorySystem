// Fill out your copyright notice in the Description page of Project Settings.


#include "InventoryManagement/Utils/InventoryStatics.h"

#include "Engine/AssetManager.h"
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
