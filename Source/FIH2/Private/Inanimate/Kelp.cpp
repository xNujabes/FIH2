// Fill out your copyright notice in the Description page of Project Settings.

#include "Inanimate/Kelp.h"
#include "Bonus/Food.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"

AKelp::AKelp()
{
	PrimaryActorTick.bCanEverTick = false;
	Mesh->SetSimulatePhysics(false);
	Mesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));
}

void AKelp::BeginPlay()
{
	Super::BeginPlay();

	// TODO: Kelp produces oxygen (integrate with ecosystem / oxygen manager)

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

	Super::EndPlay(EndPlayReason);
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
