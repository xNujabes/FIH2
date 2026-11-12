// Œuf de poisson (D-09) : pondu par un adulte, il tombe sur le sable puis éclot en alevin.
// L'incubation est pilotée par UEcosystemSubsystem (pause quand le joueur le tient, mort en pollution rouge : D-10).
// Le mesh par défaut est une sphère de base : remplacer par un Blueprint BP_FishEgg pour le rendu final.

#pragma once

#include "CoreMinimal.h"
#include "Inanimate/Inanimate.h"
#include "FishEgg.generated.h"

class AFish;

UCLASS()
class FIH2_API AFishEgg : public AInanimate
{
	GENERATED_BODY()

public:
	AFishEgg();

	/** Classe du poisson qui sortira de l'œuf (celle du parent). */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Egg")
	TSubclassOf<AFish> FishClass;

	/** Échelle adulte du parent : l'alevin grandira jusqu'à elle. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Egg")
	FVector AdultScale = FVector::OneVector;

	/** Durée totale d'incubation [s]. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Egg")
	float IncubationDuration = 0.f;

	/** Temps restant avant l'éclosion [s] (décrémenté par le sous-système d'écosystème). */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Egg")
	float IncubationRemaining = 0.f;

	/** Initialise l'œuf juste après la ponte. */
	void InitEgg(TSubclassOf<AFish> InFishClass, const FVector& InAdultScale, float InIncubationDuration);

	/** 0 à la ponte, 1 à l'éclosion. */
	UFUNCTION(BlueprintPure, Category = "Egg")
	float GetIncubationProgress() const;

	virtual FText GetName_Implementation() override;
	virtual FText GetDescription_Implementation() override;
};
