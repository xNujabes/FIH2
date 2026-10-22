// Fill out your copyright notice in the Description page of Project Settings.


#include "Inanimate/Stone.h"


AStone::AStone()
{
	PrimaryActorTick.bCanEverTick = false;
	Mesh->SetSimulatePhysics(true);
}

