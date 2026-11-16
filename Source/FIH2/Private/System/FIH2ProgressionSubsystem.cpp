#include "System/FIH2ProgressionSubsystem.h"

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Menu/LevelCatalog.h"
#include "Sound/SoundClass.h"
#include "Sound/SoundMix.h"
#include "System/FIH2ProjectSettings.h"
#include "System/LevelConfig.h"

DEFINE_LOG_CATEGORY_STATIC(LogFIH2Progression, Log, All);

UFIH2ProgressionSubsystem* UFIH2ProgressionSubsystem::Get(const UObject* WorldContext)
{
	const UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::ReturnNull) : nullptr;
	const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	return GameInstance ? GameInstance->GetSubsystem<UFIH2ProgressionSubsystem>() : nullptr;
}

void UFIH2ProgressionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	LoadOrCreate();
}

void UFIH2ProgressionSubsystem::LoadOrCreate()
{
	const UFIH2ProjectSettings* Settings = UFIH2ProjectSettings::Get();
	SaveData = nullptr;

	if (UGameplayStatics::DoesSaveGameExist(Settings->SaveSlotName, Settings->SaveUserIndex))
	{
		// Un fichier illisible ou d'une autre classe donne nullptr : on repart d'un profil neuf.
		UFIH2SaveGame* Loaded = Cast<UFIH2SaveGame>(UGameplayStatics::LoadGameFromSlot(Settings->SaveSlotName, Settings->SaveUserIndex));
		if (Loaded && Loaded->SaveVersion == UFIH2SaveGame::CurrentVersion)
		{
			SaveData = Loaded;
		}
		else
		{
			UE_LOG(LogFIH2Progression, Warning, TEXT("Sauvegarde « %s » illisible ou de version %d (attendue %d) : profil par défaut."),
				*Settings->SaveSlotName, Loaded ? Loaded->SaveVersion : -1, UFIH2SaveGame::CurrentVersion);
		}
	}

	if (!SaveData)
	{
		SaveData = Cast<UFIH2SaveGame>(UGameplayStatics::CreateSaveGameObject(UFIH2SaveGame::StaticClass()));
		SaveData->SaveVersion = UFIH2SaveGame::CurrentVersion;
		SaveData->Settings = Settings->DefaultUserSettings;
	}

	// Valeurs hors bornes (fichier modifié à la main) : ramenées dans les plages valides.
	FFIH2UserSettings& S = SaveData->Settings;
	S.MasterVolume = FMath::Clamp(S.MasterVolume, 0.f, 1.f);
	S.MusicVolume = FMath::Clamp(S.MusicVolume, 0.f, 1.f);
	S.SfxVolume = FMath::Clamp(S.SfxVolume, 0.f, 1.f);
	if (!FMath::IsFinite(S.CameraSensitivity) || S.CameraSensitivity <= 0.f)
	{
		S.CameraSensitivity = Settings->DefaultUserSettings.CameraSensitivity;
	}
}

bool UFIH2ProgressionSubsystem::SaveNow()
{
	if (!SaveData)
	{
		return false;
	}
	const UFIH2ProjectSettings* Settings = UFIH2ProjectSettings::Get();
	SaveData->SaveVersion = UFIH2SaveGame::CurrentVersion;
	const bool bSaved = UGameplayStatics::SaveGameToSlot(SaveData, Settings->SaveSlotName, Settings->SaveUserIndex);
	if (!bSaved)
	{
		UE_LOG(LogFIH2Progression, Warning, TEXT("Échec de l'écriture de la sauvegarde « %s »."), *Settings->SaveSlotName);
	}
	return bSaved;
}

// ------------------------------------------------------------------ Progression

bool UFIH2ProgressionSubsystem::IsTutorialCompleted() const
{
	return SaveData && SaveData->bTutorialCompleted;
}

bool UFIH2ProgressionSubsystem::IsLevelCompleted(FName LevelId) const
{
	return SaveData && !LevelId.IsNone() && SaveData->CompletedLevelIds.Contains(LevelId);
}

ELevelCardState UFIH2ProgressionSubsystem::GetLevelState(const FLevelCatalogEntry& Entry) const
{
	if (Entry.bComingSoon || Entry.Map.IsNull())
	{
		return ELevelCardState::Locked;
	}
	if (IsLevelCompleted(Entry.Id))
	{
		return ELevelCardState::Completed;
	}
	if (!Entry.RequiredLevelId.IsNone() && !IsLevelCompleted(Entry.RequiredLevelId))
	{
		return ELevelCardState::Locked;
	}
	return ELevelCardState::Available;
}

void UFIH2ProgressionSubsystem::MarkLevelCompleted(FName LevelId)
{
	if (!SaveData || LevelId.IsNone() || SaveData->CompletedLevelIds.Contains(LevelId))
	{
		return;
	}
	SaveData->CompletedLevelIds.Add(LevelId);
	SaveNow();
	OnProgressionChanged.Broadcast();
}

void UFIH2ProgressionSubsystem::NotifyTutorialFinished(const UObject* WorldContext, bool bWon, bool bSkipped)
{
	if (!SaveData || !bWon)
	{
		return;
	}
	if (bSkipped && !UFIH2ProjectSettings::Get()->bSkippedTutorialCountsAsCompleted)
	{
		UE_LOG(LogFIH2Progression, Log, TEXT("Tutoriel gagné après « Passer » : non compté comme complété."));
		return;
	}

	const bool bWasCompleted = SaveData->bTutorialCompleted;
	SaveData->bTutorialCompleted = true;

	// Le niveau qui porte le tutoriel est marqué complété lui aussi (carte ● du sélecteur).
	const UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::ReturnNull) : nullptr;
	const ULevelCatalog* Catalog = GetCatalog();
	const FLevelCatalogEntry* Entry = Catalog ? Catalog->FindEntryForWorld(World) : nullptr;
	if (Entry && !Entry->Id.IsNone())
	{
		SaveData->CompletedLevelIds.AddUnique(Entry->Id);
	}

	SaveNow();
	if (!bWasCompleted)
	{
		UE_LOG(LogFIH2Progression, Log, TEXT("Tutoriel complété."));
	}
	OnProgressionChanged.Broadcast();
}

void UFIH2ProgressionSubsystem::NotifyLevelWon(const UWorld* World)
{
	const ULevelCatalog* Catalog = GetCatalog();
	const FLevelCatalogEntry* Entry = Catalog ? Catalog->FindEntryForWorld(World) : nullptr;
	// Un niveau tutoriel n'est complété que par NotifyTutorialFinished (qui connaît le « Passer »).
	if (Entry && !Entry->bIsTutorial)
	{
		MarkLevelCompleted(Entry->Id);
	}
}

void UFIH2ProgressionSubsystem::ResetProgression()
{
	if (!SaveData)
	{
		return;
	}
	SaveData->bTutorialCompleted = false;
	SaveData->CompletedLevelIds.Reset();
	SaveNow();
	OnProgressionChanged.Broadcast();
}

// ------------------------------------------------------------------ Catalogue

ULevelCatalog* UFIH2ProgressionSubsystem::GetCatalog() const
{
	if (!CachedCatalog)
	{
		CachedCatalog = UFIH2ProjectSettings::Get()->LevelCatalog.LoadSynchronous();
	}
	return CachedCatalog;
}

ULevelConfig* UFIH2ProgressionSubsystem::FindLevelConfigForWorld(const UWorld* World) const
{
	const ULevelCatalog* Catalog = GetCatalog();
	const FLevelCatalogEntry* Entry = Catalog ? Catalog->FindEntryForWorld(World) : nullptr;
	return Entry ? Entry->LevelConfig.LoadSynchronous() : nullptr;
}

void UFIH2ProgressionSubsystem::ReturnToMainMenu(const UObject* WorldContextObject)
{
	SaveNow();
	const TSoftObjectPtr<UWorld>& MenuMap = UFIH2ProjectSettings::Get()->MainMenuMap;
	if (MenuMap.IsNull())
	{
		UE_LOG(LogFIH2Progression, Warning, TEXT("Aucune carte de menu (Project Settings > FIH2 > Main Menu Map)."));
		return;
	}
	UGameplayStatics::OpenLevel(WorldContextObject, FName(*MenuMap.ToSoftObjectPath().GetLongPackageName()));
}

// ------------------------------------------------------------------ Réglages

FFIH2UserSettings UFIH2ProgressionSubsystem::GetUserSettings() const
{
	return SaveData ? SaveData->Settings : UFIH2ProjectSettings::Get()->DefaultUserSettings;
}

FFIH2UserSettings UFIH2ProgressionSubsystem::GetDefaultUserSettings() const
{
	return UFIH2ProjectSettings::Get()->DefaultUserSettings;
}

float UFIH2ProgressionSubsystem::GetCameraSensitivity() const
{
	return GetUserSettings().CameraSensitivity;
}

void UFIH2ProgressionSubsystem::SetUserSettings(const UObject* WorldContextObject, const FFIH2UserSettings& NewSettings, bool bSaveNow)
{
	if (!SaveData)
	{
		return;
	}
	SaveData->Settings = NewSettings;
	ApplyAudioSettings(WorldContextObject, false);
	if (bSaveNow)
	{
		SaveNow();
	}
}

void UFIH2ProgressionSubsystem::ApplyAudioSettings(const UObject* WorldContext, bool bRepushMix)
{
	const UFIH2ProjectSettings* Settings = UFIH2ProjectSettings::Get();
	USoundMix* Mix = Settings->UserSoundMix.LoadSynchronous();
	if (!WorldContext || !Mix)
	{
		if (!Mix && !bWarnedAudioNotWired)
		{
			// À BRANCHER : renseigner UserSoundMix et les SoundClass dans Project Settings > FIH2 > Audio.
			UE_LOG(LogFIH2Progression, Warning, TEXT("Volumes non appliqués : aucun SoundMix dans Project Settings > FIH2 > Audio."));
			bWarnedAudioNotWired = true;
		}
		return;
	}

	const FFIH2UserSettings UserSettings = GetUserSettings();
	auto ApplyVolume = [WorldContext, Mix](const TSoftObjectPtr<USoundClass>& ClassPtr, float Volume)
	{
		if (USoundClass* SoundClass = ClassPtr.LoadSynchronous())
		{
			UGameplayStatics::SetSoundMixClassOverride(WorldContext, Mix, SoundClass, Volume, 1.f, 0.f, true);
		}
	};
	// À BRANCHER : il n'existe encore aucun son. Chaque futur son doit utiliser la SoundClass Musique ou Effets
	// (enfants de la classe générale) pour que ces volumes aient un effet audible.
	ApplyVolume(Settings->MasterSoundClass, UserSettings.MasterVolume);
	ApplyVolume(Settings->MusicSoundClass, UserSettings.MusicVolume);
	ApplyVolume(Settings->SfxSoundClass, UserSettings.SfxVolume);

	if (bRepushMix)
	{
		// Un seul exemplaire du mix actif, quel que soit le nombre de changements de carte.
		UGameplayStatics::PopSoundMixModifier(WorldContext, Mix);
		UGameplayStatics::PushSoundMixModifier(WorldContext, Mix);
	}
}

void UFIH2ProgressionSubsystem::HandleWorldBeginPlay(UWorld& World)
{
	// Réglages appliqués au démarrage de chaque carte (donc au lancement du jeu, pas seulement à l'ouverture du menu).
	// La vidéo est déjà appliquée par le moteur (UGameUserSettings) au démarrage.
	ApplyAudioSettings(&World, true);
}

// ------------------------------------------------------------------ Relais par monde

bool UFIH2WorldBootSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

void UFIH2WorldBootSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	if (UFIH2ProgressionSubsystem* Progression = UFIH2ProgressionSubsystem::Get(&InWorld))
	{
		Progression->HandleWorldBeginPlay(InWorld);
	}
	if (UEcosystemSubsystem* Ecosystem = InWorld.GetSubsystem<UEcosystemSubsystem>())
	{
		Ecosystem->OnStateChanged.AddUniqueDynamic(this, &UFIH2WorldBootSubsystem::HandleEcosystemStateChanged);
	}
}

void UFIH2WorldBootSubsystem::HandleEcosystemStateChanged(EEcosystemState NewState)
{
	if (NewState != EEcosystemState::Won)
	{
		return;
	}
	if (UFIH2ProgressionSubsystem* Progression = UFIH2ProgressionSubsystem::Get(this))
	{
		Progression->NotifyLevelWon(GetWorld());
	}
}
