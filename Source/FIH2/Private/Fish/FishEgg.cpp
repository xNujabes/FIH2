#include "Fish/FishEgg.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Fish/Fish.h"
#include "UObject/ConstructorHelpers.h"

AFishEgg::AFishEgg()
{
	PrimaryActorTick.bCanEverTick = false;

	// Mesh provisoire : sphère de base de l'engine.
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SphereMesh.Succeeded())
	{
		Mesh->SetStaticMesh(SphereMesh.Object);
	}

	// Physique simple : tombe doucement dans l'eau (amortissement) et peut être saisi à la souris (bloque Visibility).
	Mesh->SetCollisionProfileName(TEXT("PhysicsActor"));
	Mesh->SetSimulatePhysics(true);
	Mesh->SetEnableGravity(true);
	Mesh->SetLinearDamping(2.f);
	Mesh->SetAngularDamping(2.f);
}

void AFishEgg::InitEgg(TSubclassOf<AFish> InFishClass, const FVector& InAdultScale, float InIncubationDuration, bool bInDiesInRedPollution)
{
	bDiesInRedPollution = bInDiesInRedPollution;
	FishClass = InFishClass;
	AdultScale = InAdultScale;
	IncubationDuration = FMath::Max(0.1f, InIncubationDuration);
	IncubationRemaining = IncubationDuration;
}

float AFishEgg::GetIncubationProgress() const
{
	return IncubationDuration > 0.f ? FMath::Clamp(1.f - IncubationRemaining / IncubationDuration, 0.f, 1.f) : 0.f;
}

FText AFishEgg::GetName_Implementation()
{
	return FText::FromString(TEXT("Fish egg"));
}

FText AFishEgg::GetDescription_Implementation()
{
	return FText::FromString(FString::Printf(
		TEXT("Hatches in %.0f s.\nDies if the water gets too polluted."), FMath::Max(0.f, IncubationRemaining)));
}

void AFishEgg::AdvanceIncubation(float DeltaSeconds)
{
	const int32 ShownBefore = FMath::CeilToInt(FMath::Max(0.f, IncubationRemaining));
	IncubationRemaining -= DeltaSeconds;
	if (FMath::CeilToInt(FMath::Max(0.f, IncubationRemaining)) != ShownBefore)
	{
		FInteractableEvents::NotifyInfoChanged(this);
	}
}

bool AFishEgg::GetInfoProgress_Implementation(float& OutProgress, FText& OutLabel)
{
	OutProgress = GetIncubationProgress();
	OutLabel = FText::FromString(TEXT("Incubation"));
	return true;
}
