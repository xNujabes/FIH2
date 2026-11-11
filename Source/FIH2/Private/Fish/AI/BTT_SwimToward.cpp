// Fill out your copyright notice in the Description page of Project Settings.


#include "Fish/AI/BTT_SwimToward.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "System/Eatable.h"

UBTT_SwimToward::UBTT_SwimToward()
{
	NodeName = "Swim Toward using FishSteeringMovement";
	bNotifyTick = true;
}

uint16 UBTT_SwimToward::GetInstanceMemorySize() const
{
	return sizeof(FSwimTaskMemory);
}

EBTNodeResult::Type UBTT_SwimToward::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	// La mémoire de nœud est de la mémoire brute : on construit explicitement la structure.
	FSwimTaskMemory* MyMemory = new(NodeMemory) FSwimTaskMemory();

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

	MyMemory->CachedPawn = ControlledPawn;
	MyMemory->AcceptanceRadiusSq = AcceptanceRadius * AcceptanceRadius;

	if (UObject* TargetObject = BlackboardComp->GetValueAsObject(TargetKey.SelectedKeyName))
	{
		if (AActor* TargetActor = Cast<AActor>(TargetObject))
		{
			// Stale target (already eaten...): forget it so the tree falls back to the next branch.
			if (const IEatable* Eatable = Cast<IEatable>(TargetActor))
			{
				if (!Eatable->IsEdible())
				{
					BlackboardComp->ClearValue(TargetKey.SelectedKeyName);
					return EBTNodeResult::Failed;
				}
			}
			MyMemory->TargetActor = TargetActor;
			MyMemory->bIsTrackingActor = true;
		}
		else
		{
			return EBTNodeResult::Failed;
		}
	}
	else
	{
		MyMemory->TargetLocation = BlackboardComp->GetValueAsVector(TargetKey.SelectedKeyName);
		MyMemory->bIsTrackingActor = false;
	}

	MyMemory->TimeRunning = 0.0f;
	MyMemory->WindowStartLocation = ControlledPawn->GetActorLocation();
	MyMemory->WindowTime = 0.0f;

	return EBTNodeResult::InProgress;
}

void UBTT_SwimToward::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	FSwimTaskMemory* MyMemory = CastInstanceNodeMemory<FSwimTaskMemory>(NodeMemory);
	APawn* Pawn = MyMemory->CachedPawn.Get();
	if (!Pawn)
	{
		// Le poisson a disparu (détruit, dépossédé) : on termine la tâche au lieu de la laisser en attente.
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	// The target actor vanished or was eaten while swimming (by this fish or another one): stop at once.
	// The decorators only abort lower-priority branches, so the task has to end itself.
	if (MyMemory->bIsTrackingActor)
	{
		AActor* TrackedActor = MyMemory->TargetActor.Get();
		const IEatable* Eatable = Cast<IEatable>(TrackedActor);
		if (!TrackedActor || (Eatable && !Eatable->IsEdible()))
		{
			if (UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent())
			{
				BlackboardComp->ClearValue(TargetKey.SelectedKeyName);
			}
			FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
			return;
		}
	}

	if (Timeout > 0.0f)
	{
		MyMemory->TimeRunning += DeltaSeconds;
		if (MyMemory->TimeRunning >= Timeout)
		{
			FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
			return;
		}
	}

	// Blocked (e.g. facing a wall) and not getting anywhere: give up so the tree picks a new target.
	if (StuckCheckTime > 0.0f)
	{
		MyMemory->WindowTime += DeltaSeconds;
		if (MyMemory->WindowTime >= StuckCheckTime)
		{
			const float Moved = FVector::Dist(Pawn->GetActorLocation(), MyMemory->WindowStartLocation);
			if (Moved < StuckMinDistance)
			{
				FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
				return;
			}
			MyMemory->WindowStartLocation = Pawn->GetActorLocation();
			MyMemory->WindowTime = 0.0f;
		}
	}

	FVector CurrentTargetLoc = MyMemory->bIsTrackingActor && MyMemory->TargetActor.IsValid()
		                           ? MyMemory->TargetActor->GetActorLocation()
		                           : MyMemory->TargetLocation;

	FVector ToTarget = CurrentTargetLoc - Pawn->GetActorLocation();
	float DistSq = ToTarget.SizeSquared();

	if (DistSq <= MyMemory->AcceptanceRadiusSq)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		return;
	}

	if (DistSq > UE_SMALL_NUMBER)
	{
		Pawn->AddMovementInput(ToTarget * FMath::InvSqrt(DistSq), 1.0f);
	}
}
