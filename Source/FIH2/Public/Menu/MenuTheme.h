// Thème du menu principal : palette, tailles, durées d'animation. Toutes les valeurs visuelles du menu sont ici.
// Sans Data Asset (Project Settings > FIH2 > Menu Theme), les valeurs par défaut ci-dessous sont utilisées.
// Direction artistique : récif profond — bleu-nuit / turquoise, accent corail pour l'action principale.
// Unités : tailles en unités Slate à 1080p (mises à l'échelle par la règle DPI du projet), durées en secondes.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "MenuTheme.generated.h"

UCLASS(BlueprintType)
class FIH2_API UMenuTheme : public UDataAsset
{
	GENERATED_BODY()

public:
	UMenuTheme();

	/** Thème du projet, ou les valeurs par défaut de la classe. */
	static const UMenuTheme& Get();

	// ---------------------------------------------------------------- Textes
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Text")
	FText GameTitle;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Text")
	FText GameSubtitle;

	// ---------------------------------------------------------------- Palette
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Palette")
	FLinearColor PanelColor = FLinearColor(0.012f, 0.035f, 0.07f, 0.8f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Palette")
	FLinearColor PanelOutlineColor = FLinearColor(0.1f, 0.5f, 0.58f, 0.4f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Palette")
	FLinearColor ButtonColor = FLinearColor(0.02f, 0.07f, 0.12f, 0.85f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Palette")
	FLinearColor ButtonHoverColor = FLinearColor(0.03f, 0.17f, 0.23f, 0.95f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Palette")
	FLinearColor ButtonPressedColor = FLinearColor(0.015f, 0.05f, 0.08f, 0.95f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Palette")
	FLinearColor ButtonDisabledColor = FLinearColor(0.02f, 0.035f, 0.05f, 0.6f);

	/** Contour du widget qui a le focus (souris, clavier ou manette) : la « lueur ». */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Palette")
	FLinearColor FocusOutlineColor = FLinearColor(0.2f, 0.9f, 0.82f, 1.f);

	/** Accent corail de l'action principale. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Palette")
	FLinearColor AccentColor = FLinearColor(1.f, 0.36f, 0.24f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Palette")
	FLinearColor AccentHoverColor = FLinearColor(1.f, 0.5f, 0.32f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Palette")
	FLinearColor AccentPressedColor = FLinearColor(0.75f, 0.24f, 0.16f, 1.f);

	/** Texte posé sur l'accent (sombre, pour le contraste). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Palette")
	FLinearColor AccentTextColor = FLinearColor(0.03f, 0.02f, 0.02f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Palette")
	FLinearColor TextColor = FLinearColor(0.92f, 0.97f, 1.f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Palette")
	FLinearColor TextMutedColor = FLinearColor(0.55f, 0.72f, 0.8f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Palette")
	FLinearColor CompletedColor = FLinearColor(0.3f, 0.9f, 0.5f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Palette")
	FLinearColor LockedColor = FLinearColor(0.42f, 0.5f, 0.56f, 1.f);

	/** Ambre : compte à rebours et avertissements. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Palette")
	FLinearColor AlertColor = FLinearColor(1.f, 0.7f, 0.2f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Palette")
	FLinearColor FadeColor = FLinearColor(0.f, 0.005f, 0.012f, 1.f);

	// ---------------------------------------------------------------- Tailles
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Metrics", meta = (ClampMin = "0"))
	float CornerRadius = 14.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Metrics", meta = (ClampMin = "0"))
	float FocusOutlineWidth = 2.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Metrics", meta = (ClampMin = "0"))
	float PanelOutlineWidth = 1.f;

	/** Marge entre le bord de l'écran et les écrans du menu. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Metrics", meta = (ClampMin = "0"))
	float ScreenPadding = 72.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Metrics", meta = (ClampMin = "0"))
	float PanelPadding = 36.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Metrics")
	FVector2D ButtonSize = FVector2D(440.f, 68.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Metrics", meta = (ClampMin = "0"))
	float ButtonSpacing = 14.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Metrics")
	FVector2D CardSize = FVector2D(340.f, 430.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Metrics", meta = (ClampMin = "0"))
	float CardThumbnailHeight = 180.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Metrics", meta = (ClampMin = "0"))
	float CardSpacing = 24.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Metrics", meta = (ClampMin = "0"))
	float OptionRowHeight = 56.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Metrics", meta = (ClampMin = "0"))
	float OptionControlWidth = 380.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Metrics", meta = (ClampMin = "0"))
	float SettingsPanelWidth = 900.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Metrics", meta = (ClampMin = "0"))
	float SettingsListHeight = 560.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Metrics", meta = (ClampMin = "0"))
	float DialogWidth = 640.f;

	// ---------------------------------------------------------------- Polices (lisibilité : jamais sous 18 à 1080p)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fonts", meta = (ClampMin = "18"))
	int32 TitleFontSize = 84;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fonts", meta = (ClampMin = "18"))
	int32 SubtitleFontSize = 24;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fonts", meta = (ClampMin = "18"))
	int32 HeadingFontSize = 40;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fonts", meta = (ClampMin = "18"))
	int32 ButtonFontSize = 26;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fonts", meta = (ClampMin = "18"))
	int32 BodyFontSize = 20;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fonts", meta = (ClampMin = "18"))
	int32 SmallFontSize = 18;

	// ---------------------------------------------------------------- Animations
	/** Fondu depuis le noir à l'ouverture du menu. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation", meta = (ClampMin = "0"))
	float IntroFadeDuration = 1.2f;

	/** Changement d'écran : glissement + fondu. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation", meta = (ClampMin = "0"))
	float ScreenTransitionDuration = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation", meta = (ClampMin = "0"))
	float ScreenSlideDistance = 90.f;

	/** Entrée en cascade : décalage entre deux éléments. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation", meta = (ClampMin = "0"))
	float StaggerDelay = 0.07f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation", meta = (ClampMin = "0"))
	float EntryDuration = 0.45f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation", meta = (ClampMin = "0"))
	float EntryOffset = 60.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation", meta = (ClampMin = "0"))
	float HoverDuration = 0.12f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation", meta = (ClampMin = "1"))
	float HoverScale = 1.05f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation", meta = (ClampMin = "0"))
	float PressDuration = 0.08f;

	/** Écrasement à la pression (X, Y). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation")
	FVector2D PressScale = FVector2D(0.97f, 0.9f);

	/** Fondu au noir au lancement d'un niveau. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation", meta = (ClampMin = "0"))
	float LaunchFadeDuration = 0.7f;

	/** Flottement du titre : amplitude verticale et durée d'un aller simple. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation", meta = (ClampMin = "0"))
	float TitleFloatAmplitude = 8.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation", meta = (ClampMin = "0.1"))
	float TitleFloatHalfPeriod = 2.6f;

	// ---------------------------------------------------------------- Réglages
	/** Délai avant le retour automatique à l'ancien mode vidéo si le joueur ne confirme pas. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings", meta = (ClampMin = "3"))
	float VideoConfirmSeconds = 10.f;

	/** Limites de FPS proposées (0 = illimité). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings")
	TArray<int32> FrameRateLimits = { 30, 60, 120, 144, 0 };

	/** Plage (min, max) et pas du curseur de sensibilité caméra. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings")
	FVector2D CameraSensitivityRange = FVector2D(0.2f, 3.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings", meta = (ClampMin = "0.01"))
	float CameraSensitivityStep = 0.1f;

	/** Pas des curseurs de volume (0–1). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings", meta = (ClampMin = "0.01"))
	float VolumeStep = 0.05f;
};
