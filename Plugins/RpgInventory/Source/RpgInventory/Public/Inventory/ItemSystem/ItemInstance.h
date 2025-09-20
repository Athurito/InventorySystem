#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "ItemInstance.generated.h"

class UItemDefinition;

/** Runtime instance of an item entry. Holds per-instance mutable data like stack count. */
UCLASS(BlueprintType)
class RPGINVENTORY_API UItemInstance : public UObject
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category="Item")
	void Initialize(UItemDefinition* InDefinition, int32 InStackCount)
	{
		Definition = InDefinition;
		StackCount = FMath::Max(1, InStackCount);
	}

	UFUNCTION(BlueprintPure, Category="Item")
	UItemDefinition* GetDefinition() const { return Definition; }

	UFUNCTION(BlueprintPure, Category="Item")
	int32 GetStackCount() const { return StackCount; }

	UFUNCTION(BlueprintCallable, Category="Item")
	void SetStackCount(int32 NewCount) { StackCount = FMath::Max(0, NewCount); }

private:
	UPROPERTY()
	TObjectPtr<UItemDefinition> Definition = nullptr;

	UPROPERTY()
	int32 StackCount = 1;
};
