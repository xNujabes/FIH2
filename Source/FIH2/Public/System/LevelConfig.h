// Configuration data-driven d'un niveau : mission, jauge de dérèglement et paramètres écologiques.
// Toutes les valeurs sont éditables sans recompiler (créer un Data Asset de cette classe).
// Les noms de paramètres de conception (MAJUSCULES_SNAKE_CASE) sont rappelés en commentaire.
// Unités : secondes réelles (1 s réelle = 1 s de simulation) ; stress, pollution et oxygène sur 0–100.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "LevelConfig.generated.h"

class UTutorialSequence;


UCLASS(BlueprintType)
class FIH2_API ULevelConfig : public UDataAsset
{
	GENERATED_BODY()

public:
	// ---------------------------------------------------------------- Mission (D-01)
	/** OBJECTIF_POISSONS_BASE : adultes de base vivants requis à la fin du niveau (D-01 : 5). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mission", meta = (ClampMin = "1"))
	int32 ObjectiveAdultFish = 5;

	/** DUREE_NIVEAU [s] : la victoire est évaluée à cet instant (option B). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mission", meta = (ClampMin = "10"))
	float LevelDuration = 300.f;

	/** Objectif chronométré : victoire / défaite évaluées à DUREE_NIVEAU. Désactivé pour le tutoriel (sa dernière étape décide de la victoire) ; le chrono du HUD compte alors le temps écoulé. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mission")
	bool bUseTimedObjective = true;

	/** Tutoriel guidé du niveau (vide = pas de tutoriel). Voir UTutorialSubsystem. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mission")
	TObjectPtr<UTutorialSequence> TutorialSequence;

	/** DELAI_EXTINCTION [s] : durée pendant laquelle il peut manquer des adultes (< 2, sans alevin ni œuf) avant la défaite. Laisse le temps de reposer un poisson. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mission", meta = (ClampMin = "0"))
	float ExtinctionGraceTime = 15.f;

	// ---------------------------------------------------------------- Jauge de dérèglement (D-02, D-03)
	/** SEUIL_JAUNE : un stress à partir de cette valeur passe en zone jaune. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gauge", meta = (ClampMin = "0", ClampMax = "100"))
	float YellowThreshold = 40.f;

	/** SEUIL_ROUGE_VARIABLE : un stress à partir de cette valeur passe en zone rouge. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gauge", meta = (ClampMin = "0", ClampMax = "100"))
	float RedThreshold = 70.f;

	/** COEF_ZONE_VERT : poids d'un stress en zone verte dans la moyenne pondérée. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gauge", meta = (ClampMin = "0.01"))
	float CoefGreen = 0.5f;

	/** COEF_ZONE_JAUNE */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gauge", meta = (ClampMin = "0.01"))
	float CoefYellow = 1.f;

	/** COEF_ZONE_ROUGE */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gauge", meta = (ClampMin = "0.01"))
	float CoefRed = 2.f;

	/** SEUIL_DEFAITE_JAUGE : la jauge globale D à partir de laquelle le timer de défaite monte. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gauge", meta = (ClampMin = "1", ClampMax = "100"))
	float DefeatGaugeThreshold = 50.f;

	/** DUREE_DEFAITE [s] : le timer de défaite doit atteindre cette valeur. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gauge", meta = (ClampMin = "1"))
	float DefeatDuration = 30.f;

	/** DECROISSANCE_TIMER_DEFAITE : vitesse de descente du timer (× temps réel) quand D repasse sous le seuil. Évite le reset par oscillation. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gauge", meta = (ClampMin = "0", ClampMax = "1"))
	float DefeatTimerDecay = 0.5f;

	// ---------------------------------------------------------------- Simulation (D-07)
	/** SIM_TICK [s] : pas fixe de la simulation, découplé du framerate. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Simulation", meta = (ClampMin = "0.1", ClampMax = "2"))
	float SimTick = 0.5f;

	// ---------------------------------------------------------------- Pollution (D-04)
	/** POLLUTION_NOURRITURE_POURRIE [/s/unité] : pollution émise par un fruit à l'état Decaying. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pollution", meta = (ClampMin = "0"))
	float PollutionPerDecayingFood = 0.05f;

	/** POLLUTION_CADAVRE [/s/cadavre] : pollution émise par un poisson mort (faim ou asphyxie). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pollution", meta = (ClampMin = "0"))
	float PollutionPerCorpse = 0.15f;

	/** DUREE_CADAVRE [s] : durée pendant laquelle un cadavre pollue (le poisson mort est déjà détruit, c'est un compteur virtuel). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pollution", meta = (ClampMin = "1"))
	float CorpsePollutionDuration = 60.f;

	/** EPURATION_BASE [/s] : auto-épuration naturelle. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pollution", meta = (ClampMin = "0"))
	float BasePurification = 0.03f;

	/** EPURATION_KELP [/s/kelp mûr] */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pollution", meta = (ClampMin = "0"))
	float PurificationPerKelp = 0.02f;

	// ---------------------------------------------------------------- Oxygène (D-04)
	/** O2_DEPART [0–100] */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Oxygen", meta = (ClampMin = "0", ClampMax = "100"))
	float StartOxygen = 90.f;

	/** O2_PROD_BASE [/s] : échange avec la surface. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Oxygen", meta = (ClampMin = "0"))
	float BaseOxygenProduction = 0.3f;

	/** O2_PROD_KELP [/s/kelp mûr] */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Oxygen", meta = (ClampMin = "0"))
	float OxygenPerMatureKelp = 0.12f;

	/** O2_CONSO_ADULTE [/s/poisson] */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Oxygen", meta = (ClampMin = "0"))
	float AdultOxygenConsumption = 0.10f;

	/** Facteur appliqué à la consommation d'un alevin (0,5 = moitié d'un adulte). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Oxygen", meta = (ClampMin = "0"))
	float FryOxygenFactor = 0.5f;

	/** O2_CONSO_POLLUTION [/s par point de pollution] : la décomposition consomme de l'oxygène. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Oxygen", meta = (ClampMin = "0"))
	float OxygenConsumptionPerPollution = 0.004f;

	/** O2_SEUIL_ASPHYXIE : sous cette valeur d'oxygène, les poissons meurent. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Oxygen", meta = (ClampMin = "0", ClampMax = "100"))
	float AsphyxiaThreshold = 15.f;

	/** MORT_ASPHYXIE [/s/poisson] : probabilité de mort par seconde sous le seuil. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Oxygen", meta = (ClampMin = "0"))
	float AsphyxiaDeathRate = 0.02f;

	// ---------------------------------------------------------------- Faim
	/** Un poisson dont Hunger <= cette valeur est « affamé » (BP_Fish : Hunger -1 toutes les 20 s, détruit à 0 au tick suivant). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hunger", meta = (ClampMin = "0", ClampMax = "3"))
	int32 StarvingHungerMax = 0;

	// ---------------------------------------------------------------- Reproduction (D-06)
	/** Désactive la reproduction dans ce niveau. Les réglages de ponte, d'œuf et d'alevin sont propres à chaque
	 *  espèce : voir AFish::Reproduction dans le Blueprint du poisson. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reproduction")
	bool bReproductionEnabled = true;

	/** MAX_POISSONS_TOTAL : garde-fou de performance (adultes + alevins + œufs). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reproduction", meta = (ClampMin = "2"))
	int32 MaxFishTotal = 40;

	// ---------------------------------------------------------------- Debug
	/** Affiche l'état de l'écosystème en texte à l'écran (retiré des builds Shipping). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Debug")
	bool bShowDebugHUD = true;
};
