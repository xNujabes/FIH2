// Fill out your copyright notice in the Description page of Project Settings.


#include "FIH2/Public/Inanimate/Sand.h"

// Sets default values
ASand::ASand()
{
	PrimaryActorTick.bCanEverTick = false;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	RootComponent = Mesh;

	Mesh->SetSimulatePhysics(true);
}
