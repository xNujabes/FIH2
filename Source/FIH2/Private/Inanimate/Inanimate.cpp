// Fill out your copyright notice in the Description page of Project Settings.


#include "FIH2/Public/Inanimate/Inanimate.h"

AInanimate::AInanimate()
{
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	RootComponent = Mesh;
}
