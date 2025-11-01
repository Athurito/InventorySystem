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

URpg_ContainerComponent::URpg_ContainerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void URpg_ContainerComponent::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(URpg_ContainerComponent, Containers);
}

void URpg_ContainerComponent::OnRep_Containers()
{
	// Owner setzen, dann Mapping für jeden Container „versöhnen“
	for (int32 i = 0; i < Containers.Num(); ++i)
	{
		Containers[i].SetOwner(this);

		// Mapping-Größe herstellen
		const int32 total = Containers[i].Rows * Containers[i].Cols;
		EnsureSlotMapSize(i, total);

		// 1) Alle derzeit gemappten InstanceIds einsammeln
		TSet<FGuid> mapped;
		FContainerSlotMap& map = ContainerSlotMaps.FindOrAdd(i);
		for (const FGuid& id : map.SlotToInstance)
			if (id.IsValid()) mapped.Add(id);

		// 2) Nicht mehr existierende IDs aus Mapping entfernen
		TSet<FGuid> existing;
		for (const FInv_InventoryEntry& e : Containers[i].GetEntries())
			existing.Add(e.GetInstanceId());

		for (FGuid& id : map.SlotToInstance)
			if (id.IsValid() && !existing.Contains(id))
				id.Invalidate();

		// 3) Un-gemappte Entries auf freie Slots legen
		for (const FInv_InventoryEntry& e : Containers[i].GetEntries())
		{
			if (map.SlotToInstance.Contains(e.GetInstanceId())) continue; // schon gemappt

			// freien Slot suchen
			for (int32 s = 0; s < map.SlotToInstance.Num(); ++s)
			{
				if (!map.SlotToInstance[s].IsValid())
				{
					AssignInstanceToSlotUnique(i, s, e.GetInstanceId()); // sorgt für Eindeutigkeit
					break;
				}
			}
		}
	}

	// optional: UI-Refresh anstoßen
	FInv_InventoryEntry dummy;
	OnItemAdded.Broadcast(dummy);
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
	const int32 Total = C.Rows * C.Cols;
	const_cast<URpg_ContainerComponent*>(this)->EnsureSlotMapSize(ContainerIdx, Total);

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

void URpg_ContainerComponent::EnsureSlotMapSize(int32 ContainerIdx, int32 TotalSlots)
{
	FContainerSlotMap& Map = ContainerSlotMaps.FindOrAdd(ContainerIdx);
	if (Map.SlotToInstance.Num() != TotalSlots) {
		Map.SlotToInstance.SetNum(TotalSlots);
		// Optional: initial befüllen – z. B. erste N Entries auf Slots 0..N-1
		if (Containers.IsValidIndex(ContainerIdx)) {
			const auto& Entries = Containers[ContainerIdx].GetEntries();
			const int32 Count = FMath::Min(TotalSlots, Entries.Num());
			for (int32 i=0; i<Count; ++i) {
				Map.SlotToInstance[i] = Entries[i].GetInstanceId();
			}
		}
	}
}

void URpg_ContainerComponent::SetSlotInstance(int32 ContainerIdx, int32 SlotIdx, const FGuid& InstanceId)
{
	if (!Containers.IsValidIndex(ContainerIdx)) return;
	const int32 Total = Containers[ContainerIdx].Rows * Containers[ContainerIdx].Cols;
	EnsureSlotMapSize(ContainerIdx, Total);
	FContainerSlotMap& Map = ContainerSlotMaps.FindOrAdd(ContainerIdx);
	if (Map.SlotToInstance.IsValidIndex(SlotIdx)) {
		Map.SlotToInstance[SlotIdx] = InstanceId;
	}
}

void URpg_ContainerComponent::ClearSlot(int32 ContainerIdx, int32 SlotIdx)
{
	if (FContainerSlotMap* Map = ContainerSlotMaps.Find(ContainerIdx)) {
		if (Map->SlotToInstance.IsValidIndex(SlotIdx)) {
			Map->SlotToInstance[SlotIdx] = FGuid(); // leer
		}
	}
}

FGuid URpg_ContainerComponent::GetSlotInstance(int32 ContainerIdx, int32 SlotIdx) const
{
	if (const FContainerSlotMap* Map = ContainerSlotMaps.Find(ContainerIdx)) {
		if (Map->SlotToInstance.IsValidIndex(SlotIdx)) {
			return Map->SlotToInstance[SlotIdx];
		}
	}
	return FGuid();
}

void URpg_ContainerComponent::AssignInstanceToSlotUnique(int32 ContainerIdx, int32 SlotIdx, const FGuid& InstanceId)
{
	if (!Containers.IsValidIndex(ContainerIdx)) return;
	const int32 Total = Containers[ContainerIdx].Rows * Containers[ContainerIdx].Cols;
	EnsureSlotMapSize(ContainerIdx, Total);

	FContainerSlotMap& Map = ContainerSlotMaps.FindOrAdd(ContainerIdx);
	// gleiche InstanceId aus allen anderen Slots entfernen
	for (int32 i = 0; i < Map.SlotToInstance.Num(); ++i)
		if (i != SlotIdx && Map.SlotToInstance[i] == InstanceId)
			Map.SlotToInstance[i] = FGuid();

	Map.SlotToInstance[SlotIdx] = InstanceId;
}

void URpg_ContainerComponent::ReconcileMappingFromEntries(int32 ContainerIdx)
{
	if (!Containers.IsValidIndex(ContainerIdx)) return;
	const FInvContainer& C = Containers[ContainerIdx];
	const int32 Total = C.Rows * C.Cols;
	EnsureSlotMapSize(ContainerIdx, Total);

	// markiere belegte Instanzen
	TSet<FGuid> AlreadyMapped;
	for (int32 s=0; s<ContainerSlotMaps[ContainerIdx].SlotToInstance.Num(); ++s) {
		const FGuid Id = ContainerSlotMaps[ContainerIdx].SlotToInstance[s];
		if (Id.IsValid()) AlreadyMapped.Add(Id);
	}

	// lege nicht gemappte Entries auf erste freien Slots
	for (const FInv_InventoryEntry& E : C.GetEntries()) {
		if (AlreadyMapped.Contains(E.GetInstanceId())) continue;
		// suche freien Slot
		for (int32 s=0; s<Total; ++s) {
			if (!ContainerSlotMaps[ContainerIdx].SlotToInstance[s].IsValid()) {
				AssignInstanceToSlotUnique(ContainerIdx, s, E.GetInstanceId());
				AlreadyMapped.Add(E.GetInstanceId());
				break;
			}
		}
	}
}

void URpg_ContainerComponent::ClearMappingForInstance(int32 ContainerIdx, const FGuid& InstanceId)
{
	if (FContainerSlotMap* Map = ContainerSlotMaps.Find(ContainerIdx)) {
		for (FGuid& Id : Map->SlotToInstance)
			if (Id == InstanceId) { Id = FGuid(); }
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

    // Slot-Mappings absichern
    const int32 ATotal = ACont.Rows * ACont.Cols;
    const int32 BTotal = BCont.Rows * BCont.Cols;
    EnsureSlotMapSize(ThisContainerIndex, ATotal);
    OtherComponent->EnsureSlotMapSize(OtherContainerIndex, BTotal);

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

        // Reines Mapping-Swap
        AssignInstanceToSlotUnique(ThisContainerIndex, ThisSlotIndex, BId);
        OtherComponent->AssignInstanceToSlotUnique(OtherContainerIndex, OtherSlotIndex, AId);

        // UI-Events für beide Seiten
        if (AEntry) { OnItemRemoved.Broadcast(*AEntry); OnItemAdded.Broadcast(*AEntry); }
        if (BEntry) { OtherComponent->OnItemRemoved.Broadcast(*BEntry); OtherComponent->OnItemAdded.Broadcast(*BEntry); }

        return true;
    }

    // Fall 2: nur A belegt -> move A -> B
    if (AId.IsValid() && !BId.IsValid())
    {
        if (!BCont.IsItemAllowed(AEntry ? AEntry->GetItemType() : FGameplayTag::EmptyTag)) return false;

        // Ziel-Slot bekommt A, Quell-Slot wird geleert
        OtherComponent->AssignInstanceToSlotUnique(OtherContainerIndex, OtherSlotIndex, AId);
        ClearSlot(ThisContainerIndex, ThisSlotIndex);

        // UI-Events
        if (AEntry)
        {
            // „aus A entfernt / in B hinzugefügt“ – für unmittelbares UI-Update
            OnItemRemoved.Broadcast(*AEntry);
            OtherComponent->OnItemAdded.Broadcast(*AEntry);
        }
        return true;
    }

    // Fall 3: nur B belegt -> move B -> A
    if (!AId.IsValid() && BId.IsValid())
    {
        if (!ACont.IsItemAllowed(BEntry ? BEntry->GetItemType() : FGameplayTag::EmptyTag)) return false;

        AssignInstanceToSlotUnique(ThisContainerIndex, ThisSlotIndex, BId);
        OtherComponent->ClearSlot(OtherContainerIndex, OtherSlotIndex);

        if (BEntry)
        {
            OtherComponent->OnItemRemoved.Broadcast(*BEntry);
            OnItemAdded.Broadcast(*BEntry);
        }
        return true;
    }

    return false;
}

bool URpg_ContainerComponent::ApplyLocalMappingForTransfer(const FInventoryDragPayload& Payload,
	URpg_ContainerComponent* TargetComponent, int32 TargetContainerIndex, int32 TargetSlotIndex)
{
	URpg_ContainerComponent* SourceComponent = Payload.SourceComponent.Get();
	if (!SourceComponent || !TargetComponent) return false;

	if (!SourceComponent->Containers.IsValidIndex(Payload.SourceContainerIndex)) return false;
	if (!TargetComponent->Containers.IsValidIndex(TargetContainerIndex)) return false;

	FInvContainer& Src = SourceComponent->Containers[Payload.SourceContainerIndex];
	FInvContainer& Dst = TargetComponent->Containers[TargetContainerIndex];

	SourceComponent->EnsureSlotMapSize(Payload.SourceContainerIndex, Src.Rows * Src.Cols);
	TargetComponent->EnsureSlotMapSize(TargetContainerIndex, Dst.Rows * Dst.Cols);

	const bool bSameComp = (TargetComponent == SourceComponent);
	const bool bSameCont = bSameComp && (Payload.SourceContainerIndex == TargetContainerIndex);

	const FInv_InventoryEntry* SrcEntry = SourceComponent->GetEntryBySlot(Payload.SourceContainerIndex, Payload.SourceSlotIndex);
	if (!SrcEntry) return false;

	const int32 SrcCount = SrcEntry->GetStack();
	const int32 RequestedQty = (Payload.Quantity <= 0) ? SrcCount : FMath::Min(Payload.Quantity, SrcCount);

	// Nur: Same-Container, Target leer, voller Stack -> reines Umhängen
	const FGuid TargetInst = TargetComponent->GetSlotInstance(TargetContainerIndex, TargetSlotIndex);
	if (bSameCont && !TargetInst.IsValid() && RequestedQty >= SrcCount)
	{
		TargetComponent->AssignInstanceToSlotUnique(TargetContainerIndex, TargetSlotIndex, Payload.InstanceId);
		SourceComponent->ClearSlot(Payload.SourceContainerIndex, Payload.SourceSlotIndex);
		return true;
	}

	// alle anderen Fälle: keine Mapping-Änderung (Server entscheidet/erzeugt neue IDs)
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
	const int32 LastIndex = Cont.AddOrStack(Def->GetPrimaryAssetId(), ItemType, MaxStack, Quantity, UsedInstance, Added);
	OutAdded = Added;
	OutInstanceId = UsedInstance;

	FInv_InventoryEntry* NewEntryPtr = nullptr;
	// If a new stack was created, copy runtime data from the world item component into the new entry
	if (LastIndex != INDEX_NONE)
	{
		NewEntryPtr = Cont.FindEntryMutableByInstance(UsedInstance);
		if (NewEntryPtr)
		{
			NewEntryPtr->CopyRuntimeDataFrom(ItemComponent->GetRuntimeData());
			// Ensure the stack count matches what the container decided for the new stack
			NewEntryPtr->SetStack(NewEntryPtr->GetStack());
		}
	}
	const bool bSuccess = (LastIndex != INDEX_NONE) || (Added > 0);
	if (bSuccess)
	{
		// Broadcast a change (new entry if we have it, otherwise a minimal entry with instance id)
		if (NewEntryPtr)
		{
			OnItemAdded.Broadcast(*NewEntryPtr);
		}
		else
		{
			FInv_InventoryEntry Tmp; Tmp.SetInstanceId(UsedInstance); Tmp.SetItemId(Def->GetPrimaryAssetId());
			OnItemAdded.Broadcast(Tmp);
		}
	}
	return bSuccess;
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
	if (bRemoved && OutRemoved > 0)
	{
		// Prefer to broadcast the remaining entry if it still exists; otherwise broadcast the before copy
		FInv_InventoryEntry* AfterPtr = Cont.FindEntryMutableByInstance(InstanceId);
		if (AfterPtr)
		{
			OnItemRemoved.Broadcast(*AfterPtr);
		}
		else if (BeforePtr)
		{
			OnItemRemoved.Broadcast(BeforeCopy);
		}
		else
		{
			FInv_InventoryEntry Tmp; Tmp.SetInstanceId(InstanceId);
			OnItemRemoved.Broadcast(Tmp);
		}
	}
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

    // --- Basic guards / authority ---
    if (!GetOwner() || !GetOwner()->HasAuthority()) return false;
    if (!TargetComponent) return false;

    URpg_ContainerComponent* SourceComponent = Payload.SourceComponent.Get();
    if (!SourceComponent) return false;

    if (!SourceComponent->Containers.IsValidIndex(Payload.SourceContainerIndex)) return false;
    if (!TargetComponent->Containers.IsValidIndex(TargetContainerIndex)) return false;

    FInvContainer& Src = SourceComponent->Containers[Payload.SourceContainerIndex];
    FInvContainer& Dst = TargetComponent->Containers[TargetContainerIndex];

    // Slot-Mappings vorbereiten
    const int32 SrcTotal = Src.Rows * Src.Cols;
    const int32 DstTotal = Dst.Rows * Dst.Cols;
    SourceComponent->EnsureSlotMapSize(Payload.SourceContainerIndex, SrcTotal);
    TargetComponent->EnsureSlotMapSize(TargetContainerIndex, DstTotal);

    // Quelle lokalisieren
    const int32 SrcIdx = Src.FindIndexByInstance(Payload.InstanceId);
    if (SrcIdx == INDEX_NONE) return false;

    FInv_InventoryEntry SrcEntry = Src.GetEntries()[SrcIdx]; // Snapshot für Events

    // Menge bestimmen: 0/negativ = alles
    int32 RequestedQty = Payload.Quantity;
    if (RequestedQty <= 0) {
        RequestedQty = SrcEntry.GetStack();
    }
    if (RequestedQty <= 0) return false;

    // Definition / MaxStack
    URpg_ItemDefinition* Def = UInventoryStatics::GetItemDefinitionById(SrcEntry.GetItemId());
    if (!Def) return false;

    const FStackableFragment* Stackable = Def->GetFragmentOfType<FStackableFragment>();
    const int32 MaxStack = Stackable ? FMath::Max(1, Stackable->GetMaxStackSize()) : 1;

    // Zielslot-Instance ermitteln
    const FGuid TargetInst = TargetComponent->GetSlotInstance(TargetContainerIndex, TargetSlotIndex);

    const bool bSameComponent = (TargetComponent == SourceComponent);
    const bool bSameContainer = bSameComponent && (Payload.SourceContainerIndex == TargetContainerIndex);

    // Destination akzeptiert Item?
    if (!Dst.IsItemAllowed(SrcEntry.GetItemType())) return false;

    // ====== FALL A: Gleicher Container (Grid-internes Drag&Drop) ======
    if (bSameContainer)
    {
        // 1) Zielslot belegt?
        if (TargetInst.IsValid())
        {
            const int32 DstIdx = Dst.FindIndexByInstance(TargetInst);
            if (DstIdx == INDEX_NONE) return false;

            FInv_InventoryEntry* DstEntryPtr = Dst.FindEntryMutableByInstance(TargetInst);
            if (!DstEntryPtr) return false;

            // 1.1) Gleiches Item -> gezielt in diesen Slot stacken
            if (DstEntryPtr->GetItemId() == SrcEntry.GetItemId())
            {
                int32 Added = 0;
                if (!Dst.StackIntoIndex(DstIdx, MaxStack, RequestedQty, Added)) return false;
                if (Added <= 0) return false;

                int32 Removed = 0;
                Src.RemoveByInstance(Payload.InstanceId, Added, Removed);
                OutMoved = FMath::Min(Added, Removed);

                // Quelle leer? SourceSlot Mapping leeren
                if (!Src.FindEntryMutableByInstance(Payload.InstanceId))
                {
                    SourceComponent->ClearSlot(Payload.SourceContainerIndex, Payload.SourceSlotIndex);
                }

                // Events
                TargetComponent->OnItemAdded.Broadcast(*DstEntryPtr);
                if (FInv_InventoryEntry* RemainingPtr = Src.FindEntryMutableByInstance(Payload.InstanceId))
                    SourceComponent->OnItemRemoved.Broadcast(*RemainingPtr);
                else
                    SourceComponent->OnItemRemoved.Broadcast(SrcEntry);

                return OutMoved > 0;
            }
            else
            {
                // 1.2) Anderes Item -> Slot-SWAP (nur Mapping!)
                const FGuid A = SourceComponent->GetSlotInstance(Payload.SourceContainerIndex, Payload.SourceSlotIndex);
                const FGuid B = TargetInst;

                SourceComponent->AssignInstanceToSlotUnique(Payload.SourceContainerIndex, Payload.SourceSlotIndex, B);
                TargetComponent->AssignInstanceToSlotUnique(TargetContainerIndex, TargetSlotIndex, A);

                // Optional: Sofortige UI-Signale
                if (FInv_InventoryEntry* AE = Src.FindEntryMutableByInstance(B))
                    SourceComponent->OnItemRemoved.Broadcast(*AE); // moved innerhalb des Grids
                if (FInv_InventoryEntry* BE = Dst.FindEntryMutableByInstance(A))
                    TargetComponent->OnItemAdded.Broadcast(*BE);

                OutMoved = 0; // kein Mengen-Transfer
                return true;
            }
        }
        else
        {
        	// Ganze Menge? -> Instanz nur umhängen
        	if (RequestedQty >= SrcEntry.GetStack())
        	{
        		TargetComponent->AssignInstanceToSlotUnique(TargetContainerIndex, TargetSlotIndex, Payload.InstanceId);
        		SourceComponent->ClearSlot(Payload.SourceContainerIndex, Payload.SourceSlotIndex);
        		OutMoved = SrcEntry.GetStack();
        		// (optional) Events
        		if (FInv_InventoryEntry* MovedPtr = Src.FindEntryMutableByInstance(Payload.InstanceId))
        			TargetComponent->OnItemAdded.Broadcast(*MovedPtr);
        		SourceComponent->OnItemRemoved.Broadcast(SrcEntry);
        		return true;
        	}

        	// Teilmenge -> Stack splitten
        	FGuid NewInst;
        	if (!Src.SplitIntoNewEntry(Payload.InstanceId, RequestedQty, NewInst)) return false;
        	TargetComponent->AssignInstanceToSlotUnique(TargetContainerIndex, TargetSlotIndex, NewInst);
        	OutMoved = RequestedQty;

        	// Events
        	if (FInv_InventoryEntry* AddedPtr = Src.FindEntryMutableByInstance(NewInst))
        		TargetComponent->OnItemAdded.Broadcast(*AddedPtr);
        	if (FInv_InventoryEntry* RemainingPtr = Src.FindEntryMutableByInstance(Payload.InstanceId))
        		SourceComponent->OnItemRemoved.Broadcast(*RemainingPtr);
        	else
        		SourceComponent->OnItemRemoved.Broadcast(SrcEntry);

        	return true;
        }
    }

    // ====== FALL B: Cross-Container / anderes Component ======
    {
        // Ziel belegt?
        if (TargetInst.IsValid())
        {
            const int32 DstIdx = Dst.FindIndexByInstance(TargetInst);
            if (DstIdx == INDEX_NONE) return false;
            FInv_InventoryEntry* DstEntryPtr = Dst.FindEntryMutableByInstance(TargetInst);
            if (!DstEntryPtr) return false;

            // Nur stacken, wenn Item identisch
            if (DstEntryPtr->GetItemId() == SrcEntry.GetItemId())
            {
                int32 Added = 0;
                if (!Dst.StackIntoIndex(DstIdx, MaxStack, RequestedQty, Added)) return false;
                if (Added <= 0) return false;

                int32 Removed = 0;
                Src.RemoveByInstance(Payload.InstanceId, Added, Removed);
                OutMoved = FMath::Min(Added, Removed);

                // Quelle leer? (nur wenn Quelle ein gemapptes Grid ist)
                if (bSameComponent && !Src.FindEntryMutableByInstance(Payload.InstanceId))
                {
                    SourceComponent->ClearSlot(Payload.SourceContainerIndex, Payload.SourceSlotIndex);
                }

                TargetComponent->OnItemAdded.Broadcast(*DstEntryPtr);
                if (FInv_InventoryEntry* RemainingPtr = Src.FindEntryMutableByInstance(Payload.InstanceId))
                    SourceComponent->OnItemRemoved.Broadcast(*RemainingPtr);
                else
                    SourceComponent->OnItemRemoved.Broadcast(SrcEntry);

                return OutMoved > 0;
            }
            else
            {
                // Kein Swap über Container-Grenzen (Design-Entscheidung) – abbrechen
                return false;
            }
        }
        else
        {
        	FGuid NewInst;
        	const int32 ToCreate = RequestedQty; // exakt diese Menge als neuer Stack
        	const int32 NewIdx = Dst.AddNewStackExact(SrcEntry.GetItemId(), SrcEntry.GetItemType(), ToCreate, NewInst);
        	if (NewIdx == INDEX_NONE) return false;

        	TargetComponent->AssignInstanceToSlotUnique(TargetContainerIndex, TargetSlotIndex, NewInst);

        	int32 Removed = 0;
        	Src.RemoveByInstance(Payload.InstanceId, ToCreate, Removed);
        	OutMoved = Removed;

        	// Events
        	if (FInv_InventoryEntry* AddedPtr = Dst.FindEntryMutableByInstance(NewInst))
        		TargetComponent->OnItemAdded.Broadcast(*AddedPtr);
        	if (FInv_InventoryEntry* RemainingPtr = Src.FindEntryMutableByInstance(Payload.InstanceId))
        		SourceComponent->OnItemRemoved.Broadcast(*RemainingPtr);
        	else
        		SourceComponent->OnItemRemoved.Broadcast(SrcEntry);

        	return OutMoved > 0;
        }
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
	ApplyLocalMappingForTransfer(Payload, TargetComponent, TargetContainerIndex, TargetSlotIndex);
	ServerTransferItem(Payload, TargetComponent, TargetContainerIndex, TargetSlotIndex);
	return false;
}

bool URpg_ContainerComponent::GetEntryAtIndex(int32 ContainerIndex, int32 EntryIndex, FInv_InventoryEntry& OutEntry) const
{
	OutEntry = FInv_InventoryEntry();

	if (!Containers.IsValidIndex(ContainerIndex)) return false;

	const FInvContainer& C = Containers[ContainerIndex];
	const int32 Total = C.Rows * C.Cols;

	// WICHTIG: Mapping-Größe sicherstellen, sonst ist GetSlotInstance u.U. leer
	const_cast<URpg_ContainerComponent*>(this)->EnsureSlotMapSize(ContainerIndex, Total);

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

	// --- Client: optimistisches Mapping ---
	if (!Containers.IsValidIndex(ThisContainerIndex) || !OtherComponent->Containers.IsValidIndex(OtherContainerIndex))
		return false;

	FInvContainer& ACont = Containers[ThisContainerIndex];
	FInvContainer& BCont = OtherComponent->Containers[OtherContainerIndex];

	EnsureSlotMapSize(ThisContainerIndex, ACont.Rows * ACont.Cols);
	OtherComponent->EnsureSlotMapSize(OtherContainerIndex, BCont.Rows * BCont.Cols);

	const FGuid AId = GetSlotInstance(ThisContainerIndex, ThisSlotIndex);
	const FGuid BId = OtherComponent->GetSlotInstance(OtherContainerIndex, OtherSlotIndex);

	if (AId.IsValid() && BId.IsValid()) {
		AssignInstanceToSlotUnique(ThisContainerIndex, ThisSlotIndex, BId);
		OtherComponent->AssignInstanceToSlotUnique(OtherContainerIndex, OtherSlotIndex, AId);
	} else if (AId.IsValid()) { // move A -> B
		OtherComponent->AssignInstanceToSlotUnique(OtherContainerIndex, OtherSlotIndex, AId);
		ClearSlot(ThisContainerIndex, ThisSlotIndex);
	} else if (BId.IsValid()) { // move B -> A
		AssignInstanceToSlotUnique(ThisContainerIndex, ThisSlotIndex, BId);
		OtherComponent->ClearSlot(OtherContainerIndex, OtherSlotIndex);
	} // beide leer: no-op

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

