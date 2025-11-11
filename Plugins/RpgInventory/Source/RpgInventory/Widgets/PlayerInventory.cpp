// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerInventory.h"

#include "CommonActivatableWidgetSwitcher.h"
#include "CommonButtonBase.h"
#include "CommonTabListWidgetBase.h"
#include "InventoryTabButton.h"
#include "TabList.h"
#include "Grid/ContainerGrid.h"
#include "RpgInventory/InventoryManagement/Components/Rpg_ContainerComponent.h"
#include "RpgInventory/InventoryManagement/Container/InventoryContainerDefinition.h"
#include "RpgInventory/InventoryManagement/Utils/InventoryStatics.h"


void UPlayerInventory::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (PlayerTabList)
	{
		PlayerTabList->OnTabSelected.RemoveAll(this);
		PlayerTabList->OnTabSelected.AddDynamic(this, &UPlayerInventory::HandlePlayerTabSelected);
	}
	
	BuildPlayerTabsAndContent();
}

void UPlayerInventory::BuildPlayerTabsAndContent()
{
	if (!PlayerTabList || !PlayerTabContentSwitcher) return;

	EnsurePlayerComponent();

	// Aufräumen
	PlayerTabList->RemoveAllTabs();
	TabMap.Reset();
	TabToContentIndex.Reset();
	PlayerTabContentSwitcher->ClearChildren();

	if (!PlayerContainerComponent.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("UPlayerInventory: No PlayerContainerComponent available."));
		return;
	}
	if (!PlayerTabButtonClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("UPlayerInventory: PlayerTabButtonClass not set."));
		return;
	}
	if (!ContainerGridClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("UPlayerInventory: ContainerGridClass not set."));
		return;
	}

	// const int32 Count = PlayerContainerComponent->GetContainerCount();
	// if (Count <= 0) return;
	//
	// for (int32 i = 0; i < Count; ++i)
	// {
	// 	const UInventoryContainerDefinition* Def = PlayerContainerComponent->GetContainerDefinition(i);
	// 	if (!Def) continue;
	//
	// 	// 1) TabId
	// 	const FName TabId = *FString::Printf(TEXT("Player_%d"), i);
	// 	TabMap.Add(TabId, { PlayerContainerComponent, i });
	//
	// 	// 2) Tab Button registrieren (ohne Content -> wir verwalten Switcher manuell)
	// 	const bool bOk = PlayerTabList->RegisterTab(TabId, PlayerTabButtonClass, /*ContentWidget*/ nullptr);
	// 	if (!bOk) continue;
	//
	// 	// 3) Grid erzeugen (ein Widget pro Tab) und binden
	// 	UContainerGrid* NewGridWidget = CreateWidget<UContainerGrid>(this, ContainerGridClass);
	// 	if (!NewGridWidget) continue;
	// 	NewGridWidget->BindToContainer(PlayerContainerComponent.Get(), i);
	// 	PlayerTabContentSwitcher->AddChild(NewGridWidget);
	// 	const int32 SwitcherIndex = PlayerTabContentSwitcher->GetChildIndex(NewGridWidget);
	// 	TabToContentIndex.Add(TabId, SwitcherIndex);
	// 	if (SwitcherIndex == INDEX_NONE)
	// 	{
	// 		UE_LOG(LogTemp, Warning, TEXT("UPlayerInventory: Failed to resolve switcher index for %s"), *TabId.ToString());
	// 	}
	//
	// 	// 4) Button betiteln
	// 	if (UCommonButtonBase* RawBtn = PlayerTabList->GetTabButtonBaseByID(TabId))
	// 	{
	// 		if (UInventoryTabButton* Btn = Cast<UInventoryTabButton>(RawBtn))
	// 		{
	// 			const FText Label = !Def->DisplayName.IsEmpty()
	// 				? Def->DisplayName
	// 				: FText::FromString(FString::Printf(TEXT("Bag %d"), i + 1));
	// 			Btn->SetLabelAndIcon(Label, Def->TabIcon);
	// 		}
	// 	}
	// }

	// // Start-Tab wählen
	// const int32 StartIdx = FMath::Clamp(RequestedStartTabIndex, 0, Count - 1);
	// const FName StartTabId = *FString::Printf(TEXT("Player_%d"), StartIdx);
	// PlayerTabList->SelectTabByID(StartTabId, /*bSuppressClickFeedback*/ true);
	//
	// // Content switchen
	// if (const int32* ContentIdx = TabToContentIndex.Find(StartTabId))
	// {
	// 	PlayerTabContentSwitcher->SetActiveWidgetIndex(*ContentIdx);
	// }
}

void UPlayerInventory::EnsurePlayerComponent()
{
	// Auto-resolve the local player's container if not explicitly initialized
	if (!PlayerContainerComponent.IsValid())
	{
		if (APlayerController* PC = GetOwningPlayer())
		{
			AActor* OwnerActor = PC->PlayerState ? static_cast<AActor*>(PC->PlayerState) : static_cast<AActor*>(PC);
			if (OwnerActor)
			{
				if (URpg_ContainerComponent* AutoComp = UInventoryStatics::GetContainerComponent(OwnerActor))
				{
					PlayerContainerComponent = AutoComp;
				}
			}
		}
	}
}

void UPlayerInventory::HandlePlayerTabSelected(FName TabId)
{
	if (const FContainerRef* Ref = TabMap.Find(TabId))
	{
		ContextContainerComponent = Ref->Comp;
		ContextContainerIndex = Ref->Index;

		if (const int32* ContentIdx = TabToContentIndex.Find(TabId))
		{
			if (PlayerTabContentSwitcher)
			{
				PlayerTabContentSwitcher->SetActiveWidgetIndex(*ContentIdx);
			}
		}
	}
}
