// Fill out your copyright notice in the Description page of Project Settings.


#include "Fish/AI/FishSteeringMovement.h"

#include "DrawDebugHelpers.h"
#include "Math/RotationMatrix.h"

namespace
{
	/**
	 * Sweeps and returns the NEAREST real obstacle along the path.
	 * Objects the shape already overlaps at the start (the water volume the fish swims in, the sand it rests on...)
	 * are skipped: a single sweep would stop on them and hide every obstacle behind.
	 */
	bool SweepNearestObstacle(UWorld* World, FHitResult& OutHit, const FVector& Start, const FVector& End,
	                          const FCollisionObjectQueryParams& ObjectParams, const FCollisionShape& Shape,
	                          const FCollisionQueryParams& QueryParams)
	{
		TArray<FHitResult> Hits;
		World->SweepMultiByObjectType(Hits, Start, End, FQuat::Identity, ObjectParams, Shape, QueryParams);

		bool bFound = false;
		for (const FHitResult& Hit : Hits)
		{
			if (Hit.bStartPenetrating)
			{
				continue;
			}
			if (!bFound || Hit.Time < OutHit.Time)
			{
				OutHit = Hit;
				bFound = true;
			}
		}
		return bFound;
	}
}

void UFishSteeringMovement::TickComponent(float DeltaTime, enum ELevelTick TickType,
                                          FActorComponentTickFunction* ThisTickFunction)
{
	if (PawnOwner && UpdatedPrimitive)
	{
		if (UpdatedPrimitive->IsSimulatingPhysics())
		{
			// The player is holding the fish (physics handle): physics owns the body, steering stands aside.
			ConsumeInputVector();
			Velocity = FVector::ZeroVector;
		}
		else
		{
			// Take the raw "I want to go there" input...
			const FVector DesiredInput = ConsumeInputVector();

			if (!DesiredInput.IsNearlyZero())
			{
				const FVector DesiredDirection = DesiredInput.GetSafeNormal();
				const FVector SafeDirection = CalculateAvoidance(DesiredDirection);

				// ...and hand the corrected direction (same magnitude) to the real movement.
				// bForce: the input already passed IsMoveInputIgnored() when it was first added.
				AddInputVector(SafeDirection * DesiredInput.Size(), true);

				const FRotator NewRotation = FMath::RInterpTo(PawnOwner->GetActorRotation(),
				                                              SafeDirection.Rotation(), DeltaTime,
				                                              RotationInterpSpeed);
				PawnOwner->SetActorRotation(NewRotation);
			}
		}
	}

	// Super turns the pending input into Velocity and moves the fish (sweeping, sliding along surfaces).
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

FVector UFishSteeringMovement::CalculateAvoidance(const FVector& DesiredDirection) const
{
	UWorld* World = GetWorld();
	if (!PawnOwner || !World)
	{
		return DesiredDirection;
	}

	const FVector PawnLoc = PawnOwner->GetActorLocation();

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(PawnOwner);

	// Obstacles = every object of the configured object types (WorldStatic + WorldDynamic by default).
	FCollisionObjectQueryParams ObjectParams;
	for (const TEnumAsByte<ECollisionChannel>& ObjectType : ObstacleObjectTypes)
	{
		ObjectParams.AddObjectTypesToQuery(ObjectType);
	}

	// Sphere probes (not thin lines): the fish has a body, a line from its center lets it clip corners.
	const FCollisionShape ProbeShape = FCollisionShape::MakeSphere(ProbeRadius);

	// 1. Probe straight along where the fish WANTS to go (not where it currently faces).
	FHitResult ForwardHit;
	const bool bBlocked = SweepNearestObstacle(World, ForwardHit, PawnLoc,
	                                           PawnLoc + DesiredDirection * AvoidanceDistance, ObjectParams, ProbeShape,
	                                           QueryParams);
#if ENABLE_DRAW_DEBUG
	if (bDrawDebug)
	{
		DrawDebugLine(World, PawnLoc, PawnLoc + DesiredDirection * AvoidanceDistance,
		              bBlocked ? FColor::Red : FColor::Green, false, -1.f, 0, 2.f);
	}
#endif
	if (!bBlocked)
	{
		LastAvoidDirection = FVector::ZeroVector;
		return DesiredDirection;
	}

	// 2. Where would the fish naturally slide along the obstacle? (fallback: stay close to the desired direction)
	const FVector SlideDirection = FVector::VectorPlaneProject(DesiredDirection, ForwardHit.ImpactNormal)
		.GetSafeNormal();
	const FVector PreferredDirection = SlideDirection.IsNearlyZero() ? DesiredDirection : SlideDirection;

	// 3. Whiskers fanned around the desired direction (local frame: X = desired direction).
	static const FRotator WhiskerOffsets[] = {
		FRotator(45.0f, 45.0f, 0.0f), FRotator(45.0f, -45.0f, 0.0f),
		FRotator(-45.0f, 45.0f, 0.0f), FRotator(-45.0f, -45.0f, 0.0f),
		FRotator(0.0f, 90.0f, 0.0f), FRotator(0.0f, -90.0f, 0.0f),
		FRotator(90.0f, 0.0f, 0.0f), FRotator(-90.0f, 0.0f, 0.0f)
	};

	const FQuat DesiredFrame = FRotationMatrix::MakeFromX(DesiredDirection).ToQuat();

	FVector BestDirection = FVector::ZeroVector;
	float BestScore = -TNumericLimits<float>::Max();

	for (const FRotator& Offset : WhiskerOffsets)
	{
		const FVector WhiskerDir = DesiredFrame.RotateVector(Offset.Vector());

		FHitResult WhiskerHit;
		const bool bWhiskerBlocked = SweepNearestObstacle(World, WhiskerHit, PawnLoc,
		                                                  PawnLoc + WhiskerDir * AvoidanceDistance, ObjectParams,
		                                                  ProbeShape, QueryParams);
#if ENABLE_DRAW_DEBUG
		if (bDrawDebug)
		{
			DrawDebugLine(World, PawnLoc, PawnLoc + WhiskerDir * AvoidanceDistance,
			              bWhiskerBlocked ? FColor::Red : FColor::Green, false, -1.f, 0, 1.f);
		}
#endif
		if (bWhiskerBlocked)
		{
			continue;
		}

		// Among the free whiskers, keep the one that deviates least from the preferred direction.
		float Score = FVector::DotProduct(WhiskerDir, PreferredDirection);
		// Keep the side chosen last frame (hysteresis) unless it became clearly worse.
		if (!LastAvoidDirection.IsNearlyZero() && FVector::DotProduct(WhiskerDir, LastAvoidDirection) > 0.7f)
		{
			Score += AvoidanceStickiness;
		}
		if (Score > BestScore)
		{
			BestScore = Score;
			BestDirection = WhiskerDir;
		}
	}

	// Everything is blocked (corner): back away along the surface normal.
	if (BestDirection.IsNearlyZero())
	{
		BestDirection = ForwardHit.ImpactNormal;
	}

	LastAvoidDirection = BestDirection;

	// 4. Full steering once the obstacle is within FullAvoidanceFraction of the probe length, ramping up before that
	//    (ForwardHit.Time: 0 = touching, 1 = at max distance).
	const float RampLength = FMath::Max(1.0f - FullAvoidanceFraction, KINDA_SMALL_NUMBER);
	const float Strength = FMath::Clamp((1.0f - ForwardHit.Time) / RampLength, 0.0f, 1.0f);
	const FVector Result = FMath::Lerp(DesiredDirection, BestDirection, Strength).GetSafeNormal();

#if ENABLE_DRAW_DEBUG
	if (bDrawDebug)
	{
		DrawDebugLine(World, PawnLoc, PawnLoc + Result * AvoidanceDistance, FColor::Yellow, false, -1.f, 0, 3.f);
	}
#endif

	return Result.IsNearlyZero() ? DesiredDirection : Result;
}
