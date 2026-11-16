// Bouton du menu : fond arrondi, contour lumineux au focus, léger agrandissement au survol, écrasement à la pression.
// Le survol de la souris donne le focus clavier au bouton : souris, clavier et manette partagent un seul état
// « sélectionné », donc un seul élément est mis en avant à la fois.
// Arbre construit en C++ ; ULevelCardWidget en dérive et remplace seulement le contenu.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MenuButtonWidget.generated.h"

class UButton;
class USizeBox;
class UTextBlock;

UCLASS()
class FIH2_API UMenuButtonWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual bool Initialize() override;

	/** Libellé et variante (bAccent = action principale, couleur corail). */
	void Setup(const FText& Label, bool bInAccent);
	void SetLabel(const FText& Label);
	void SetButtonSize(const FVector2D& Size);
	void SetFontSize(int32 Size);

	/** Widget à qui donner le focus clavier / manette. */
	UWidget* GetFocusTarget() const;

	/** Émis au clic, à Entrée / Espace ou au bouton A de la manette. */
	FSimpleMulticastDelegate OnClicked;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeOnAddedToFocusPath(const FFocusEvent& InFocusEvent) override;
	virtual void NativeOnRemovedFromFocusPath(const FFocusEvent& InFocusEvent) override;

	/** Contenu placé dans le bouton (par défaut : le libellé). */
	virtual UWidget* BuildContent();
	/** Taille par défaut (thème). */
	virtual FVector2D GetDefaultSize() const;
	/** Vrai : le contenu remplit le bouton (cartes). Faux : il est centré (libellé). */
	virtual bool ShouldContentFill() const { return false; }

	void RefreshStyle();

	UPROPERTY(Transient)
	TObjectPtr<USizeBox> SizeBox;

	UPROPERTY(Transient)
	TObjectPtr<UButton> Button;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> LabelText;

	bool bAccent = false;
	bool bFocused = false;

private:
	UFUNCTION()
	void HandleButtonClicked();

	UFUNCTION()
	void HandleButtonHovered();

	UFUNCTION()
	void HandleButtonPressed();

	UFUNCTION()
	void HandleButtonReleased();

	void TweenScale(const FVector2D& Target, float Duration);
};
