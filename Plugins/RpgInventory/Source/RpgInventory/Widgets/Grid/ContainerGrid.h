#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "ContainerGrid.generated.h"

class UInventoryManagerComponent;

UENUM(BlueprintType)
enum class EInventorySourceType : uint8
{
	Player      UMETA(DisplayName="Player Inventory"),
	Storage     UMETA(DisplayName="Storage Container")
};

UCLASS()
class RPGINVENTORY_API UContainerGrid : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Container|UI")
	EInventorySourceType SourceType = EInventorySourceType::Player;
	
	UPROPERTY(BlueprintReadWrite, Category="Container|UI")
	TWeakObjectPtr<UInventoryManagerComponent> StorageManager;
	
	// Helfer für den Resolver:
	UFUNCTION(BlueprintPure, Category="Container|UI")
	UInventoryManagerComponent* ResolveManagerForViewModel() const;

	UFUNCTION(BlueprintPure, Category="Container|UI")
	int32 ResolveContainerIndexForViewModel() const;
	
	UPROPERTY(BlueprintReadWrite, Category="Container|UI")
	int32 StorageContainerIndex = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Container|UI")
	int32 PlayerContainerIndex = 0;
	
protected:
	virtual void NativeDestruct() override;
};
