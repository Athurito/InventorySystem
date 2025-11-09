// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "SlotArray.generated.h"

/** Slots **/

USTRUCT()
struct FInv_Slot : public FFastArraySerializerItem
{
	GENERATED_BODY()
	

	UPROPERTY() int32 SlotIndex;   // 0..(Rows*Cols-1)
	UPROPERTY() FGuid InstanceId;               // leer == frei
};

USTRUCT()
struct FInvSlotArray : public FFastArraySerializer
{
	GENERATED_BODY()

	UPROPERTY() TArray<FInv_Slot> Items;

	// Owner zeigt auf ContainerComponent (wie bei FInvContainer::OwnerComponent)
	UPROPERTY(Transient) TObjectPtr<UActorComponent> OwnerComponent = nullptr;
	UPROPERTY(Transient) int32 OwnerContainerIndex = INDEX_NONE;
	

	// Helper
	void Init(int32 NumSlots)
	{
		Items.SetNum(NumSlots);
		for (int32 i=0;i<NumSlots;++i){ Items[i].SlotIndex = i; Items[i].InstanceId.Invalidate(); }
	}

	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
	{
		return FastArrayDeltaSerialize<FInv_Slot, FInvSlotArray>(Items, DeltaParms, *this);
	}

	// UI-Delta-Events
	void PreReplicatedRemove(const TArrayView<int32> RemovedIndices, int32 FinalSize);
	void PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize);
	void PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize);
};

template<>
struct TStructOpsTypeTraits<FInvSlotArray> : public TStructOpsTypeTraitsBase2<FInvSlotArray>
{
	enum { WithNetDeltaSerializer = true };
};