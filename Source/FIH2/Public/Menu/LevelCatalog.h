// Catalogue data-driven des niveaux : une entrée = une carte du sélecteur de niveau.
// Ajouter un niveau = ajouter une entrée dans le Data Asset (aucun code).

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "LevelCatalog.generated.h"

class ULevelConfig;
class UTexture2D;
class UWorld;

USTRUCT(BlueprintType)
struct FLevelCatalogEntry
{
	GENERATED_BODY()

	/** Identifiant stable, écrit dans la sauvegarde : ne jamais le renommer une fois publié. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Level")
	FName Id;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Level")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Level", meta = (MultiLine = true))
	FText Description;

	/** Vignette de la carte (vide = le numéro d'ordre est affiché à la place). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Level")
	TSoftObjectPtr<UTexture2D> Thumbnail;

	/** Carte à ouvrir. Vide = niveau non jouable (carte « Bientôt »). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Level")
	TSoftObjectPtr<UWorld> Map;

	/** Config d'écosystème du niveau (vide = config par défaut de UEcosystemSubsystem). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Level")
	TSoftObjectPtr<ULevelConfig> LevelConfig;

	/** Ordre d'affichage (croissant). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Level")
	int32 SortOrder = 0;

	/** Niveau à avoir complété pour déverrouiller celui-ci (None = aucun prérequis). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Level")
	FName RequiredLevelId;

	/** Le niveau lancé par « Jouer » tant que le tutoriel n'est pas complété. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Level")
	bool bIsTutorial = false;

	/** Carte d'annonce : toujours verrouillée, affichée « Bientôt ». */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Level")
	bool bComingSoon = false;
};

UCLASS(BlueprintType)
class FIH2_API ULevelCatalog : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Levels", meta = (TitleProperty = "Id"))
	TArray<FLevelCatalogEntry> Entries;

	const FLevelCatalogEntry* FindEntry(FName Id) const;

	/** Entrée dont la carte est celle de World (préfixe PIE ignoré). */
	const FLevelCatalogEntry* FindEntryForWorld(const UWorld* World) const;

	/** Première entrée marquée bIsTutorial, dans l'ordre d'affichage. */
	const FLevelCatalogEntry* FindTutorialEntry() const;

	/** Copie triée par SortOrder (ordre du tableau conservé à égalité). */
	TArray<FLevelCatalogEntry> GetSortedEntries() const;
};
