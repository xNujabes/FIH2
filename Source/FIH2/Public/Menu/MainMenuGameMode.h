// GameMode et PlayerController de la carte du menu principal.
// Le GameMode met l'écosystème en mode « ambiant » (personne ne meurt, ni victoire ni défaite, pas de tutoriel)
// uniquement avec des données et les interrupteurs existants de UEcosystemSubsystem, puis peuple la cuve.
// Le PlayerController n'a ni pion ni action de jeu : pas de drag, pas de sélection, pas de HUD.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerController.h"
#include "Engine/EngineBaseTypes.h"
#include "MainMenuGameMode.generated.h"

class UMainMenuRootWidget;
class UMenuAmbientConfig;

UCLASS()
class FIH2_API AMainMenuGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AMainMenuGameMode();

	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;

	/** Peuplement et config d'écosystème du fond vivant. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Menu")
	TObjectPtr<UMenuAmbientConfig> AmbientConfig;

protected:
	virtual void BeginPlay() override;

private:
	void SpawnPopulation();
};

UCLASS()
class FIH2_API AMainMenuPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AMainMenuPlayerController();

	/** Widget racine du menu (vide = UMainMenuRootWidget natif, arbre construit en C++). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Menu")
	TSubclassOf<UMainMenuRootWidget> MenuWidgetClass;

	/**
	 * Rend au viewport le mode d'entrée qu'il avait avant le menu. Le mode « UI seulement » est porté par le
	 * viewport, pas par le contrôleur : sans cela il survit au changement de carte et le niveau n'aurait plus
	 * aucune entrée de jeu. Appelé avant l'ouverture d'un niveau et à la fin du contrôleur (sans effet la 2e fois).
	 */
	void RestoreGameInput();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UPROPERTY(Transient)
	TObjectPtr<UMainMenuRootWidget> MenuWidget;

	// État du viewport relevé avant de passer en « UI seulement ».
	bool bInputModeOverridden = false;
	bool bPreviousIgnoreInput = false;
	EMouseCaptureMode PreviousCaptureMode = EMouseCaptureMode::CapturePermanently_IncludingInitialMouseDown;
	EMouseLockMode PreviousLockMode = EMouseLockMode::LockOnCapture;
};
