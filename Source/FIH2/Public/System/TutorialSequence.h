// Séquence de tutoriel data-driven : une liste d'étapes (message + condition pour passer à la suivante).
// Créer un Data Asset de cette classe : il est prérempli avec le tutoriel du niveau 1 (étapes 0 à 8),
// modifiable sans recompiler. Le niveau l'utilise via ULevelConfig::TutorialSequence.
// Unités : secondes réelles, distances en cm (unités Unreal).

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "System/EcosystemSubsystem.h"
#include "TutorialSequence.generated.h"

class AActor;
class UInputAction;
class UMaterialInterface;
class UTutorialPanelWidget;

/** Ce qui fait passer une étape à la suivante. */
UENUM(BlueprintType)
enum class ETutorialCondition : uint8
{
	/** Le joueur clique sur « Suivant ». */
	NextButton UMETA(DisplayName = "Bouton Suivant"),
	/** Une des actions d'entrée (caméra...) est déclenchée. */
	InputAction UMETA(DisplayName = "Action d'entrée"),
	/** Le joueur sélectionne un acteur de la classe TargetClass. */
	SelectActorOfClass UMETA(DisplayName = "Sélection d'un acteur"),
	/** Le joueur utilise la loupe. */
	FocusUsed UMETA(DisplayName = "Loupe utilisée"),
	/** Duration secondes se sont écoulées. */
	Delay UMETA(DisplayName = "Délai"),
	/** La variable GaugeVariable atteint au moins la zone Zone. */
	GaugeZoneReached UMETA(DisplayName = "Zone de jauge atteinte"),
	/** Un poisson mange (option : seulement une nourriture que le joueur a lâchée pendant l'étape). */
	FishAte UMETA(DisplayName = "Un poisson mange"),
	/** Tous les acteurs tagués TargetTag sont à moins de NearRadius d'un acteur tagué ReferenceTag. */
	TaggedActorsNear UMETA(DisplayName = "Acteurs rapprochés"),
	/** Étape finale : défaite et mort par la faim réactivées, maintenir la jauge hors du rouge pendant Duration. */
	Maintain UMETA(DisplayName = "Maintien (étape finale)"),
	/** Un poisson pond un œuf. */
	EggLaid UMETA(DisplayName = "Œuf pondu"),
	/** Un œuf éclot. Si l'œuf suivi meurt, retour à l'étape EggLaid précédente. */
	EggHatched UMETA(DisplayName = "Œuf éclos"),
	/** Le joueur sélectionne un alevin (ou l'alevin qui vient de naître, même devenu adulte). */
	SelectFry UMETA(DisplayName = "Sélection d'un alevin")
};

/** Variable observée par la condition GaugeZoneReached. */
UENUM(BlueprintType)
enum class ETutorialGaugeVariable : uint8
{
	Pollution,
	Oxygen,
	Hunger,
	/** Jauge globale de dérèglement (rouge = SEUIL_DEFAITE_JAUGE, comme le HUD). */
	Global
};

USTRUCT(BlueprintType)
struct FTutorialStep
{
	GENERATED_BODY()

	/** Message affiché en haut de l'écran (2 lignes maximum conseillées). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial", meta = (MultiLine = true))
	FText Message;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial")
	ETutorialCondition Condition = ETutorialCondition::NextButton;

	/** InputAction : actions qui valident l'étape (n'importe laquelle). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial",
		meta = (EditCondition = "Condition == ETutorialCondition::InputAction", EditConditionHides))
	TArray<TSoftObjectPtr<UInputAction>> InputActions;

	/** SelectActorOfClass : classe à sélectionner. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial",
		meta = (EditCondition = "Condition == ETutorialCondition::SelectActorOfClass", EditConditionHides))
	TSubclassOf<AActor> TargetClass;

	/** Delay : durée [s]. Maintain : durée du maintien TUTO_DUREE_MAINTIEN [s] (60 [30–120]). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial", meta = (ClampMin = "0",
		EditCondition = "Condition == ETutorialCondition::Delay || Condition == ETutorialCondition::Maintain", EditConditionHides))
	float Duration = 15.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial",
		meta = (EditCondition = "Condition == ETutorialCondition::GaugeZoneReached", EditConditionHides))
	ETutorialGaugeVariable GaugeVariable = ETutorialGaugeVariable::Hunger;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial",
		meta = (EditCondition = "Condition == ETutorialCondition::GaugeZoneReached", EditConditionHides))
	EEcosystemZone Zone = EEcosystemZone::Yellow;

	/** FishAte : ne compte que la nourriture que le joueur a lâchée pendant l'étape (sinon les repas de l'IA suffisent). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial",
		meta = (EditCondition = "Condition == ETutorialCondition::FishAte", EditConditionHides))
	bool bRequirePlayerFood = true;

	/** TaggedActorsNear : tag des acteurs à rapprocher (tag d'acteur, à poser dans le niveau). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial",
		meta = (EditCondition = "Condition == ETutorialCondition::TaggedActorsNear", EditConditionHides))
	FName TargetTag;

	/** TaggedActorsNear : tag des acteurs de référence (vide = n'importe quel poisson). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial",
		meta = (EditCondition = "Condition == ETutorialCondition::TaggedActorsNear", EditConditionHides))
	FName ReferenceTag;

	/** TUTO_RAYON_KELP_PROCHE [cm] : distance horizontale maximale (1500 [1000–3000]). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial", meta = (ClampMin = "0",
		EditCondition = "Condition == ETutorialCondition::TaggedActorsNear", EditConditionHides))
	float NearRadius = 1500.f;

	/** En entrant dans l'étape : caméra cadrée et surbrillance sur le dernier acteur apparu (œuf pondu, alevin né). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial")
	bool bFocusEventActor = false;

	/** TUTO_DELAI_PONTE [s] (8 [5–20]) : en entrant dans l'étape, la prochaine ponte de chaque adulte arrive au plus tard dans ce délai, s'il est nourri (0 = inchangé). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial", meta = (ClampMin = "0"))
	float ShortenReproTimersTo = 0.f;

	/** La caméra se cadre sur les acteurs portant ce tag en entrant dans l'étape (vide = caméra laissée au joueur). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial")
	FName CameraFocusTag;

	/** Faim (0–3) imposée à tous les poissons en entrant dans l'étape (-1 = inchangée). Sert à déclencher la famine du tutoriel. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial", meta = (ClampMin = "-1", ClampMax = "3"))
	int32 SetFishHungerOnEnter = -1;

	/** Acteurs mis en surbrillance pendant l'étape (tag d'acteur ; vide = aucun). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial")
	FName HighlightTag;

	/** Temps minimal d'affichage du message [s], même si la condition est déjà remplie. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial", meta = (ClampMin = "0"))
	float MinDisplayTime = 1.5f;

	/** TUTO_TIMEOUT_ETAPE [s] : passe à l'étape suivante même si la condition n'est pas remplie (0 = jamais). Anti-blocage. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial", meta = (ClampMin = "0"))
	float Timeout = 0.f;
};

UCLASS(BlueprintType)
class FIH2_API UTutorialSequence : public UDataAsset
{
	GENERATED_BODY()

public:
	/** Préremplit le tutoriel du niveau 1. */
	UTutorialSequence();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial")
	TArray<FTutorialStep> Steps;

	/** Démarre le tutoriel au lancement du niveau. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial")
	bool bStartAutomatically = true;

	/** Panneau de guidage (vide = panneau par défaut construit en C++ ; un WBP enfant de UTutorialPanelWidget permet de le restyler). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial")
	TSubclassOf<UTutorialPanelWidget> PanelWidgetClass;

	/** Matériau de surbrillance des acteurs ciblés (vide = celui de la sélection du joueur). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial")
	TObjectPtr<UMaterialInterface> HighlightMaterial;

	/** Message affiché quand la jauge passe au rouge pendant le maintien. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial|Maintain")
	FText MaintainResetMessage;

	/** Message affiché quand l'œuf suivi meurt (retour à l'étape de ponte). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial")
	FText EggLostMessage;

	/** TUTO_DUREE_MSG_RESET [s] (2 [1–4]). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial|Maintain", meta = (ClampMin = "0.5"))
	float MaintainResetMessageDuration = 2.f;

	/** TUTO_FAIM_MIN_ENTREE_ETAPE8 (1 [0–2]) : faim minimale donnée aux poissons à l'entrée du maintien, pour qu'ils ne meurent pas aussitôt. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial|Maintain", meta = (ClampMin = "0", ClampMax = "3"))
	int32 MinHungerOnMaintain = 1;
};
