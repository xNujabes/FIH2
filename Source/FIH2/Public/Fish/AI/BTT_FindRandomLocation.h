// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTT_FindRandomLocation.generated.h"

/**
 * 
 */
UCLASS()
class FIH2_API UBTT_FindRandomLocation : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTT_FindRandomLocation();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

protected:
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector TargetLocationKey;

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector WaterTankBoxCollisionActor;


	UPROPERTY(EditAnywhere, Category = "Search")
	float SearchRadius = 1500.0f;

	UPROPERTY(EditAnywhere, Category = "Search")
	float MaxZOffset = 500.0f;

	/** Number of random points tried before giving up (the point must be outside every obstacle). */
	UPROPERTY(EditAnywhere, Category = "Search", meta = (ClampMin = "1"))
	int32 MaxAttempts = 10;

	/** Free space required around the target point (about the fish's half-width or a bit more). */
	UPROPERTY(EditAnywhere, Category = "Search", meta = (ClampMin = "1.0"))
	float ClearanceRadius = 60.0f;

	/** Reject targets that have an obstacle on the straight line from the fish (avoids aiming behind a wall). */
	UPROPERTY(EditAnywhere, Category = "Search")
	bool bRequireClearPath = true;

	/** Hauteur minimale [cm] de la cible au-dessus du sol (fond WorldStatic) : rejette les points sous le relief. */
	UPROPERTY(EditAnywhere, Category = "Search", meta = (ClampMin = "0.0"))
	float MinHeightAboveGround = 150.0f;

	/** Demi-hauteur [cm] du tracé vertical qui cherche le sol au-dessus et au-dessous de la cible. */
	UPROPERTY(EditAnywhere, Category = "Search", meta = (ClampMin = "0.0"))
	float GroundTraceHalfHeight = 20000.0f;
};
