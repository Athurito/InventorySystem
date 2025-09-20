#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ItemPickupComponent.generated.h"

class UItemDefinition;
class UItemInstance;
class APawn;

UCLASS(ClassGroup=(Inventory), meta=(BlueprintSpawnableComponent))
class RPGINVENTORY_API UItemPickupComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UItemPickupComponent();

	UFUNCTION(BlueprintCallable, Category="Pickup")
	void InitializeFromDefinition(UItemDefinition* InDefinition, int32 InStackCount);

	UFUNCTION(BlueprintPure, Category="Pickup")
	UItemDefinition* GetDefinition() const { return Definition; }

	UFUNCTION(BlueprintPure, Category="Pickup")
	int32 GetStackCount() const { return StackCount; }

	// Called to attempt interacting (server-authoritative expected)
	UFUNCTION(BlueprintCallable, Category="Pickup")
	bool TryPickup(APawn* InstigatorPawn);

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(Replicated)
	TObjectPtr<UItemDefinition> Definition = nullptr;

	UPROPERTY(Replicated)
	int32 StackCount = 1;

	UFUNCTION()
	void OnRep_Data();

	void UpdateVisuals() const;

	// replication
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
