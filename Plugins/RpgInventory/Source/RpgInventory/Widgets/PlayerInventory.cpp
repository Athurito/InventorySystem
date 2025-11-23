// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerInventory.h"

#include "CommonTabListWidgetBase.h"
#include "RpgInventory/InventoryManagement/Components/InventoryManagerComponent.h"
#include "RpgInventory/InventoryManagement/Utils/InventoryStatics.h"


void UPlayerInventory::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	
}

void UPlayerInventory::EnsurePlayerComponent()
{
	// Auto-resolve the local player's container if not explicitly initialized
	if (!PlayerContainerComponent.IsValid())
	{
		if (APlayerController* PC = GetOwningPlayer())
		{
			AActor* OwnerActor = PC->PlayerState ? static_cast<AActor*>(PC->PlayerState) : static_cast<AActor*>(PC);
			if (OwnerActor)
			{
				if (UInventoryManagerComponent* AutoComp = UInventoryStatics::GetContainerComponent(OwnerActor))
				{
					PlayerContainerComponent = AutoComp;
				}
			}
		}
	}
}

