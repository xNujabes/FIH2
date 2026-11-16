// Progression et réglages du joueur, persistants d'une session à l'autre.
// UGameInstanceSubsystem : vit pendant toute la session, fonctionne avec le GameInstance par défaut du moteur.
// Piloté par évènements (aucun Tick) : la sauvegarde est écrite quand quelque chose change.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Subsystems/WorldSubsystem.h"
#include "System/EcosystemSubsystem.h"
#include "System/FIH2SaveGame.h"
#include "FIH2ProgressionSubsystem.generated.h"

class ULevelCatalog;
class ULevelConfig;
struct FLevelCatalogEntry;

/** État d'une carte du sélecteur de niveau. */
UENUM(BlueprintType)
enum class ELevelCardState : uint8
{
	Available UMETA(DisplayName = "Available"),
	Locked UMETA(DisplayName = "Locked"),
	Completed UMETA(DisplayName = "Completed")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnFIH2ProgressionChanged);

UCLASS()
class FIH2_API UFIH2ProgressionSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	/** Raccourci : le sous-système du GameInstance de WorldContext (nullptr hors jeu). */
	static UFIH2ProgressionSubsystem* Get(const UObject* WorldContext);

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	// ---------------------------------------------------------------- Progression
	UFUNCTION(BlueprintPure, Category = "FIH2|Progression")
	bool IsTutorialCompleted() const;

	UFUNCTION(BlueprintPure, Category = "FIH2|Progression")
	bool IsLevelCompleted(FName LevelId) const;

	/** Complété > verrouillé (« Bientôt » ou prérequis manquant) > disponible. */
	ELevelCardState GetLevelState(const FLevelCatalogEntry& Entry) const;

	UFUNCTION(BlueprintCallable, Category = "FIH2|Progression")
	void MarkLevelCompleted(FName LevelId);

	/**
	 * Point d'accroche du tutoriel (appelé par UTutorialSubsystem::FinishTutorial).
	 * Complété = victoire, et pas de « Passer le tutoriel » (sauf UFIH2ProjectSettings::bSkippedTutorialCountsAsCompleted).
	 */
	void NotifyTutorialFinished(const UObject* WorldContext, bool bWon, bool bSkipped);

	/** Victoire d'un niveau hors tutoriel (UEcosystemSubsystem passé à Won). */
	void NotifyLevelWon(const UWorld* World);

	/** Efface la progression (les réglages sont conservés). Utile pour tester le premier lancement. */
	UFUNCTION(BlueprintCallable, Category = "FIH2|Progression")
	void ResetProgression();

	// ---------------------------------------------------------------- Catalogue
	UFUNCTION(BlueprintPure, Category = "FIH2|Levels")
	ULevelCatalog* GetCatalog() const;

	/** Config d'écosystème du niveau dont World est la carte (nullptr = pas d'entrée ou pas de config). */
	ULevelConfig* FindLevelConfigForWorld(const UWorld* World) const;

	/** Enregistre puis ouvre la carte du menu principal. */
	UFUNCTION(BlueprintCallable, Category = "FIH2|Levels", meta = (WorldContext = "WorldContextObject"))
	void ReturnToMainMenu(const UObject* WorldContextObject);

	// ---------------------------------------------------------------- Réglages
	UFUNCTION(BlueprintPure, Category = "FIH2|Settings")
	FFIH2UserSettings GetUserSettings() const;

	/** Remplace les réglages et applique l'audio tout de suite. bSaveNow = écrire la sauvegarde. */
	UFUNCTION(BlueprintCallable, Category = "FIH2|Settings", meta = (WorldContext = "WorldContextObject"))
	void SetUserSettings(const UObject* WorldContextObject, const FFIH2UserSettings& NewSettings, bool bSaveNow);

	UFUNCTION(BlueprintPure, Category = "FIH2|Settings")
	FFIH2UserSettings GetDefaultUserSettings() const;

	/**
	 * Multiplicateur de sensibilité de la caméra choisi par le joueur.
	 * À BRANCHER : la vitesse de caméra vit dans le graphe de BP_Tank (PanSpeed / ZoomSpeed) ; multiplier ces
	 * valeurs par ce résultat (nœud « Get FIH2ProgressionSubsystem » → GetCameraSensitivity).
	 */
	UFUNCTION(BlueprintPure, Category = "FIH2|Settings")
	float GetCameraSensitivity() const;

	UFUNCTION(BlueprintCallable, Category = "FIH2|Save")
	bool SaveNow();

	/** Appelé au début de chaque monde de jeu (UFIH2WorldBootSubsystem) : réapplique les réglages. */
	void HandleWorldBeginPlay(UWorld& World);

	/** Émis quand la progression change (niveau complété, remise à zéro). */
	UPROPERTY(BlueprintAssignable, Category = "FIH2|Progression")
	FOnFIH2ProgressionChanged OnProgressionChanged;

private:
	/** Charge le slot ; absent, illisible ou d'une autre version → profil par défaut (jamais nullptr ensuite). */
	void LoadOrCreate();
	void ApplyAudioSettings(const UObject* WorldContext, bool bRepushMix);

	UPROPERTY(Transient)
	TObjectPtr<UFIH2SaveGame> SaveData;

	UPROPERTY(Transient)
	mutable TObjectPtr<ULevelCatalog> CachedCatalog;

	bool bWarnedAudioNotWired = false;
};

/**
 * Relais par monde : prévient UFIH2ProgressionSubsystem au début de chaque monde de jeu et écoute la victoire
 * de l'écosystème. Évite d'ajouter du code de menu / sauvegarde dans les classes de simulation.
 */
UCLASS()
class FIH2_API UFIH2WorldBootSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;

private:
	UFUNCTION()
	void HandleEcosystemStateChanged(EEcosystemState NewState);
};
