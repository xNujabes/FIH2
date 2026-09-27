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
			if (InputMapping)
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
	Input->BindAction(LookAction, ETriggerEvent::Triggered, this, &AMouseController::OnMouseMoved);
}


void AMouseController::OnMouseClick()
{
	bClickHeld = true;

	TArray<FHitResult> OutHits;
	FCollisionQueryParams TraceParams = FCollisionQueryParams(FName(TEXT("MouseClick")), true);

	FVector WorldLocation, WorldDirection;
	DeprojectMousePositionToWorld(WorldLocation, WorldDirection);
	FVector TraceEnd = WorldLocation + (WorldDirection * TraceRange);

	GetWorld()->LineTraceMultiByChannel(OutHits, WorldLocation, TraceEnd, ECC_Visibility, TraceParams);

	if (OutHits.Num() > 0)
	{
		for (FHitResult HitResult : OutHits)
		{
			AActor* HitActor = HitResult.GetActor();
			if (HitActor && HitActor->Implements<UInteractable>())
			{
				GrabbedActor = HitActor;
				ClickHitResult = HitResult; 
				
				OnActorClicked.Broadcast(GrabbedActor);
				break;
			}
		}
	}
}

void AMouseController::OnMouseRelease()
{
	PhysicsHandle->ReleaseComponent();
	GrabbedActor = nullptr;
	bIsDragging = false;
}

void AMouseController::OnMouseMoved()
{
	if (bClickHeld && GrabbedActor && !bIsDragging)
	{
		bIsDragging = true;

		IInteractable::Execute_OnGrab(GrabbedActor);
		UPrimitiveComponent* RootComp = Cast<UPrimitiveComponent>(GrabbedActor->GetRootComponent());

		if (RootComp)
		{
			PhysicsHandle->GrabComponentAtLocation(RootComp, NAME_None, ClickHitResult.ImpactPoint);
			RootComp->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Overlap);
		}
	}
	
	if (bIsDragging && PhysicsHandle->GetGrabbedComponent())
	{
		TArray<FHitResult> OutHits;
		FCollisionQueryParams TraceParams = FCollisionQueryParams(FName(TEXT("MouseDragTrace")), true);
		TraceParams.AddIgnoredActor(GrabbedActor);

		FVector WorldLocation, WorldDirection;
		DeprojectMousePositionToWorld(WorldLocation, WorldDirection);
		FVector TraceEnd = WorldLocation + (WorldDirection * TraceRange);

		GetWorld()->LineTraceMultiByChannel(OutHits, WorldLocation, TraceEnd, ECC_Visibility, TraceParams);

		if (OutHits.Num() > 0)
		{
			PhysicsHandle->SetTargetLocation(OutHits[0].ImpactPoint);
		}
		else
		{
			PhysicsHandle->SetTargetLocation(WorldLocation + (WorldDirection * (TraceRange - 4500.f)));
		}
	}
}