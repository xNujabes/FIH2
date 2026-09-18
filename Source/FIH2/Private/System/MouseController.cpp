// Fill out your copyright notice in the Description page of Project Settings.


#include "System/MouseController.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "InputAction.h"
#include "PhysicsEngine/PhysicsHandleComponent.h"
#include "System/Interactable.h"


AMouseController::AMouseController()
{
	PhysicsHandle = CreateDefaultSubobject<UPhysicsHandleComponent>(TEXT("PhysicsHandle"));

	bIsDragging = false;
	GrabbedActor = nullptr;
	bShowMouseCursor = true;
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;
}

void AMouseController::BeginPlay()
{
	Super::BeginPlay();
	bShowMouseCursor = true;
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;

	if (ULocalPlayer* LocalPlayer = Cast<ULocalPlayer>(Player))
	{
		if (UEnhancedInputLocalPlayerSubsystem* InputSystem = LocalPlayer->GetSubsystem<
			UEnhancedInputLocalPlayerSubsystem>())
		{
			if (!InputMapping.IsNull())
			{
				InputSystem->AddMappingContext(InputMapping.LoadSynchronous(), 1);
			}
		}
	}
}

void AMouseController::SetupInputComponent()
{
	Super::SetupInputComponent();

	UEnhancedInputComponent* Input = Cast<UEnhancedInputComponent>(InputComponent);

	Input->BindAction(ClickAction, ETriggerEvent::Started, this, &AMouseController::OnMouseClick);
	Input->BindAction(ClickAction, ETriggerEvent::Completed, this, &AMouseController::OnMouseRelease);
}

void AMouseController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (bIsDragging && PhysicsHandle->GetGrabbedComponent())
	{
		TArray<FHitResult> OutHits;

		FCollisionQueryParams TraceParams = FCollisionQueryParams(FName(TEXT("MouseDragTrace")), true);
		TraceParams.AddIgnoredActor(GrabbedActor);
		FVector WorldLocation, WorldDirection;
		DeprojectMousePositionToWorld(WorldLocation, WorldDirection);
		FVector TraceEnd = WorldLocation + (WorldDirection * TraceRange);

		GetWorld()->LineTraceMultiByChannel(OutHits, WorldLocation, TraceEnd, ECC_Visibility, TraceParams);

		UE_LOG(LogTemp, Warning, TEXT("%d"), OutHits.Num());
		if (OutHits.Num() > 1)
		{
			PhysicsHandle->SetTargetLocation(OutHits[0].ImpactPoint);
		}
		else if (OutHits.Num() == 1)
		{
			PhysicsHandle->SetTargetLocation(OutHits[0].ImpactPoint);
		}
		else
		{
			PhysicsHandle->SetTargetLocation(WorldLocation + (WorldDirection * (TraceRange - 4500.f)));
		}
	}
}

void AMouseController::OnMouseClick()
{
	TArray<FHitResult> OutHits;

	FCollisionQueryParams TraceParams = FCollisionQueryParams(FName(TEXT("MouseClick")), true);

	FVector WorldLocation, WorldDirection;
	DeprojectMousePositionToWorld(WorldLocation, WorldDirection);
	FVector TraceEnd = WorldLocation + (WorldDirection * TraceRange);

	GetWorld()->LineTraceMultiByChannel(OutHits, WorldLocation, TraceEnd, ECC_Visibility, TraceParams);
	UE_LOG(LogTemp, Warning, TEXT("Click, hits %d"), OutHits.Num());

#if ENABLE_DRAW_DEBUG
	GetWorld()->DebugDrawTraceTag = "MouseClick";
#endif;
	if (OutHits.Num() > 0)
	{
		AActor* HitActor;
		for (FHitResult HitResult : OutHits)
		{
			HitActor = HitResult.GetActor();
			UE_LOG(LogTemp, Warning, TEXT("Actor: %s"), *HitActor->GetName());
			UE_LOG(LogTemp, Warning, TEXT("Hit Actor: %s | Hit Component: %s"),
			       *HitActor->GetName(), *HitResult.GetComponent()->GetName());
			if (HitActor->Implements<UInteractable>())
			{
				UE_LOG(LogTemp, Warning, TEXT("SUCCESS: Interface found! Grabbing Bone: %s"),
				       *HitResult.BoneName.ToString());
				GrabbedActor = HitActor;
				bIsDragging = true;

				IInteractable::Execute_OnGrab(HitActor);
				UPrimitiveComponent* RootComp = Cast<UPrimitiveComponent>(HitActor->GetRootComponent());

				if (RootComp)
				{
					PhysicsHandle->GrabComponentAtLocation(RootComp, NAME_None, HitResult.ImpactPoint);
					RootComp->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Overlap);
				}
				break;
			}
			UE_LOG(LogTemp, Error, TEXT("FAILED: %s does NOT implement UInteractable!"), *HitActor->GetName());
		}
	}
}

void AMouseController::OnMouseRelease()
{
	PhysicsHandle->ReleaseComponent();
	GrabbedActor = nullptr;
	bIsDragging = false;
}
