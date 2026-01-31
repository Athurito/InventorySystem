// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HotbarComponent.generated.h"

class UInventoryManagerComponent;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class RPGINVENTORY_API UHotbarComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UHotbarComponent();

protected:
	virtual void BeginPlay() override;

public:
	UFUNCTION(BlueprintCallable, Category="Hotbar")
	void UseHotbarSlot(int32 SlotIndex);

	UFUNCTION(Server, Reliable)
	void ServerUseHotbarSlot(int32 SlotIndex);

private:
	UPROPERTY(Transient)
	TObjectPtr<UInventoryManagerComponent> InventoryManager;

	int32 HotbarContainerIndex = INDEX_NONE;
};
