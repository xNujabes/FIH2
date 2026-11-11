// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTT_SwimToward.generated.h"

/**
 * 
 */
struct FSwimTaskMemory
{
	TWeakObjectPtr<APawn> CachedPawn;
	TWeakObjectPtr<AActor> TargetActor;
	FVector TargetLocation = FVector::ZeroVector;
	float AcceptanceRadiusSq = 0.0f;
	bool bIsTrackingActor = false;
	float TimeRunning = 0.0f;
	// Stuck detection: position at the start of the current window and time spent in it.
	FVector WindowStartLocation = FVector::ZeroVector;
	float WindowTime = 0.0f;
};

UCLASS()
class FIH2_API UBTT_SwimToward : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTT_SwimToward();

	virtual uint16 GetInstanceMemorySize() const override;
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

protected:
	UPROPERTY(EditAnywhere, Category = "Targeting")
	FBlackboardKeySelector TargetKey;

	UPROPERTY(EditAnywhere, Category = "Movement")
	float AcceptanceRadius = 100.0f;

	UPROPERTY(EditAnywhere, Category = "Movement")
	float RotationInterpSpeed = 5.0f;

	UPROPERTY(EditAnywhere, Category = "Avoidance")
	float AvoidanceDistance = 300.0f;

	UPROPERTY(EditAnywhere, Category = "Avoidance")
	TEnumAsByte<ECollisionChannel> ObstacleChannel = ECC_WorldStatic;

	UPROPERTY(EditAnywhere, Category = "Avoidance")
	bool bDrawDebug = false;

	UPROPERTY(EditAnywhere, Category = "Node")
	float Timeout = 0.0f;

	/** Stuck detection: length (s) of the window in which the fish must make progress. 0 = disabled. */
	UPROPERTY(EditAnywhere, Category = "Stuck", meta = (ClampMin = "0.0"))
	float StuckCheckTime = 1.5f;

	/** Minimum distance (cm) the fish must cover during one window, otherwise the task fails (a new target is then picked). */
	UPROPERTY(EditAnywhere, Category = "Stuck", meta = (ClampMin = "0.0"))
	float StuckMinDistance = 100.0f;
};
