#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "RPGInventoryComponent.generated.h"

class UItemDefinition;
class UItemInstance;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInventoryChanged);

USTRUCT(BlueprintType)
struct RPGINVENTORY_API FInventoryEntry : public FFastArraySerializerItem
{
	GENERATED_BODY()

	UPROPERTY()
	int32 EntryId = INDEX_NONE;

	// Replicate plain data only (no UObject in FastArray)
	UPROPERTY()
	TSoftObjectPtr<UItemDefinition> Definition;

	UPROPERTY()
	int32 StackCount = 1;

	// Fixed slot index in the inventory grid
	UPROPERTY()
	int32 SlotIndex = INDEX_NONE;
};

USTRUCT()
struct RPGINVENTORY_API FInventoryList : public FFastArraySerializer
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FInventoryEntry> Entries;

	// Not replicated directly; points back to owning component
	class URPGInventoryComponent* Owner = nullptr;

	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FInventoryEntry, FInventoryList>(Entries, DeltaParms, *this);
	}
};

template<>
struct TStructOpsTypeTraits<FInventoryList> : public TStructOpsTypeTraitsBase2<FInventoryList>
{
	enum { WithNetDeltaSerializer = true };
};

UCLASS(ClassGroup=(Inventory), meta=(BlueprintSpawnableComponent))
class RPGINVENTORY_API URPGInventoryComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	URPGInventoryComponent();

	// Capacity management
	UFUNCTION(BlueprintPure, Category="Inventory")
	int32 GetCapacity() const { return Capacity; }

	UFUNCTION(BlueprintPure, Category="Inventory")
	bool HasFreeSlot() const;

	// Minimal API
	UFUNCTION(BlueprintCallable, Category="Inventory")
	bool AddItem(UItemDefinition* Definition, int32 StackCount);

	UFUNCTION(BlueprintCallable, Category="Inventory")
	bool RemoveByEntryId(int32 EntryId, int32 Amount);

	UFUNCTION(BlueprintPure, Category="Inventory")
	const TArray<FInventoryEntry>& GetEntries() const { return Items.Entries; }

	// Slot based helpers
	UFUNCTION(BlueprintPure, Category="Inventory")
	int32 GetEntryIdAtSlot(int32 Slot) const;

	UFUNCTION(BlueprintCallable, Category="Inventory")
	bool MoveSlotToSlot(int32 FromSlot, int32 ToSlot); // server-authoritative (if called on server)

	UFUNCTION(Server, Reliable)
	void Server_MoveSlotToSlot(int32 FromSlot, int32 ToSlot);

	// Change notifications
	UPROPERTY(BlueprintAssignable, Category="Inventory")
	FOnInventoryChanged OnInventoryChanged;

protected:
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION()
	void OnRep_Items();

private:
	UPROPERTY(ReplicatedUsing=OnRep_Items)
	FInventoryList Items;

	// Fixed number of slots; one item per slot
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Inventory", meta=(AllowPrivateAccess="true", ClampMin="1"))
	int32 Capacity = 20;

	int32 NextId = 1;

	int32 FindEntryIndexBySlot(int32 Slot) const;
};
