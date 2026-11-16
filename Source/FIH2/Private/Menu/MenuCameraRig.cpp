#include "Menu/MenuCameraRig.h"

#include "Camera/CameraComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"

namespace
{
	/** sin(2π t / Période), 0 si la période est nulle. Phase en tours pour décaler les axes entre eux. */
	float Wave(float Time, float Period, float PhaseTurns = 0.f)
	{
		return Period > KINDA_SMALL_NUMBER ? FMath::Sin(2.f * PI * (Time / Period + PhaseTurns)) : 0.f;
	}
}

AMenuCameraRig::AMenuCameraRig()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	RootComponent = Camera;
}

void AMenuCameraRig::BeginPlay()
{
	Super::BeginPlay();
	BaseTransform = GetActorTransform();
}

void AMenuCameraRig::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	DriftTime += DeltaSeconds;

	// Souris normalisée dans [-1, 1] (centre de l'écran = 0). Sans souris (manette), la cible reste au centre.
	FVector2D MouseTarget = FVector2D::ZeroVector;
	if (const UGameViewportClient* Viewport = GetWorld() ? GetWorld()->GetGameViewport() : nullptr)
	{
		FVector2D MousePosition;
		FVector2D ViewportSize;
		Viewport->GetViewportSize(ViewportSize);
		if (Viewport->GetMousePosition(MousePosition) && ViewportSize.X > 1.f && ViewportSize.Y > 1.f)
		{
			MouseTarget.X = FMath::Clamp(MousePosition.X / ViewportSize.X * 2.f - 1.f, -1.f, 1.f);
			MouseTarget.Y = FMath::Clamp(MousePosition.Y / ViewportSize.Y * 2.f - 1.f, -1.f, 1.f);
		}
	}
	SmoothedMouse = FMath::Vector2DInterpTo(SmoothedMouse, MouseTarget, DeltaSeconds, ParallaxInterpSpeed);

	// Dérive de Lissajous dans le repère local de la caméra.
	const FVector LocalOffset(
		DriftAmplitude.X * Wave(DriftTime, DriftPeriod.X),
		DriftAmplitude.Y * Wave(DriftTime, DriftPeriod.Y, 0.25f),
		DriftAmplitude.Z * Wave(DriftTime, DriftPeriod.Z, 0.5f));

	const FRotator LocalRotation(
		SwayAmplitude.Pitch * Wave(DriftTime, SwayPeriod.X) - SmoothedMouse.Y * ParallaxAngle.Y,
		SwayAmplitude.Yaw * Wave(DriftTime, SwayPeriod.Y, 0.25f) + SmoothedMouse.X * ParallaxAngle.X,
		SwayAmplitude.Roll * Wave(DriftTime, SwayPeriod.Z, 0.5f));

	const FQuat BaseRotation = BaseTransform.GetRotation();
	SetActorLocationAndRotation(
		BaseTransform.GetLocation() + BaseRotation.RotateVector(LocalOffset),
		BaseRotation * LocalRotation.Quaternion());
}
