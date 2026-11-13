// Sous-système du tutoriel guidé : déroule une UTutorialSequence étape par étape.
//
// Entièrement piloté par évènements (aucun Tick) : sélection / loupe / lâcher (FGameplayEvents),
// repas des poissons, actions d'entrée de la caméra, pas de simulation de l'écosystème (OnUpdated),
// et minuteries pour les délais. Avant l'étape de maintien, la défaite et la mort par la faim sont
// désactivées dans UEcosystemSubsystem ; l'étape de maintien les réactive et décide de la victoire.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "System/EcosystemSubsystem.h"
#include "System/TutorialSequence.h"
#include "TutorialSubsystem.generated.h"

class AFish;
class AFishEgg;
class UMaterialInterface;
class UMeshComponent;
class UTutorialPanelWidget;
struct FInputActionInstance;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTutorialStepChanged, int32, StepIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnTutorialMaintainUpdated, float, RemainingSeconds, bool, bWaitingOutOfRed);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTutorialMaintainReset);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTutorialFinished, bool, bWon);

UCLASS()
class FIH2_API UTutorialSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	static UTutorialSubsystem* Get(const UObject* WorldContext);

	// UWorldSubsystem
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

	/** Lance la séquence (appelé automatiquement si la séquence du niveau a bStartAutomatically). */
	UFUNCTION(BlueprintCallable, Category = "Tutorial")
	void StartTutorial(UTutorialSequence* InSequence);

	/** Bouton « Suivant » : ne fait avancer que les étapes de type NextButton. */
	UFUNCTION(BlueprintCallable, Category = "Tutorial")
	void Next();

	/** Bouton « Passer le tutoriel » : saute directement à l'étape de maintien. */
	UFUNCTION(BlueprintCallable, Category = "Tutorial")
	void Skip();

	UFUNCTION(BlueprintPure, Category = "Tutorial")
	bool IsRunning() const { return Sequence != nullptr && CurrentIndex != INDEX_NONE; }

	UFUNCTION(BlueprintPure, Category = "Tutorial")
	int32 GetCurrentStepIndex() const { return CurrentIndex; }

	UPROPERTY(BlueprintAssignable, Category = "Tutorial")
	FOnTutorialStepChanged OnStepChanged;

	UPROPERTY(BlueprintAssignable, Category = "Tutorial")
	FOnTutorialMaintainUpdated OnMaintainUpdated;

	UPROPERTY(BlueprintAssignable, Category = "Tutorial")
	FOnTutorialMaintainReset OnMaintainReset;

	UPROPERTY(BlueprintAssignable, Category = "Tutorial")
	FOnTutorialFinished OnTutorialFinished;

private:
	const FTutorialStep* CurrentStep() const;
	void EnterStep(int32 Index);
	void LeaveCurrentStep();
	void CompleteCurrentStep();
	void AdvanceStep();
	void FinishTutorial(bool bWon);

	// Conditions
	void EvaluateWorldConditions();
	bool IsGaugeConditionMet(const FEcosystemSnapshot& Snapshot) const;
	bool AreTaggedActorsNear() const;

	// Maintien
	void EnterMaintain();
	void UpdateMaintain(const FEcosystemSnapshot& Snapshot);

	// Surbrillance des acteurs ciblés par l'étape
	void ApplyHighlight(FName Tag);
	void RemoveHighlight();

	/** Acteurs du niveau portant ce tag. */
	TArray<AActor*> GetActorsWithTag(FName Tag) const;

	// Actions d'entrée (caméra)
	void BindInputActions(const FTutorialStep& Step);
	void UnbindInputActions();
	void HandleInputAction(const FInputActionInstance& Instance);

	// Évènements de jeu
	void HandleActorSelected(AActor* Actor);
	void HandleFocusUsed(AActor* Actor);
	void HandleActorReleased(AActor* Actor);
	void HandleFishAte(AFish* Fish, AActor* Food);
	void HandleEggLaid(AFishEgg* Egg);
	void HandleEggHatched(AFishEgg* Egg, AFish* Fry);
	void HandleEggDied(AFishEgg* Egg);

	/** Surligne des acteurs via le contrôleur (retirés par RemoveHighlight). */
	void HighlightActors(const TArray<AActor*>& Actors);

	UFUNCTION()
	void HandleEcosystemUpdated(const FEcosystemSnapshot& Snapshot);

	UFUNCTION()
	void HandleEcosystemStateChanged(EEcosystemState NewState);

	UPROPERTY(Transient)
	TObjectPtr<UTutorialSequence> Sequence;

	UPROPERTY(Transient)
	TObjectPtr<UTutorialPanelWidget> Panel;

	TWeakObjectPtr<UEcosystemSubsystem> Ecosystem;

	int32 CurrentIndex = INDEX_NONE;
	double StepEnterTime = 0.0;
	bool bStepCompleting = false;

	FTimerHandle StepTimer;     // Delay
	FTimerHandle TimeoutTimer;  // Timeout anti-blocage
	FTimerHandle CompleteTimer; // attente de MinDisplayTime

	TArray<uint32> InputBindingHandles;

	/** Nourriture lâchée par le joueur pendant l'étape (condition FishAte avec bRequirePlayerFood). */
	TSet<TWeakObjectPtr<AActor>> PlayerReleasedFood;

	/** Acteurs surlignés par l'étape en cours (via AMouseController::SetPersistentHighlight). */
	TArray<TWeakObjectPtr<AActor>> HighlightedActors;

	// Maintien
	float MaintainRemaining = 0.f;
	float LastSnapshotTime = -1.f;
	bool bMaintainWaiting = false;

	FDelegateHandle SelectedHandle;
	FDelegateHandle FocusHandle;
	FDelegateHandle ReleasedHandle;
	FDelegateHandle FishAteHandle;
	FDelegateHandle EggLaidHandle;
	FDelegateHandle EggHatchedHandle;
	FDelegateHandle EggDiedHandle;

	/** Dernier acteur apparu qui intéresse le tutoriel (œuf pondu, puis alevin né). */
	TWeakObjectPtr<AActor> LastEventActor;
};
