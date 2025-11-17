#include "ContainerGrid.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "RpgInventory/InventoryManagement/Components/InventoryManagerComponent.h"
#include "RpgInventory/Widgets/GridSlots/ContainerSlotButton.h"


void UContainerGrid::BindDelegates()
{
	if (UInventoryManagerComponent* Comp = ContainerComponent.Get())
	{
		// Comp->OnSlotChanged.AddDynamic(this, &UContainerGrid::HandleSlotChanged);
		// Comp->OnEntryChanged.AddDynamic(this, &UContainerGrid::HandleEntryChanged);
	}
}

void UContainerGrid::HandleSlotChanged(int32 ContainerIdx, int32 SlotIdx, FGuid InstanceId)
{
	if (ContainerIdx != ContainerIndex) return;
	
	UpdateOneSlot(SlotIdx);
}

void UContainerGrid::UpdateOneSlot(int32 SlotIdx)
{
	// if (!ContainerComponent.IsValid()) return;
	// if (!SlotWidgets.IsValidIndex(SlotIdx)) return;
	//
	// UContainerSlotButton* SlotWidget = SlotWidgets[SlotIdx];
	// if (!SlotWidget) return;
	//
	// // Entry anhand Slot ermitteln
	// const FInv_InventoryEntry* EntryPtr = ContainerComponent->GetEntryBySlot(ContainerIndex, SlotIdx);
	//
	// if (EntryPtr)
	// {
	// 	const FPrimaryAssetId& ItemId = EntryPtr->GetItemId();
	// 	UInventoryItemDefinition* Def = UInventoryStatics::GetItemDefinitionById(ItemId);
	//
	// 	SlotWidget->SetStackCount(EntryPtr->GetStack());
	// 	SlotWidget->UpdateIcon(Def ? Def->GetIcon() : nullptr);
	// 	SlotWidget->UpdateText();
	// }
	// else
	// {
	// 	// Leerer Slot
	// 	SlotWidget->SetStackCount(0);
	// 	SlotWidget->UpdateIcon(nullptr);
	// 	SlotWidget->UpdateText();
	// }
}

void UContainerGrid::UnbindFromCurrent() const
{
	// if (URpg_ContainerComponent* Comp = ContainerComponent.Get())
	// {
	// 	Comp->OnSlotChanged.RemoveAll(this);
	// 	Comp->OnEntryChanged.RemoveAll(this);
	// }
}

void UContainerGrid::HandleEntryChanged(FGuid InstanceId, const FInventoryEntry& Entry)
{
	// int32 SlotIdx;
	// if (ContainerComponent->FindSlotIndexByInstanceId(ContainerIndex, InstanceId, SlotIdx))
	// {
	// 	UpdateOneSlot(SlotIdx);
	// }
}

void UContainerGrid::NativeDestruct()
{
	UnbindFromCurrent();
	Super::NativeDestruct();
}

void UContainerGrid::BindToContainer(UInventoryManagerComponent* InComponent, int32 InContainerIndex)
{
	UnbindFromCurrent();
	ContainerComponent = InComponent;
	ContainerIndex = InContainerIndex;
	CacheFromDefinition();
	BindDelegates();
	RebuildGrid();
}

void UContainerGrid::CacheFromDefinition()
{
	// CachedRows = 0;
	// CachedCols = 0;
	//
	// const URpg_ContainerComponent* Comp = ContainerComponent.Get();
	// if (!Comp || !Comp->Containers.IsValidIndex(ContainerIndex))
	// {
	// 	return;
	// }
	//
	// const FInvContainer& C = Comp->Containers[ContainerIndex];
	// CachedRows = FMath::Max(1, C.Rows);
	// CachedCols = FMath::Max(1, C.Cols);
}

void UContainerGrid::RebuildGrid()
{
	if (!GridRoot || !SlotButtonClass) return;

	GridRoot->ClearChildren();
	SlotWidgets.Reset();

	// Buttons in row-major order erstellen
	const int32 Total = GetTotalSlots();
	SlotWidgets.SetNum(Total);

	for (int32 Index = 0; Index < Total; ++Index)
	{
		const int32 Row = CachedCols > 0 ? Index / CachedCols : 0;
		const int32 Col = CachedCols > 0 ? Index % CachedCols : 0;

		UContainerSlotButton* SlotWidget = CreateWidget<UContainerSlotButton>(GetOwningPlayer(), SlotButtonClass);
		if (!SlotWidget) continue;

		SlotWidget->SetSlotIndex(Index);
		SlotWidget->InitializeSlotContext(ContainerComponent.Get(), ContainerIndex);

		// Initiale Anzeige aus dem aktuellen Zustand
		if (UUniformGridSlot* GridSlot = GridRoot->AddChildToUniformGrid(SlotWidget, Row, Col))
		{
			// Spacing/Alignment falls benötigt
		}

		SlotWidgets[Index] = SlotWidget;
	}

	// Nach dem Aufbau einmalig alle Slots initial aktualisieren
	for (int32 i = 0; i < SlotWidgets.Num(); ++i)
	{
		UpdateOneSlot(i);
	}
}
