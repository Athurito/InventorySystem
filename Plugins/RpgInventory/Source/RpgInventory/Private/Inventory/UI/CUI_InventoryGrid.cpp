#include "Inventory/UI/CUI_InventoryGrid.h"
#include "Inventory/UI/CUI_InventorySlot.h"
#include "Inventory/Runtime/RPGInventoryComponent.h"

#include "Blueprint/WidgetTree.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"

UInventoryGridWidget::UInventoryGridWidget()
{
}

void UInventoryGridWidget::NativeConstruct()
{
	Super::NativeConstruct();
	EnsureGridPanel();
	RebuildGrid();
}

void UInventoryGridWidget::SetInventory(URPGInventoryComponent* InInventory)
{
	if (Inventory == InInventory) return;
	if (Inventory)
	{
		Inventory->OnInventoryChanged.RemoveAll(this);
	}
	Inventory = InInventory;
	if (Inventory)
	{
		Inventory->OnInventoryChanged.AddUniqueDynamic(this, &UInventoryGridWidget::OnInventoryChanged);
	}
	RebuildGrid();
}

void UInventoryGridWidget::OnInventoryChanged()
{
	RefreshSlots();
}

void UInventoryGridWidget::EnsureGridPanel()
{
	if (!GridPanel)
	{
		if (WidgetTree)
		{
			GridPanel = WidgetTree->ConstructWidget<UUniformGridPanel>(UUniformGridPanel::StaticClass(), TEXT("GridPanel"));
			WidgetTree->RootWidget = GridPanel;
		}
	}
}

void UInventoryGridWidget::RebuildGrid()
{
	EnsureGridPanel();
	if (!GridPanel) return;

	GridPanel->ClearChildren();

	const int32 Cap = Inventory ? Inventory->GetCapacity() : (Columns * Columns);
	const int32 Cols = FMath::Max(1, Columns);

	for (int32 i = 0; i < Cap; ++i)
	{
		UInventorySlotWidget* SlotWidget = nullptr;
		if (SlotWidgetClass)
		{
			UUserWidget* Created = CreateWidget<UUserWidget>(this, SlotWidgetClass);
			SlotWidget = Cast<UInventorySlotWidget>(Created);
		}
		else
		{
			UUserWidget* Created = CreateWidget<UUserWidget>(this, UInventorySlotWidget::StaticClass());
			SlotWidget = Cast<UInventorySlotWidget>(Created);
		}
		if (!SlotWidget) continue;

		SlotWidget->Init(this, i, Inventory);

		UUniformGridSlot* GridSlot = GridPanel->AddChildToUniformGrid(SlotWidget, i / Cols, i % Cols);
		if (GridSlot)
		{
			GridSlot->SetHorizontalAlignment(HAlign_Fill);
			GridSlot->SetVerticalAlignment(VAlign_Fill);
		}
	}

	RefreshSlots();
}

void UInventoryGridWidget::RefreshSlots()
{
	if (!GridPanel) return;
	const int32 NumChildren = GridPanel->GetChildrenCount();
	for (int32 i = 0; i < NumChildren; ++i)
	{
		if (UInventorySlotWidget* SlotWidget = Cast<UInventorySlotWidget>(GridPanel->GetChildAt(i)))
		{
			SlotWidget->Refresh();
		}
	}
}

bool UInventoryGridWidget::HandleDropOnSlot(int32 FromSlot, int32 ToSlot)
{
	if (!Inventory) return false;
	if (AActor* Owner = Inventory->GetOwner())
	{
		if (Owner->HasAuthority())
		{
			return Inventory->MoveSlotToSlot(FromSlot, ToSlot);
		}
	}
	// client path
	Inventory->Server_MoveSlotToSlot(FromSlot, ToSlot);
	return true;
}
