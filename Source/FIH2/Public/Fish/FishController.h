// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Runtime/AIModule/Classes/AIController.h"
#include "FishController.generated.h"

class UAIPerceptionComponent;
class UAISenseConfig_Sight;

UCLASS()
class FIH2_API AFishController : public AAIController
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AFishController();
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	UBehaviorTree* AIBehaviorTree;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	UBlackboardComponent* BlackboardComp;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI|Perception")
	UAIPerceptionComponent* FishPerceptionComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI|Perception")
	UAISenseConfig_Sight* SightConfig;


protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
};
