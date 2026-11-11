// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Eatable.generated.h"

/**
 * 
 */
UINTERFACE(MinimalAPI, Blueprintable)
class UEatable : public UInterface
{
	GENERATED_BODY()
};

class FIH2_API IEatable
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Destruction")
	void TriggerEat(AActor* EatInstigator, FVector HitLocation, float DamageAmount);

	/** C++ only: false once the item was eaten (or is about to disappear). AI uses it to drop a stale target. */
	virtual bool IsEdible() const { return true; }
};
