#include "Items/Fragments/ConsumableFragment.h"

#include "Items/Runtime/ItemRuntimeData.h"
#include "Items/Fragments/StackableFragment.h"
#include "Items/Rpg_ItemDefinition.h"
#include "Items/Fragments/Rpg_FragmentTags.h"

bool FConsumableFragment::AllowsContext(EUseContext Ctx) const
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

bool FConsumableFragment::PreflightCanUse(const FItemRuntimeDataContainer& Runtime, const URpg_ItemDefinition* Def) const
{
	if (!Def) return false;
	if (bReduceStack)
	{
		const FStackableRuntimeData* S = Runtime.FindConst<FStackableRuntimeData>(FragmentTags::StackableFragment);
		const int32 Have = S ? S->CurrentStackCount : 1;
		if (Have < FMath::Max(1, QuantityPerUse)) return false;
	}
	// Durability checks could be added here later
	return true;
}

bool FConsumableFragment::ReduceStackAfterUse(FItemRuntimeDataContainer& Runtime, const URpg_ItemDefinition* Def, int32 Uses) const
{
	bool bChanged = false;
	Uses = FMath::Max(0, Uses);
	if (bReduceStack)
	{
		if (const FStackableFragment* Stackable = Def->GetFragmentOfTypeWithTag<FStackableFragment>(FragmentTags::StackableFragment))
		{
			if (auto* S = Runtime.FindOrAddMutable<FStackableRuntimeData>(FragmentTags::StackableFragment))
			{
				const int32 Max = FMath::Max(1, Stackable->GetMaxStackSize());
				const int32 Cost = FMath::Max(1, QuantityPerUse) * Uses;
				S->CurrentStackCount = FMath::Clamp(S->CurrentStackCount - Cost, 0, Max);
				Runtime.MarkDirty(FragmentTags::StackableFragment);
				bChanged = true;
			}
		}
	}
	// Durability cost could be applied here later
	return bChanged;
}
