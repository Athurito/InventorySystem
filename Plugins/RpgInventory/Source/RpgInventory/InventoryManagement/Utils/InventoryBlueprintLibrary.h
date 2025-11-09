#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "UObject/PrimaryAssetId.h"
#include "InventoryBlueprintLibrary.generated.h"

class URpg_ContainerComponent;
class UInventoryItemComponent;

UCLASS()
class RPGINVENTORY_API UInventoryBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	/**
	 * Reduce the stack size of a specific inventory entry by InstanceId.
	 * - If called on a client, it will route to the server via the container's ServerRemoveItemFromContainer RPC.
	 * - If called on the server, it will mutate the FastArray and replicate the delta.
	 * UI can listen to URpg_ContainerComponent::OnItemConsumed and OnItemRemoved to refresh.
	 */
	UFUNCTION(BlueprintCallable, Category="Inventory|Container", meta=(DefaultToSelf="Container"))
	static bool ConsumeStackByInstance(URpg_ContainerComponent* Container, int32 ContainerIndex, const FGuid& InstanceId, int32 Quantity, int32& OutConsumed);

	/**
	 * Reduce the stack on a world item component directly (server only).
	 * Destroys the owning actor if the stack hits 0. Returns true if any amount was consumed.
	 */
	UFUNCTION(BlueprintCallable, Category="Inventory|WorldItem")
	static bool ConsumeWorldItemStack(UInventoryItemComponent* ItemComponent, int32 Quantity, int32& OutConsumed);
};