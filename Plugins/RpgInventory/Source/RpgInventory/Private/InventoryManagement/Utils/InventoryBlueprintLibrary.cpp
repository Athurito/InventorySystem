#include "InventoryManagement/Utils/InventoryBlueprintLibrary.h"

#include "InventoryManagement/Components/Rpg_ContainerComponent.h"
#include "Items/Components/Rpg_ItemComponent.h"
#include "Items/Runtime/ItemRuntimeData.h"
#include "Items/Fragments/Rpg_FragmentTags.h"

bool UInventoryBlueprintLibrary::ConsumeStackByInstance(URpg_ContainerComponent* Container, int32 ContainerIndex, const FGuid& InstanceId, int32 Quantity, int32& OutConsumed)
{
	OutConsumed = 0;
	if (!Container || Quantity <= 0)
	{
		return false;
	}
	AActor* Owner = Container->GetOwner();
	if (Owner && Owner->HasAuthority())
	{
		int32 Removed = 0;
		const bool bOk = Container->RemoveItemFromContainer(ContainerIndex, InstanceId, Quantity, Removed);
		OutConsumed = Removed;
		if (Removed > 0)
		{
			// Notify generic consumption for UI/audio hooks
			Container->OnItemConsumed.Broadcast(nullptr, Removed);
		}
		return bOk && Removed > 0;
	}
	else
	{
		// Route to server; replication will update UI/clients
		Container->ServerRemoveItemFromContainer(ContainerIndex, InstanceId, Quantity);
		return false;
	}
}

bool UInventoryBlueprintLibrary::ConsumeWorldItemStack(URpg_ItemComponent* ItemComponent, int32 Quantity, int32& OutConsumed)
{
	OutConsumed = 0;
	if (!ItemComponent || Quantity <= 0)
	{
		return false;
	}
	AActor* Owner = ItemComponent->GetOwner();
	if (!Owner || !Owner->HasAuthority())
	{
		return false;
	}

	// Reduce stack in the item's runtime data
	FItemRuntimeDataContainer const& ConstRuntime = ItemComponent->GetRuntimeData();
	FItemRuntimeDataContainer& Runtime = const_cast<FItemRuntimeDataContainer&>(ConstRuntime);
	FStackableRuntimeData* StackData = Runtime.FindMutable<FStackableRuntimeData>(FragmentTags::StackableFragment);
	if (!StackData)
	{
		// Non-stackable items: treat as single use
		OutConsumed = 1;
		if (AActor* OwnerActor = ItemComponent->GetOwner())
		{
			OwnerActor->Destroy();
		}
		return true;
	}

	const int32 Before = StackData->CurrentStackCount;
	if (Before <= 0)
	{
		return false;
	}
	const int32 ToConsume = FMath::Clamp(Quantity, 0, Before);
	StackData->CurrentStackCount = Before - ToConsume;
	Runtime.MarkDirty(FragmentTags::StackableFragment);
	OutConsumed = ToConsume;

	if (StackData->CurrentStackCount <= 0)
	{
		if (AActor* OwnerActor = ItemComponent->GetOwner())
		{
			OwnerActor->Destroy();
		}
	}
	return ToConsume > 0;
}
