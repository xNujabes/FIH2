// Fill out your copyright notice in the Description page of Project Settings.

#include "Inanimate/Kelp.h"
#include "Bonus/Food.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "PhysicsEngine/PhysicsConstraintComponent.h"
#include "System/SupportSurface.h"
#include "DrawDebugHelpers.h"
#include "HAL/IConsoleManager.h"
#include "LandscapeProxy.h"
#include "System/FIH2Log.h"

// Tracé de debug du rayon d'accrochage du kelp (désactivé par défaut). En jeu : fih2.Kelp.DrawAttachDebug 1
static TAutoConsoleVariable<bool> CVarKelpDrawAttachDebug(
	TEXT("fih2.Kelp.DrawAttachDebug"),
	false,
	TEXT("Dessine le rayon d'accrochage des kelps (vert = sol trouvé, rouge = rien)."));

AKelp::AKelp()
{
	Mesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	
	AttachConstraint = CreateDefaultSubobject<UPhysicsConstraintComponent>(TEXT("AttachConstraint"));
	AttachConstraint->SetupAttachment(Mesh);
	AttachConstraint->SetDisableCollision(true);
	AttachConstraint->SetLinearXLimit(LCM_Locked, 0.f);
	AttachConstraint->SetLinearYLimit(LCM_Locked, 0.f);
	AttachConstraint->SetLinearZLimit(LCM_Locked, 0.f);
	AttachConstraint->SetAngularSwing1Limit(ACM_Locked, 0.f);
	AttachConstraint->SetAngularSwing2Limit(ACM_Locked, 0.f);
	AttachConstraint->SetAngularTwistLimit(ACM_Locked, 0.f);
	
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
}

void AKelp::BeginPlay()
{
	Super::BeginPlay();

	// TODO: Kelp produces oxygen (integrate with ecosystem / oxygen manager)
	AttachConstraint->BreakConstraint();
	// Démarrage : tracé long partant d'au-dessus, le kelp est posé directement sur le relief.
	TryAttachWithTrace(SpawnAttachTraceUp, SpawnAttachTraceDown, false);
	SetActorTickEnabled(!bIsAttached);

	// Kelp posé au-dessus du sol : il tombe, on réessaie à intervalles réguliers jusqu'à ce qu'il se pose.
	if (!bIsAttached && SpawnAttachRetries > 0 && ReattachRetryDelay > 0.f)
	{
		SpawnAttachAttemptsLeft = SpawnAttachRetries;
		GetWorldTimerManager().SetTimer(ReattachTimerHandle, this, &AKelp::HandleReattachRetry, ReattachRetryDelay, false);
	}
	
	if (bStartMature)
	{
		GrowthProgress = 1.0f;
		SetActorScale3D(MatureScale);
		OnMatured();
	}
	else
	{
		StartGrowth();
	}
}

void AKelp::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(GrowthTimerHandle);
	GetWorldTimerManager().ClearTimer(FruitSpawnTimerHandle);
	GetWorldTimerManager().ClearTimer(ReattachTimerHandle);

	Super::EndPlay(EndPlayReason);
}

void AKelp::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bIsAttached || !Mesh || !Mesh->IsSimulatingPhysics())
	{
		return;
	}

	const FVector Up = Mesh->GetUpVector();
	
	FVector Axis = FVector::CrossProduct(Up, FVector::UpVector);
	if (Axis.IsNearlyZero() && FVector::DotProduct(Up, FVector::UpVector) < 0.f)
	{
		Axis = Mesh->GetRightVector();
	}

	const FVector AngVel = Mesh->GetPhysicsAngularVelocityInRadians();
	const FVector Torque = Axis * UprightStrength - AngVel * UprightDamping;

	Mesh->AddTorqueInRadians(Torque, NAME_None, true);
}

bool AKelp::TryAttach()
{
	return TryAttachWithTrace(AttachTraceUp, AttachTraceDown, true);
}

bool AKelp::TryAttachWithTrace(float TraceUp, float TraceDown, bool bScaleBySize)
{
	if (bIsAttached) return true;
	const FTransform MeshXf = Mesh->GetComponentTransform();
	const FVector Base = MeshXf.TransformPosition(BaseLocalOffset); // includes scale and rotation

	const float SizeScale = bScaleBySize ? FMath::Abs(MeshXf.GetScale3D().Z) : 1.f;
	const FVector Start = Base + FVector::UpVector * TraceUp * SizeScale;
	const FVector End = Base - FVector::UpVector * TraceDown * SizeScale;

	FCollisionQueryParams Params(SCENE_QUERY_STAT(KelpAttach), false, this);
	FHitResult Hit;
	const bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, Start, End, AttachTraceChannel, Params);

	if (CVarKelpDrawAttachDebug.GetValueOnGameThread())
	{
		DrawDebugLine(GetWorld(), Start, End, bHit ? FColor::Green : FColor::Red, false, 5.f, 0, 1.f);
	}

	if (!bHit)
	{
		UE_LOG(LogKelp, Warning, TEXT("[Kelp %s] Attach FAILED: ray hit nothing (base %s, up %.0f / down %.0f cm)"),
			*GetNameSafe(this), *Base.ToString(), TraceUp * SizeScale, TraceDown * SizeScale);
		return false;
	}

	AActor* SupportActor = Hit.GetActor();
	UPrimitiveComponent* SupportComp = Hit.GetComponent();
	UE_LOG(LogKelp, Verbose, TEXT("[Kelp %s] Ray hit actor=%s comp=%s"),
		*GetNameSafe(this), *GetNameSafe(SupportActor), *GetNameSafe(SupportComp));

	if (!SupportActor || !SupportComp)
	{
		return false;
	}
	bool bIsValidSupport = false;
	
	if (SupportActor->Implements<USupportSurface>())
	{
		bIsValidSupport = ISupportSurface::Execute_CanSupportAttachment(SupportActor, this, Hit);
	}
	else if (SupportComponentTag != NAME_None && SupportComp->ComponentHasTag(SupportComponentTag))
	{
		// Composant marqué comme support (ex. le fond de la cuve), même si l'acteur ne l'est pas.
		bIsValidSupport = true;
	}

	if (!bIsValidSupport)
	{
		UE_LOG(LogKelp, Warning, TEXT("[Kelp %s] Attach FAILED: %s (%s) does not implement SupportSurface, has no '%s' component tag and is not a Landscape"),
		   *GetNameSafe(this), *GetNameSafe(SupportActor), *GetNameSafe(SupportComp), *SupportComponentTag.ToString());
		return false;
	}
	if (!Mesh->IsSimulatingPhysics())
	{
		UE_LOG(LogKelp, Warning, TEXT("[Kelp %s] Attach FAILED: kelp mesh is not simulating"), *GetNameSafe(this));
		return false;
	}

	// Snap: upright, base on the impact point
	const FRotator NewRot(0.f, GetActorRotation().Yaw, 0.f);
	const FTransform Rotated(NewRot, FVector::ZeroVector, GetActorScale3D());
	const FVector NewLoc = Hit.ImpactPoint - Rotated.TransformPosition(BaseLocalOffset);

	Mesh->SetPhysicsLinearVelocity(FVector::ZeroVector);
	Mesh->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
	SetActorLocationAndRotation(NewLoc, NewRot, false, nullptr, ETeleportType::TeleportPhysics);

	// Rigid joint at the base
	AttachConstraint->SetWorldLocation(Hit.ImpactPoint);
	AttachConstraint->SetConstrainedComponents(SupportComp, NAME_None, Mesh, NAME_None);

	if (!AttachConstraint->ConstraintInstance.IsValidConstraintInstance())
	{
		UE_LOG(LogKelp, Warning,
			TEXT("[Kelp %s] Attach FAILED: constraint invalid. %s probably has no physics body (collision must be 'Query and Physics')"),
			*GetNameSafe(this),*GetNameSafe(SupportComp));
		return false;
	}

	SetActorTickEnabled(false);
	bIsAttached = true;
	AttachedSupport = SupportActor;
	StrainTimer = 0.f;
	UE_LOG(LogKelp, Verbose, TEXT("[Kelp %s] Attached to %s"), *GetNameSafe(this), *GetNameSafe(SupportActor));
	OnAttachedToSupport.Broadcast(this);
	return true;
}

void AKelp::Detach()
{
	if (!bIsAttached)
	{
		return;
	}

	AttachConstraint->BreakConstraint();
	bIsAttached = false;
	AttachedSupport = nullptr;
	StrainTimer = 0.f;
	Mesh->WakeAllRigidBodies();
	SetActorTickEnabled(true);
	OnDetachedFromSupport.Broadcast(this);
}

void AKelp::ReportGrabStrain(float Strain, float DeltaTime)
{
	// Le kelp est de nouveau tenu : on annule la tentative d'attache différée (et celles d'après l'apparition).
	SpawnAttachAttemptsLeft = 0;
	if (ReattachTimerHandle.IsValid())
	{
		GetWorldTimerManager().ClearTimer(ReattachTimerHandle);
	}

	if (!bIsAttached)
	{
		return;
	}

	if (Strain > DetachStrainThreshold)
	{
		StrainTimer += DeltaTime;
	}
	else
	{
		StrainTimer = FMath::Max(0.f, StrainTimer - DeltaTime * StrainRecoveryMultiplier);
	}

	if (StrainTimer >= DetachHoldTime)
	{
		Detach();
	}
}

void AKelp::NotifyReleased()
{
	StrainTimer = 0.f;
	if (bIsAttached)
	{
		return;
	}

	// Tentative immédiate, puis une seconde après ReattachRetryDelay (le kelp a pu se poser entre-temps).
	if (!TryAttach() && ReattachRetryDelay > 0.f)
	{
		GetWorldTimerManager().SetTimer(ReattachTimerHandle, this, &AKelp::HandleReattachRetry, ReattachRetryDelay, false);
	}
}

void AKelp::HandleReattachRetry()
{
	// Tentatives d'après l'apparition : même tracé long qu'au démarrage ; après un relâchement : tracé court.
	const bool bAttached = bIsAttached || (SpawnAttachAttemptsLeft > 0
		? TryAttachWithTrace(SpawnAttachTraceUp, SpawnAttachTraceDown, false)
		: TryAttach());
	if (bAttached)
	{
		SpawnAttachAttemptsLeft = 0;
		return;
	}

	// Encore en chute après l'apparition : nouvelle tentative plus tard.
	if (SpawnAttachAttemptsLeft > 0 && ReattachRetryDelay > 0.f)
	{
		--SpawnAttachAttemptsLeft;
		GetWorldTimerManager().SetTimer(ReattachTimerHandle, this, &AKelp::HandleReattachRetry, ReattachRetryDelay, false);
	}
}

void AKelp::StartGrowth()
{
	bIsMature = false;
	GrowthProgress = 0.0f;
	CurrentGrowthTime = 0.0f;

	const FVector StartingScale = bScaleHeightOnly ? FVector(MatureScale.X, MatureScale.Y, InitialScale.Z) : InitialScale;
	SetActorScale3D(StartingScale);

	if (GrowthDuration <= 0.0f)
	{
		GrowthProgress = 1.0f;
		SetActorScale3D(MatureScale);
		OnMatured();
	}
	else
	{
		GetWorldTimerManager().SetTimer(GrowthTimerHandle, this, &AKelp::UpdateGrowth, GrowthUpdateInterval, true);
	}
}

void AKelp::SetGrowthProgress(float InProgress)
{
	GrowthProgress = FMath::Clamp(InProgress, 0.0f, 1.0f);
	CurrentGrowthTime = GrowthProgress * GrowthDuration;

	FVector NewScale;
	if (bScaleHeightOnly)
	{
		NewScale = FVector(MatureScale.X, MatureScale.Y, FMath::Lerp(InitialScale.Z, MatureScale.Z, GrowthProgress));
	}
	else
	{
		NewScale = FMath::Lerp(InitialScale, MatureScale, GrowthProgress);
	}
	SetActorScale3D(NewScale);

	OnGrowthProgressChanged.Broadcast(this, GrowthProgress);
	FInteractableEvents::NotifyInfoChanged(this);

	if (GrowthProgress >= 1.0f && !bIsMature)
	{
		GetWorldTimerManager().ClearTimer(GrowthTimerHandle);
		OnMatured();
	}
}

void AKelp::UpdateGrowth()
{
	CurrentGrowthTime += GrowthUpdateInterval;
	GrowthProgress = FMath::Clamp(CurrentGrowthTime / GrowthDuration, 0.0f, 1.0f);

	FVector NewScale;
	if (bScaleHeightOnly)
	{
		NewScale = FVector(MatureScale.X, MatureScale.Y, FMath::Lerp(InitialScale.Z, MatureScale.Z, GrowthProgress));
	}
	else
	{
		NewScale = FMath::Lerp(InitialScale, MatureScale, GrowthProgress);
	}
	SetActorScale3D(NewScale);

	OnGrowthProgressChanged.Broadcast(this, GrowthProgress);
	FInteractableEvents::NotifyInfoChanged(this);

	if (GrowthProgress >= 1.0f)
	{
		GetWorldTimerManager().ClearTimer(GrowthTimerHandle);
		OnMatured();
	}
}

void AKelp::OnMatured()
{
	if (bIsMature)
	{
		return;
	}

	bIsMature = true;
	GrowthProgress = 1.0f;

	// TODO: Kelp produces oxygen (e.g. register with AOxygenManager or broadcast oxygen production per interval)

	OnKelpMatured.Broadcast(this);
	FInteractableEvents::NotifyInfoChanged(this);
	StartFruitSpawning();
}

void AKelp::StartFruitSpawning()
{
	GetWorldTimerManager().ClearTimer(FruitSpawnTimerHandle);

	if (SpawnInterval > 0.0f)
	{
		GetWorldTimerManager().SetTimer(
			FruitSpawnTimerHandle,
			this,
			&AKelp::HandleSpawnFruitTimer,
			SpawnInterval,
			true,
			InitialSpawnDelay
		);
	}
}

void AKelp::HandleSpawnFruitTimer()
{
	TrySpawnFruit();
}

int32 AKelp::GetActiveFruitCount() const
{
	int32 Count = 0;
	for (AFood* Fruit : ActiveFruits)
	{
		if (IsValid(Fruit) &&
		    Fruit->GetGrowthState() != EFoodGrowthState::Detached &&
		    Fruit->GetGrowthState() != EFoodGrowthState::Decaying)
		{
			Count++;
		}
	}
	return Count;
}

TSubclassOf<AFood> AKelp::GetFruitClassToSpawn() const
{
	if (FruitClasses.Num() > 0)
	{
		const int32 RandomIndex = FMath::RandRange(0, FruitClasses.Num() - 1);
		if (FruitClasses[RandomIndex])
		{
			return FruitClasses[RandomIndex];
		}
	}

	return FruitClass;
}

bool AKelp::GetAvailableSpawnTransform(FName& OutSocketName, FTransform& OutTransform)
{
	ActiveFruits.RemoveAll([](AFood* Fruit)
	{
		return !IsValid(Fruit) ||
		       Fruit->GetGrowthState() == EFoodGrowthState::Detached ||
		       Fruit->GetGrowthState() == EFoodGrowthState::Decaying;
	});

	if (ActiveFruits.Num() >= MaxFruitCount)
	{
		return false;
	}
	
	TSet<FName> OccupiedSockets;
	for (AFood* Fruit : ActiveFruits)
	{
		if (IsValid(Fruit))
		{
			const FName Occupied = Fruit->GetAttachedSocketName();
			if (Occupied != NAME_None)
			{
				OccupiedSockets.Add(Occupied);
			}
		}
	}
	
	if (FruitSocketNames.Num() > 0)
	{
		for (const FName& CandidateSocket : FruitSocketNames)
		{
			if (!OccupiedSockets.Contains(CandidateSocket))
			{
				OutSocketName = CandidateSocket;
				if (Mesh && Mesh->DoesSocketExist(CandidateSocket))
				{
					OutTransform = Mesh->GetSocketTransform(CandidateSocket);
				}
				else
				{
					OutTransform = GetActorTransform();
				}
				return true;
			}
		}
		return false;
	}
	
	if (Mesh && Mesh->GetStaticMesh())
	{
		const TArray<FName> AllMeshSockets = Mesh->GetAllSocketNames();
		for (const FName& CandidateSocket : AllMeshSockets)
		{
			if (!OccupiedSockets.Contains(CandidateSocket))
			{
				OutSocketName = CandidateSocket;
				OutTransform = Mesh->GetSocketTransform(CandidateSocket);
				return true;
			}
		}
		if (AllMeshSockets.Num() > 0)
		{
			return false; 
		}
	}
	
	if (FruitSpawnOffsets.Num() > 0)
	{
		const int32 AvailableIndex = ActiveFruits.Num();
		if (AvailableIndex < FruitSpawnOffsets.Num())
		{
			OutSocketName = NAME_None;
			const FVector WorldLocation = GetActorTransform().TransformPosition(FruitSpawnOffsets[AvailableIndex]);
			OutTransform = FTransform(GetActorRotation(), WorldLocation, FVector::OneVector);
			return true;
		}
		return false;
	}
	
	OutSocketName = NAME_None;
	const FVector WorldLocation = GetActorTransform().TransformPosition(DefaultFruitSpawnOffset);
	OutTransform = FTransform(GetActorRotation(), WorldLocation, FVector::OneVector);
	return true;
}

bool AKelp::TrySpawnFruit()
{
	if (!bIsMature)
	{
		return false;
	}
	
	ActiveFruits.RemoveAll([](AFood* Fruit)
	{
		return !IsValid(Fruit) ||
		       Fruit->GetGrowthState() == EFoodGrowthState::Detached ||
		       Fruit->GetGrowthState() == EFoodGrowthState::Decaying;
	});

	if (ActiveFruits.Num() >= MaxFruitCount)
	{
		return false;
	}

	const TSubclassOf<AFood> ClassToSpawn = GetFruitClassToSpawn();
	if (!ClassToSpawn)
	{
		return false;
	}

	FName TargetSocketName = NAME_None;
	FTransform SpawnTransform;
	if (!GetAvailableSpawnTransform(TargetSocketName, SpawnTransform))
	{
		return false;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.Instigator = GetInstigator();
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AFood* NewFruit = World->SpawnActor<AFood>(ClassToSpawn, SpawnTransform, SpawnParams);
	if (NewFruit)
	{
		NewFruit->AttachToKelp(this, TargetSocketName);
		ActiveFruits.Add(NewFruit);

		NewFruit->OnDetached.AddDynamic(this, &AKelp::HandleFruitDetached);
		NewFruit->OnDestroyed.AddDynamic(this, &AKelp::HandleFruitDestroyed);

		OnFruitSpawned.Broadcast(this, NewFruit);
		FInteractableEvents::NotifyInfoChanged(this);
		return true;
	}

	return false;
}

void AKelp::HandleFruitDetached(AFood* DetachedFruit)
{
	if (DetachedFruit)
	{
		ActiveFruits.Remove(DetachedFruit);
		OnFruitDetached.Broadcast(this, DetachedFruit);
		FInteractableEvents::NotifyInfoChanged(this);
	}
}

void AKelp::HandleFruitDestroyed(AActor* DestroyedActor)
{
	AFood* DestroyedFruit = Cast<AFood>(DestroyedActor);
	if (DestroyedFruit)
	{
		ActiveFruits.Remove(DestroyedFruit);
	}
}

FText AKelp::GetName_Implementation()
{
	return FText::FromString("Kelp");
}

FText AKelp::GetDescription_Implementation()
{
	FString MatureStr = bIsMature ? TEXT("Fully Grown") : TEXT("Growing");
	int32 Percent = FMath::RoundToInt(GrowthProgress * 100.0f);
    
	FString Desc = FString::Printf(TEXT("Status: %s (%d%%)\nFruits: %d / %d"), 
		*MatureStr, Percent, GetActiveFruitCount(), MaxFruitCount);
        
	return FText::FromString(Desc);
}

bool AKelp::GetInfoProgress_Implementation(float& OutProgress, FText& OutLabel)
{
	if (bIsMature)
	{
		return false;
	}
	OutProgress = GrowthProgress;
	OutLabel = FText::FromString(TEXT("Growth"));
	return true;
}
