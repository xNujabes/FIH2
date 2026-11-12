// Fill out your copyright notice in the Description page of Project Settings.


#include "Fish/PredatorFish.h"


// Sets default values
APredatorFish::APredatorFish()
{
	PrimaryActorTick.bCanEverTick = false;
}

// Called when the game starts or when spawned
void APredatorFish::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called to bind functionality to input
void APredatorFish::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

