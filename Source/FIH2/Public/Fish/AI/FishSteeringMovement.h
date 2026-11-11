// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "FishSteeringMovement.generated.h"

/**
 * The ONLY thing allowed to move a fish.
 *
 * Whoever wants the fish to go somewhere (behavior tree, controller...) calls AddMovementInput() with the
 * direction it wants. Each tick this component:
 *   1. takes that desired direction,
 *   2. probes the water ahead *along that direction* and bends it around obstacles,
 *   3. feeds the corrected direction back as the real movement input (so it really drives Velocity),
 *   4. turns the fish toward the corrected direction.
 *
 * The fish body must be kinematic (not simulating physics): the component moves it by sweeping.
 * While the player holds a fish with the physics handle the body simulates physics, and steering stands aside.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class FIH2_API UFishSteeringMovement : public UFloatingPawnMovement
{
	GENERATED_BODY()

public:
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;

protected:
	UPROPERTY(EditAnywhere, Category = "Steering")
	float RotationInterpSpeed = 5.0f;

	/** How far ahead (along the desired direction) obstacles are detected. */
	UPROPERTY(EditAnywhere, Category = "Avoidance")
	float AvoidanceDistance = 300.0f;

	/** Radius of the probe spheres (about the fish's half-width). */
	UPROPERTY(EditAnywhere, Category = "Avoidance", meta = (ClampMin = "1.0"))
	float ProbeRadius = 40.0f;

	/** Fraction of AvoidanceDistance under which the fish steers at 100% (above it, steering ramps up progressively). */
	UPROPERTY(EditAnywhere, Category = "Avoidance", meta = (ClampMin = "0.0", ClampMax = "0.95"))
	float FullAvoidanceFraction = 0.6f;

	/** Object types the fish steers around (every object of these types counts as an obstacle). */
	UPROPERTY(EditAnywhere, Category = "Avoidance")
	TArray<TEnumAsByte<ECollisionChannel>> ObstacleObjectTypes = {ECC_WorldStatic};

	/** Bonus score for the whisker already used last frame: stops the fish from flip-flopping (and circling) in front of a wall. */
	UPROPERTY(EditAnywhere, Category = "Avoidance", meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float AvoidanceStickiness = 0.4f;

	/** Draw the probe rays (red = blocked, green = free, yellow = chosen direction). */
	UPROPERTY(EditAnywhere, Category = "Avoidance")
	bool bDrawDebug = false;

private:
	/** Direction chosen while avoiding last frame (zero when the way ahead was free). */
	mutable FVector LastAvoidDirection = FVector::ZeroVector;

	/** Returns DesiredDirection bent away from whatever blocks it (unit vector). DesiredDirection must be normalized. */
	FVector CalculateAvoidance(const FVector& DesiredDirection) const;
};
