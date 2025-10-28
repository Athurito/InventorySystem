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

	// Find a mutable pointer to the struct of type T stored under Key
	template<typename T>
	T* FindMutable(const FGameplayTag& Key)
	{
		FItemRuntimeEntry* Found = Entries.FindByPredicate([&](const FItemRuntimeEntry& E){ return E.Key == Key; });
		return Found ? &Found->Data.GetMutable<T>() : static_cast<T*>(nullptr);
	}

	// Find a const pointer to the struct of type T stored under Key
	template<typename T>
	const T* FindConst(const FGameplayTag& Key) const
	{
		const FItemRuntimeEntry* Found = Entries.FindByPredicate([&](const FItemRuntimeEntry& E){ return E.Key == Key; });
		return Found ? &Found->Data.Get<T>() : static_cast<const T*>(nullptr);
	}

	// Create or return the existing struct of type T under Key
	template<typename T>
	T* FindOrAddMutable(const FGameplayTag& Key)
	{
		if (T* Existing = FindMutable<T>(Key))
		{
			return Existing;
		}
		MarkArrayDirty();
		FItemRuntimeEntry& NewEntry = Entries.AddDefaulted_GetRef();
		NewEntry.Key = Key;
		NewEntry.Data.InitializeAs<T>();
		MarkItemDirty(NewEntry);
		return &NewEntry.Data.GetMutable<T>();
	}

	// Remove an entry by key
	bool RemoveByKey(const FGameplayTag& Key)
	{
		int32 Index = Entries.IndexOfByPredicate([&](const FItemRuntimeEntry& E){ return E.Key == Key; });
		if (Index != INDEX_NONE)
		{
			FItemRuntimeEntry& Entry = Entries[Index];
			Entries.RemoveAt(Index);
			MarkItemDirty(Entry);
			return true;
		}
		return false;
	}

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
		return FFastArraySerializer::FastArrayDeltaSerialize(Entries, DeltaParms, *this);
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
