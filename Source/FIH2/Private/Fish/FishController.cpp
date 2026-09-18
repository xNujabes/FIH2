// Fill out your copyright notice in the Description page of Project Settings.


#include "Fish/FishController.h"

#include "BrainComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "System/Eatable.h"


// Sets default values
AFishController::AFishController()
{
	PrimaryActorTick.bCanEverTick = false;

	FishPerceptionComp = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("FishPerceptionComp"));
	SetPerceptionComponent(*FishPerceptionComp);

	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
	SightConfig->SightRadius = 1500.0f;
	SightConfig->LoseSightRadius = 1800.0f;
	SightConfig->PeripheralVisionAngleDegrees = 180.0f;     
	SightConfig->DetectionByAffiliation.bDetectEnemies = true;
	SightConfig->DetectionByAffiliation.bDetectNeutrals = true;
	SightConfig->DetectionByAffiliation.bDetectFriendlies = true;

	FishPerceptionComp->ConfigureSense(*SightConfig);
	FishPerceptionComp->SetDominantSense(SightConfig->GetSenseImplementation());
}

// Called when the game starts or when spawned
void AFishController::BeginPlay()
{
	Super::BeginPlay();
	
	BlackboardComp = FindComponentByClass<UBlackboardComponent>();
	RunBehaviorTree(AIBehaviorTree);
	BrainComponent->StopLogic("Do not Start Until In Water");
}
