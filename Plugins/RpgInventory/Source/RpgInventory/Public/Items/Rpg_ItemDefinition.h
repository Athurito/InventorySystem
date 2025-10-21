// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/DataAsset.h"
#include "StructUtils/InstancedStruct.h"
#include "Rpg_ItemDefinition.generated.h"

struct FItemFragment;
/**
 * 
 */
UCLASS()
class RPGINVENTORY_API URpg_ItemDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	
	virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId(TEXT("Item"), GetFName());
	}
	
	const TArray<TInstancedStruct<FItemFragment>>& GetFragments() const { return Fragments; }
	TArray<TInstancedStruct<FItemFragment>>& GetFragmentsMutable() { return Fragments; }
	FText GetInteractionText() const { return InteractionText; }
	const FGameplayTag& GetItemType() const { return ItemType; }
	
	template<typename T>
	requires std::derived_from<T, FItemFragment>
	const T* GetFragmentOfTypeWithTag(const FGameplayTag& FragmentTag) const;

	template<typename T>
	requires std::derived_from<T, FItemFragment>
	const T* GetFragmentOfType() const;

	template<typename T>
	requires std::derived_from<T, FItemFragment>
	T* GetFragmentOfTypeMutable();

	template<typename T>
	requires std::derived_from<T, FItemFragment>
	TArray<const T*> GetAllFragmentsOfType() const;
	
	template<typename T>
	bool HasFragment() const { return GetFragmentOfType<T>() != nullptr; }

	void SpawnPickupActor(const UObject* WorldContextObject, const FVector& SpawnLocation, const FRotator& SpawnRotation);

	
private:

	UPROPERTY(EditDefaultsOnly, Category = "Inventory", meta = (ExcludeBaseStruct))
	TArray<TInstancedStruct<FItemFragment>> Fragments;

	UPROPERTY(EditAnywhere, Category = "Inventory", meta = (Categories="GameItems"))
	FGameplayTag ItemType;
	
	UPROPERTY(EditDefaultsOnly, Category = "Inventory")
	TSoftClassPtr<AActor> PickupActorClass;
	
	UPROPERTY(EditDefaultsOnly, Category = "Inventory")
	FText InteractionText;
};

template <typename T> requires std::derived_from<T, FItemFragment>
const T* URpg_ItemDefinition::GetFragmentOfTypeWithTag(const FGameplayTag& FragmentTag) const
{
	for (const TInstancedStruct<FItemFragment>& Fragment : Fragments)
	{
		if (const T* FragmentPtr = Fragment.GetPtr<T>())
		{
			if (!FragmentPtr->GetFragmentTag().MatchesTagExact(FragmentTag))
				continue;
				
			return FragmentPtr;
		}
	}
	return nullptr;
}

template <typename T> requires std::derived_from<T, FItemFragment>
const T* URpg_ItemDefinition::GetFragmentOfType() const
{
	for (const TInstancedStruct<FItemFragment>& Fragment : Fragments)
	{
		if (const T* FragmentPtr = Fragment.GetPtr<T>())
		{
			return FragmentPtr;
		}
	}
	return nullptr;
}

template <typename T> requires std::derived_from<T, FItemFragment>
T* URpg_ItemDefinition::GetFragmentOfTypeMutable()
{
	for (TInstancedStruct<FItemFragment>& Fragment : Fragments)
	{
		if (T* FragmentPtr = Fragment.GetMutablePtr<T>())
		{
			return FragmentPtr;
		}
	}
	return nullptr;
}

template <typename T> requires std::derived_from<T, FItemFragment>
TArray<const T*> URpg_ItemDefinition::GetAllFragmentsOfType() const
{
	TArray<const T*> Results;
	for (const TInstancedStruct<FItemFragment>& Fragment : Fragments)
	{
		if (const T* FragmentPtr = Fragment.GetPtr<T>())
		{
			Results.Add(FragmentPtr);
		}
	}
	return Results;
}
