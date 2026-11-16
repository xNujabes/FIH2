// Caméra du menu principal : lente dérive (courbe de Lissajous) autour de sa position de placement,
// avec un léger parallaxe qui suit la souris. La profondeur de champ et la vignette se règlent sur le
// composant caméra (Post Process) dans la carte.
// Seul acteur du menu qui tique : une dérive continue est une animation, pas du gameplay.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MenuCameraRig.generated.h"

class UCameraComponent;

UCLASS()
class FIH2_API AMenuCameraRig : public AActor
{
	GENERATED_BODY()

public:
	AMenuCameraRig();

	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<UCameraComponent> Camera;

	/** DERIVE_AMPLITUDE [cm] : amplitude de la dérive sur les axes locaux (avant, droite, haut). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drift")
	FVector DriftAmplitude = FVector(150.f, 400.f, 120.f);

	/** DERIVE_PERIODE [s] : période par axe. Des périodes sans rapport simple donnent un mouvement qui ne se répète pas à l'œil. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drift")
	FVector DriftPeriod = FVector(47.f, 31.f, 39.f);

	/** BALANCEMENT_AMPLITUDE [°] : léger balancement de l'orientation (tangage, lacet, roulis). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drift")
	FRotator SwayAmplitude = FRotator(0.6f, 1.2f, 0.4f);

	/** BALANCEMENT_PERIODE [s] : (tangage, lacet, roulis). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Drift")
	FVector SwayPeriod = FVector(23.f, 37.f, 29.f);

	/** PARALLAXE_ANGLE [°] : rotation maximale (lacet, tangage) quand la souris est au bord de l'écran. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Parallax")
	FVector2D ParallaxAngle = FVector2D(2.5f, 1.5f);

	/** PARALLAXE_LISSAGE [1/s] : vitesse à laquelle la caméra rejoint la cible du parallaxe. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Parallax", meta = (ClampMin = "0"))
	float ParallaxInterpSpeed = 2.f;

protected:
	virtual void BeginPlay() override;

private:
	FTransform BaseTransform;
	FVector2D SmoothedMouse = FVector2D::ZeroVector;
	float DriftTime = 0.f;
};
