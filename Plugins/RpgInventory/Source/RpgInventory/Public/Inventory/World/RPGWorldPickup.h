#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "InventoryManagement/Interact/Interface/Interactable.h"
#include "RPGWorldPickup.generated.h"

class UItemPickupComponent;
class UItemDefinition;
class USceneComponent;
class UStaticMeshComponent;
class APawn;

UCLASS()
class RPGINVENTORY_API ARPGWorldPickup : public AActor, public IInteractable
{
	GENERATED_BODY()
public:
	ARPGWorldPickup();

	UFUNCTION(BlueprintCallable, Category="Pickup")
	void Initialize(UItemDefinition* InDefinition, int32 InStackCount);

	// IInteractable
	virtual FInteractDisplayData GetDisplayData_Implementation() const override;
	virtual bool CanInteract_Implementation(APawn* Instigator) const override;
	virtual void Interact_Implementation(APawn* Instigator) override;

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess="true"))
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess="true"))
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess="true"))
	TObjectPtr<UItemPickupComponent> PickupComponent;
};
