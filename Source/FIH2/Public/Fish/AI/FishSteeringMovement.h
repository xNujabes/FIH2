// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "FishSteeringMovement.generated.h"


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class FIH2_API UFishSteeringMovement : public UFloatingPawnMovement
{
	GENERATED_BODY()

public:
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) override;

protected:
	UPROPERTY(EditAnywhere, Category = "Steering")
	float RotationInterpSpeed = 5.0f;

	UPROPERTY(EditAnywhere, Category = "Avoidance")
	float AvoidanceDistance = 300.0f;

	UPROPERTY(EditAnywhere, Category = "Avoidance")
	TEnumAsByte<ECollisionChannel> ObstacleChannel = ECC_WorldStatic;

private:
	FVector CalculateAvoidance(const FVector& DesiredDirection);
};
