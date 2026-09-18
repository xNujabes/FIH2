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
};
