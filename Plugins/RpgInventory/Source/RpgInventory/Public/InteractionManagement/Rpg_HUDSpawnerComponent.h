// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Rpg_HUDSpawnerComponent.generated.h"
class URpg_HUDWidget;
class URpg_InteractionComponent;
class APlayerController;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), Blueprintable)
class RPGINVENTORY_API URpg_HUDSpawnerComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	URpg_HUDSpawnerComponent();

	// Welche HUD-Klasse soll erstellt werden? (BP-overridebar)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="HUD")
	TSubclassOf<URpg_HUDWidget> HUDWidgetClass;

	// Z-Order im Viewport (optional)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="HUD")
	int32 HUDZOrder = 0;

	// Zugriff für Blueprints (optional)
	UFUNCTION(BlueprintCallable, Category="HUD")
	URpg_HUDWidget* GetHUD() const { return HUDWidget; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UPROPERTY() URpg_HUDWidget* HUDWidget = nullptr;
	TWeakObjectPtr<APlayerController> CachedPC;
	
	// Bindet HUD einmalig an die InteractionComponent am PlayerController
	void BindHUDToControllerInteraction();
};
