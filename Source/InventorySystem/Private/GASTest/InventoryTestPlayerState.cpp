// Fill out your copyright notice in the Description page of Project Settings.


#include "GASTest/InventoryTestPlayerState.h"

#include "AbilitySystemComponent.h"
#include "GASTest/InventoryTestAttributeSet.h"

AInventoryTestPlayerState::AInventoryTestPlayerState()
{
	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AttributeSet = CreateDefaultSubobject<UInventoryTestAttributeSet>(TEXT("InventoryTestAttributeSet"));
}

UAbilitySystemComponent* AInventoryTestPlayerState::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}
