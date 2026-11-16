// Fond vivant du menu principal : config d'écosystème « ambiante » et peuplement de la cuve.
// Ajouter une espèce au fond du menu = ajouter une ligne dans Population (aucun code).

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "MenuAmbientConfig.generated.h"

class ULevelConfig;

USTRUCT(BlueprintType)
struct FMenuPopulationEntry
{
	GENERATED_BODY()

	/** Classe à faire apparaître (ex. BP_Fish). Un Pawn sans contrôleur reçoit son contrôleur d'IA par défaut. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Population")
	TSubclassOf<AActor> ActorClass;

	/** EFFECTIF */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Population", meta = (ClampMin = "0", UIMax = "30"))
	int32 Count = 1;

	/** PLAGE_ECHELLE (min, max) : échelle uniforme tirée au hasard pour chaque individu. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Population")
	FVector2D ScaleRange = FVector2D(1.f, 1.f);
};

UCLASS(BlueprintType)
class FIH2_API UMenuAmbientConfig : public UDataAsset
{
	GENERATED_BODY()

public:
	/**
	 * Config d'écosystème du menu (copiée au lancement). Le mode ambiant impose de toute façon : pas de tutoriel,
	 * pas d'objectif chronométré, pas de reproduction, pas d'asphyxie, pas d'affichage de debug.
	 * Vide = valeurs par défaut de ULevelConfig avec ces mêmes garde-fous.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ecosystem")
	TObjectPtr<ULevelConfig> EcosystemConfig;

	/** Classe de la cuve placée dans la carte (ex. BP_Tank) : son volume d'eau borne les apparitions. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tank")
	TSubclassOf<AActor> TankClass;

	/** Nom du UBoxComponent « eau » de la cuve. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tank")
	FName WaterComponentName = TEXT("Water");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Population", meta = (TitleProperty = "ActorClass"))
	TArray<FMenuPopulationEntry> Population;

	/** MARGE_HORIZONTALE [0–0,45] : part de la largeur du volume d'eau laissée libre près de chaque paroi. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Population", meta = (ClampMin = "0", ClampMax = "0.45"))
	float HorizontalMargin = 0.2f;

	/** PLAGE_HAUTEUR (min, max) [0–1] : tranche de la hauteur d'eau utilisée (0 = fond, 1 = surface). Évite le relief du fond. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Population")
	FVector2D HeightRange = FVector2D(0.25f, 0.7f);

	/** Graine aléatoire : 0 = disposition différente à chaque lancement. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Population")
	int32 RandomSeed = 0;
};
