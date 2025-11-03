// Fill out your copyright notice in the Description page of Project Settings.


#include "InventoryManagement/Components/Rpg_ContainerComponent.h"


#include "GameFramework/Pawn.h"
#include "GameFramework/Controller.h"
#include "Items/Components/Rpg_ItemComponent.h"
#include "Items/Rpg_ItemDefinition.h"
#include "Items/Fragments/ConsumableFragment.h"
#include "Items/Fragments/StackableFragment.h"
#include "Net/UnrealNetwork.h"
#include "InventoryManagement/Utils/InventoryStatics.h"
#include "InventoryManagement/Use/ItemUseSource_Inventory.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "InventoryManagement/FastArray/SlotArray.h"

URpg_ContainerComponent::URpg_ContainerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void URpg_ContainerComponent::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(URpg_ContainerComponent, Containers);
	DOREPLIFETIME(URpg_ContainerComponent, Slots);
}

void URpg_ContainerComponent::OnRep_Containers()
{
	// Owner setzen (wie bisher) und Slot-Arrays initialisieren/angleichen
	for (int32 i=0;i<Containers.Num();++i)
	{
		Containers[i].SetOwner(this);
	}

	for (FInvContainer& C : Containers)
	{
		for (FInv_InventoryEntry& E : C.GetEntries())
		{
			if (E.GetRuntimeData().OwnerComponent != this)
				E.SetRuntimeDataOwner(this);
		}
	}
	InitSlotsArrayFromContainers();
}

void URpg_ContainerComponent::BroadcastSlotChangedForOwner(const FInvSlotArray* OwnerArray, int32 SlotIndex, const FGuid& InstanceId)
{
	int32 ContainerIdx = INDEX_NONE;

	// Primär: nutze OwnerContainerIndex, wenn gesetzt
	if (OwnerArray && OwnerArray->OwnerContainerIndex != INDEX_NONE)
	{
		ContainerIdx = OwnerArray->OwnerContainerIndex;
	}
	else if (OwnerArray)
	{
		// Fallback: Pointer-Identität im Slots-Array suchen
		ContainerIdx = Slots.IndexOfByPredicate(
			[OwnerArray](const FInvSlotArray& A){ return &A == OwnerArray; });
	}

	if (ContainerIdx != INDEX_NONE)
	{
		OnSlotChanged.Broadcast(ContainerIdx, SlotIndex, InstanceId);
	}
}

int32 URpg_ContainerComponent::GetContainerCount() const
{
	return InitialContainerDefs.Num();
}

void URpg_ContainerComponent::TryConsumeItem(URpg_ItemComponent* ItemComponent, const int32 Quantity)
{
	// Backward compatibility: route to unified world-use path
	TryUseWorldItem(ItemComponent, Quantity);
}

void URpg_ContainerComponent::TryUseItemByInstance(int32 ContainerIndex, const FGuid& InstanceId, int32 Quantity)
{
	if (Quantity <= 0) return;
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		InternalUseItem_Inventory(ContainerIndex, InstanceId, Quantity);
	}
	else
	{
		ServerUseItemByInstance(ContainerIndex, InstanceId, Quantity);
	}
}

void URpg_ContainerComponent::TryUseWorldItem(URpg_ItemComponent* ItemComponent, int32 Quantity)
{
	if (!ItemComponent || Quantity <= 0) return;
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		InternalUseItem_World(ItemComponent, Quantity);
	}
	else
	{
		ServerUseWorldItem(ItemComponent, Quantity);
	}
}

UInventoryContainerDefinition* URpg_ContainerComponent::GetContainerDefinition(const int32 Index) const
{
	if (!InitialContainerDefs.IsValidIndex(Index)) return nullptr;
	
	TSoftObjectPtr<UInventoryContainerDefinition> Def = InitialContainerDefs[Index];
	
	return Def.IsValid() ? Def.Get() : Def.LoadSynchronous();
}

const FInv_InventoryEntry* URpg_ContainerComponent::GetEntryBySlot(int32 ContainerIdx, int32 SlotIdx) const
{
	if (!Containers.IsValidIndex(ContainerIdx)) return nullptr;

	const FInvContainer& C = Containers[ContainerIdx];

	const FGuid Id = GetSlotInstance(ContainerIdx, SlotIdx);
	if (!Id.IsValid()) return nullptr;

	const int32 EntryIdx = C.FindIndexByInstance(Id);
	if (EntryIdx == INDEX_NONE) return nullptr;

	return &C.GetEntries()[EntryIdx];
}

FInv_InventoryEntry* URpg_ContainerComponent::GetEntryBySlotMutable(int32 ContainerIdx, int32 SlotIdx)
{
	if (!Containers.IsValidIndex(ContainerIdx)) return nullptr;
	const FGuid Id = GetSlotInstance(ContainerIdx, SlotIdx);
	if (!Id.IsValid()) return nullptr;
	return Containers[ContainerIdx].FindEntryMutableByInstance(Id);
}

bool URpg_ContainerComponent::FindSlotIndexByInstanceId(int32 ContainerIdx, const FGuid& InstanceId,
	int32& OutSlotIdx) const
{
	OutSlotIdx = INDEX_NONE;
	if (!Slots.IsValidIndex(ContainerIdx)) return false;
	const auto& Arr = Slots[ContainerIdx].Items;
	for (int32 i=0;i<Arr.Num();++i)
	{
		if (Arr[i].InstanceId == InstanceId) { OutSlotIdx = i; return true; }
	}
	return false;
}

void URpg_ContainerComponent::EnsureSlotsInitializedFor(URpg_ContainerComponent* Self, int32 ContainerIdx)
{
	if (!Self->Containers.IsValidIndex(ContainerIdx)) return;
	if (Self->Slots.Num() < Self->Containers.Num())
		Self->Slots.SetNum(Self->Containers.Num());

	auto& Arr = Self->Slots[ContainerIdx];
	Arr.OwnerComponent = Self;

	const int32 Total = Self->Containers[ContainerIdx].Rows * Self->Containers[ContainerIdx].Cols;
	if (Arr.Items.Num() != Total)
	{
		Arr.Init(Total);
		Arr.MarkArrayDirty();
	}
	
}


void URpg_ContainerComponent::ClearSlot(int32 ContainerIdx, int32 SlotIdx)
{
	if (!Slots.IsValidIndex(ContainerIdx)) return;
	auto& Arr = Slots[ContainerIdx];
	if (!Arr.Items.IsValidIndex(SlotIdx)) return;
	if (Arr.Items[SlotIdx].InstanceId.IsValid())
	{
		Arr.Items[SlotIdx].InstanceId.Invalidate();
		Arr.MarkItemDirty(Arr.Items[SlotIdx]);
	}
}

void URpg_ContainerComponent::AddRepSubObject(UObject* SubObject)
{
	if (IsUsingRegisteredSubObjectList() && IsReadyForReplication() && IsValid(SubObject))
	{
		AddReplicatedSubObject(SubObject);
	}
}

void URpg_ContainerComponent::ServerConsumeItem_Implementation(URpg_ItemComponent* ItemComponent, const int32 Quantity)
{
	if (!ItemComponent || !IsValid(ItemComponent) || !IsValid(ItemComponent->GetOwner())) return;
	// Route to unified world-use path
	ServerUseWorldItem(ItemComponent, Quantity);
}

bool URpg_ContainerComponent::InternalConsume(URpg_ItemComponent* ItemComponent, int32 const Quantity) const
{
	// Legacy path kept for compatibility; route into unified world use
	URpg_ContainerComponent* MutableThis = const_cast<URpg_ContainerComponent*>(this);
	return MutableThis->InternalUseItem_World(ItemComponent, Quantity);
}

void URpg_ContainerComponent::ServerUseItemByInstance_Implementation(int32 ContainerIndex, const FGuid& InstanceId, int32 Quantity)
{
	InternalUseItem_Inventory(ContainerIndex, InstanceId, Quantity);
}

void URpg_ContainerComponent::ServerUseWorldItem_Implementation(URpg_ItemComponent* ItemComponent, int32 Quantity)
{
	InternalUseItem_World(ItemComponent, Quantity);
}

static const FConsumableFragment* GetConsumable(const URpg_ItemDefinition* Def)
{
	return Def ? Def->GetFragmentOfTypeWithTag<FConsumableFragment>(FragmentTags::ConsumableFragment) : nullptr;
}

bool URpg_ContainerComponent::InternalUseItem_Inventory(int32 ContainerIndex, const FGuid& InstanceId, int32 Quantity)
{
	if (!(GetOwner() && GetOwner()->HasAuthority())) return false;
	if (Quantity <= 0) return false;
	if (!Containers.IsValidIndex(ContainerIndex)) return false;

	FInv_InventoryEntry* Entry = Containers[ContainerIndex].FindEntryMutableByInstance(InstanceId);
	if (!Entry) return false;
	URpg_ItemDefinition* Def = UInventoryStatics::GetItemDefinitionById(Entry->GetItemId());
	if (!Def) return false;
	const FConsumableFragment* Cons = GetConsumable(Def);
	if (!Cons) return false;
	
	if (!(Cons && Cons->AllowsContext(EUseContext::Inventory))) return false;

	// Preferred path: event-triggered ability if tag is set (editor/Blueprint-driven)
	if (Cons->UseEventTag.IsValid())
	{
		FGameplayEventData EventData;
		EventData.EventTag = Cons->UseEventTag;
		EventData.Instigator = GetOwner();
		EventData.EventMagnitude = Quantity;
		UObject* SourceObj = UItemUseSource_Inventory::Make(this, this, ContainerIndex, InstanceId);
		EventData.OptionalObject = SourceObj;
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(GetOwner(), Cons->UseEventTag, EventData);
		OnItemConsumed.Broadcast(nullptr, Quantity);
		return true;
	}
	return false;
}

bool URpg_ContainerComponent::InternalUseItem_World(URpg_ItemComponent* ItemComponent, int32 Quantity)
{
	if (!(GetOwner() && GetOwner()->HasAuthority())) return false;
	if (!ItemComponent || Quantity <= 0) return false;
	const URpg_ItemDefinition* Def = ItemComponent->GetItemDefinition();
	if (!Def) return false;
	const FConsumableFragment* Cons = GetConsumable(Def);
	if (!Cons) return false;
	
	if (!(Cons && Cons->AllowsContext(EUseContext::World))) return false;

	// Preferred: event-triggered ability when UseEventTag is configured
	if (Cons->UseEventTag.IsValid())
	{
		FGameplayEventData EventData;
		EventData.EventTag = Cons->UseEventTag;
		EventData.Instigator = GetOwner();
		EventData.Target = ItemComponent->GetOwner();
		EventData.EventMagnitude = Quantity;
		EventData.OptionalObject = ItemComponent; // world item context
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(GetOwner(), Cons->UseEventTag, EventData);
		OnItemConsumed.Broadcast(ItemComponent, Quantity);
		return true;
	}

	return false;
}

void URpg_ContainerComponent::OnRep_Slots()
{
	for (int32 i=0;i<Slots.Num();++i)
	{
		Slots[i].OwnerComponent = this;
		Slots[i].OwnerContainerIndex = i;
	}
}

void URpg_ContainerComponent::InitSlotsArrayFromContainers()
{
	Slots.SetNum(Containers.Num());

	for (int32 c = 0; c < Containers.Num(); ++c)
	{
		auto& SlotArray = Slots[c];
		SlotArray.OwnerComponent = this;
		SlotArray.OwnerContainerIndex = c;
		const int32 Total = Containers[c].Rows * Containers[c].Cols;

		const bool bSizeChanged = (SlotArray.Items.Num() != Total);
		if (bSizeChanged)
		{
			SlotArray.Init(Total);
			SlotArray.MarkArrayDirty();

			// Nur beim erstmaligen Anlegen: Initial-Layout 0..N-1
			const TArray<FInv_InventoryEntry>& Es = Containers[c].GetEntries();
			for (int32 s = 0; s < SlotArray.Items.Num(); ++s)
			{
				const FGuid NewId = (Es.IsValidIndex(s) ? Es[s].GetInstanceId() : FGuid());
				FInv_Slot& Slot = SlotArray.Items[s];
				if (Slot.InstanceId != NewId)
				{
					Slot.InstanceId = NewId;
					SlotArray.MarkItemDirty(Slot);
				}
			}
		}
		// Wenn Größe gleich bleibt: NICHT erneut aus Entries „überbügeln“
	}
}

FGuid URpg_ContainerComponent::GetSlotInstance(int32 ContainerIdx, int32 SlotIdx) const
{
	if (!Slots.IsValidIndex(ContainerIdx)) return FGuid();
	const auto& A = Slots[ContainerIdx].Items;
	return A.IsValidIndex(SlotIdx) ? A[SlotIdx].InstanceId : FGuid();
}

void URpg_ContainerComponent::SetSlotInstance(int32 ContainerIdx, int32 SlotIdx, const FGuid& InstanceId)
{
	if (!Slots.IsValidIndex(ContainerIdx)) return;
	auto& Arr = Slots[ContainerIdx];
	if (!Arr.Items.IsValidIndex(SlotIdx)) return;

	// 1) Vorherige Vorkommen der gleichen InstanceId in DIESEM Container leeren + broadcasten
	for (int32 i = 0; i < Arr.Items.Num(); ++i)
	{
		if (i == SlotIdx) continue;
		if (Arr.Items[i].InstanceId == InstanceId)
		{
			Arr.Items[i].InstanceId.Invalidate();
			Arr.MarkItemDirty(Arr.Items[i]);
		}
	}
	
	Arr.Items[SlotIdx].InstanceId = InstanceId;
	Arr.MarkItemDirty(Arr.Items[SlotIdx]);
}

void URpg_ContainerComponent::BeginPlay()
{
	Super::BeginPlay();

	if (!GetOwner() || !GetOwner()->HasAuthority()) return;

	Containers.Reset();
	for (const auto& SoftDef : InitialContainerDefs)
	{
		const UInventoryContainerDefinition* Def = SoftDef.IsValid() ? SoftDef.Get() : SoftDef.LoadSynchronous();
		if (!Def) continue;

		FInvContainer C(this);
		C.DisplayName  = Def->DisplayName;
		C.Type         = Def->Type;
		C.Rows         = Def->Rows;
		C.Cols         = Def->Cols;
		C.AllowedItems = Def->AllowedItems;
		C.TabIcon      = Def->TabIcon;
		Containers.Add(MoveTemp(C));
	}

	// NEU: Owner setzen + RuntimeData-Owner backfill + Slot-Arrays initialisieren
	for (int32 i = 0; i < Containers.Num(); ++i) { Containers[i].SetOwner(this); }
	EnsureEntryRuntimeOwners();       // kleine Helper-Funktion, siehe unten
	InitSlotsArrayFromContainers();   // jetzt auch auf dem SERVER aufrufen
}


void URpg_ContainerComponent::EnsureEntryRuntimeOwners()
{
	for (FInvContainer& C : Containers)
	{
		for (FInv_InventoryEntry& E : C.GetEntries())
		{
			if (E.GetRuntimeData().OwnerComponent != this)
			{
				E.SetRuntimeDataOwner(this);
			}
		}
	}
}

bool URpg_ContainerComponent::InternalSwapSlots(URpg_ContainerComponent* OtherComponent, int32 ThisContainerIndex,
	int32 ThisSlotIndex, int32 OtherContainerIndex, int32 OtherSlotIndex)
{
	if (!Containers.IsValidIndex(ThisContainerIndex)) return false;
    if (!OtherComponent->Containers.IsValidIndex(OtherContainerIndex)) return false;

    FInvContainer& ACont = Containers[ThisContainerIndex];
    FInvContainer& BCont = OtherComponent->Containers[OtherContainerIndex];

    // No-op: gleicher Slot
    if (OtherComponent == this &&
        ThisContainerIndex == OtherContainerIndex &&
        ThisSlotIndex == OtherSlotIndex) return false;
	

    // Aktuelle InstanceIds aus den Slots lesen
    const FGuid AId = GetSlotInstance(ThisContainerIndex, ThisSlotIndex);
    const FGuid BId = OtherComponent->GetSlotInstance(OtherContainerIndex, OtherSlotIndex);

    // Wenn beide leer -> nix zu tun
    if (!AId.IsValid() && !BId.IsValid()) return false;

    // Entry-Lookups (optional für Allowed/Events)
    auto FindEntry = [](FInvContainer& C, const FGuid& Id) -> FInv_InventoryEntry*
    {
        return Id.IsValid() ? C.FindEntryMutableByInstance(Id) : nullptr;
    };

    FInv_InventoryEntry* AEntry = FindEntry(ACont, AId);
    FInv_InventoryEntry* BEntry = FindEntry(BCont, BId);

    // Allowed-Checks (nur für die Seite, die tatsächlich bewegt wird)
    // Fall 1: beide belegt -> prüfen, ob A ins B darf und B ins A darf
    if (AId.IsValid() && BId.IsValid())
    {
        if (!BCont.IsItemAllowed(AEntry ? AEntry->GetItemType() : FGameplayTag::EmptyTag)) return false;
        if (!ACont.IsItemAllowed(BEntry ? BEntry->GetItemType() : FGameplayTag::EmptyTag)) return false;
    	
        SetSlotInstance(ThisContainerIndex, ThisSlotIndex, BId);
        OtherComponent->SetSlotInstance(OtherContainerIndex, OtherSlotIndex, AId);

        return true;
    }

    // Fall 2: nur A belegt -> move A -> B
    if (AId.IsValid() && !BId.IsValid())
    {
        if (!BCont.IsItemAllowed(AEntry ? AEntry->GetItemType() : FGameplayTag::EmptyTag)) return false;

        // Ziel-Slot bekommt A, Quell-Slot wird geleert
        OtherComponent->SetSlotInstance(OtherContainerIndex, OtherSlotIndex, AId);
        ClearSlot(ThisContainerIndex, ThisSlotIndex);
        return true;
    }

    // Fall 3: nur B belegt -> move B -> A
    if (!AId.IsValid() && BId.IsValid())
    {
        if (!ACont.IsItemAllowed(BEntry ? BEntry->GetItemType() : FGameplayTag::EmptyTag)) return false;

        SetSlotInstance(ThisContainerIndex, ThisSlotIndex, BId);
        OtherComponent->ClearSlot(OtherContainerIndex, OtherSlotIndex);
    	
        return true;
    }

    return false;
}

bool URpg_ContainerComponent::InternalAddItem(int32 ContainerIndex, URpg_ItemComponent* ItemComponent, int32 Quantity, int32& OutAdded, FGuid& OutInstanceId)
{
	OutAdded = 0;
	if (!GetOwner() || !GetOwner()->HasAuthority()) return false;
	if (!ItemComponent || Quantity <= 0) return false;
	if (!Containers.IsValidIndex(ContainerIndex)) return false;

	const URpg_ItemDefinition* Def = ItemComponent->GetItemDefinition();
	if (!Def) return false;

	FInvContainer& Cont = Containers[ContainerIndex];

	const FGameplayTag ItemType = Def->GetItemType();
	const FStackableFragment* Stackable = Def->GetFragmentOfType<FStackableFragment>();
	const int32 MaxStack = Stackable ? FMath::Max(1, Stackable->GetMaxStackSize()) : 1;

	int32 Added = 0;
	FGuid UsedInstance;
	const int32 EntryIndex = Cont.AddOrStack(
		Def->GetPrimaryAssetId(), ItemType, MaxStack,
		Quantity, UsedInstance, Added
	);

	OutAdded = Added;
	OutInstanceId = UsedInstance;

	// Erfolg, wenn wir entweder in einen vorhandenen Stack eingelagert
	// ODER einen neuen Stack (Entry) erzeugt haben.
	const bool bSuccess = (EntryIndex != INDEX_NONE) || (Added > 0);
	if (!bSuccess) return false;

	// Wenn ein NEUER Stack entstanden ist, RuntimeData kopieren und einen freien Slot zuweisen.
	if (EntryIndex != INDEX_NONE)
	{
		if (FInv_InventoryEntry* NewEntryPtr = Cont.FindEntryMutableByInstance(UsedInstance))
		{
			NewEntryPtr->CopyRuntimeDataFrom(ItemComponent->GetRuntimeData());
			// Stackzahl bleibt, FastArray-Delta übernimmt Replikation/Events
			NewEntryPtr->SetRuntimeDataOwner(this);
		}

		// Positionieren: ersten freien Slot wählen und InstanceId eintragen.
		EnsureSlotsInitializedFor(this, ContainerIndex);  
		const int32 TargetSlot = FindFirstFreeSlot(ContainerIndex);
		if (TargetSlot != INDEX_NONE)
		{
			SetSlotInstance(ContainerIndex, TargetSlot, UsedInstance);
		}
	}

	return true;
}

int32 URpg_ContainerComponent::FindFirstFreeSlot(int32 ContainerIdx) const
{
	if (!Slots.IsValidIndex(ContainerIdx)) return INDEX_NONE;
	const TArray<FInv_Slot>& S = Slots[ContainerIdx].Items;
	for (int32 i = 0; i < S.Num(); ++i)
	{
		if (!S[i].InstanceId.IsValid())
			return i;
	}
	return INDEX_NONE;
}

bool URpg_ContainerComponent::InternalRemoveItem(int32 ContainerIndex, const FGuid& InstanceId, int32 Quantity, int32& OutRemoved)
{
	OutRemoved = 0;
	if (!GetOwner() || !GetOwner()->HasAuthority()) return false;
	if (!Containers.IsValidIndex(ContainerIndex)) return false;
	FInvContainer& Cont = Containers[ContainerIndex];
	// Capture a copy before removal for UI update context
	FInv_InventoryEntry* BeforePtr = Cont.FindEntryMutableByInstance(InstanceId);
	FInv_InventoryEntry BeforeCopy;
	if (BeforePtr)
	{
		BeforeCopy = *BeforePtr;
	}
	const bool bRemoved = Cont.RemoveByInstance(InstanceId, Quantity, OutRemoved);
	
	return bRemoved;
}

bool URpg_ContainerComponent::InternalAddItemById(int32 ContainerIndex, const FPrimaryAssetId& ItemId, int32 Quantity, int32& OutAdded, FGuid& OutInstanceId)
{
	OutAdded = 0;
	if (!GetOwner() || !GetOwner()->HasAuthority()) return false;
	if (Quantity <= 0) return false;
	if (!Containers.IsValidIndex(ContainerIndex)) return false;
	
	URpg_ItemDefinition* Def = UInventoryStatics::GetItemDefinitionById(ItemId);

	if (!Def)
	{
		return false; // could not resolve definition from id
	}
	
 	FInvContainer& Cont = Containers[ContainerIndex];
	const FGameplayTag ItemType = Def->GetItemType();
	const FStackableFragment* Stackable = Def->GetFragmentOfType<FStackableFragment>();
	const int32 MaxStack = Stackable ? FMath::Max(1, Stackable->GetMaxStackSize()) : 1;

	int32 Added = 0;
	FGuid UsedInstance;
	const int32 LastIndex = Cont.AddOrStack(ItemId, ItemType, MaxStack, Quantity, UsedInstance, Added);
	OutAdded = Added;
	OutInstanceId = UsedInstance;
	return LastIndex != INDEX_NONE || Added > 0;
}

bool URpg_ContainerComponent::AddItemToContainerById(int32 ContainerIndex, FPrimaryAssetId ItemId, int32 Quantity, int32& OutAdded, FGuid& OutInstanceId)
{
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		return InternalAddItemById(ContainerIndex, ItemId, Quantity, OutAdded, OutInstanceId);
	}
	else
	{
		ServerAddItemToContainerById(ContainerIndex, ItemId, Quantity);
		OutAdded = 0; // will update via replication
		OutInstanceId.Invalidate();
		return false;
	}
}

bool URpg_ContainerComponent::InternalTransferItem(const FInventoryDragPayload& Payload,
    URpg_ContainerComponent* TargetComponent,
    int32 TargetContainerIndex,
    int32 TargetSlotIndex,
    int32& OutMoved)
{
	    OutMoved = 0;

    // --- Guards / Authority ---
    if (!GetOwner() || !GetOwner()->HasAuthority()) return false;
    if (!TargetComponent) return false;

    URpg_ContainerComponent* SourceComponent = Payload.SourceComponent.Get();
    if (!SourceComponent) return false;

    if (!SourceComponent->Containers.IsValidIndex(Payload.SourceContainerIndex)) return false;
    if (!TargetComponent->Containers.IsValidIndex(TargetContainerIndex)) return false;

    FInvContainer& Src = SourceComponent->Containers[Payload.SourceContainerIndex];
    FInvContainer& Dst = TargetComponent->Containers[TargetContainerIndex];

    const int32 SrcIdx = Src.FindIndexByInstance(Payload.InstanceId);
    if (SrcIdx == INDEX_NONE) return false;

    const FInv_InventoryEntry SrcEntrySnapshot = Src.GetEntries()[SrcIdx];

    // Menge
    int32 RequestedQty = (Payload.Quantity <= 0) ? SrcEntrySnapshot.GetStack() : Payload.Quantity;
    if (RequestedQty <= 0) return false;

    // Definition / MaxStack
    URpg_ItemDefinition* Def = UInventoryStatics::GetItemDefinitionById(SrcEntrySnapshot.GetItemId());
    if (!Def) return false;
    const FStackableFragment* Stackable = Def->GetFragmentOfType<FStackableFragment>();
    const int32 MaxStack = Stackable ? FMath::Max(1, Stackable->GetMaxStackSize()) : 1;

    // Zielslot + Regeln
    const FGuid TargetInst = TargetComponent->GetSlotInstance(TargetContainerIndex, TargetSlotIndex);
    const bool bSameComponent = (TargetComponent == SourceComponent);
    const bool bSameContainer = bSameComponent && (Payload.SourceContainerIndex == TargetContainerIndex);

    if (!Dst.IsItemAllowed(SrcEntrySnapshot.GetItemType())) return false;

    // ================= FALL A: gleicher Container =================
    if (bSameContainer)
    {
        // A1) Ziel belegt
        if (TargetInst.IsValid())
        {
            const int32 DstIdx = Dst.FindIndexByInstance(TargetInst);
            if (DstIdx == INDEX_NONE) return false;
            FInv_InventoryEntry* DstEntryPtr = Dst.FindEntryMutableByInstance(TargetInst);
            if (!DstEntryPtr) return false;

            // A1.1) Gleiches Item -> stacken
            if (DstEntryPtr->GetItemId() == SrcEntrySnapshot.GetItemId())
            {
                int32 Added = 0;
                if (!Dst.StackIntoIndex(DstIdx, MaxStack, RequestedQty, Added)) return false;
                if (Added <= 0) return false;

                int32 Removed = 0;
                Src.RemoveByInstance(Payload.InstanceId, Added, Removed);
                OutMoved = FMath::Min(Added, Removed);

                // Quelle leer? Slot leeren (gleiches Container-Grid)
                if (!Src.FindEntryMutableByInstance(Payload.InstanceId))
                {
                    SourceComponent->ClearSlot(Payload.SourceContainerIndex, Payload.SourceSlotIndex);
                }
                return OutMoved > 0;
            }

            // A1.2) Anderes Item -> Slot-SWAP (reines Mapping)
            const FGuid A = SourceComponent->GetSlotInstance(Payload.SourceContainerIndex, Payload.SourceSlotIndex);
            const FGuid B = TargetInst;
            SourceComponent->SetSlotInstance(Payload.SourceContainerIndex, Payload.SourceSlotIndex, B);
            TargetComponent->SetSlotInstance(TargetContainerIndex, TargetSlotIndex, A);
            OutMoved = 0;
            return true;
        }

        // A2) Ziel leer
        // Volle Menge -> Instanz umhängen
        if (RequestedQty >= SrcEntrySnapshot.GetStack())
        {
            TargetComponent->SetSlotInstance(TargetContainerIndex, TargetSlotIndex, Payload.InstanceId);
            SourceComponent->ClearSlot(Payload.SourceContainerIndex, Payload.SourceSlotIndex);
            OutMoved = SrcEntrySnapshot.GetStack();
            return true;
        }

        // Teilmenge -> Split in neue Instanz
        FGuid NewInst;
        if (!Src.SplitIntoNewEntry(Payload.InstanceId, RequestedQty, NewInst)) return false;
        TargetComponent->SetSlotInstance(TargetContainerIndex, TargetSlotIndex, NewInst);
        OutMoved = RequestedQty;
        return true;
    }

    // ================= FALL B: Cross-Container / anderes Component =================
    {
        // B1) Ziel belegt
        if (TargetInst.IsValid())
        {
            const int32 DstIdx = Dst.FindIndexByInstance(TargetInst);
            if (DstIdx == INDEX_NONE) return false;
            FInv_InventoryEntry* DstEntryPtr = Dst.FindEntryMutableByInstance(TargetInst);
            if (!DstEntryPtr) return false;

            // Nur stacken, wenn identisches Item
            if (DstEntryPtr->GetItemId() == SrcEntrySnapshot.GetItemId())
            {
                int32 Added = 0;
                if (!Dst.StackIntoIndex(DstIdx, MaxStack, RequestedQty, Added)) return false;
                if (Added <= 0) return false;

                int32 Removed = 0;
                Src.RemoveByInstance(Payload.InstanceId, Added, Removed);
                OutMoved = FMath::Min(Added, Removed);

                // WICHTIG: Quelle leer? Immer Slot leeren (auch Cross-Component)
                if (!Src.FindEntryMutableByInstance(Payload.InstanceId))
                {
                    SourceComponent->ClearSlot(Payload.SourceContainerIndex, Payload.SourceSlotIndex);
                }
                return OutMoved > 0;
            }

            // Kein Cross-Container-Swap
            return false;
        }

        // B2) Ziel leer -> neuen Stack erzeugen
        FGuid NewInst;
        const int32 ToCreate = RequestedQty; // exakt diese Menge
        const int32 NewIdx = Dst.AddNewStackExact(SrcEntrySnapshot.GetItemId(), SrcEntrySnapshot.GetItemType(), ToCreate, NewInst);
        if (NewIdx == INDEX_NONE) return false;

        TargetComponent->SetSlotInstance(TargetContainerIndex, TargetSlotIndex, NewInst);

        int32 Removed = 0;
        Src.RemoveByInstance(Payload.InstanceId, ToCreate, Removed);
        OutMoved = Removed;

        // Quelle leer? Immer Slot leeren (auch Cross-Component)
        if (!Src.FindEntryMutableByInstance(Payload.InstanceId))
        {
            SourceComponent->ClearSlot(Payload.SourceContainerIndex, Payload.SourceSlotIndex);
        }

        return OutMoved > 0;
    }
}

bool URpg_ContainerComponent::AddItemToContainer(int32 ContainerIndex, URpg_ItemComponent* ItemComponent, int32 Quantity, int32& OutAdded, FGuid& OutInstanceId)
{
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		return InternalAddItem(ContainerIndex, ItemComponent, Quantity, OutAdded, OutInstanceId);
	}
	else
	{
		ServerAddItemToContainer(ContainerIndex, ItemComponent, Quantity);
		OutAdded = 0; // will update via replication
		OutInstanceId.Invalidate();
		return false;
	}
}

bool URpg_ContainerComponent::RemoveItemFromContainer(int32 ContainerIndex, const FGuid& InstanceId, int32 Quantity, int32& OutRemoved)
{
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		return InternalRemoveItem(ContainerIndex, InstanceId, Quantity, OutRemoved);
	}
	else
	{
		ServerRemoveItemFromContainer(ContainerIndex, InstanceId, Quantity);
		OutRemoved = 0;
		return false;
	}
}

bool URpg_ContainerComponent::TransferItem(const FInventoryDragPayload& Payload,
	URpg_ContainerComponent* TargetComponent,
	int32 TargetContainerIndex,																	
	int32 TargetSlotIndex,
	int32& OutMoved)
{
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		return InternalTransferItem(Payload, TargetComponent, TargetContainerIndex, TargetSlotIndex, OutMoved);
	}
	
	ServerTransferItem(Payload, TargetComponent, TargetContainerIndex, TargetSlotIndex);
	return false;
}

bool URpg_ContainerComponent::GetEntryAtIndex(int32 ContainerIndex, int32 EntryIndex, FInv_InventoryEntry& OutEntry) const
{
	OutEntry = FInv_InventoryEntry();

	if (!Containers.IsValidIndex(ContainerIndex)) return false;

	const FInvContainer& C = Containers[ContainerIndex];
	
	const FInv_InventoryEntry* Ptr = GetEntryBySlot(ContainerIndex, EntryIndex);
	if (!Ptr) return false;             // <- leerer Slot: sauber false zurückgeben

	OutEntry = *Ptr;
	return true;
}

bool URpg_ContainerComponent::FindIndexByInstance(int32 ContainerIndex, const FGuid& InstanceId, int32& OutEntryIndex) const
{
	OutEntryIndex = INDEX_NONE;
	if (!Containers.IsValidIndex(ContainerIndex)) return false;
	OutEntryIndex = Containers[ContainerIndex].FindIndexByInstance(InstanceId);
	return OutEntryIndex != INDEX_NONE;
}

bool URpg_ContainerComponent::TransferFromPayloadTo(const FInventoryDragPayload& Payload,
	URpg_ContainerComponent* TargetComponent,
	int32 TargetContainerIndex,
	int32 TargetSlotIndex,
	int32& OutMoved)
{
	OutMoved = 0;
	URpg_ContainerComponent* SrcComp = Payload.SourceComponent.Get();
	if (!SrcComp) return false;
	
	return SrcComp->TransferItem(Payload, TargetComponent, TargetContainerIndex, TargetSlotIndex, OutMoved);
}

bool URpg_ContainerComponent::SwapSlots(URpg_ContainerComponent* OtherComponent, int32 ThisContainerIndex, int32 ThisSlotIndex, int32 OtherContainerIndex, int32 OtherSlotIndex)
{
	if (!OtherComponent) return false;

	const bool bHasAuthority = GetOwner() && GetOwner()->HasAuthority();
	if (bHasAuthority) {
		return InternalSwapSlots(OtherComponent, ThisContainerIndex, ThisSlotIndex, OtherContainerIndex, OtherSlotIndex);
	}
	
	// RPC
	ServerSwapSlots(OtherComponent, ThisContainerIndex, ThisSlotIndex, OtherContainerIndex, OtherSlotIndex);
	return true;
}

void URpg_ContainerComponent::ServerSwapSlots_Implementation(URpg_ContainerComponent* OtherComponent, int32 ThisContainerIndex, int32 ThisSlotIndex, int32 OtherContainerIndex, int32 OtherSlotIndex)
{
	InternalSwapSlots(OtherComponent, ThisContainerIndex, ThisSlotIndex, OtherContainerIndex, OtherSlotIndex);
}

bool URpg_ContainerComponent::AutoDepositMatchingTo(URpg_ContainerComponent* TargetComponent, int32 TargetContainerIndex, int32& OutTotalMoved)
{
	OutTotalMoved = 0;

	// Route to server if not authoritative
	if (!(GetOwner() && GetOwner()->HasAuthority()))
	{
		ServerAutoDepositMatchingTo(TargetComponent, TargetContainerIndex);
		return false; // UI will update via replication callbacks
	}

	if (!TargetComponent) return false;
	if (!TargetComponent->Containers.IsValidIndex(TargetContainerIndex)) return false;

	FInvContainer& Dst = TargetComponent->Containers[TargetContainerIndex];

	// 1) Collect item ids that already exist in destination
	TSet<FPrimaryAssetId> ExistingIds;
	for (const FInv_InventoryEntry& E : Dst.GetEntries())
	{
		ExistingIds.Add(E.GetItemId());
	}
	if (ExistingIds.Num() == 0)
	{
		// Nothing in destination — don't introduce new types
		return false;
	}

	// 2) Iterate all our source containers
	for (int32 SrcIdx = 0; SrcIdx < Containers.Num(); ++SrcIdx)
	{
		if (!Containers.IsValidIndex(SrcIdx)) continue;
		FInvContainer& Src = Containers[SrcIdx];

		// Optional rule: within same component block moves between identical types
		if (TargetComponent == this && Src.Type == Dst.Type)
		{
			continue;
		}

		// Copy candidate instance ids up-front to avoid iterator invalidation on remove
		TArray<FGuid> CandidateInstances;
		CandidateInstances.Reserve(Src.GetEntries().Num());
		for (const FInv_InventoryEntry& E : Src.GetEntries())
		{
			if (ExistingIds.Contains(E.GetItemId()))
			{
				CandidateInstances.Add(E.GetInstanceId());
			}
		}

		for (const FGuid& InstanceId : CandidateInstances)
		{
			const int32 Index = Src.FindIndexByInstance(InstanceId);
			if (Index == INDEX_NONE) continue;
			const FInv_InventoryEntry SrcEntry = Src.GetEntries()[Index];

			// Respect destination allowed items
			if (!Dst.IsItemAllowed(SrcEntry.GetItemType())) continue;

			// Resolve MaxStack from definition
			URpg_ItemDefinition* Def = UInventoryStatics::GetItemDefinitionById(SrcEntry.GetItemId());
			if (!Def) continue;
			const FStackableFragment* Stackable = Def->GetFragmentOfType<FStackableFragment>();
			const int32 MaxStack = Stackable ? FMath::Max(1, Stackable->GetMaxStackSize()) : 1;

			int32 Remaining = SrcEntry.GetStack();
			if (Remaining <= 0) continue;

			FGuid NewInstanceId = InstanceId;
			int32 Added = 0;
			Dst.AddOrStack(SrcEntry.GetItemId(), SrcEntry.GetItemType(), MaxStack, Remaining, NewInstanceId, Added);
			if (Added <= 0) continue;

			int32 Removed = 0;
			Src.RemoveByInstance(InstanceId, Added, Removed);
			OutTotalMoved += FMath::Min(Added, Removed);
		}
	}

	return OutTotalMoved > 0;
}

void URpg_ContainerComponent::PreNetReceive()
{
	Super::PreNetReceive();
	// Sorgt dafür, dass Slot-FastArray-Callbacks auf dem Client
	// bereits gültige Owner/Indices haben, BEVOR die Deltas angewendet werden.
	for (int32 i = 0; i < Slots.Num(); ++i)
	{
		Slots[i].OwnerComponent     = this;
		Slots[i].OwnerContainerIndex = i;
	}

	// Gleiches für die Item-Container, falls du da Owner brauchst
	for (int32 i = 0; i < Containers.Num(); ++i)
	{
		Containers[i].SetOwner(this);
	}
	
}

void URpg_ContainerComponent::ServerAddItemToContainer_Implementation(int32 ContainerIndex, URpg_ItemComponent* ItemComponent, int32 Quantity)
{
	int32 DummyAdded; FGuid DummyId; InternalAddItem(ContainerIndex, ItemComponent, Quantity, DummyAdded, DummyId);
}

void URpg_ContainerComponent::ServerRemoveItemFromContainer_Implementation(int32 ContainerIndex, const FGuid& InstanceId, int32 Quantity)
{
	int32 DummyRemoved; InternalRemoveItem(ContainerIndex, InstanceId, Quantity, DummyRemoved);
}

void URpg_ContainerComponent::ServerAddItemToContainerById_Implementation(int32 ContainerIndex, FPrimaryAssetId ItemId, int32 Quantity)
{
	int32 DummyAdded; FGuid DummyId; InternalAddItemById(ContainerIndex, ItemId, Quantity, DummyAdded, DummyId);
}

void URpg_ContainerComponent::ServerTransferItem_Implementation(const FInventoryDragPayload& Payload,
	URpg_ContainerComponent* TargetComponent,
	int32 TargetContainerIndex,
	int32 TargetSlotIndex)
{
	FInventoryDragPayload ServerPayload = Payload;
	ServerPayload.SourceComponent = this; // <— WICHTIG
	int32 Dummy; InternalTransferItem(ServerPayload, TargetComponent, TargetContainerIndex, TargetSlotIndex, Dummy);
}

void URpg_ContainerComponent::ServerAutoDepositMatchingTo_Implementation(URpg_ContainerComponent* TargetComponent, int32 TargetContainerIndex)
{
	int32 DummyMoved = 0;
	AutoDepositMatchingTo(TargetComponent, TargetContainerIndex, DummyMoved);
}

