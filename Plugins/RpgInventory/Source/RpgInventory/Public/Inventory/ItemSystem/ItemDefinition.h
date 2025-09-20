#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ItemDefinition.generated.h"

class UItemFragment;

/** Primary Data Asset representing an item type. Holds a list of fragments that define behavior and UI. */
UCLASS(BlueprintType)
class RPGINVENTORY_API UItemDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()
public:
	// List of fragments that define this item
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Instanced, Category="Item")
	TArray<TObjectPtr<UItemFragment>> Fragments;

	// Helper to find a fragment of a given class
	template<typename T>
	const T* FindFragment() const
	{
		for (const UItemFragment* Frag : Fragments)
		{
			if (const T* AsT = Cast<T>(Frag))
			{
				return AsT;
			}
		}
		return nullptr;
	}
};
