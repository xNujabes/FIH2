// Fill out your copyright notice in the Description page of Project Settings.


#include "Fish/Fish.h"

#include "Components/SphereComponent.h"
#include "Kismet/KismetMathLibrary.h"


// Sets default values
AFish::AFish()
{
	// Pas de tick d'acteur : le mouvement est géré dans UFishSteeringMovement (son propre tick).
	PrimaryActorTick.bCanEverTick = false;
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

void AFish::SetHunger(int32 NewHunger)
{
	Hunger = FMath::Clamp(NewHunger, 0, 3);
	OnHungerChanged.Broadcast();
	FInteractableEvents::NotifyInfoChanged(this);
}

void AFish::AddHunger(int32 AddedHunger)
{
	Hunger = FMath::Clamp(Hunger + AddedHunger, 0, 3);
	OnHungerChanged.Broadcast();
	FInteractableEvents::NotifyInfoChanged(this);
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
	if (bIsStimulated == bNewStimulated)
	{
		return;
	}
	bIsStimulated = bNewStimulated;
	FInteractableEvents::NotifyInfoChanged(this);
}

void AFish::SetGrowthProgress(float InProgress)
{
	const float NewProgress = FMath::Clamp(InProgress, 0.f, 1.f);
	const bool bPercentChanged = FMath::FloorToInt(NewProgress * 100.f) != FMath::FloorToInt(GrowthProgress * 100.f);
	GrowthProgress = NewProgress;
	if (bPercentChanged)
	{
		OnGrowthProgressChanged.Broadcast(this, GrowthProgress);
		FInteractableEvents::NotifyInfoChanged(this);
	}
}

bool AFish::GetInfoProgress_Implementation(float& OutProgress, FText& OutLabel)
{
	if (IsAdult())
	{
		return false;
	}
	OutProgress = GrowthProgress;
	OutLabel = FText::FromString(TEXT("Growth"));
	return true;
}

FText AFish::GetName_Implementation()
{
	return FText::FromString("Fish");
}

FText AFish::GetDescription_Implementation()
{
	FString StimulatedStr = bIsStimulated ? TEXT("Yes") : TEXT("No");
	const FString StageStr = IsAdult()
		? FString(TEXT("Adult"))
		: FString::Printf(TEXT("Fry (%d%%)"), FMath::FloorToInt(GrowthProgress * 100.f));
	FString Desc = FString::Printf(TEXT("Stage: %s\nHunger Level: %d\nStimulated: %s"), *StageStr, Hunger, *StimulatedStr);
    
	return FText::FromString(Desc);
}
