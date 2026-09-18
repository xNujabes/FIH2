// Fill out your copyright notice in the Description page of Project Settings.


#include "Fish/AI/BTT_SwimToward.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"

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
    FSwimTaskMemory* MyMemory = CastInstanceNodeMemory<FSwimTaskMemory>(NodeMemory);
    
    AAIController* AIController = OwnerComp.GetAIOwner();
    UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
    if (!AIController || !BlackboardComp) return EBTNodeResult::Failed;

    APawn* ControlledPawn = AIController->GetPawn();
    if (!ControlledPawn) return EBTNodeResult::Failed;

    MyMemory->CachedPawn = ControlledPawn;
    MyMemory->AcceptanceRadiusSq = AcceptanceRadius * AcceptanceRadius; 

    if (UObject* TargetObject = BlackboardComp->GetValueAsObject(TargetKey.SelectedKeyName))
    {
        if (AActor* TargetActor = Cast<AActor>(TargetObject))
        {
            MyMemory->TargetActor = TargetActor;
            MyMemory->bIsTrackingActor = true;
        }
        else return EBTNodeResult::Failed;
    }
    else
    {
        MyMemory->TargetLocation = BlackboardComp->GetValueAsVector(TargetKey.SelectedKeyName);
        MyMemory->bIsTrackingActor = false;
    }
    
    MyMemory->TimeRunning = 0.0f;
    
    return EBTNodeResult::InProgress;
}

void UBTT_SwimToward::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
    FSwimTaskMemory* MyMemory = CastInstanceNodeMemory<FSwimTaskMemory>(NodeMemory);
    APawn* Pawn = MyMemory->CachedPawn.Get();
    if (!Pawn) return;
    
    if (Timeout > 0.0f)
    {
        MyMemory->TimeRunning += DeltaSeconds;
        if (MyMemory->TimeRunning >= Timeout)
        {
            FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
            return;
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