// Évènements de jeu natifs (non exposés aux Blueprints), sur le modèle de FInteractableEvents.
// Ils servent aux systèmes qui observent le joueur sans dépendre de ses classes (ex. UTutorialSubsystem).
// Un Broadcast sans abonné ne coûte presque rien : personne n'écoute hors tutoriel.

#pragma once

#include "CoreMinimal.h"

class AActor;
class AFish;
class AFishEgg;

/** Un poisson vient de manger (Food = la nourriture consommée). */
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnFishAteNative, AFish* /*Fish*/, AActor* /*Food*/);
/** Évènement joueur portant un acteur (nullptr possible pour la sélection : désélection). */
DECLARE_MULTICAST_DELEGATE_OneParam(FOnPlayerActorNative, AActor* /*Actor*/);
/** Évènement de cycle de vie d'un œuf. */
DECLARE_MULTICAST_DELEGATE_OneParam(FOnEggNative, AFishEgg* /*Egg*/);
/** Un œuf vient d'éclore (Egg est en cours de destruction ; Fry peut être nul si l'apparition a échoué). */
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnEggHatchedNative, AFishEgg* /*Egg*/, AFish* /*Fry*/);

struct FIH2_API FGameplayEvents
{
	/** Émis par AFood quand un poisson la mange. */
	static FOnFishAteNative& OnFishAte();

	/** Émis par AMouseController quand la sélection change (nullptr = plus rien de sélectionné). */
	static FOnPlayerActorNative& OnActorSelected();

	/** Émis par AMouseController quand le joueur lâche (ou lance) un acteur qu'il tenait. */
	static FOnPlayerActorNative& OnActorReleased();

	/** Émis par AMouseController quand la loupe déplace la caméra vers l'acteur sélectionné. */
	static FOnPlayerActorNative& OnFocusUsed();

	/** Émis par UEcosystemSubsystem quand un poisson pond. */
	static FOnEggNative& OnEggLaid();

	/** Émis par UEcosystemSubsystem quand un œuf éclot. */
	static FOnEggHatchedNative& OnEggHatched();

	/** Émis par UEcosystemSubsystem quand un œuf meurt (pollution en zone rouge). */
	static FOnEggNative& OnEggDied();
};
