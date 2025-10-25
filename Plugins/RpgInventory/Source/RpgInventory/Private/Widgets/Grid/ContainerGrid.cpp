#include "Widgets/Grid/ContainerGrid.h"

#include "InventoryManagement/Components/Rpg_ContainerComponent.h"
#include "InventoryManagement/FastArray/Rpg_FastArray.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "InventoryManagement/Utils/InventoryStatics.h"
#include "Items/Rpg_ItemDefinition.h"
#include "Widgets/GridSlots/ContainerSlotButton.h"

void UContainerGrid::NativeOnInitialized()
{
	Super::NativeOnInitialized();
}

void UContainerGrid::BindToContainer(URpg_ContainerComponent* InComponent, int32 InContainerIndex)
{
	ContainerComponent = InComponent;
	ContainerIndex = InContainerIndex;
	CacheFromDefinition();
	RebuildGrid();
}

void UContainerGrid::CacheFromDefinition()
{
	CachedRows = 0;
	CachedCols = 0;

	const URpg_ContainerComponent* Comp = ContainerComponent.Get();
	if (!Comp || !Comp->Containers.IsValidIndex(ContainerIndex))
	{
		return;
	}

	const FInvContainer& C = Comp->Containers[ContainerIndex];
	CachedRows = FMath::Max(1, C.Rows);
	CachedCols = FMath::Max(1, C.Cols);
}

void UContainerGrid::RebuildGrid()
{
	if (!GridRoot)
	{
		return;
	}

	GridRoot->ClearChildren();

	if (!SlotButtonClass)
	{
		return;
	}

	// Create buttons in row-major order
	const int32 Total = GetTotalSlots();
	for (int32 Index = 0; Index < Total; ++Index)
	{
		const int32 Row = CachedCols > 0 ? Index / CachedCols : 0;
		const int32 Col = CachedCols > 0 ? Index % CachedCols : 0;

		UContainerSlotButton* SlotWidget = CreateWidget<UContainerSlotButton>(GetOwningPlayer(), SlotButtonClass);
		if (!SlotWidget)
		{
			continue;
		}
		SlotWidget->SetSlotIndex(Index);

		// Optional: simple stack display if entries align 1:1 with slots
		if (const URpg_ContainerComponent* Comp = ContainerComponent.Get())
		{
			if (Comp->Containers.IsValidIndex(ContainerIndex))
			{
				const FInvContainer& C = Comp->Containers[ContainerIndex];
				if (C.GetEntries().IsValidIndex(Index))
				{
					auto& Entry = C.GetEntries()[Index];
					const auto* ItemDefinition = UInventoryStatics::GetItemDefinitionById(Entry.GetItemId());
					SlotWidget->SetStackCount(Entry.GetStack());
					auto icon = ItemDefinition->GetIcon();
					SlotWidget->UpdateIcon(icon);
					SlotWidget->UpdateText();
					
				}
			}
		}

		if (UUniformGridSlot* GridSlot = GridRoot->AddChildToUniformGrid(SlotWidget, Row, Col))
		{
			// Optional spacing/alignment could be adjusted here
		}
	}
}
