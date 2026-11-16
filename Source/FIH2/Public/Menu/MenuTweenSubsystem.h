// Tweens légers pour l'UI : opacité + translation / échelle / rotation du RenderTransform d'un widget, avec easings.
// Aucun Tick de widget : un FTSTicker est enregistré seulement tant qu'au moins un tween est actif, puis retiré.
// Temps réel (insensible à la pause et à la dilatation du temps de jeu).

#pragma once

#include "CoreMinimal.h"
#include "Containers/Ticker.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "MenuTweenSubsystem.generated.h"

class UWidget;

enum class EMenuEase : uint8
{
	Linear,
	InCubic,
	OutCubic,
	InOutCubic,
	InOutSine,
	/** Dépasse légèrement la cible puis revient (entrées « vivantes »). */
	OutBack
};

/** Description d'un tween. Une valeur « From » absente = valeur actuelle du widget au démarrage. */
struct FMenuTweenSpec
{
	float Duration = 0.3f;
	float Delay = 0.f;
	EMenuEase Ease = EMenuEase::OutCubic;
	/** Aller-retour sans fin entre From et To (flottement). À arrêter avec Stop. */
	bool bPingPongLoop = false;

	TOptional<float> FromOpacity;
	TOptional<float> ToOpacity;
	TOptional<FVector2D> FromTranslation;
	TOptional<FVector2D> ToTranslation;
	TOptional<FVector2D> FromScale;
	TOptional<FVector2D> ToScale;
	TOptional<float> FromAngle;
	TOptional<float> ToAngle;

	/** Appelé une fois à la fin (jamais pour une boucle, ni si le tween est remplacé ou arrêté). */
	TFunction<void()> OnFinished;
};

UCLASS()
class FIH2_API UMenuTweenSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	static UMenuTweenSubsystem* Get(const UObject* WorldContext);

	virtual void Deinitialize() override;

	/**
	 * Lance un tween sur Target. Un tween déjà actif sur le même widget ET le même canal est remplacé.
	 * Utiliser des canaux différents pour animer en parallèle (ex. « Enter » pour l'entrée, « Hover » pour l'échelle).
	 */
	void Play(UWidget* Target, const FMenuTweenSpec& Spec, FName Channel = NAME_None);

	/** Arrête les tweens de Target sur ce canal (NAME_None = tous ses canaux), sans appeler OnFinished. */
	void Stop(UWidget* Target, FName Channel = NAME_None);

	bool IsAnimating() const { return Active.Num() > 0; }

	static float ApplyEase(EMenuEase Ease, float Alpha);

private:
	struct FActiveTween
	{
		TWeakObjectPtr<UWidget> Target;
		FName Channel;
		FMenuTweenSpec Spec;
		float Elapsed = 0.f;
		bool bStarted = false;
		float StartOpacity = 1.f;
		FVector2D StartTranslation = FVector2D::ZeroVector;
		FVector2D StartScale = FVector2D::UnitVector;
		float StartAngle = 0.f;
	};

	bool TickTweens(float DeltaTime);
	static void ResolveStart(FActiveTween& Tween, const UWidget& Widget);
	static void ApplyTween(const FActiveTween& Tween, UWidget& Widget, float EasedAlpha);

	TArray<FActiveTween> Active;
	FTSTicker::FDelegateHandle TickerHandle;
};
