// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Net/UnrealNetwork.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "FastArrayDemoComponent.generated.h"

UCLASS(BlueprintType)
class UInventoryItemInstanceTest : public UObject
{
	GENERATED_BODY()
public:
	UInventoryItemInstanceTest() { SetIsReplicatedByDefault(true); }

	UPROPERTY(Replicated, BlueprintReadOnly)
	int32 CurrentStackCount = 1;

	// Beispiel: per-Fragment-States (einfach gehalten)
	UPROPERTY(Replicated)
	TMap<TSubclassOf<UObject>, int32> IntStates;

	virtual bool IsSupportedForNetworking() const override { return true; }
	

	// Rep
	void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& Out) const override
	{
		Super::GetLifetimeReplicatedProps(Out);
		DOREPLIFETIME(UInventoryItemInstanceTest, CurrentStackCount);
		DOREPLIFETIME(UInventoryItemInstanceTest, IntStates);
	}
};

USTRUCT(BlueprintType)
struct FDemoItem : public FFastArraySerializerItem
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category="Demo")
	int32 Id = 0;

	UPROPERTY(BlueprintReadOnly, Category="Demo")
	FString Name;

	UPROPERTY(BlueprintReadOnly, Category="Demo")
	int32 Count = 1;
};

USTRUCT()
struct FDemoArray : public FFastArraySerializer
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FDemoItem> Items;

	// Nicht repliziert; nur Owner-Kontext für Callbacks
	UPROPERTY(NotReplicated)
	UFastArrayDemoComponent* Owner = nullptr;

	// FastArray-Serialization aktivieren
	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParams)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FDemoItem, FDemoArray>(Items, DeltaParams, *this);
	}

	// Batched-Callbacks (UE5)
	void PreReplicatedRemove(const TArrayView<int32> RemovedIndices, int32 FinalSize);
	void PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize);
	void PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize);
};

template<>
struct TStructOpsTypeTraits<FDemoArray> : public TStructOpsTypeTraitsBase2<FDemoArray>
{
	enum { WithNetDeltaSerializer = true };
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnDemoItemAdded, int32, Index, const FDemoItem&, Item);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnDemoItemChanged, int32, Index, const FDemoItem&, Item);
// Removed: Element existiert nach dem Remove nicht mehr -> Kopie mitgeben
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnDemoItemRemoved, int32, FormerIndex, const FDemoItem&, ItemCopy);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class RPGINVENTORY_API UFastArrayDemoComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFastArrayDemoComponent();

	// === Blueprint Events (Client-Seite) ===
	UPROPERTY(BlueprintAssignable, Category="Demo|Events")
	FOnDemoItemAdded OnItemAdded;

	UPROPERTY(BlueprintAssignable, Category="Demo|Events")
	FOnDemoItemChanged OnItemChanged;

	UPROPERTY(BlueprintAssignable, Category="Demo|Events")
	FOnDemoItemRemoved OnItemRemoved;

	// === Repliziertes FastArray ===
	UPROPERTY(Replicated)
	FDemoArray Inventory;

	// === Server-RPCs, direkt aus Blueprints aufrufbar ===
	UFUNCTION(Server, Reliable, BlueprintCallable, Category="Demo|Server")
	void ServerAddItem(const FString& Name, int32 Count);

	UFUNCTION(Server, Reliable, BlueprintCallable, Category="Demo|Server")
	void ServerChangeItemCount(int32 Index, int32 NewCount);

	UFUNCTION(Server, Reliable, BlueprintCallable, Category="Demo|Server")
	void ServerRemoveItem(int32 Index);

	// === Hilfsfunktionen (BP-Lesen) ===
	UFUNCTION(BlueprintPure, Category="Demo|Query")
	void GetAllItems(TArray<FDemoItem>& OutItems) const { OutItems = Inventory.Items; }

	// Required replication boilerplate
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;

	// Nur Server: Mutationen + Dirty-Marks
	void AddItem_ServerImpl(const FString& Name, int32 Count, int32& OutIndex);
	void ChangeItemCount_ServerImpl(int32 Index, int32 NewCount);
	void RemoveItem_ServerImpl(int32 Index);

public:
	void EmitAdded(int32 Index, const FDemoItem& Item);
	void EmitChanged(int32 Index, const FDemoItem& Item);
	void EmitRemoved(int32 FormerIndex, const FDemoItem& ItemCopy);
};
