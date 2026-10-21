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

	// Proxy de collision utilisé pour attraper l'objet
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Physics")
	UBoxComponent* PhysicsProxy;

	// Composant Chaos Destruction
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Destruction")
	UGeometryCollectionComponent* GeometryCollection;

	virtual void TriggerEat_Implementation(AActor* EatInstigator, FVector HitLocation, float DamageAmount) override;

	virtual void TriggerBreak_Implementation(AActor* BreakInstigator, FVector HitLocation, float DamageAmount) override;
};
