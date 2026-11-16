// Réglages de projet de FIH2 (Project Settings > Game > FIH2), stockés dans DefaultGame.ini.
// Tout ce qui relie le code aux assets du menu et de la sauvegarde est ici : rien en dur dans le code.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "System/FIH2SaveGame.h"
#include "FIH2ProjectSettings.generated.h"

class ULevelCatalog;
class UMenuTheme;
class USoundClass;
class USoundMix;
class UWorld;

UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "FIH2"))
class FIH2_API UFIH2ProjectSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	static const UFIH2ProjectSettings* Get() { return GetDefault<UFIH2ProjectSettings>(); }

	virtual FName GetCategoryName() const override { return TEXT("Game"); }

	// ---------------------------------------------------------------- Sauvegarde
	/** Nom du slot unique de sauvegarde. */
	UPROPERTY(Config, EditAnywhere, Category = "Save")
	FString SaveSlotName = TEXT("FIH2_Profile");

	UPROPERTY(Config, EditAnywhere, Category = "Save", meta = (ClampMin = "0"))
	int32 SaveUserIndex = 0;

	// ---------------------------------------------------------------- Progression
	/**
	 * Faux (défaut) : seule une victoire du tutoriel SANS « Passer le tutoriel » le marque comme complété.
	 * Vrai : passer le tutoriel puis gagner l'étape de maintien suffit.
	 */
	UPROPERTY(Config, EditAnywhere, Category = "Progression")
	bool bSkippedTutorialCountsAsCompleted = false;

	// ---------------------------------------------------------------- Niveaux
	/** Catalogue des niveaux affichés par le sélecteur. */
	UPROPERTY(Config, EditAnywhere, Category = "Levels")
	TSoftObjectPtr<ULevelCatalog> LevelCatalog;

	/** Carte du menu principal (bouton « Retour au menu »). */
	UPROPERTY(Config, EditAnywhere, Category = "Levels")
	TSoftObjectPtr<UWorld> MainMenuMap;

	// ---------------------------------------------------------------- Menu
	/** Thème du menu (vide = valeurs par défaut de UMenuTheme). */
	UPROPERTY(Config, EditAnywhere, Category = "Menu")
	TSoftObjectPtr<UMenuTheme> MenuTheme;

	// ---------------------------------------------------------------- Audio
	/** SoundMix qui porte les volumes du joueur (poussé à chaque début de monde). Vide = volumes non appliqués. */
	UPROPERTY(Config, EditAnywhere, Category = "Audio")
	TSoftObjectPtr<USoundMix> UserSoundMix;

	UPROPERTY(Config, EditAnywhere, Category = "Audio")
	TSoftObjectPtr<USoundClass> MasterSoundClass;

	UPROPERTY(Config, EditAnywhere, Category = "Audio")
	TSoftObjectPtr<USoundClass> MusicSoundClass;

	UPROPERTY(Config, EditAnywhere, Category = "Audio")
	TSoftObjectPtr<USoundClass> SfxSoundClass;

	// ---------------------------------------------------------------- Valeurs par défaut
	/** Réglages d'un nouveau profil, et cible du bouton « Réinitialiser ». */
	UPROPERTY(Config, EditAnywhere, Category = "Defaults")
	FFIH2UserSettings DefaultUserSettings;
};
