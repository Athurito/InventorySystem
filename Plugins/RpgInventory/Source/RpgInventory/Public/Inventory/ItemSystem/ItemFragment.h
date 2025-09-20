#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "ItemFragment.generated.h"

/** Base class for all Item Fragments (DataAsset). Pure data, logic is driven by components/systems reading these. */
UCLASS(Abstract, BlueprintType, EditInlineNew)
class RPGINVENTORY_API UItemFragment : public UPrimaryDataAsset
{
	GENERATED_BODY()
public:
	// Optional free-form tags to help systems query fragments
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Tags")
	FGameplayTagContainer FragmentTags;
};


/** Fragment: Stackable */
UCLASS(BlueprintType)
class RPGINVENTORY_API UItemFragment_Stackable : public UItemFragment
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stack")
	int32 MaxStack = 1;
};

/** Fragment: Consumable (GAS) */
UCLASS(BlueprintType)
class RPGINVENTORY_API UItemFragment_Consumable : public UItemFragment
{
	GENERATED_BODY()
public:
	// One of these can be set; both allowed but systems will decide precedence.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="GAS")
	TSubclassOf<class UGameplayEffect> GameplayEffectClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="GAS")
	TSubclassOf<class UGameplayAbility> AbilityClass;

	// Optional cooldown tag to gate consumption
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="GAS")
	FGameplayTag CooldownTag;
};

/** Fragment: Equippable (GAS) */
UCLASS(BlueprintType)
class RPGINVENTORY_API UItemFragment_Equippable : public UItemFragment
{
	GENERATED_BODY()
public:
	// Allowed equipment slots (game-specific)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Equip")
	FGameplayTagContainer AllowedSlots;

	// Abilities granted while equipped
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Equip")
	TArray<TSubclassOf<class UGameplayAbility>> GrantedAbilities;
};

/** Fragment: Visuals */
UCLASS(BlueprintType)
class RPGINVENTORY_API UItemFragment_Visual : public UItemFragment
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Visual")
	TObjectPtr<class UTexture2D> Icon = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Visual")
	TObjectPtr<class UStaticMesh> WorldMesh = nullptr;

	// Optional actor template used by world pickup spawner
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Visual")
	TSubclassOf<class AActor> PickupTemplate;
};

/** Fragment: Weight */
UCLASS(BlueprintType)
class RPGINVENTORY_API UItemFragment_Weight : public UItemFragment
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weight", meta=(ClampMin="0.0"))
	float Weight = 0.f;
};

/** Fragment: Free Gameplay Tags */
UCLASS(BlueprintType)
class RPGINVENTORY_API UItemFragment_Tags : public UItemFragment
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Tags")
	FGameplayTagContainer ItemTags;
};

/** UI fragment: Description */
UCLASS(BlueprintType)
class RPGINVENTORY_API UItemFragment_UI_Description : public UItemFragment
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="UI")
	FText Title;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="UI", meta=(MultiLine="true"))
	FText Flavor;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="UI", meta=(MultiLine="true"))
	FText RichText;
};

/** UI fragment: Stats slices */
USTRUCT(BlueprintType)
struct RPGINVENTORY_API FUIStatSlice
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName Key;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText Value;
};

UCLASS(BlueprintType)
class RPGINVENTORY_API UItemFragment_UI_StatsSlice : public UItemFragment
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="UI")
	TArray<FUIStatSlice> Slices;
};

/** UI fragment: Badges */
UCLASS(BlueprintType)
class RPGINVENTORY_API UItemFragment_UI_Badges : public UItemFragment
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="UI")
	TArray<FText> Badges;
};
