#include "InventoryFragment_Consumable.h"

#include "InventoryFragment_Stackable.h"

bool UInventoryFragment_Consumable::AllowsContext(EUseContext Ctx) const
{
	switch (UseAvailability)
	{
		case EUseAvailability::WorldOnly: return Ctx == EUseContext::World;
		case EUseAvailability::InventoryOnly: return Ctx != EUseContext::World;
		case EUseAvailability::WorldOrInventory: return true;
		case EUseAvailability::PickupThenUseIfWorld: return Ctx != EUseContext::World; // must be picked up first
	}
	return false;
}

bool UInventoryFragment_Consumable::PreflightCanUse(const FItemRuntimeDataContainer& Runtime, const UInventoryItemDefinition* Def) const
{
	if (!Def) return false;
	if (bReduceStack)
	{
		// // const FStackableRuntimeData* S = Runtime.FindConst<FStackableRuntimeData>(FragmentTags::StackableFragment);
		// const int32 Have = S ? S->CurrentStackCount : 1;
		// if (Have < FMath::Max(1, QuantityPerUse)) return false;
	}
	// Durability checks could be added here later
	return true;
}

bool UInventoryFragment_Consumable::ReduceStackAfterUse(FItemRuntimeDataContainer& Runtime, const UInventoryItemDefinition* Def, int32 Uses) const
{
	bool bChanged = false;
	Uses = FMath::Max(0, Uses);
	// if (bReduceStack)
	// {
	// 	if (const FInventoryFragment_Stackable* Stackable = Def->GetFragmentOfTypeWithTag<FInventoryFragment_Stackable>(FragmentTags::StackableFragment))
	// 	{
	//
	// 		if (auto* S = Runtime.Modify<FStackableRuntimeData>(FragmentTags::StackableFragment,[&](FStackableRuntimeData& D)
	// 		{
	// 			const int32 Max  = FMath::Max(1, Stackable->GetMaxStackSize());
	// 			const int32 Cost = FMath::Max(1, QuantityPerUse) * Uses;
	//
	// 			// Optionaler Fallback, falls der Eintrag neu war
	// 			if (D.CurrentStackCount <= 0)
	// 			{
	// 				D.CurrentStackCount = 1;
	// 			}
	//
	// 			D.CurrentStackCount = FMath::Clamp(D.CurrentStackCount - Cost, 0, Max);
	// 		}))
	// 		{
	// 			bChanged = true;  // Modify gibt != nullptr zurück → es wurde geschrieben & dirty markiert
	// 		}
	// 	}
	// }
	// // Durability cost could be applied here later
	return bChanged;
}
