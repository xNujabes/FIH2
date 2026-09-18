// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/StaticMeshComponent.h"
#include "System/Interactable.h"
#include "Inanimate.generated.h"


/**
 * 
 */


UCLASS(Abstract)
class FIH2_API AInanimate : public AActor, public IInteractable
{
	GENERATED_BODY()

public:
	AInanimate();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* Mesh;
};
