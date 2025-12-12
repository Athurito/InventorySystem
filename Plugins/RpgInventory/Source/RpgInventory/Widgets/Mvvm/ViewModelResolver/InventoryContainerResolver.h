// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "View/MVVMViewModelContextResolver.h"
#include "InventoryContainerResolver.generated.h"

/**
 * 
 */
UCLASS()
class RPGINVENTORY_API UInventoryContainerResolver : public UMVVMViewModelContextResolver
{
	GENERATED_BODY()
	
	virtual UObject* CreateInstance(const UClass* ExpectedType, const UUserWidget* UserWidget, const UMVVMView* View) const override;
};
