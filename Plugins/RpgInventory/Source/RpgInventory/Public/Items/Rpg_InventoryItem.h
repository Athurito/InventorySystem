// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Rpg_ItemDefinition.h"
#include "UObject/Object.h"
#include "Rpg_InventoryItem.generated.h"

/**
 * 
 */
UCLASS()
class RPGINVENTORY_API URpg_InventoryItem : public UObject
{
	GENERATED_BODY()

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual bool IsSupportedForNetworking() const override {return true;};

	void SetItemManifest(const URpg_ItemDefinition& Manifest);
};
