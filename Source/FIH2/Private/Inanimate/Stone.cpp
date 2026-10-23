// Fill out your copyright notice in the Description page of Project Settings.


#include "Inanimate/Stone.h"


AStone::AStone()
{
	PrimaryActorTick.bCanEverTick = false;
	Mesh->SetSimulatePhysics(true);
}

FText AStone::GetName_Implementation()
{
	return FText::FromString("Stone");
}

FText AStone::GetDescription_Implementation()
{
	return FText::FromString("A heavy, solid stone.");
}