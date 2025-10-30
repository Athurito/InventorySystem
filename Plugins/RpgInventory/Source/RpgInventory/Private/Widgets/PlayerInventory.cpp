// Fill out your copyright notice in the Description page of Project Settings.


#include "Widgets/PlayerInventory.h"

#include "CommonActivatableWidgetSwitcher.h"
#include "CommonButtonBase.h"
#include "CommonTabListWidgetBase.h"
#include "InventoryManagement/Utils/InventoryStatics.h"
#include "Widgets/InventoryTabButton.h"
#include "Widgets/TabList.h"
#include "Widgets/Grid/ContainerGrid.h"

void UPlayerInventory::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (PlayerTabList)
	{
		PlayerTabList->OnTabSelected.RemoveAll(this);
		PlayerTabList->OnTabSelected.AddDynamic(this, &UPlayerInventory::HandlePlayerTabSelected);
		PlayerTabList->SetLinkedSwitcher(PlayerTabContentSwitcher);
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

	const int32 Count = PlayerContainerComponent->GetContainerCount();
	if (Count <= 0) return;

	for (int32 i = 0; i < Count; ++i)
	{
		const UInventoryContainerDefinition* Def = PlayerContainerComponent->GetContainerDefinition(i);
		if (!Def) continue;

		// 1) TabId
		const FName TabId = *FString::Printf(TEXT("Player_%d"), i);
		TabMap.Add(TabId, { PlayerContainerComponent, i });

		// 2) Tab Button registrieren
		const bool bOk = PlayerTabList->RegisterTab(TabId, PlayerTabButtonClass, /*ContentWidget*/ nullptr);
		if (!bOk) continue;

		// 3) Button betiteln
		if (UCommonButtonBase* RawBtn = PlayerTabList->GetTabButtonBaseByID(TabId))
		{
			if (auto* Btn = Cast<UInventoryTabButton>(RawBtn))
			{
				const FText Label = !Def->DisplayName.IsEmpty()
					? Def->DisplayName
					: FText::FromString(FString::Printf(TEXT("Bag %d"), i + 1));
				Btn->SetLabelAndIcon(Label, Def->TabIcon);
			}
		}

		// 4) Grid erzeugen (ein Widget pro Tab)
		UContainerGrid* NewGridWidget = CreateWidget<UContainerGrid>(this, ContainerGridClass);
		if (!NewGridWidget) continue;

		// Falls deine Gridklasse UContainerGrid ist:
		if (UContainerGrid* Grid = Cast<UContainerGrid>(NewGridWidget))
		{
			Grid->BindToContainer(PlayerContainerComponent.Get(), i);
		}

		const int32 SwitcherIndex = PlayerTabContentSwitcher->AddChild(NewGridWidget)->GetLinkerIndex();
		TabToContentIndex.Add(TabId, SwitcherIndex);
	}

	// Start-Tab wählen
	const int32 StartIdx = FMath::Clamp(RequestedStartTabIndex, 0, Count - 1);
	const FName StartTabId = *FString::Printf(TEXT("Player_%d"), StartIdx);
	PlayerTabList->SelectTabByID(StartTabId, /*bSuppressClickFeedback*/ true);

	// Content switchen
	if (const int32* ContentIdx = TabToContentIndex.Find(StartTabId))
	{
		PlayerTabContentSwitcher->SetActiveWidgetIndex(*ContentIdx);
	}
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
		PlayerContainerComponent = Ref->Comp;
		EnsurePlayerComponent();
	}
}
