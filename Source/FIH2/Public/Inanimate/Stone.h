// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Inanimate.h"
#include "System/SupportSurface.h"
#include "Stone.generated.h"

UCLASS()
class FIH2_API AStone : public AInanimate, public ISupportSurface
{
	GENERATED_BODY()

public:
	AStone();
	
	virtual FText GetName_Implementation() override;
	virtual FText GetDescription_Implementation() override;
	
};
