#include "Inventory/UI/CUI_InventorySlot.h"
#include "Inventory/UI/CUI_InventoryGrid.h"
#include "Inventory/Runtime/RPGInventoryComponent.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Image.h"
#include "Components/Border.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Blueprint/DragDropOperation.h"

UInventorySlotWidget::UInventorySlotWidget()
{
	bIsFocusable = true;
}

void UInventorySlotWidget::Init(UInventoryGridWidget* InOwnerGrid, int32 InSlotIndex, URPGInventoryComponent* InInventory)
{
	OwnerGrid = InOwnerGrid;
	SlotIndex = InSlotIndex;
	Inventory = InInventory;
	Refresh();
}

void UInventorySlotWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (!Border)
	{
		if (WidgetTree)
		{
			Border = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Border"));
			IconImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("Icon"));
			if (Border && IconImage)
			{
				Border->SetPadding(FMargin(2.f));
				Border->SetHorizontalAlignment(HAlign_Fill);
				Border->SetVerticalAlignment(VAlign_Fill);
				Border->SetBrushColor(FLinearColor(0.04f,0.04f,0.04f,1.f));
				Border->SetContent(IconImage);
				WidgetTree->RootWidget = Border;
			}
		}
	}
	Refresh();
}

void UInventorySlotWidget::Refresh()
{
	// Determine occupancy using owner's inventory
	URPGInventoryComponent* Inv = Inventory;

	// Visual minimal: gray empty, green if occupied
	bool bOccupied = false;
	if (!Inv)
	{
		if (APlayerController* PC = GetOwningPlayer())
		{
			if (APawn* P = PC->GetPawn())
			{
				Inv = P->FindComponentByClass<URPGInventoryComponent>();
			}
		}
	}
	if (Inv)
	{
		const TArray<FInventoryEntry>& Entries = Inv->GetEntries();
		for (const FInventoryEntry& E : Entries)
		{
			if (E.SlotIndex == SlotIndex)
			{
				bOccupied = true;
				CachedEntryId = E.EntryId;
				break;
			}
		}
	}

	if (Border)
	{
		Border->SetBrushColor(bOccupied ? FLinearColor(0.05f, 0.25f, 0.05f, 1.f) : FLinearColor(0.06f, 0.06f, 0.06f, 1.f));
	}
}

FReply UInventorySlotWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		return UWidgetBlueprintLibrary::DetectDragIfPressed(InMouseEvent, this, EKeys::LeftMouseButton).NativeReply;
	}
	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

void UInventorySlotWidget::NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation)
{
	UDragDropOperation* Op = UWidgetBlueprintLibrary::CreateDragDropOperation(UDragDropOperation::StaticClass());
	if (Op)
	{
		Op->Payload = this; // payload is the source slot
		Op->DefaultDragVisual = this;
		OutOperation = Op;
	}
}

bool UInventorySlotWidget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	if (!OwnerGrid || !InOperation) return false;
	if (UInventorySlotWidget* FromSlot = Cast<UInventorySlotWidget>(InOperation->Payload))
	{
		if (FromSlot == this) return false;
		return OwnerGrid->HandleDropOnSlot(FromSlot->GetSlotIndex(), SlotIndex);
	}
	return false;
}
