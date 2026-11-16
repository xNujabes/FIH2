// Ligne de réglage : libellé à gauche, contrôle à droite.
//  - Curseur (volume, sensibilité) : USlider, piloté à la souris, au clavier et à la manette (gauche / droite).
//  - Choix (mode d'affichage, résolution, oui / non...) : valeur encadrée de « < » et « > » ; gauche / droite au
//    clavier ou à la manette, clic sur la valeur = choix suivant.
// La ligne s'éclaire quand un de ses contrôles a le focus.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MenuOptionRowWidget.generated.h"

class UBorder;
class UButton;
class UHorizontalBox;
class USlider;
class UTextBlock;

UCLASS()
class FIH2_API UMenuOptionRowWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual bool Initialize() override;

	/** Curseur. Format transforme la valeur en texte affiché (ex. « 80 % »). */
	void SetupSlider(const FText& Label, float MinValue, float MaxValue, float Step, float Value, TFunction<FText(float)> Format);

	/** Liste de choix. */
	void SetupChoice(const FText& Label, const TArray<FText>& InOptions, int32 Index);

	/** Change la valeur sans émettre d'évènement (rafraîchissement depuis les réglages). */
	void SetSliderValue(float Value);
	void SetChoiceIndex(int32 Index);
	void SetChoiceOptions(const TArray<FText>& InOptions, int32 Index);

	float GetSliderValue() const;
	int32 GetChoiceIndex() const { return ChoiceIndex; }

	UWidget* GetFocusTarget() const;

	TMulticastDelegate<void(float)> OnSliderChanged;
	TMulticastDelegate<void(int32)> OnChoiceChanged;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeOnAddedToFocusPath(const FFocusEvent& InFocusEvent) override;
	virtual void NativeOnRemovedFromFocusPath(const FFocusEvent& InFocusEvent) override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

private:
	void RefreshBackground();
	void RefreshValueText();
	void StepChoice(int32 Direction);

	UFUNCTION()
	void HandleSliderValueChanged(float Value);

	UFUNCTION()
	void HandlePreviousClicked();

	UFUNCTION()
	void HandleNextClicked();

	UFUNCTION()
	void HandleControlHovered();

	UPROPERTY(Transient)
	TObjectPtr<UBorder> Background;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> LabelText;

	UPROPERTY(Transient)
	TObjectPtr<UHorizontalBox> ControlBox;

	UPROPERTY(Transient)
	TObjectPtr<USlider> Slider;

	UPROPERTY(Transient)
	TObjectPtr<UButton> ValueButton;

	UPROPERTY(Transient)
	TObjectPtr<UButton> PreviousButton;

	UPROPERTY(Transient)
	TObjectPtr<UButton> NextButton;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ValueText;

	TArray<FText> Options;
	int32 ChoiceIndex = 0;
	TFunction<FText(float)> SliderFormat;
	bool bFocused = false;
};
