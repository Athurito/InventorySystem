// Fill out your copyright notice in the Description page of Project Settings.


#include "InventoryFragment_Hud.h"

UTexture2D* UInventoryFragment_Hud::GetIcon() const
{
	return Icon.Get();
}

TSoftObjectPtr<UTexture2D> UInventoryFragment_Hud::GetIconSoft() const
{
	return Icon;
}