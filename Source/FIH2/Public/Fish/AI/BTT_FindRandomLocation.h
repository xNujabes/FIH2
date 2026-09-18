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
};