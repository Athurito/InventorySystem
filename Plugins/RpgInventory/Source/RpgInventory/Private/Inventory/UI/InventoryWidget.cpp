#include "Inventory/UI/InventoryWidget.h"
#include "Inventory/UI/CUI_InventoryGrid.h"
#include "Inventory/Runtime/RPGInventoryComponent.h"
#include "Components/PanelWidget.h"

void UInventoryWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (bAutoBindOnConstruct)
	{
		// Try find inventory on owning pawn if not set
		if (!Inventory)
		{
			if (APlayerController* PC = GetOwningPlayer())
			{
				if (APawn* P = PC->GetPawn())
				{
					Inventory = P->FindComponentByClass<URPGInventoryComponent>();
				}
			}
		}
		if (BackpackGrid && Inventory)
		{
			BackpackGrid->SetInventory(Inventory);
		}
	}
}

void UInventoryWidget::SetInventory(URPGInventoryComponent* InInventory)
{
	Inventory = InInventory;
	if (BackpackGrid)
	{
		BackpackGrid->SetInventory(Inventory);
	}
}
