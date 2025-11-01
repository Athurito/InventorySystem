#include "Widgets/Grid/ContainerGrid.h"

#include "InventoryManagement/Components/Rpg_ContainerComponent.h"
#include "InventoryManagement/FastArray/Rpg_FastArray.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "InventoryManagement/Utils/InventoryStatics.h"
#include "Items/Rpg_ItemDefinition.h"
#include "Widgets/GridSlots/ContainerSlotButton.h"

void UContainerGrid::BindDelegates()
{
	if (URpg_ContainerComponent* Comp = ContainerComponent.Get())
	{
		Comp->OnItemAdded.RemoveAll(this);
		Comp->OnItemRemoved.RemoveAll(this);
		Comp->OnItemAdded.AddDynamic(this, &UContainerGrid::HandleItemAdded);
		Comp->OnItemRemoved.AddDynamic(this, &UContainerGrid::HandleItemRemoved);
	}
}

void UContainerGrid::UnbindFromCurrent()
{
	if (URpg_ContainerComponent* Comp = ContainerComponent.Get())
	{
		Comp->OnItemAdded.RemoveAll(this);
		Comp->OnItemRemoved.RemoveAll(this);
	}
}

void UContainerGrid::NativeOnInitialized()
{
	Super::NativeOnInitialized();
}

void UContainerGrid::NativeDestruct()
{
	UnbindFromCurrent();
	Super::NativeDestruct();
}

void UContainerGrid::BindToContainer(URpg_ContainerComponent* InComponent, int32 InContainerIndex)
{
	UnbindFromCurrent();
	ContainerComponent = InComponent;
	ContainerIndex = InContainerIndex;
	CacheFromDefinition();
	BindDelegates();
	RebuildGrid();
}

void UContainerGrid::HandleItemAdded(FInv_InventoryEntry Item)
{
	RebuildGrid();
}

void UContainerGrid::HandleItemRemoved(FInv_InventoryEntry Item)
{
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
	if (!GridRoot || !SlotButtonClass) return;
	GridRoot->ClearChildren();
	// Cache für Item Definitions erstellen
	TMap<FPrimaryAssetId, URpg_ItemDefinition*> DefinitionCache;

	// Create buttons in row-major order
	const int32 Total = GetTotalSlots();
	
	// WICHTIG: Stelle sicher, dass das Mapping existiert und die richtige Größe hat
	if (const URpg_ContainerComponent* Comp = ContainerComponent.Get())
	{
		if (Comp->Containers.IsValidIndex(ContainerIndex))
		{
			// non-const helper wäre besser, hier ggf. const_cast oder expose Ensure über const
			const_cast<URpg_ContainerComponent*>(Comp)->EnsureSlotMapSize(ContainerIndex, Total);
		}
	}
	
	for (int32 Index = 0; Index < Total; ++Index)
	{
		const int32 Row = CachedCols > 0 ? Index / CachedCols : 0;
		const int32 Col = CachedCols > 0 ? Index % CachedCols : 0;

		UContainerSlotButton* SlotWidget = CreateWidget<UContainerSlotButton>(GetOwningPlayer(), SlotButtonClass);
		if (!SlotWidget) continue;

		SlotWidget->SetSlotIndex(Index);
		SlotWidget->InitializeSlotContext(ContainerComponent.Get(), ContainerIndex);

		// Anzeige anhand Slot→InstanceId Mapping
		const FInv_InventoryEntry* EntryPtr = nullptr;
		if (const URpg_ContainerComponent* Comp = ContainerComponent.Get())
		{
			EntryPtr = Comp->GetEntryBySlot(ContainerIndex, Index);
		}

		if (EntryPtr)
		{
			const FPrimaryAssetId& ItemId = EntryPtr->GetItemId();

			URpg_ItemDefinition*& ItemDefinitionRef =
				DefinitionCache.FindOrAdd(ItemId, UInventoryStatics::GetItemDefinitionById(ItemId));

			if (ItemDefinitionRef)
			{
				SlotWidget->SetStackCount(EntryPtr->GetStack());
				const auto Icon = ItemDefinitionRef->GetIcon();
				SlotWidget->UpdateIcon(Icon);
				SlotWidget->UpdateText();
			}
			else
			{
				// Fallback falls Def fehlt
				SlotWidget->SetStackCount(EntryPtr->GetStack());
				SlotWidget->UpdateIcon(nullptr);
				SlotWidget->UpdateText();
			}
		}
		else
		{
			// Leerer Slot visuell „clearen“
			SlotWidget->SetStackCount(0);
			SlotWidget->UpdateIcon(nullptr);
			SlotWidget->UpdateText();
		}

		if (UUniformGridSlot* GridSlot = GridRoot->AddChildToUniformGrid(SlotWidget, Row, Col))
		{
			// spacing/alignment optional
		}
	}
}
