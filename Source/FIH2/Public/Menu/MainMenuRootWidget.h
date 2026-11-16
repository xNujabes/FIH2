// Racine du menu principal : pile d'écrans avec transitions (glissement + fondu), verrouillage des entrées
// pendant une transition, fondu d'ouverture, et fondu au noir + indicateur de chargement au lancement d'un niveau.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Menu/LevelCatalog.h"
#include "MainMenuRootWidget.generated.h"

class UBorder;
class UMenuScreenWidget;
class UOverlay;

UCLASS()
class FIH2_API UMainMenuRootWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual bool Initialize() override;

	/** Affiche l'écran de cette classe au-dessus de la pile (instance réutilisée d'une fois sur l'autre). */
	UMenuScreenWidget* PushScreen(TSubclassOf<UMenuScreenWidget> ScreenClass);

	/** Revient à l'écran précédent. Sans effet sur le dernier écran. */
	void PopScreen();

	/** Instance (créée au besoin) de l'écran de cette classe, sans l'afficher. */
	UMenuScreenWidget* GetOrCreateScreen(TSubclassOf<UMenuScreenWidget> ScreenClass);

	/** Bouton « Jouer » : tutoriel direct tant qu'il n'est pas complété, sinon sélecteur de niveau. */
	void RequestPlay();

	/** Bouton « Quitter » : demande confirmation. */
	void RequestQuit();

	/** Boîte de confirmation. Les rappels sont appelés une fois la boîte refermée. */
	void ShowConfirm(const FText& Title, const FText& Message, const FText& ConfirmLabel, const FText& CancelLabel,
		TFunction<void()> OnConfirm, TFunction<void()> OnCancel = nullptr, float AutoCancelSeconds = 0.f);

	/** Fondu au noir, indicateur de chargement, puis ouverture du niveau. Les entrées restent verrouillées. */
	void LaunchLevel(const FLevelCatalogEntry& Entry);

	/** Vrai pendant une transition ou un lancement : toutes les entrées sont ignorées. */
	bool IsInputLocked() const { return bInputLocked; }

protected:
	virtual void NativeConstruct() override;
	virtual FReply NativeOnPreviewKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

private:
	void BuildTree();
	void SetInputLocked(bool bLocked);
	void TransitionScreens(UMenuScreenWidget* From, UMenuScreenWidget* To, bool bForward);
	void FocusCurrentScreen();
	UMenuScreenWidget* GetCurrentScreen() const;

	void HandleLaunchFadeFinished();
	void OpenPendingLevel();

	UPROPERTY(Transient)
	TObjectPtr<UOverlay> ScreenHost;

	UPROPERTY(Transient)
	TObjectPtr<UBorder> FadeOverlay;

	UPROPERTY(Transient)
	TObjectPtr<UWidget> LoadingBox;

	/** Pile d'écrans : le dernier est l'écran affiché. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UMenuScreenWidget>> ScreenStack;

	/** Une instance par classe d'écran, créée à la première utilisation. */
	UPROPERTY(Transient)
	TMap<TObjectPtr<UClass>, TObjectPtr<UMenuScreenWidget>> ScreenInstances;

	TSoftObjectPtr<UWorld> PendingMap;
	bool bInputLocked = false;
	bool bLaunching = false;
};
