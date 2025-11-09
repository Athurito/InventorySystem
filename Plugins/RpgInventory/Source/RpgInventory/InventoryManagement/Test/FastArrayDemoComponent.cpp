// Fill out your copyright notice in the Description page of Project Settings.


#include "FastArrayDemoComponent.h"

#include "Net/UnrealNetwork.h"

void UFastArrayDemoComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UFastArrayDemoComponent, Inventory);
}

void UFastArrayDemoComponent::ServerAddItem_Implementation(const FString& Name, int32 Count)
{
	int32 OutIndex;
	AddItem_ServerImpl(Name, Count, OutIndex);
}

void UFastArrayDemoComponent::ServerChangeItemCount_Implementation(int32 Index, int32 NewCount)
{
	ChangeItemCount_ServerImpl(Index, NewCount);
}

void UFastArrayDemoComponent::ServerRemoveItem_Implementation(int32 Index)
{
	RemoveItem_ServerImpl(Index);
}

void FDemoArray::PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize)
{
	if (!Owner) return;
	for (int32 Idx : AddedIndices)
	{
		if (Items.IsValidIndex(Idx))
		{
			Owner->EmitAdded(Idx, Items[Idx]);
		}
	}
}

void FDemoArray::PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize)
{
	if (!Owner) return;
	for (int32 Idx : ChangedIndices)
	{
		if (Items.IsValidIndex(Idx))
		{
			Owner->EmitChanged(Idx, Items[Idx]);
		}
	}
}

void FDemoArray::PreReplicatedRemove(const TArrayView<int32> RemovedIndices, int32 FinalSize)
{
	if (!Owner) return;
	for (int32 Idx : RemovedIndices)
	{
		if (Items.IsValidIndex(Idx))
		{
			FDemoItem Copy = Items[Idx]; // sichern, bevor Netcode es löscht
			Owner->EmitRemoved(Idx, Copy);
		}
	}
}

// Sets default values for this component's properties
UFastArrayDemoComponent::UFastArrayDemoComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);
}


// Called when the game starts
void UFastArrayDemoComponent::BeginPlay()
{
	Super::BeginPlay();
	Inventory.Owner = this; 
}

void UFastArrayDemoComponent::AddItem_ServerImpl(const FString& Name, int32 Count, int32& OutIndex)
{
	if (!GetOwner()->HasAuthority()) return;

	FDemoItem& NewItem = Inventory.Items.Emplace_GetRef();
	NewItem.Id = Inventory.Items.Num(); // simpel: laufende Nummer
	NewItem.Name = Name;
	NewItem.Count = Count;

	Inventory.MarkItemDirty(NewItem); // entscheidend!
	OutIndex = Inventory.Items.Num() - 1;
	EmitAdded(OutIndex, NewItem);
}

void UFastArrayDemoComponent::ChangeItemCount_ServerImpl(int32 Index, int32 NewCount)
{
	if (!GetOwner()->HasAuthority()) return;
	if (!Inventory.Items.IsValidIndex(Index)) return;

	FDemoItem& It = Inventory.Items[Index];
	It.Count = NewCount;

	Inventory.MarkItemDirty(It);
	EmitChanged(Index, It);
}

void UFastArrayDemoComponent::RemoveItem_ServerImpl(int32 Index)
{
	if (!GetOwner()->HasAuthority()) return;
	if (!Inventory.Items.IsValidIndex(Index)) return;

	FDemoItem Copy = Inventory.Items[Index];   // für Event sichern
	Inventory.MarkItemDirty(Inventory.Items[Index]);
	Inventory.Items.RemoveAtSwap(Index);

	EmitRemoved(Index, Copy);
}

void UFastArrayDemoComponent::EmitAdded(int32 Index, const FDemoItem& Item)
{
	OnItemAdded.Broadcast(Index, Item);  
}

void UFastArrayDemoComponent::EmitChanged(int32 Index, const FDemoItem& Item)
{
	OnItemChanged.Broadcast(Index, Item);
}

void UFastArrayDemoComponent::EmitRemoved(int32 FormerIndex, const FDemoItem& ItemCopy)
{
	OnItemRemoved.Broadcast(FormerIndex, ItemCopy);
}



