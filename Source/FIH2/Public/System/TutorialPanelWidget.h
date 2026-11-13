// Panneau de guidage du tutoriel (haut-centre de l'écran).
// Vue passive : UTutorialSubsystem lui pousse les étapes, le compte à rebours et le message de remise à zéro.
// Utilisable tel quel (l'arbre de widgets est construit en C++ si aucun WBP ne le fournit), ou comme parent
// d'un WBP : les widgets nommés MessageText, StepText, TimerText, NextButton, SkipButton sont alors repris.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TutorialPanelWidget.generated.h"

class UButton;
class UTextBlock;

UCLASS()
class FIH2_API UTutorialPanelWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual bool Initialize() override;

	/** Affiche une étape. StepIndex commence à 0. */
	void ShowStep(const FText& Message, int32 StepIndex, int32 StepCount, bool bShowNextButton, bool bShowTimer);

	/** Met à jour le compte à rebours du maintien. bWaiting : jauge au rouge, le compte à rebours attend. */
	void ShowTimer(float RemainingSeconds, bool bWaiting);

	/** Affiche le message de remise à zéro pendant Duration secondes, puis revient au message de l'étape. */
	void ShowResetMessage(const FText& Message, float Duration);

	/** Pour animer le panneau dans un WBP enfant. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Tutorial")
	void OnStepShown(int32 StepIndex);

	UFUNCTION(BlueprintImplementableEvent, Category = "Tutorial")
	void OnMaintainReset();

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(BlueprintReadOnly, Category = "Tutorial", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> MessageText;

	UPROPERTY(BlueprintReadOnly, Category = "Tutorial", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> StepText;

	UPROPERTY(BlueprintReadOnly, Category = "Tutorial", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TimerText;

	UPROPERTY(BlueprintReadOnly, Category = "Tutorial", meta = (BindWidgetOptional))
	TObjectPtr<UButton> NextButton;

	UPROPERTY(BlueprintReadOnly, Category = "Tutorial", meta = (BindWidgetOptional))
	TObjectPtr<UButton> SkipButton;

	/** Couleurs (modifiables dans un WBP enfant). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial|Style")
	FLinearColor TextColor = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial|Style")
	FLinearColor AlertColor = FLinearColor(1.f, 0.25f, 0.2f, 1.f);

private:
	void BuildDefaultTree();
	void RestoreStepMessage();

	UFUNCTION()
	void HandleNextClicked();

	UFUNCTION()
	void HandleSkipClicked();

	FText CurrentStepMessage;
	FTimerHandle ResetMessageTimer;
};
