// Sous-système d'écosystème (phase 1) : pollution, oxygène, faim, jauge de dérèglement,
// reproduction des poissons de base et conditions de victoire / défaite du niveau.
//
// Il ne modifie aucune classe existante : il découvre les poissons, kelps et fruits au démarrage du monde
// (TActorIterator, une seule fois) puis via l'événement d'apparition d'acteurs, et détecte leur fin de vie
// via OnEndPlay. Simulation à pas fixe (ULevelConfig::SimTick), découplée du framerate.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "EcosystemSubsystem.generated.h"

class AFish;
struct FFishReproductionSettings;
class AFishEgg;
class AKelp;
class AFood;
class ULevelConfig;

UENUM(BlueprintType)
enum class EEcosystemState : uint8
{
	Playing UMETA(DisplayName = "Playing"),
	Won UMETA(DisplayName = "Won"),
	Lost UMETA(DisplayName = "Lost")
};

UENUM(BlueprintType)
enum class EEcosystemEndReason : uint8
{
	None UMETA(DisplayName = "None"),
	GaugeTooHigh UMETA(DisplayName = "Gauge too high for too long"),
	Extinction UMETA(DisplayName = "Extinction"),
	ObjectiveMissed UMETA(DisplayName = "Objective missed at end of level")
};

UENUM(BlueprintType)
enum class EEcosystemZone : uint8
{
	Green UMETA(DisplayName = "Green"),
	Yellow UMETA(DisplayName = "Yellow"),
	Red UMETA(DisplayName = "Red")
};

/** Photographie de l'état de l'écosystème : tout ce dont un HUD a besoin, en une seule lecture. */
USTRUCT(BlueprintType)
struct FEcosystemSnapshot
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Ecosystem") EEcosystemState State = EEcosystemState::Playing;
	UPROPERTY(BlueprintReadOnly, Category = "Ecosystem") EEcosystemEndReason EndReason = EEcosystemEndReason::None;

	UPROPERTY(BlueprintReadOnly, Category = "Ecosystem") float ElapsedTime = 0.f;
	UPROPERTY(BlueprintReadOnly, Category = "Ecosystem") float TimeRemaining = 0.f;
	/** Chrono du HUD : temps restant si le niveau a un objectif chronométré (ULevelConfig::bUseTimedObjective), sinon temps écoulé. */
	UPROPERTY(BlueprintReadOnly, Category = "Ecosystem") float ChronoTime = 0.f;
	/** true : ChronoTime décompte ; false : il compte le temps écoulé (tutoriel). */
	UPROPERTY(BlueprintReadOnly, Category = "Ecosystem") bool bChronoCountsDown = true;

	UPROPERTY(BlueprintReadOnly, Category = "Ecosystem") int32 AdultFish = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Ecosystem") int32 FryFish = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Ecosystem") int32 Eggs = 0;
	/** Œufs morts depuis le début (pollution rouge). */
	UPROPERTY(BlueprintReadOnly, Category = "Ecosystem") int32 EggsLost = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Ecosystem") int32 ObjectiveAdultFish = 0;

	/** Variables brutes (0–100). Oxygen : 100 = saturé. */
	UPROPERTY(BlueprintReadOnly, Category = "Ecosystem") float Pollution = 0.f;
	UPROPERTY(BlueprintReadOnly, Category = "Ecosystem") float Oxygen = 100.f;
	/** Pourcentage de poissons affamés (0–100). */
	UPROPERTY(BlueprintReadOnly, Category = "Ecosystem") float StarvingPercent = 0.f;

	/** Stress par variable (0–100, plus haut = pire) et leur zone. Ce sont les 3 barres du HUD. */
	UPROPERTY(BlueprintReadOnly, Category = "Ecosystem") float StressPollution = 0.f;
	UPROPERTY(BlueprintReadOnly, Category = "Ecosystem") float StressOxygen = 0.f;
	UPROPERTY(BlueprintReadOnly, Category = "Ecosystem") float StressHunger = 0.f;
	UPROPERTY(BlueprintReadOnly, Category = "Ecosystem") EEcosystemZone ZonePollution = EEcosystemZone::Green;
	UPROPERTY(BlueprintReadOnly, Category = "Ecosystem") EEcosystemZone ZoneOxygen = EEcosystemZone::Green;
	UPROPERTY(BlueprintReadOnly, Category = "Ecosystem") EEcosystemZone ZoneHunger = EEcosystemZone::Green;

	/** Jauge globale D : moyenne pondérée des 3 stress (poids selon la zone de chacun). */
	UPROPERTY(BlueprintReadOnly, Category = "Ecosystem") float Gauge = 0.f;
	UPROPERTY(BlueprintReadOnly, Category = "Ecosystem") float DefeatTimer = 0.f;
	UPROPERTY(BlueprintReadOnly, Category = "Ecosystem") float DefeatDuration = 0.f;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEcosystemStateChanged, EEcosystemState, NewState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEcosystemUpdated, const FEcosystemSnapshot&, Snapshot);

UCLASS()
class FIH2_API UEcosystemSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	/** Raccourci : le sous-système du monde de WorldContext (nullptr hors monde de jeu). */
	static UEcosystemSubsystem* Get(const UObject* WorldContext);

	// USubsystem / UTickableWorldSubsystem
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;

	/** Remplace la config (ex. depuis le GameMode du niveau). À appeler avant le début de la partie. */
	UFUNCTION(BlueprintCallable, Category = "Ecosystem")
	void SetLevelConfig(ULevelConfig* NewConfig);

	UFUNCTION(BlueprintPure, Category = "Ecosystem")
	ULevelConfig* GetLevelConfig() const { return Config; }

	UFUNCTION(BlueprintPure, Category = "Ecosystem")
	FEcosystemSnapshot GetSnapshot() const { return Snapshot; }

	UFUNCTION(BlueprintPure, Category = "Ecosystem")
	EEcosystemState GetGameState() const { return State; }

	/** Remet pollution, oxygène, timers et état à zéro (la population existante est conservée). */
	UFUNCTION(BlueprintCallable, Category = "Ecosystem")
	void ResetEcosystem();

	/** Active / désactive toutes les défaites (jauge, extinction, objectif manqué). Le tutoriel les coupe avant son étape finale. */
	UFUNCTION(BlueprintCallable, Category = "Ecosystem")
	void SetDefeatEnabled(bool bEnabled);

	UFUNCTION(BlueprintPure, Category = "Ecosystem")
	bool IsDefeatEnabled() const { return bDefeatEnabled; }

	/** Active / désactive la mort par la faim (lue par BP_Fish via AFish::CanStarveToDeath). */
	UFUNCTION(BlueprintCallable, Category = "Ecosystem")
	void SetStarvationEnabled(bool bEnabled);

	UFUNCTION(BlueprintPure, Category = "Ecosystem")
	bool IsStarvationEnabled() const { return bStarvationEnabled; }

	/** Termine la partie depuis l'extérieur (ex. victoire du tutoriel). Sans effet si elle est déjà finie. */
	UFUNCTION(BlueprintCallable, Category = "Ecosystem")
	void FinishLevel(bool bWon);

	/** Émis une seule fois quand la partie passe à Won ou Lost. */
	UPROPERTY(BlueprintAssignable, Category = "Ecosystem")
	FOnEcosystemStateChanged OnStateChanged;

	/** Émis à chaque pas de simulation (SimTick) : pratique pour un HUD UMG. */
	UPROPERTY(BlueprintAssignable, Category = "Ecosystem")
	FOnEcosystemUpdated OnUpdated;

protected:
	/** Chemin d'un Data Asset ULevelConfig chargé automatiquement s'il existe ; sinon valeurs par défaut du code. */
	static const TCHAR* DefaultConfigPath;

private:
	struct FFishRecord
	{
		double BornTime = 0.0;
		bool bAdult = true;
		float ReproTimer = 0.f;
		FVector AdultScale = FVector::OneVector;
	};

	void EnsureConfig();
	void TrackActor(AActor* Actor);
	void HandleActorSpawned(AActor* Actor);
	UFUNCTION()
	void HandleActorEndPlay(AActor* Actor, EEndPlayReason::Type EndPlayReason);

	void Step(float Dt);
	void UpdateFryScales();
	static float NextReproCooldown(const FFishReproductionSettings& Settings);
	void LayEgg(AFish* Parent);
	void HatchEgg(AFishEgg* Egg);
	void SpawnFry(TSubclassOf<AFish> FishClass, const FVector& Location, const FRotator& Rotation, const FVector& AdultScale);
	/** Acteur actuellement tenu par la poignée physique du joueur (nullptr sinon). */
	const AActor* GetHeldActor() const;
	void SetState(EEcosystemState NewState, EEcosystemEndReason Reason);
	EEcosystemZone ZoneOf(float Stress) const;
	float CoefOf(float Stress) const;
	void DrawDebug() const;

	UPROPERTY(Transient)
	TObjectPtr<ULevelConfig> Config;

	TMap<TWeakObjectPtr<AFish>, FFishRecord> Fish;
	TArray<TWeakObjectPtr<AKelp>> Kelps;
	TArray<TWeakObjectPtr<AFood>> Foods;
	TArray<TWeakObjectPtr<AFishEgg>> Eggs;
	/** Temps restant (s) de chaque cadavre virtuel qui pollue encore. */
	TArray<float> CorpsePulses;

	FDelegateHandle ActorSpawnedHandle;

	EEcosystemState State = EEcosystemState::Playing;
	EEcosystemEndReason EndReason = EEcosystemEndReason::None;
	FEcosystemSnapshot Snapshot;

	float Pollution = 0.f;
	float Oxygen = 90.f;
	float Elapsed = 0.f;
	float DefeatTimer = 0.f;
	float ExtinctionTimer = 0.f;
	float Accumulator = 0.f;
	int32 EggsLost = 0;
	bool bLevelStarted = false;
	/** Interrupteurs d'exécution (le tutoriel les coupe), indépendants de ResetEcosystem. */
	bool bDefeatEnabled = true;
	bool bStarvationEnabled = true;

	// Naissance en cours : la découverte d'acteur (HandleActorSpawned) doit savoir que c'est un alevin.
	bool bSpawningFry = false;
	FVector PendingFryAdultScale = FVector::OneVector;
};
