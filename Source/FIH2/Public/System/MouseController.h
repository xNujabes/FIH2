// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "InputMappingContext.h"
#include "MouseController.generated.h"

// AI Given solution 
class UPhysicsHandleComponent;
class UInputAction;
/**
 * 
 */
UCLASS()
class FIH2_API AMouseController : public APlayerController
{
	GENERATED_BODY()

public:
	AMouseController();

	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Interaction")
	UPhysicsHandleComponent* PhysicsHandle;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TSoftObjectPtr<UInputMappingContext> InputMapping;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* ClickAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction")
	float TraceRange = 2500.f;


	bool bIsDragging;
	AActor* GrabbedActor;


	void OnMouseClick();
	void OnMouseRelease();
};
