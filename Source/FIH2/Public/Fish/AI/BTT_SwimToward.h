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
	FVector TargetLocation;
	float AcceptanceRadiusSq;
	bool bIsTrackingActor;
	float TimeRunning;
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
};
