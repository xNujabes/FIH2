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
	Mesh->SetSimulatePhysics(true);
}

// Called when the game starts or when spawned
void AFish::BeginPlay()
{
	Super::BeginPlay();
}

void AFish::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	OnHungerChanged.Broadcast();
}

// Called every frame
void AFish::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
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


bool AFish::IsStimulated() const
{
	return bIsStimulated;
}

void AFish::SetIsStimulated(bool bNewStimulated)
{
	bIsStimulated = bNewStimulated;
}
