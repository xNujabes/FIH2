// Fill out your copyright notice in the Description page of Project Settings.

#include "Bonus/Food.h"
#include "Components/BoxComponent.h"
#include "Fish/Fish.h"

// Sets default values
AFood::AFood()
{
	// Physic proxy was done with Chaos Destruction, leave it there for now
	PhysicsProxy = CreateDefaultSubobject<UBoxComponent>(TEXT("FoodPhysicsProxy"));
	SetRootComponent(PhysicsProxy);
	PhysicsProxy->SetSimulatePhysics(true);
	PhysicsProxy->SetCollisionProfileName(TEXT("PhysicsActor"));

	// TODO: Setup  destruction system here in the future
}

void AFood::TriggerEat_Implementation(AActor* EatInstigator, FVector HitLocation, float DamageAmount)
{
	AFish* FishInstigator = Cast<AFish>(EatInstigator);
	if (FishInstigator)
	{
		// Feed the fish
		FishInstigator->AddHunger(1);

		// TODO: Call TriggerBreak when done

		// Destroy the food actor immediately
		Destroy();
	}
}

void AFood::TriggerBreak_Implementation(AActor* BreakInstigator, FVector HitLocation, float DamageAmount)
{
	// TODO: Implement future destruction logic here if needed.
	// For now, it does nothing because TriggerEat handles destroying the actor.
}
