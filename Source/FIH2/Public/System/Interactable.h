// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Interactable.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UInteractable : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class FIH2_API IInteractable
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Interaction")
    void OnHover();
	
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Interaction")
	void OnGrab();
	
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Interaction")
	void OnDropped();
	
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Interaction")
	void OnThrown(FVector ThrowVelocity);
	
	


};
