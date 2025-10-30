// Fill out your copyright notice in the Description page of Project Settings.


#include "Widgets/TabList.h"

#include "CommonButtonBase.h"
#include "Components/HorizontalBox.h"

void UTabList::HandleTabCreation_Implementation(FName TabNameID, UCommonButtonBase* TabButton)
{
	TabListContainer->AddChild(TabButton);
}

void UTabList::HandleTabRemoval_Implementation(FName TabNameID, UCommonButtonBase* TabButton)
{
	TabListContainer->RemoveChild(TabButton);
}
