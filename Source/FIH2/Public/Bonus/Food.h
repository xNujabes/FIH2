// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Inanimate/Inanimate.h"
#include "System/Breakable.h"
#include "System/Eatable.h"
#include "Components/BoxComponent.h"
#include "Food.generated.h"

UCLASS()
class FIH2_API AFood : public AInanimate, public IBreakable, public IEatable
{
	GENERATED_BODY()

public:
	AFood();
    
	// workaround given by AI to grab BP_FOOD because GeometryCollection cannot be grabbed
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Physics")
	UBoxComponent* PhysicsProxy;
    
	// TODO: Add destruction system here in the future
    
	virtual void TriggerEat_Implementation(AActor* EatInstigator, FVector HitLocation, float DamageAmount) override;
    
	// Kept the override to satisfy the IBreakable interface, but stripped the old logic
	virtual void TriggerBreak_Implementation(AActor* BreakInstigator, FVector HitLocation, float DamageAmount) override;
};