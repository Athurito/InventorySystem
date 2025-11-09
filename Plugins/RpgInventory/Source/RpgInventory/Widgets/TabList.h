// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CommonTabListWidgetBase.h"
#include "TabList.generated.h"

class UHorizontalBox;
/**
 * 
 */
UCLASS()
class RPGINVENTORY_API UTabList : public UCommonTabListWidgetBase
{
	GENERATED_BODY()

private:
	virtual void HandleTabCreation_Implementation(FName TabNameID, UCommonButtonBase* TabButton) override;
	virtual void HandleTabRemoval_Implementation(FName TabNameID, UCommonButtonBase* TabButton) override;
	
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UHorizontalBox> TabListContainer;
};
