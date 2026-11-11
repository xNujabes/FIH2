// Fill out your copyright notice in the Description page of Project Settings.


#include "Fish/Fish.h"

#include "Components/SphereComponent.h"
#include "Kismet/KismetMathLibrary.h"


// Sets default values
AFish::AFish()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	RootComponent = Mesh;

	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("Eat Sphere Collision"));
	CollisionSphere->SetupAttachment(RootComponent);

	// Kinematic body: the fish is moved only by UFishSteeringMovement (no physics, no gravity).
	Mesh->SetSimulatePhysics(false);
	Mesh->SetEnableGravity(false);
}

// Called when the game starts or when spawned
void AFish::BeginPlay()
{
	Super::BeginPlay();

	// Guarantee it whatever a Blueprint default says: steering is the only mover.
	SetHeldByPlayer(false);
}

void AFish::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	OnHungerChanged.Broadcast();
}



int AFish::GetHunger() const
{
	return Hunger;
}

void AFish::SetHunger(float NewHunger)
{
	Hunger = NewHunger;
	OnHungerChanged.Broadcast();
}

void AFish::AddHunger(float AddedHunger)
{
	Hunger = UKismetMathLibrary::Clamp(Hunger + AddedHunger, 0, 3);
	OnHungerChanged.Broadcast();
}


void AFish::SetHeldByPlayer(bool bHeld)
{
	bIsHeldByPlayer = bHeld;

	if (Mesh)
	{
		// Physics only while the player drags the fish; otherwise kinematic, driven by UFishSteeringMovement.
		Mesh->SetSimulatePhysics(bHeld);
		Mesh->SetEnableGravity(false);
	}
}

bool AFish::IsStimulated() const
{
	return bIsStimulated;
}

void AFish::SetIsStimulated(bool bNewStimulated)
{
	bIsStimulated = bNewStimulated;
}

FText AFish::GetName_Implementation()
{
	return FText::FromString("Fish");
}

FText AFish::GetDescription_Implementation()
{
	FString StimulatedStr = bIsStimulated ? TEXT("Yes") : TEXT("No");
	FString Desc = FString::Printf(TEXT("Hunger Level: %d\nStimulated: %s"), Hunger, *StimulatedStr);
    
	return FText::FromString(Desc);
}
