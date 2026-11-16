// Sauvegarde unique du profil : progression (tutoriel, niveaux complétés) et réglages audio / jeu.
// Les réglages vidéo ne sont pas ici : UGameUserSettings les persiste déjà dans son propre .ini.
// Format versionné : une version inconnue ou un fichier illisible redonne les valeurs par défaut
// (voir UFIH2ProgressionSubsystem::LoadOrCreate).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "FIH2SaveGame.generated.h"

/** Réglages du joueur persistés dans la sauvegarde (audio + jeu). */
USTRUCT(BlueprintType)
struct FFIH2UserSettings
{
	GENERATED_BODY()

	/** VOLUME_GENERAL [0–1] */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio", meta = (ClampMin = "0", ClampMax = "1"))
	float MasterVolume = 1.f;

	/** VOLUME_MUSIQUE [0–1] */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio", meta = (ClampMin = "0", ClampMax = "1"))
	float MusicVolume = 0.8f;

	/** VOLUME_EFFETS [0–1] */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio", meta = (ClampMin = "0", ClampMax = "1"))
	float SfxVolume = 1.f;

	/** SENSIBILITE_CAMERA : multiplicateur (1 = vitesse d'origine). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game", meta = (ClampMin = "0.1", ClampMax = "5"))
	float CameraSensitivity = 1.f;
};

UCLASS()
class FIH2_API UFIH2SaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	/** Version courante du format. À incrémenter à chaque changement incompatible. */
	static const int32 CurrentVersion;

	UPROPERTY()
	int32 SaveVersion = 0;

	/** Le tutoriel a été gagné au moins une fois (voir UFIH2ProgressionSubsystem::NotifyTutorialFinished). */
	UPROPERTY()
	bool bTutorialCompleted = false;

	/** Identifiants (FLevelCatalogEntry::Id) des niveaux gagnés. */
	UPROPERTY()
	TArray<FName> CompletedLevelIds;

	UPROPERTY()
	FFIH2UserSettings Settings;
};
