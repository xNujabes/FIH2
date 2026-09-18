// Fill out your copyright notice in the Description page of Project Settings.


#include "Fish/AI/BTT_FindRandomLocation.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Components/BoxComponent.h"

UBTT_FindRandomLocation::UBTT_FindRandomLocation()
{
	NodeName = "Find Random 3D Location";
}

EBTNodeResult::Type UBTT_FindRandomLocation::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();

	if (!AIController || !BlackboardComp)
	{
		return EBTNodeResult::Failed;
	}

	APawn* ControlledPawn = AIController->GetPawn();
	if (!ControlledPawn)
	{
		return EBTNodeResult::Failed;
	}
	FVector Origin = ControlledPawn->GetActorLocation();
	FVector RandomDirection = FMath::VRand();
	RandomDirection.Z = FMath::Clamp(RandomDirection.Z, -0.5f, 0.5f);
	RandomDirection.Normalize();
	float RandomDistance = FMath::FRandRange(200.0f, SearchRadius);
	FVector RandomPoint = Origin + (RandomDirection * RandomDistance);
	RandomPoint.Z = FMath::Clamp(RandomPoint.Z, Origin.Z - MaxZOffset, Origin.Z + MaxZOffset);

	UObject* BoundsObject = BlackboardComp->GetValueAsObject(WaterTankBoxCollisionActor.SelectedKeyName);
	if (UBoxComponent* WaterBox = Cast<UBoxComponent>(BoundsObject))
	{
		FBox Bounds = WaterBox->Bounds.GetBox();
		RandomPoint = Bounds.GetClosestPointTo(RandomPoint);
	}

	BlackboardComp->SetValueAsVector(TargetLocationKey.SelectedKeyName, RandomPoint);
	return EBTNodeResult::Succeeded;
}
