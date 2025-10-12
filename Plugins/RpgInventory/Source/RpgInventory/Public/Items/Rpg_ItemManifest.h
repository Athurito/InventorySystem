// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/DataAsset.h"
#include "StructUtils/InstancedStruct.h"
#include "Rpg_ItemManifest.generated.h"

struct FItemFragment;
/**
 * 
 */
UCLASS()
class RPGINVENTORY_API URpg_ItemManifest : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	TArray<TInstancedStruct<FItemFragment>>& GetFragmentsMutable() { return Fragments; }
	TArray<TInstancedStruct<FItemFragment>> GetFragments() { return Fragments; }
	FText GetInteractionText() const { return InteractionText; }


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
	
	UPROPERTY(EditAnywhere, Category = "Inventory")
	FText InteractionText;
};

template <typename T> requires std::derived_from<T, FItemFragment>
const T* URpg_ItemManifest::GetFragmentOfTypeWithTag(const FGameplayTag& FragmentTag) const
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
const T* URpg_ItemManifest::GetFragmentOfType() const
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
T* URpg_ItemManifest::GetFragmentOfTypeMutable()
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
TArray<const T*> URpg_ItemManifest::GetAllFragmentsOfType() const
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
