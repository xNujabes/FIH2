// Écrans du menu principal, empilés par UMainMenuRootWidget :
//   UMainMenuScreen (Jouer / Réglages / Quitter), ULevelSelectScreen (cartes de niveau),
//   USettingsScreen (audio, vidéo, jeu), UConfirmDialogScreen (confirmation, avec compte à rebours optionnel).
// Arbres UMG construits en C++ (même technique que UTutorialPanelWidget).

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Engine/TimerHandle.h"
#include "Menu/LevelCatalog.h"
#include "Menu/MenuButtonWidget.h"
#include "System/FIH2ProgressionSubsystem.h"
#include "System/FIH2SaveGame.h"
#include "MenuScreens.generated.h"

class UBorder;
class UHorizontalBox;
class UImage;
class UMainMenuRootWidget;
class UMenuOptionRowWidget;
class UPanelWidget;
class UTextBlock;
class UVerticalBox;

// ====================================================================== Base

UCLASS(Abstract)
class FIH2_API UMenuScreenWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual bool Initialize() override;

	void SetRoot(UMainMenuRootWidget* InRoot) { Root = InRoot; }

	/** L'écran devient visible (début de la transition d'entrée). */
	virtual void OnScreenShown();
	/** L'écran n'est plus affiché. */
	virtual void OnScreenHidden() {}
	/** Widget qui reçoit le focus à l'arrivée sur l'écran. */
	virtual UWidget* GetDefaultFocusWidget() const { return DefaultFocus.Get(); }
	/** Échap / bouton B. Renvoie true si l'écran a traité le retour. */
	virtual bool HandleBack();

protected:
	virtual void BuildScreen() {}

	UMainMenuRootWidget* GetRoot() const { return Root.Get(); }

	/** Panneau semi-transparent arrondi du thème. */
	UBorder* MakePanel(FName Name = NAME_None);
	UMenuButtonWidget* MakeButton(const FText& Label, bool bAccent, FName Name = NAME_None);

	/** Entrée en cascade (fondu + glissement) des widgets de StaggerWidgets. */
	void PlayStagger(const FVector2D& FromOffset);

	TArray<TWeakObjectPtr<UWidget>> StaggerWidgets;
	TWeakObjectPtr<UWidget> DefaultFocus;

private:
	TWeakObjectPtr<UMainMenuRootWidget> Root;
};

// ====================================================================== Écran principal

UCLASS()
class FIH2_API UMainMenuScreen : public UMenuScreenWidget
{
	GENERATED_BODY()

public:
	virtual void OnScreenShown() override;
	virtual void OnScreenHidden() override;
	virtual bool HandleBack() override;

protected:
	virtual void BuildScreen() override;

private:
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> TitleText;
};

// ====================================================================== Sélecteur de niveau

/** Carte de niveau : vignette (ou numéro), nom, description, état. Hérite du comportement de bouton. */
UCLASS()
class FIH2_API ULevelCardWidget : public UMenuButtonWidget
{
	GENERATED_BODY()

public:
	void SetupCard(const FLevelCatalogEntry& Entry, ELevelCardState State, int32 DisplayIndex);

protected:
	virtual UWidget* BuildContent() override;
	virtual FVector2D GetDefaultSize() const override;
	virtual bool ShouldContentFill() const override { return true; }

private:
	UPROPERTY(Transient)
	TObjectPtr<UBorder> ThumbnailFrame;

	UPROPERTY(Transient)
	TObjectPtr<UImage> ThumbnailImage;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> NumberText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> NameText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> DescriptionText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> StateText;
};

UCLASS()
class FIH2_API ULevelSelectScreen : public UMenuScreenWidget
{
	GENERATED_BODY()

public:
	virtual void OnScreenShown() override;

protected:
	virtual void BuildScreen() override;

private:
	void RebuildCards();

	UPROPERTY(Transient)
	TObjectPtr<UHorizontalBox> CardRow;

	UPROPERTY(Transient)
	TObjectPtr<UMenuButtonWidget> BackButton;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> HeadingText;
};

// ====================================================================== Réglages

UCLASS()
class FIH2_API USettingsScreen : public UMenuScreenWidget
{
	GENERATED_BODY()

public:
	virtual void OnScreenShown() override;
	virtual bool HandleBack() override;

protected:
	virtual void BuildScreen() override;

private:
	UMenuOptionRowWidget* AddRow(UVerticalBox* List, FName Name);
	void AddSectionTitle(UVerticalBox* List, const FText& Title);

	/** Relit les réglages courants (sauvegarde + UGameUserSettings) et met les lignes à jour. */
	void RefreshFromSettings();
	void HandleUserSettingChanged();
	void HandleApply();
	void HandleReset();
	void ApplyVideoSettings();

	UPROPERTY(Transient) TObjectPtr<UMenuOptionRowWidget> MasterRow;
	UPROPERTY(Transient) TObjectPtr<UMenuOptionRowWidget> MusicRow;
	UPROPERTY(Transient) TObjectPtr<UMenuOptionRowWidget> SfxRow;
	UPROPERTY(Transient) TObjectPtr<UMenuOptionRowWidget> WindowModeRow;
	UPROPERTY(Transient) TObjectPtr<UMenuOptionRowWidget> ResolutionRow;
	UPROPERTY(Transient) TObjectPtr<UMenuOptionRowWidget> VSyncRow;
	UPROPERTY(Transient) TObjectPtr<UMenuOptionRowWidget> QualityRow;
	UPROPERTY(Transient) TObjectPtr<UMenuOptionRowWidget> FrameRateRow;
	UPROPERTY(Transient) TObjectPtr<UMenuOptionRowWidget> SensitivityRow;

	/** Valeurs proposées par les lignes de choix (même ordre que leurs options). */
	TArray<FIntPoint> Resolutions;
	TArray<int32> QualityLevels;
	TArray<int32> FrameRates;
};

// ====================================================================== Confirmation

UCLASS()
class FIH2_API UConfirmDialogScreen : public UMenuScreenWidget
{
	GENERATED_BODY()

public:
	/**
	 * Prépare la boîte de dialogue avant son affichage.
	 * AutoCancelSeconds > 0 : compte à rebours, puis annulation automatique (confirmation d'un mode vidéo).
	 */
	void Configure(const FText& Title, const FText& Message, const FText& ConfirmLabel, const FText& CancelLabel,
		TFunction<void()> InOnConfirm, TFunction<void()> InOnCancel, float AutoCancelSeconds);

	virtual void OnScreenShown() override;
	virtual void OnScreenHidden() override;
	virtual bool HandleBack() override;

protected:
	virtual void BuildScreen() override;

private:
	void Resolve(bool bConfirmed);
	void HandleCountdownTick();
	void RefreshCountdownText();

	UPROPERTY(Transient) TObjectPtr<UTextBlock> TitleText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> MessageText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> CountdownText;
	UPROPERTY(Transient) TObjectPtr<UMenuButtonWidget> ConfirmButton;
	UPROPERTY(Transient) TObjectPtr<UMenuButtonWidget> CancelButton;

	TFunction<void()> OnConfirm;
	TFunction<void()> OnCancel;
	int32 SecondsRemaining = 0;
	bool bResolved = false;
	FTimerHandle CountdownTimer;
};
