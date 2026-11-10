// Fill out your copyright notice in the Description page of Project Settings.


#include "Fish/PredatorFish.h"


// Sets default values
APredatorFish::APredatorFish()
{
	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void APredatorFish::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void APredatorFish::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

// Called to bind functionality to input
void APredatorFish::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

