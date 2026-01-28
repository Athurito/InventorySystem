#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "CommonActivatableWidget.h"
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
	// Manuelle Aktivierung/Deaktivierung, da dieses Widget meist eingebettet ist
	UFUNCTION(BlueprintCallable, Category="Container|UI")
	void ActivateWidget();

	UFUNCTION(BlueprintCallable, Category="Container|UI")
	void DeactivateWidget();
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

	/** Name des ViewModels im MVVM-Panel des Widgets. Erlaubt dynamisches Binding ohne Hardcoding. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Container|UI")
	FName ViewModelName = FName("InventoryViewModel");
	
protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
};
