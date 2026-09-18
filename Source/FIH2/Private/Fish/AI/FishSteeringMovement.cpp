// Fill out your copyright notice in the Description page of Project Settings.


#include "Fish/AI/FishSteeringMovement.h"

void UFishSteeringMovement::TickComponent(float DeltaTime, enum ELevelTick TickType,
                                          FActorComponentTickFunction* ThisTickFunction)
{
	FVector PendingInput = GetPendingInputVector();

	if (!PendingInput.IsNearlyZero() && PawnOwner)
	{
		FVector SafeDirection = CalculateAvoidance(PendingInput);

		FRotator TargetRotation = SafeDirection.Rotation();
		FRotator NewRotation = FMath::RInterpTo(PawnOwner->GetActorRotation(), TargetRotation, DeltaTime,
		                                        RotationInterpSpeed);
		PawnOwner->SetActorRotation(NewRotation);

		PendingInput = SafeDirection;
	}

	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

FVector UFishSteeringMovement::CalculateAvoidance(const FVector& DesiredDirection)
{
	if (!PawnOwner)
	{
		return DesiredDirection;
	}

	UWorld* World = GetWorld();
	FVector PawnLoc = PawnOwner->GetActorLocation();
	FVector ForwardVector = PawnOwner->GetActorForwardVector();

	FHitResult Hit;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(PawnOwner);

	if (!World->LineTraceSingleByChannel(Hit, PawnLoc, PawnLoc + (ForwardVector * AvoidanceDistance), ObstacleChannel,
	                                     QueryParams))
	{
		return DesiredDirection;
	}

	static const FRotator WhiskerOffsets[] = {
		FRotator(45.0f, 45.0f, 0.0f), FRotator(45.0f, -45.0f, 0.0f),
		FRotator(-45.0f, 45.0f, 0.0f), FRotator(-45.0f, -45.0f, 0.0f),
		FRotator(0.0f, 90.0f, 0.0f), FRotator(0.0f, -90.0f, 0.0f),
		FRotator(90.0f, 0.0f, 0.0f), FRotator(-90.0f, 0.0f, 0.0f)
	};

	for (const FRotator& Offset : WhiskerOffsets)
	{
		FVector WhiskerDir = Offset.RotateVector(ForwardVector);
		if (!World->LineTraceSingleByChannel(Hit, PawnLoc, PawnLoc + (WhiskerDir * AvoidanceDistance), ObstacleChannel,
		                                     QueryParams))
		{
			return WhiskerDir;
		}
	}

	return Hit.Normal;
}
