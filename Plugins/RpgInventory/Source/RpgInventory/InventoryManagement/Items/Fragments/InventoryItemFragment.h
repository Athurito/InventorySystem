// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "InventoryItemFragment.generated.h"

class UInventoryItemInstance;
/**
 * 
 */
UCLASS(MinimalAPI, DefaultToInstanced, EditInlineNew, Abstract)
class UInventoryItemFragment : public UObject
{
	GENERATED_BODY()
	
public:
	
	virtual void OnInstanceCreated(UInventoryItemInstance* Instance) const {}
};