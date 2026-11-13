// Fill out your copyright notice in the Description page of Project Settings.


#include "Fish/AI/BTT_FindRandomLocation.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Components/BoxComponent.h"
#include "Engine/World.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/OverlapResult.h"

namespace
{
	/** True if the sphere at Location touches a real obstacle. Volumes the fish itself sits in (water box...) don't count. */
	bool IsBlockedAt(UWorld* World, const FVector& Location, const FVector& FishLocation,
	                 const FCollisionObjectQueryParams& ObjectParams, const FCollisionShape& Shape,
	                 const FCollisionQueryParams& QueryParams)
	{
		TArray<FOverlapResult> Overlaps;
		World->OverlapMultiByObjectType(Overlaps, Location, FQuat::Identity, ObjectParams, Shape, QueryParams);

		const FCollisionShape Probe = FCollisionShape::MakeSphere(1.0f);
		for (const FOverlapResult& Overlap : Overlaps)
		{
			const UPrimitiveComponent* Comp = Overlap.GetComponent();
			if (Comp && !Comp->OverlapComponent(FishLocation, FQuat::Identity, Probe))
			{
				return true; // a real obstacle, not the environment the fish is already inside
			}
		}
		return false;
	}

	/**
	 * Hauteur du sol à la verticale de Location : premier obstacle WorldStatic rencontré en descendant depuis
	 * Location + HalfHeight. Le fond de la cuve est une surface en triangles : sous le relief, l'espace est « vide »
	 * pour le moteur, donc un point sous le sable passe le test d'overlap. Les volumes qui contiennent le point
	 * (boîte d'eau...) sont ignorés.
	 */
	bool FindGroundZ(UWorld* World, const FVector& Location, float HalfHeight,
	                 const FCollisionQueryParams& QueryParams, float& OutGroundZ)
	{
		TArray<FHitResult> Hits;
		const FCollisionObjectQueryParams GroundTypes(ECC_WorldStatic);
		World->LineTraceMultiByObjectType(Hits, Location + FVector(0.f, 0.f, HalfHeight),
		                                  Location - FVector(0.f, 0.f, HalfHeight), GroundTypes, QueryParams);

		const FCollisionShape Probe = FCollisionShape::MakeSphere(1.0f);
		for (const FHitResult& Hit : Hits) // triés du plus proche au plus loin
		{
			const UPrimitiveComponent* Comp = Hit.GetComponent();
			if (Hit.bStartPenetrating || !Comp || Comp->OverlapComponent(Location, FQuat::Identity, Probe))
			{
				continue;
			}
			OutGroundZ = Hit.ImpactPoint.Z;
			return true;
		}
		return false;
	}

	/** Nearest real obstacle on the path (start overlaps ignored, same idea as above). */
	bool IsPathBlocked(UWorld* World, const FVector& Start, const FVector& End,
	                   const FCollisionObjectQueryParams& ObjectParams, const FCollisionShape& Shape,
	                   const FCollisionQueryParams& QueryParams)
	{
		TArray<FHitResult> Hits;
		World->SweepMultiByObjectType(Hits, Start, End, FQuat::Identity, ObjectParams, Shape, QueryParams);
		for (const FHitResult& Hit : Hits)
		{
			if (!Hit.bStartPenetrating)
			{
				return true;
			}
		}
		return false;
	}
}

UBTT_FindRandomLocation::UBTT_FindRandomLocation()
{
	NodeName = "Find Random 3D Location";
}

EBTNodeResult::Type UBTT_FindRandomLocation::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();

	if (!AIController || !BlackboardComp)
	{
		return EBTNodeResult::Failed;
	}

	APawn* ControlledPawn = AIController->GetPawn();
	if (!ControlledPawn)
	{
		return EBTNodeResult::Failed;
	}
	const FVector Origin = ControlledPawn->GetActorLocation();

	UWorld* World = ControlledPawn->GetWorld();

	// Bounds of the water tank (optional): candidates are clamped inside.
	FBox Bounds(ForceInit);
	if (UBoxComponent* WaterBox = Cast<UBoxComponent>(BlackboardComp->GetValueAsObject(WaterTankBoxCollisionActor.SelectedKeyName)))
	{
		Bounds = WaterBox->Bounds.GetBox();
	}

	// Obstacles = everything of these object types (same default as UFishSteeringMovement).
	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_WorldStatic);
	ObjectParams.AddObjectTypesToQuery(ECC_WorldDynamic);

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(ControlledPawn);
	const FCollisionShape ClearanceShape = FCollisionShape::MakeSphere(ClearanceRadius);

	// Draw random candidates until one is outside every obstacle (and reachable in a straight line).
	FVector RandomPoint = Origin;
	bool bFound = false;
	for (int32 Attempt = 0; Attempt < MaxAttempts && !bFound; ++Attempt)
	{
		FVector RandomDirection = FMath::VRand();
		RandomDirection.Z = FMath::Clamp(RandomDirection.Z, -0.5f, 0.5f);
		RandomDirection.Normalize();
		const float RandomDistance = FMath::FRandRange(200.0f, SearchRadius);

		FVector Candidate = Origin + (RandomDirection * RandomDistance);
		Candidate.Z = FMath::Clamp(Candidate.Z, Origin.Z - MaxZOffset, Origin.Z + MaxZOffset);
		if (Bounds.IsValid)
		{
			Candidate = Bounds.GetClosestPointTo(Candidate);
		}

		// 0. Pas de cible sous le relief (ni collée au sol).
		float GroundZ = 0.f;
		if (World && FindGroundZ(World, Candidate, GroundTraceHalfHeight, QueryParams, GroundZ)
			&& Candidate.Z < GroundZ + MinHeightAboveGround)
		{
			continue;
		}

		// 1. The point itself must not be inside (or touching) an obstacle.
		if (World && IsBlockedAt(World, Candidate, Origin, ObjectParams, ClearanceShape, QueryParams))
		{
			continue;
		}

		// 2. Optionally, nothing must stand between the fish and the point (no target behind a wall).
		if (bRequireClearPath && World)
		{
			if (IsPathBlocked(World, Origin, Candidate, ObjectParams, ClearanceShape, QueryParams))
			{
				continue;
			}
		}

		RandomPoint = Candidate;
		bFound = true;
	}

	if (!bFound)
	{
		// No free point found (e.g. the fish itself starts embedded in terrain): never freeze, fall back to an
		// unchecked random point so the fish keeps moving and can free itself.
		FVector RandomDirection = FMath::VRand();
		RandomDirection.Z = FMath::Clamp(RandomDirection.Z, -0.5f, 0.5f);
		RandomDirection.Normalize();
		RandomPoint = Origin + RandomDirection * FMath::FRandRange(200.0f, SearchRadius);
		RandomPoint.Z = FMath::Clamp(RandomPoint.Z, Origin.Z - MaxZOffset, Origin.Z + MaxZOffset);
		if (Bounds.IsValid)
		{
			RandomPoint = Bounds.GetClosestPointTo(RandomPoint);
		}
		// Même sans vérification d'obstacle, on ne vise jamais sous le relief.
		float GroundZ = 0.f;
		if (World && FindGroundZ(World, RandomPoint, GroundTraceHalfHeight, QueryParams, GroundZ))
		{
			RandomPoint.Z = FMath::Max(RandomPoint.Z, GroundZ + MinHeightAboveGround);
			if (Bounds.IsValid)
			{
				RandomPoint = Bounds.GetClosestPointTo(RandomPoint);
			}
		}
		UE_LOG(LogTemp, Verbose, TEXT("FindRandomLocation: no free point after %d attempts, unchecked fallback"), MaxAttempts);
	}

	BlackboardComp->SetValueAsVector(TargetLocationKey.SelectedKeyName, RandomPoint);
	return EBTNodeResult::Succeeded;
}
