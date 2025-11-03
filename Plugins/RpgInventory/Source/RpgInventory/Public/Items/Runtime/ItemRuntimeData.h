#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "StructUtils/InstancedStruct.h"
#include "ItemRuntimeData.generated.h"

/**
 * Generic runtime entry keyed by a gameplay tag with arbitrary struct payload via FInstancedStruct
 */
USTRUCT()
struct RPGINVENTORY_API FItemRuntimeEntry : public FFastArraySerializerItem
{
	GENERATED_BODY()

	// Key that identifies which fragment this data belongs to
	UPROPERTY()
	FGameplayTag Key;

	// The actual runtime data, typed per-fragment via UStruct
	UPROPERTY()
	FInstancedStruct Data;
};

/**
 * FastArray container to replicate fragment runtime data efficiently (delta replication)
 */
USTRUCT()
struct RPGINVENTORY_API FItemRuntimeDataContainer : public FFastArraySerializer
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FItemRuntimeEntry> Entries;


	// Neu: Owner-Kontext
	UPROPERTY(Transient) TObjectPtr<class URpg_ContainerComponent> OwnerComponent = nullptr;
	UPROPERTY(Transient) FGuid OwnerInstanceId; // die Instanz, zu der diese RuntimeData gehört

	// ... bestehende Methoden ...

	// Neu: Delta-Callbacks
	void PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize);
	void PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize);
	void PreReplicatedRemove(const TArrayView<int32> RemovedIndices, int32 FinalSize);
	
	void CopyFrom(const FItemRuntimeDataContainer& Src);

	template<typename T>
	T* SetValue(const FGameplayTag& KeyTag, const T& NewValue)
	{
		T* Ptr = FindOrAddMutable<T>(KeyTag);   // legt an und markiert Array/Item dirty beim Anlegen
		if (!Ptr) return nullptr;

		*Ptr = NewValue;                     // immer schreiben
		// Item erneut explizit dirty markieren, damit ein Delta sicher rausgeht
		if (FItemRuntimeEntry* Found = Entries.FindByPredicate(
				[&](const FItemRuntimeEntry& E){ return E.Key == KeyTag; }))
		{
			MarkItemDirty(*Found);
		}
		return Ptr;
	}

	// Führt eine Mutation per Lambda aus und markiert immer dirty
	template<typename T, typename Fn>
	T* Modify(const FGameplayTag& Key, Fn&& Mutate)
	{
		T* Ptr = FindOrAddMutable<T>(Key);
		if (!Ptr) return nullptr;
		Mutate(*Ptr);
		MarkDirty(Key); // << garantiert Item-Delta
		return Ptr;
	}

	template<typename T>
	const T* FindConst(const FGameplayTag& Key) const
	{
		const FItemRuntimeEntry* Found = Entries.FindByPredicate([&](const FItemRuntimeEntry& E){ return E.Key == Key; });
		return Found ? &Found->Data.Get<T>() : static_cast<const T*>(nullptr);
	}
	// Remove an entry by key
	bool RemoveByKey(const FGameplayTag& Key);

	// Mark an entry as dirty to trigger replication
	void MarkDirty(const FGameplayTag& Key)
	{
		if (FItemRuntimeEntry* Found = Entries.FindByPredicate([&](const FItemRuntimeEntry& E){ return E.Key == Key; }))
		{
			MarkItemDirty(*Found);
		}
	}

	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
	{
		return FastArrayDeltaSerialize(Entries, DeltaParms, *this);
	}

private:
	// Create or return the existing struct of type T under Key
	template<typename T>
	T* FindOrAddMutable(const FGameplayTag& Key)
	{
		if (FItemRuntimeEntry* Found = Entries.FindByPredicate([&](const FItemRuntimeEntry& E){ return E.Key == Key; }))
		{
			return &Found->Data.GetMutable<T>();
		}
		MarkArrayDirty();
		FItemRuntimeEntry& NewEntry = Entries.AddDefaulted_GetRef();
		NewEntry.Key = Key;
		NewEntry.Data.InitializeAs<T>();
		MarkItemDirty(NewEntry);
		return &NewEntry.Data.GetMutable<T>();
	}
};

template<>
struct TStructOpsTypeTraits<FItemRuntimeDataContainer> : public TStructOpsTypeTraitsBase2<FItemRuntimeDataContainer>
{
	enum { WithNetDeltaSerializer = true };
};

/** Example runtime data for the Stackable fragment */
USTRUCT(BlueprintType)
struct FStackableRuntimeData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
	int32 CurrentStackCount = 1;
};


/** Optional per-instance cooldown for consumables */
USTRUCT(BlueprintType)
struct FUseCooldownRuntimeData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
    float LastUseServerTime = -BIG_NUMBER;
};
