

#include "InventoryBlueprintLibrary.h"

#include "RpgInventory/InventoryManagement/Components/Rpg_ContainerComponent.h"

bool UInventoryBlueprintLibrary::ConsumeStackByInstance(URpg_ContainerComponent* Container, int32 ContainerIndex, const FGuid& InstanceId, int32 Quantity, int32& OutConsumed)
{
	return false;
	// OutConsumed = 0;
	// if (!Container || Quantity <= 0)
	// {
	// 	return false;
	// }
	// AActor* Owner = Container->GetOwner();
	// if (Owner && Owner->HasAuthority())
	// {
	// 	int32 Removed = 0;
	// 	const bool bOk = Container->RemoveItemFromContainer(ContainerIndex, InstanceId, Quantity, Removed);
	// 	OutConsumed = Removed;
	// 	if (Removed > 0)
	// 	{
	// 		// Notify generic consumption for UI/audio hooks
	// 		Container->OnItemConsumed.Broadcast(nullptr, Removed);
	// 	}
	// 	return bOk && Removed > 0;
	// }
	// else
	// {
	// 	// Route to server; replication will update UI/clients
	// 	Container->ServerRemoveItemFromContainer(ContainerIndex, InstanceId, Quantity);
	// 	return false;
	// }
}

bool UInventoryBlueprintLibrary::ConsumeWorldItemStack(UInventoryItemComponent* ItemComponent, int32 Quantity, int32& OutConsumed)
{
	// OutConsumed = 0;
	// if (!ItemComponent || Quantity <= 0) return false;
	//
	// AActor* Owner = ItemComponent->GetOwner();
	// if (!Owner || !Owner->HasAuthority()) return false;
	//
	// // Optional: über Definition prüfen, ob überhaupt stackable
	// const UInventoryItemDefinition* Def = ItemComponent->GetItemDefinition();
	// const bool bIsStackable = (Def && Def->GetFragmentOfTypeWithTag<FInventoryFragment_Stackable>(FragmentTags::StackableFragment) != nullptr);
	//
	// // Zugriff auf RuntimeData (non-const wäre besser; sonst const_cast)
	// const FItemRuntimeDataContainer& ConstRuntime = ItemComponent->GetRuntimeData();
	// FItemRuntimeDataContainer& Runtime = const_cast<FItemRuntimeDataContainer&>(ConstRuntime);
	//
	// // Aktuellen Stack nur lesen (legt nichts an)
	// const FStackableRuntimeData* StackConst = Runtime.FindConst<FStackableRuntimeData>(FragmentTags::StackableFragment);
	//
	// if (!bIsStackable || !StackConst)
	// {
	// 	// Non-stackable oder kein Laufzeit-Stack vorhanden → 1x konsumieren und zerstören
	// 	OutConsumed = 1;
	// 	if (AActor* OwnerActor = ItemComponent->GetOwner())
	// 	{
	// 		OwnerActor->Destroy();
	// 	}
	// 	return true;
	// }
	//
	// const int32 Before = StackConst->CurrentStackCount;
	// if (Before <= 0) return false;
	//
	// const int32 ToConsume = FMath::Clamp(Quantity, 0, Before);
	// const int32 After     = Before - ToConsume;
	//
	// // Schreiben: legt bei Bedarf an und markiert automatisch dirty
	// bool bNowZero = false;
	// Runtime.Modify<FStackableRuntimeData>(FragmentTags::StackableFragment,
	// 	[&](FStackableRuntimeData& D)
	// 	{
	// 		D.CurrentStackCount = After;
	// 		bNowZero = (D.CurrentStackCount <= 0);
	// 	});
	//
	// OutConsumed = ToConsume;
	//
	// if (bNowZero)
	// {
	// 	if (AActor* OwnerActor = ItemComponent->GetOwner())
	// 	{
	// 		OwnerActor->Destroy();
	// 	}
	// }
	// return ToConsume > 0;
	return false;
}
