// Fill out your copyright notice in the Description page of Project Settings.


#include "System/MouseController.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "InputAction.h"
#include "Fish/Fish.h"
#include "Inanimate/Kelp.h"
#include "PhysicsEngine/PhysicsHandleComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Components/MeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "System/Interactable.h"
#include "System/GameplayEvents.h"


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
				SelectActor(GrabbedActor);
				OnActorClicked.Broadcast(GrabbedActor);
				break;
			}
		}
	}

	// Clic dans le vide ou sur un décor non interactable : on ferme le panneau d'infos.
	if (!GrabbedActor)
	{
		ClearSelection();
	}
}

// ============================================================================ Sélection

void AMouseController::SelectActor(AActor* Actor)
{
	if (Actor == SelectedActor)
	{
		return;
	}
	if (SelectedActor)
	{
		SelectedActor->OnEndPlay.RemoveDynamic(this, &AMouseController::HandleSelectedEndPlay);
		RemoveHighlight();
	}

	SelectedActor = Actor;

	if (SelectedActor)
	{
		SelectedActor->OnEndPlay.AddUniqueDynamic(this, &AMouseController::HandleSelectedEndPlay);
		ApplyHighlight(SelectedActor);
		// Abonnement aux changements d'infos uniquement pendant qu'un panneau est ouvert.
		if (!InfoChangedHandle.IsValid())
		{
			InfoChangedHandle = FInteractableEvents::OnInfoChanged().AddUObject(this, &AMouseController::HandleInteractableInfoChanged);
		}
	}
	else if (InfoChangedHandle.IsValid())
	{
		FInteractableEvents::OnInfoChanged().Remove(InfoChangedHandle);
		InfoChangedHandle.Reset();
	}

	OnSelectionChanged.Broadcast(SelectedActor);
	FGameplayEvents::OnActorSelected().Broadcast(SelectedActor);
}

void AMouseController::ClearSelection()
{
	if (SelectedActor)
	{
		SelectActor(nullptr);
	}
}

void AMouseController::HandleInteractableInfoChanged(AActor* Actor)
{
	if (Actor && Actor == SelectedActor)
	{
		OnSelectedInfoChanged.Broadcast(SelectedActor);
	}
}

void AMouseController::HandleSelectedEndPlay(AActor* Actor, EEndPlayReason::Type EndPlayReason)
{
	if (Actor == SelectedActor)
	{
		ClearSelection();
	}
}

void AMouseController::ApplyHighlight(AActor* Actor)
{
	if (!Actor || !SelectionOverlayMaterial)
	{
		return;
	}
	TArray<UMeshComponent*> Meshes;
	Actor->GetComponents<UMeshComponent>(Meshes);
	for (UMeshComponent* MeshComp : Meshes)
	{
		if (!MeshComp || !MeshComp->IsVisible())
		{
			continue;
		}
		SavedOverlays.Add(MeshComp, MeshComp->GetOverlayMaterial());
		MeshComp->SetOverlayMaterial(SelectionOverlayMaterial);
	}
}

void AMouseController::RemoveHighlight()
{
	for (const auto& Pair : SavedOverlays)
	{
		if (UMeshComponent* MeshComp = Pair.Key.Get())
		{
			MeshComp->SetOverlayMaterial(Pair.Value.Get());
		}
	}
	SavedOverlays.Reset();
}

void AMouseController::FocusOnSelection()
{
	if (!SelectedActor)
	{
		return;
	}
	FVector Origin, Extent;
	SelectedActor->GetActorBounds(true, Origin, Extent);
	if (StartFocus(Origin, FMath::Max(Extent.Size(), 1.f)))
	{
		FGameplayEvents::OnFocusUsed().Broadcast(SelectedActor);
	}
}

void AMouseController::FocusOnActors(const TArray<AActor*>& Actors)
{
	FBox Bounds(ForceInit);
	for (const AActor* Actor : Actors)
	{
		if (Actor)
		{
			FVector Origin, Extent;
			Actor->GetActorBounds(true, Origin, Extent);
			Bounds += FBox(Origin - Extent, Origin + Extent);
		}
	}
	if (Bounds.IsValid)
	{
		StartFocus(Bounds.GetCenter(), FMath::Max(Bounds.GetExtent().Size(), 1.f));
	}
}

bool AMouseController::StartFocus(const FVector& Origin, float Radius)
{
	APawn* ViewPawn = GetPawn();
	USpringArmComponent* Arm = ViewPawn ? ViewPawn->FindComponentByClass<USpringArmComponent>() : nullptr;
	if (!Arm)
	{
		return false;
	}

	FocusArm = Arm;
	FocusStartLocation = Arm->GetComponentLocation();
	FocusEndLocation = Origin;
	FocusStartArmLength = Arm->TargetArmLength;
	FocusEndArmLength = FMath::Max(FocusMinArmLength, Radius * FocusDistanceFactor);
	FocusElapsed = 0.f;

	if (FocusDuration <= KINDA_SMALL_NUMBER)
	{
		UpdateFocus(0.f);
	}
	return true;
}

void AMouseController::SetPersistentHighlight(AActor* Actor, UMaterialInterface* Material)
{
	if (!Actor)
	{
		return;
	}
	TArray<UMeshComponent*> Meshes;
	Actor->GetComponents<UMeshComponent>(Meshes);
	for (UMeshComponent* MeshComp : Meshes)
	{
		if (!MeshComp)
		{
			continue;
		}
		if (Actor == SelectedActor)
		{
			// La surbrillance de sélection reste ; on change ce qui sera remis à la désélection.
			if (TWeakObjectPtr<UMaterialInterface>* Saved = SavedOverlays.Find(MeshComp))
			{
				*Saved = Material;
			}
		}
		else if (!Material || MeshComp->IsVisible())
		{
			MeshComp->SetOverlayMaterial(Material);
		}
	}
}

void AMouseController::UpdateFocus(float DeltaTime)
{
	if (FocusElapsed < 0.f)
	{
		return;
	}
	USpringArmComponent* Arm = FocusArm.Get();
	if (!Arm)
	{
		FocusElapsed = -1.f;
		return;
	}

	FocusElapsed += DeltaTime;
	const float Alpha = FocusDuration > KINDA_SMALL_NUMBER ? FMath::Clamp(FocusElapsed / FocusDuration, 0.f, 1.f) : 1.f;
	const float Eased = FMath::InterpEaseInOut(0.f, 1.f, Alpha, 2.f);

	Arm->SetWorldLocation(FMath::Lerp(FocusStartLocation, FocusEndLocation, Eased));
	Arm->TargetArmLength = FMath::Lerp(FocusStartArmLength, FocusEndArmLength, Eased);

	if (Alpha >= 1.f)
	{
		FocusElapsed = -1.f;
	}
}

void AMouseController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (InfoChangedHandle.IsValid())
	{
		FInteractableEvents::OnInfoChanged().Remove(InfoChangedHandle);
		InfoChangedHandle.Reset();
	}
	RemoveHighlight();
	Super::EndPlay(EndPlayReason);
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

	if (Released)
	{
		FGameplayEvents::OnActorReleased().Broadcast(Released);
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
	UpdateFocus(DeltaTime);
}