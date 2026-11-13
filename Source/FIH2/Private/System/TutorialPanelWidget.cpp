#include "System/TutorialPanelWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/World.h"
#include "System/TutorialSubsystem.h"
#include "TimerManager.h"

#define LOCTEXT_NAMESPACE "FIH2Tutorial"

namespace
{
	/** Taille de police des textes construits en C++. */
	void SetFontSize(UTextBlock* Text, int32 Size)
	{
		if (!Text)
		{
			return;
		}
		FSlateFontInfo Font = Text->GetFont();
		Font.Size = Size;
		Text->SetFont(Font);
	}
}

bool UTutorialPanelWidget::Initialize()
{
	const bool bSuperInitialized = Super::Initialize();
	// Classe native sans WBP : on construit le panneau par défaut. Un WBP enfant fournit son propre arbre.
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildDefaultTree();
	}
	return bSuperInitialized;
}

void UTutorialPanelWidget::BuildDefaultTree()
{
	UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
	Root->SetVisibility(ESlateVisibility::SelfHitTestInvisible); // ne bloque pas les clics sur la cuve
	WidgetTree->RootWidget = Root;

	// Panneau en haut au centre : le bloc chrono / objectif du HUD est masqué pendant le tutoriel (pas d'objectif chronométré).
	// Le bas-gauche est au HUD, le bas-droite au panneau d'infos.
	UBorder* Panel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Panel"));
	Panel->SetBrushColor(FLinearColor(0.02f, 0.05f, 0.08f, 0.78f));
	Panel->SetPadding(FMargin(24.f, 14.f));
	if (UCanvasPanelSlot* PanelSlot = Root->AddChildToCanvas(Panel))
	{
		PanelSlot->SetAnchors(FAnchors(0.5f, 0.f));
		PanelSlot->SetAlignment(FVector2D(0.5f, 0.f));
		PanelSlot->SetPosition(FVector2D(0.f, 32.f));
		PanelSlot->SetAutoSize(true);
	}

	USizeBox* Size = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("PanelSize"));
	Size->SetMaxDesiredWidth(760.f);
	Panel->SetContent(Size);

	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Column"));
	Size->SetContent(Column);

	StepText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("StepText"));
	SetFontSize(StepText, 12);
	StepText->SetColorAndOpacity(FSlateColor(FLinearColor(0.7f, 0.8f, 0.85f, 1.f)));
	StepText->SetJustification(ETextJustify::Center);
	if (UVerticalBoxSlot* StepSlot = Column->AddChildToVerticalBox(StepText))
	{
		StepSlot->SetHorizontalAlignment(HAlign_Center);
	}

	MessageText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("MessageText"));
	SetFontSize(MessageText, 20);
	MessageText->SetAutoWrapText(true);
	MessageText->SetJustification(ETextJustify::Center);
	if (UVerticalBoxSlot* MessageSlot = Column->AddChildToVerticalBox(MessageText))
	{
		MessageSlot->SetPadding(FMargin(0.f, 4.f));
		MessageSlot->SetHorizontalAlignment(HAlign_Center);
	}

	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ButtonRow"));
	if (UVerticalBoxSlot* RowSlot = Column->AddChildToVerticalBox(Row))
	{
		RowSlot->SetPadding(FMargin(0.f, 8.f, 0.f, 0.f));
		RowSlot->SetHorizontalAlignment(HAlign_Center);
	}

	TimerText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TimerText"));
	SetFontSize(TimerText, 24);
	if (UHorizontalBoxSlot* TimerSlot = Row->AddChildToHorizontalBox(TimerText))
	{
		TimerSlot->SetVerticalAlignment(VAlign_Center);
		TimerSlot->SetPadding(FMargin(0.f, 0.f, 16.f, 0.f));
	}

	auto MakeButton = [this, Row](const TCHAR* Name, const FText& Label) -> UButton*
	{
		UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), Name);
		UTextBlock* ButtonLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		ButtonLabel->SetText(Label);
		ButtonLabel->SetColorAndOpacity(FSlateColor(FLinearColor(0.05f, 0.05f, 0.05f, 1.f)));
		SetFontSize(ButtonLabel, 14);
		Button->AddChild(ButtonLabel);
		if (UHorizontalBoxSlot* ButtonSlot = Row->AddChildToHorizontalBox(Button))
		{
			ButtonSlot->SetPadding(FMargin(6.f, 0.f));
			ButtonSlot->SetVerticalAlignment(VAlign_Center);
		}
		return Button;
	};
	NextButton = MakeButton(TEXT("NextButton"), LOCTEXT("Next", "Suivant"));
	SkipButton = MakeButton(TEXT("SkipButton"), LOCTEXT("Skip", "Passer le tutoriel"));
}

void UTutorialPanelWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);

	if (NextButton)
	{
		NextButton->OnClicked.AddUniqueDynamic(this, &UTutorialPanelWidget::HandleNextClicked);
	}
	if (SkipButton)
	{
		SkipButton->OnClicked.AddUniqueDynamic(this, &UTutorialPanelWidget::HandleSkipClicked);
	}
	if (TimerText)
	{
		TimerText->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UTutorialPanelWidget::ShowStep(const FText& Message, int32 StepIndex, int32 StepCount, bool bShowNextButton, bool bShowTimer)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ResetMessageTimer);
	}

	CurrentStepMessage = Message;
	RestoreStepMessage();

	if (StepText)
	{
		StepText->SetText(FText::Format(LOCTEXT("StepCounter", "Tutoriel · étape {0} / {1}"), StepIndex + 1, StepCount));
	}
	if (NextButton)
	{
		NextButton->SetVisibility(bShowNextButton ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
	if (TimerText)
	{
		TimerText->SetVisibility(bShowTimer ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
	if (SkipButton)
	{
		// Étape de maintien (la seule avec un compte à rebours) : plus rien à passer.
		SkipButton->SetVisibility(bShowTimer ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
	}
	OnStepShown(StepIndex);
}

void UTutorialPanelWidget::ShowTimer(float RemainingSeconds, bool bWaiting)
{
	if (!TimerText)
	{
		return;
	}
	const int32 Seconds = FMath::Max(0, FMath::CeilToInt(RemainingSeconds));
	TimerText->SetText(FText::FromString(FString::Printf(TEXT("%d:%02d"), Seconds / 60, Seconds % 60)));
	TimerText->SetColorAndOpacity(FSlateColor(bWaiting ? AlertColor : TextColor));
}

void UTutorialPanelWidget::ShowResetMessage(const FText& Message, float Duration)
{
	if (MessageText)
	{
		MessageText->SetText(Message);
		MessageText->SetColorAndOpacity(FSlateColor(AlertColor));
	}
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(ResetMessageTimer,
			FTimerDelegate::CreateUObject(this, &UTutorialPanelWidget::RestoreStepMessage), FMath::Max(0.1f, Duration), false);
	}
	OnMaintainReset();
}

void UTutorialPanelWidget::RestoreStepMessage()
{
	if (MessageText)
	{
		MessageText->SetText(CurrentStepMessage);
		MessageText->SetColorAndOpacity(FSlateColor(TextColor));
	}
}

void UTutorialPanelWidget::HandleNextClicked()
{
	if (UTutorialSubsystem* Tutorial = UTutorialSubsystem::Get(this))
	{
		Tutorial->Next();
	}
}

void UTutorialPanelWidget::HandleSkipClicked()
{
	if (UTutorialSubsystem* Tutorial = UTutorialSubsystem::Get(this))
	{
		Tutorial->Skip();
	}
}

#undef LOCTEXT_NAMESPACE
