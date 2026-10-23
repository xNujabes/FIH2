// Fill out your copyright notice in the Description page of Project Settings.


#include "FIH2/Public/Inanimate/Sand.h"

// Sets default values
ASand::ASand()
{
	PrimaryActorTick.bCanEverTick = false;
	Mesh->SetSimulatePhysics(true);
}

FText ASand::GetName_Implementation()
{
	return FText::FromString("Sand");
}

FText ASand::GetDescription_Implementation()
{
	return FText::FromString("A pile of loose sand.");
}
