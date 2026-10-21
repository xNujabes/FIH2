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
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnActorClicked, AActor*, ClickedActor);
UCLASS()
class FIH2_API AMouseController : public APlayerController
{
	GENERATED_BODY()

public:
	AMouseController();

	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TSoftObjectPtr<UInputMappingContext> InputMapping;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* ClickAction;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* LookAction;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Interaction")
	UPhysicsHandleComponent* PhysicsHandle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	float TraceRange = 2500.f;
	
	UPROPERTY(BlueprintAssignable, BlueprintCallable, Category = "Interaction")
	FOnActorClicked OnActorClicked;
	
	UPROPERTY(EditAnywhere, Category="Interaction") 
	float GrabScrollSpeed = 0.05f;
	
	UPROPERTY(EditAnywhere, Category="Interaction")
	float MinGrabAlpha = 0.02f;

	float GrabAlpha = 0.5f;
	bool bIsDragging;
	bool bClickHeld;
	AActor* GrabbedActor;
	
	FHitResult ClickHitResult;

	UFUNCTION(BlueprintCallable) void AdjustGrabDistance(float AxisValue);
	UFUNCTION(BlueprintPure) bool GetIsDragging() const { return bIsDragging; }

	void UpdateDragTarget();
	void OnMouseClick();
	void OnMouseRelease();
	void OnMouseMoved();
	virtual void PlayerTick(float DeltaTime) override;
};
