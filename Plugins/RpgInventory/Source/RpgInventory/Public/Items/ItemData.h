// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/DataAsset.h"
#include "StructUtils/InstancedStruct.h"
#include "ItemData.generated.h"

struct FItemFragment;
/**
 * 
 */
UCLASS()
class RPGINVENTORY_API UItemData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	TArray<TInstancedStruct<FItemFragment>>& GetFragmentsMutable() { return Fragments; }
	TArray<TInstancedStruct<FItemFragment>> GetFragments() { return Fragments; }


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

	void SpawnPickupActor(const UObject* WorldContextObject, const FVector& SpawnLocation, const FRotator& SpawnRotation);

	
private:

	UPROPERTY(EditAnywhere, Category = "Inventory", meta = (ExcludeBaseStruct))
	TArray<TInstancedStruct<FItemFragment>> Fragments;

	UPROPERTY(EditAnywhere, Category = "Inventory")
	TSubclassOf<AActor> PickupActorClass;
};

template <typename T> requires std::derived_from<T, FItemFragment>
const T* UItemData::GetFragmentOfTypeWithTag(const FGameplayTag& FragmentTag) const
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
const T* UItemData::GetFragmentOfType() const
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
T* UItemData::GetFragmentOfTypeMutable()
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
TArray<const T*> UItemData::GetAllFragmentsOfType() const
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
