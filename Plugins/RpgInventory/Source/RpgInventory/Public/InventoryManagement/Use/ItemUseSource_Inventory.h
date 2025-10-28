#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "ItemUseSource_Inventory.generated.h"

class URpg_ContainerComponent;

UCLASS()
class RPGINVENTORY_API UItemUseSource_Inventory : public UObject
{
	GENERATED_BODY()
public:
	UPROPERTY()
	TObjectPtr<URpg_ContainerComponent> OwnerContainer = nullptr;

	UPROPERTY()
	int32 ContainerIndex = INDEX_NONE;

	UPROPERTY()
	FGuid InstanceId;

	static UItemUseSource_Inventory* Make(UObject* Outer, URpg_ContainerComponent* Comp, int32 Index, const FGuid& Id)
	{
		UItemUseSource_Inventory* Obj = NewObject<UItemUseSource_Inventory>(Outer);
		Obj->OwnerContainer = Comp;
		Obj->ContainerIndex = Index;
		Obj->InstanceId = Id;
		return Obj;
	}
};
