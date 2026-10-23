// Fill out your copyright notice in the Description page of Project Settings.

#include "Bonus/Food.h"
#include "Components/BoxComponent.h"
#include "Engine/World.h"
#include "Fish/Fish.h"
#include "GeometryCollection/GeometryCollectionComponent.h"
#include "Inanimate/Kelp.h"
#include "TimerManager.h"
#include "Perception/AIPerceptionStimuliSourceComponent.h"
#include "Perception/AISense_Sight.h"

AFood::AFood()
{
	PrimaryActorTick.bCanEverTick = false;

	PhysicsProxy = CreateDefaultSubobject<UBoxComponent>(TEXT("FoodPhysicsProxy"));
	SetRootComponent(PhysicsProxy);
	PhysicsProxy->SetSimulatePhysics(true);
	PhysicsProxy->SetCollisionProfileName(TEXT("PhysicsActor"));
	Mesh->SetupAttachment(PhysicsProxy);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	
	GeometryCollection = CreateDefaultSubobject<UGeometryCollectionComponent>(TEXT("GeometryCollection"));
	GeometryCollection->SetupAttachment(PhysicsProxy);
	GeometryCollection->SetSimulatePhysics(false);
	GeometryCollection->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GeometryCollection->SetVisibility(false);
	
	StimuliSource = CreateDefaultSubobject<UAIPerceptionStimuliSourceComponent>(TEXT("AIPerceptionStimuliSource"));
	StimuliSource->bAutoRegister = false;
}

void AFood::BeginPlay()
{
	Super::BeginPlay();

	if (PhysicsProxy)
	{
		PhysicsProxy->OnComponentHit.AddDynamic(this, &AFood::OnPhysicsProxyHit);
	}

	CurrentGrowthState = InitialGrowthState;

	if (CurrentGrowthState == EFoodGrowthState::Growing)
	{
		if (GetAttachParentActor())
		{
			StartGrowth();
		}
		else if (!bRequireAttachParentToGrow)
		{
			StartGrowth();
		}
		else
		{
			// Standalone food placed in level without parent: default to detached physics behavior
			CurrentGrowthState = EFoodGrowthState::Detached;
			if (PhysicsProxy)
			{
				PhysicsProxy->SetCollisionProfileName(TEXT("PhysicsActor"));
				PhysicsProxy->SetSimulatePhysics(true);
				PhysicsProxy->SetNotifyRigidBodyCollision(true);
			}
		}
	}
	else if (CurrentGrowthState == EFoodGrowthState::FullyGrown)
	{
		if (PhysicsProxy)
		{
			PhysicsProxy->SetCollisionProfileName(TEXT("PhysicsActor"));
			PhysicsProxy->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
			PhysicsProxy->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
		}
		GetWorldTimerManager().SetTimer(FullyGrownTimerHandle, this, &AFood::DetachFruit, FullyGrownDuration, false);
	}
	else if (CurrentGrowthState == EFoodGrowthState::Detached)
	{
		if (PhysicsProxy)
		{
			PhysicsProxy->SetCollisionProfileName(TEXT("PhysicsActor"));
			PhysicsProxy->SetSimulatePhysics(true);
			PhysicsProxy->SetNotifyRigidBodyCollision(true);
		}

		if (!bDecayStartsOnLanding)
		{
			StartDecay();
		}
		else if (DetachedFallbackDecayDelay > 0.0f)
		{
			GetWorldTimerManager().SetTimer(DetachedFallbackTimerHandle, this, &AFood::StartDecay, DetachedFallbackDecayDelay, false);
		}
	}
	else if (CurrentGrowthState == EFoodGrowthState::Decaying)
	{
		StartDecay();
	}
}

void AFood::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(GrowthTimerHandle);
	GetWorldTimerManager().ClearTimer(FullyGrownTimerHandle);
	GetWorldTimerManager().ClearTimer(DetachedFallbackTimerHandle);
	GetWorldTimerManager().ClearTimer(DecayTimerHandle);

	Super::EndPlay(EndPlayReason);
}

void AFood::AttachToKelp(AKelp* InKelp, FName InSocketName)
{
	if (!InKelp)
	{
		return;
	}

	AttachParentKelp = InKelp;
	AttachedSocketName = InSocketName;

	// Clear any pending timers
	GetWorldTimerManager().ClearTimer(GrowthTimerHandle);
	GetWorldTimerManager().ClearTimer(FullyGrownTimerHandle);
	GetWorldTimerManager().ClearTimer(DetachedFallbackTimerHandle);
	GetWorldTimerManager().ClearTimer(DecayTimerHandle);

	if (PhysicsProxy)
	{
		PhysicsProxy->SetSimulatePhysics(false);
		// Keep scale absolute so food never inherits kelp's growth scale or stretching
		PhysicsProxy->SetAbsolute(false, false, true);

		if (bInteractableOnlyWhenFullyGrown)
		{
			PhysicsProxy->SetCollisionProfileName(TEXT("NoCollision"));
		}
		else
		{
			PhysicsProxy->SetCollisionProfileName(TEXT("PhysicsActor"));
			PhysicsProxy->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
			PhysicsProxy->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
		}

		PhysicsProxy->IgnoreActorWhenMoving(InKelp, true);
	}

	// Attach to kelp's mesh component
	if (InSocketName != NAME_None && InKelp->Mesh && InKelp->Mesh->DoesSocketExist(InSocketName))
	{
		AttachToComponent(InKelp->Mesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, InSocketName);
	}
	else if (InKelp->Mesh)
	{
		AttachToComponent(InKelp->Mesh, FAttachmentTransformRules::KeepWorldTransform);
	}

	SetActorScale3D(InitialGrowthScale);

	CurrentGrowthState = EFoodGrowthState::Growing;
	StartGrowth();
}

void AFood::StartGrowth()
{
	CurrentGrowthTime = 0.0f;
	SetGrowthState(EFoodGrowthState::Growing);

	if (PhysicsProxy)
	{
		PhysicsProxy->SetAbsolute(false, false, true);
	}
	SetActorScale3D(InitialGrowthScale);

	if (GrowthDuration <= 0.0f)
	{
		if (StimuliSource)
		{
			StimuliSource->RegisterForSense(UAISense_Sight::StaticClass());
		}
		SetActorScale3D(MatureGrowthScale);
		SetGrowthState(EFoodGrowthState::FullyGrown);

		if (PhysicsProxy)
		{
			// Make interactable when fully grown: enable query collision and block ECC_Visibility for mouse trace/grab
			PhysicsProxy->SetCollisionProfileName(TEXT("PhysicsActor"));
			PhysicsProxy->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
			PhysicsProxy->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
			if (AttachParentKelp)
			{
				PhysicsProxy->IgnoreActorWhenMoving(AttachParentKelp, true);
			}
		}

		OnFullyGrown.Broadcast(this);
		OnGrowthStateChanged.Broadcast(this, EFoodGrowthState::FullyGrown);
		GetWorldTimerManager().SetTimer(FullyGrownTimerHandle, this, &AFood::DetachFruit, FullyGrownDuration, false);
		return;
	}

	GetWorldTimerManager().SetTimer(GrowthTimerHandle, this, &AFood::UpdateGrowth, GrowthUpdateInterval, true);
}

void AFood::UpdateGrowth()
{
	CurrentGrowthTime += GrowthUpdateInterval;
	const float Alpha = FMath::Clamp(CurrentGrowthTime / GrowthDuration, 0.0f, 1.0f);
	SetActorScale3D(FMath::Lerp(InitialGrowthScale, MatureGrowthScale, Alpha));

	if (Alpha >= 1.0f)
	{
		GetWorldTimerManager().ClearTimer(GrowthTimerHandle);
		SetGrowthState(EFoodGrowthState::FullyGrown);
		SetActorScale3D(MatureGrowthScale);

		if (PhysicsProxy)
		{
			// Make interactable when fully grown: enable query collision and block ECC_Visibility for mouse trace/grab
			PhysicsProxy->SetCollisionProfileName(TEXT("PhysicsActor"));
			PhysicsProxy->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
			PhysicsProxy->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
			if (AttachParentKelp)
			{
				PhysicsProxy->IgnoreActorWhenMoving(AttachParentKelp, true);
			}
		}
		
		if (StimuliSource)
		{
			StimuliSource->RegisterForSense(UAISense_Sight::StaticClass());
		}

		OnFullyGrown.Broadcast(this);
		OnGrowthStateChanged.Broadcast(this, EFoodGrowthState::FullyGrown);

		GetWorldTimerManager().SetTimer(FullyGrownTimerHandle, this, &AFood::DetachFruit, FullyGrownDuration, false);
	}
}

void AFood::DetachFruit()
{
	if (bIsEaten)
	{
		return;
	}

	if (CurrentGrowthState != EFoodGrowthState::Growing && CurrentGrowthState != EFoodGrowthState::FullyGrown)
	{
		return;
	}

	GetWorldTimerManager().ClearTimer(GrowthTimerHandle);
	GetWorldTimerManager().ClearTimer(FullyGrownTimerHandle);

	if (PhysicsProxy)
	{
		PhysicsProxy->SetAbsolute(false, false, false);
	}

	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	SetActorScale3D(MatureGrowthScale);

	if (PhysicsProxy)
	{
		PhysicsProxy->SetCollisionProfileName(TEXT("PhysicsActor"));
		PhysicsProxy->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		PhysicsProxy->SetSimulatePhysics(true);
		PhysicsProxy->SetNotifyRigidBodyCollision(true);

		if (AttachParentKelp)
		{
			PhysicsProxy->IgnoreActorWhenMoving(AttachParentKelp, false);
		}
	}

	SetGrowthState(EFoodGrowthState::Detached);
	OnDetached.Broadcast(this);
	OnGrowthStateChanged.Broadcast(this, EFoodGrowthState::Detached);

	if (!bDecayStartsOnLanding)
	{
		StartDecay();
	}
	else if (DetachedFallbackDecayDelay > 0.0f)
	{
		GetWorldTimerManager().SetTimer(DetachedFallbackTimerHandle, this, &AFood::StartDecay, DetachedFallbackDecayDelay, false);
	}
}

void AFood::OnPhysicsProxyHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (CurrentGrowthState == EFoodGrowthState::Detached && bDecayStartsOnLanding && !bIsEaten)
	{
		// Avoid triggering landing decay when colliding with the kelp it just detached from
		if (OtherActor && OtherActor == AttachParentKelp)
		{
			return;
		}

		StartDecay();
	}
}

void AFood::StartDecay()
{
	if (CurrentGrowthState == EFoodGrowthState::Decaying || bIsEaten)
	{
		return;
	}

	GetWorldTimerManager().ClearTimer(DetachedFallbackTimerHandle);

	SetGrowthState(EFoodGrowthState::Decaying);

	// TODO: Decaying food increases environment pollution (notify pollution manager / subsystem)

	OnDecayStarted.Broadcast(this);
	OnGrowthStateChanged.Broadcast(this, EFoodGrowthState::Decaying);

	if (DecayDuration <= 0.0f)
	{
		HandleDecayExpired();
	}
	else
	{
		GetWorldTimerManager().SetTimer(DecayTimerHandle, this, &AFood::HandleDecayExpired, DecayDuration, false);
	}
}

void AFood::HandleDecayExpired()
{
	if (!bIsEaten)
	{
		Destroy();
	}
}

void AFood::SetGrowthState(EFoodGrowthState NewState)
{
	CurrentGrowthState = NewState;
}

AKelp* AFood::GetAttachParentKelp() const
{
	return AttachParentKelp;
}

void AFood::TriggerEat_Implementation(AActor* EatInstigator, FVector HitLocation, float DamageAmount)
{
	if (bIsEaten)
	{
		return;
	}

	if (CurrentGrowthState == EFoodGrowthState::Growing && !bCanBeEatenWhileGrowing)
	{
		return;
	}

	AFish* FishInstigator = Cast<AFish>(EatInstigator);
	if (FishInstigator)
	{
		bIsEaten = true;

		// Clear all active timers
		GetWorldTimerManager().ClearTimer(GrowthTimerHandle);
		GetWorldTimerManager().ClearTimer(FullyGrownTimerHandle);
		GetWorldTimerManager().ClearTimer(DetachedFallbackTimerHandle);
		GetWorldTimerManager().ClearTimer(DecayTimerHandle);

		// If eaten while still attached to Kelp, detach cleanly and notify listeners
		if (GetAttachParentActor())
		{
			DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
			OnDetached.Broadcast(this);
		}

		FishInstigator->AddHunger(HungerRestoreAmount);
		Mesh->SetVisibility(false);
		PhysicsProxy->SetSimulatePhysics(false);
		PhysicsProxy->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Execute_TriggerBreak(this, EatInstigator, HitLocation, DamageAmount);
		
		SetLifeSpan(EatenLifeSpan); 
	}
}

void AFood::TriggerBreak_Implementation(AActor* BreakInstigator, FVector HitLocation, float DamageAmount)
{
	if (GeometryCollection)
	{
		GeometryCollection->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
		GeometryCollection->SetVisibility(true);
		GeometryCollection->SetCollisionProfileName(TEXT("Destructible"));
		GeometryCollection->SetSimulatePhysics(true);
		
		AFood* RawThis = this;
		GetWorldTimerManager().SetTimerForNextTick([RawThis, HitLocation, DamageAmount]()
		{
			UE_LOG(LogTemp, Warning, TEXT("TriggerBreak Timer expired"));
			if (!IsValid(RawThis) || !RawThis->GeometryCollection) return;
			RawThis->GeometryCollection->AddRadialImpulse(
				HitLocation, RawThis->BreakImpulseRadius, DamageAmount,
				ERadialImpulseFalloff::RIF_Linear, true);
			RawThis->GeometryCollection->ApplyExternalStrain(
				RawThis->StrainItemIndex,
				HitLocation,
				RawThis->BreakImpulseRadius,
				0,
				RawThis->BreakPropagationFactor,
				RawThis->BreakExternalStrain);
			
			UE_LOG(LogTemp, Warning, TEXT("Impulses should have been done"));
		});
	}
}

void AFood::OnHover_Implementation()
{
}

void AFood::OnGrab_Implementation()
{
	// Detach immediately if grabbed while attached to kelp so physics simulation starts for PhysicsHandle
	if (CurrentGrowthState == EFoodGrowthState::FullyGrown || CurrentGrowthState == EFoodGrowthState::Growing)
	{
		DetachFruit();
	}
}

void AFood::OnDropped_Implementation()
{
	if (PhysicsProxy)
	{
		PhysicsProxy->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	}
}

void AFood::OnThrown_Implementation(FVector ThrowVelocity)
{
	if (PhysicsProxy)
	{
		PhysicsProxy->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
		PhysicsProxy->SetPhysicsLinearVelocity(ThrowVelocity);
	}
	
	
}

FText AFood::GetName_Implementation()
{
	return FText::FromString("Kelp Fruit");
}

FText AFood::GetDescription_Implementation()
{
	FString StateStr;
	switch(CurrentGrowthState) 
	{
	case EFoodGrowthState::Growing: StateStr = TEXT("Growing"); break;
	case EFoodGrowthState::FullyGrown: StateStr = TEXT("Fresh"); break;
	case EFoodGrowthState::Detached: StateStr = TEXT("Dropped"); break;
	case EFoodGrowthState::Decaying: StateStr = TEXT("Decaying"); break;
	default: StateStr = TEXT("Unknown"); break;
	}

	FString Desc = FString::Printf(TEXT("State: %s\nNutrition: +%.1f Hunger"), *StateStr, HungerRestoreAmount);
	return FText::FromString(Desc);
}
