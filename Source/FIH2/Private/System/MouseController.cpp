// Fill out your copyright notice in the Description page of Project Settings.


#include "System/MouseController.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "InputAction.h"
#include "Fish/Fish.h"
#include "Inanimate/Kelp.h"
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
				InputSystem->AddMappingContext(InputMapping, 1);
			}
		}
	}
}

void AMouseController::SetupInputComponent()
{
	Super::SetupInputComponent();

	UEnhancedInputComponent* Input = Cast<UEnhancedInputComponent>(InputComponent);
	if (!Input)
	{
		UE_LOG(LogTemp, Error, TEXT("MouseController: InputComponent n'est pas un EnhancedInputComponent, aucune action liée."));
		return;
	}

	if (ClickAction)
	{
		Input->BindAction(ClickAction, ETriggerEvent::Started, this, &AMouseController::OnMouseClick);
		Input->BindAction(ClickAction, ETriggerEvent::Completed, this, &AMouseController::OnMouseRelease);
	}
	if (LookAction)
	{
		Input->BindAction(LookAction, ETriggerEvent::Triggered, this, &AMouseController::OnMouseMoved);
	}
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
				UPrimitiveComponent* RootComp = Cast<UPrimitiveComponent>(HitActor->GetRootComponent());
				if (!RootComp)
				{
					continue; // rien à saisir sur cet acteur
				}

				GrabbedActor = HitActor;
				ClickHitResult = HitResult; 

				// Fish are kinematic (moved only by FishSteeringMovement): let physics take over while held.
				if (AFish* Fish = Cast<AFish>(GrabbedActor))
				{
					Fish->SetHeldByPlayer(true);
				}
				
				// GetMass() exige une simulation physique active (sinon warning, ex. fruit encore accroché au kelp,
				// qui ne passe en physique qu'au OnGrab). CalculateMass() lit la masse depuis le body setup dans tous les cas.
				const float RawMass = RootComp->IsSimulatingPhysics() ? RootComp->GetMass() : RootComp->CalculateMass();
				const float Mass = FMath::Max(RawMass, KINDA_SMALL_NUMBER);
				
				const float Factor = FMath::Clamp(
					FMath::Pow(50 / Mass, 1.f),
					0.1f, 1.f);

				PhysicsHandle->SetLinearStiffness(BaseStifness.X * Factor);
				PhysicsHandle->SetAngularStiffness(BaseStifness.Y * Factor);
				PhysicsHandle->SetInterpolationSpeed(BaseInterpolationSpeed * Factor);
				
				GrabAlpha = FMath::Clamp(HitResult.Distance / TraceRange, MinGrabAlpha, 1.f);
				OnActorClicked.Broadcast(GrabbedActor);
				break;
			}
		}
	}
}

void AMouseController::AdjustGrabDistance(float AxisValue)
{
	if (!bIsDragging) return;
	GrabAlpha = FMath::Clamp(GrabAlpha + -AxisValue * GrabScrollSpeed, MinGrabAlpha, 1.f);
}

void AMouseController::OnMouseRelease()
{
	AActor* Released = GrabbedActor;
	
	PhysicsHandle->ReleaseComponent();
	PhysicsHandle->LinearStiffness = BaseStifness.X;
	PhysicsHandle->AngularStiffness = BaseStifness.Y;
	PhysicsHandle->InterpolationSpeed = BaseInterpolationSpeed;
	GrabbedActor = nullptr;
	bIsDragging = false;
	bClickHeld = false;
	
	if (AKelp* Kelp = Cast<AKelp>(Released))
	{
		Kelp->NotifyReleased();
	}

	if (AFish* Fish = Cast<AFish>(Released))
	{
		Fish->SetHeldByPlayer(false);
	}
	
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
			// Point saisi, en espace local du composant (sert à mesurer la tension du kelp).
			GrabLocalPoint = RootComp->GetComponentTransform().InverseTransformPosition(ClickHitResult.ImpactPoint);
			PhysicsHandle->GrabComponentAtLocation(RootComp, NAME_None, ClickHitResult.ImpactPoint);
		}
	}
	// La cible du drag est mise à jour à un seul endroit : UpdateDragTarget (appelé à chaque PlayerTick).
}

void AMouseController::UpdateDragTarget(float DeltaTime)
{
	if (!bIsDragging || !PhysicsHandle->GetGrabbedComponent()) return;

	FVector WorldLocation, WorldDirection;
	if (!DeprojectMousePositionToWorld(WorldLocation, WorldDirection)) return;

	const FVector TraceEnd = WorldLocation + WorldDirection * TraceRange;
	FVector Target = FMath::Lerp(WorldLocation, TraceEnd, GrabAlpha);
	
	FHitResult Hit;
	FCollisionQueryParams Params(FName(TEXT("MouseDragTrace")), true);
	Params.AddIgnoredActor(GrabbedActor);
	if (GetWorld()->LineTraceSingleByChannel(Hit, WorldLocation, Target, ECC_Visibility, Params))
	{
		Target = Hit.ImpactPoint;
	}

	PhysicsHandle->SetTargetLocation(Target);
	
	if (AKelp* Kelp = Cast<AKelp>(GrabbedActor))
	{
		UPrimitiveComponent* Grabbed = PhysicsHandle->GetGrabbedComponent();
		const FVector GrabWorldPoint = Grabbed->GetComponentTransform().TransformPosition(GrabLocalPoint);
		Kelp->ReportGrabStrain(FVector::Dist(GrabWorldPoint, Target), DeltaTime);
	}
}

void AMouseController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);
	UpdateDragTarget(DeltaTime);
}