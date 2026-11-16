#include "Menu/MenuOptionRowWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Input/Events.h"
#include "InputCoreTypes.h"
#include "Menu/MenuTheme.h"
#include "MenuWidgetHelpers.h"

#define LOCTEXT_NAMESPACE "FIH2Menu"

namespace
{
	/** Part de la largeur du contrôle occupée par chaque flèche d'une ligne de choix. */
	const float ArrowWidthRatio = 0.14f;
}

bool UMenuOptionRowWidget::Initialize()
{
	const bool bSuperInitialized = Super::Initialize();
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		const UMenuTheme& Theme = UMenuTheme::Get();

		USizeBox* RowSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("RowSize"));
		RowSize->SetHeightOverride(Theme.OptionRowHeight);
		WidgetTree->RootWidget = RowSize;

		Background = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Background"));
		Background->SetPadding(FMargin(18.f, 0.f));
		Background->SetVerticalAlignment(VAlign_Center);
		RowSize->SetContent(Background);

		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("Row"));
		Background->SetContent(Row);

		LabelText = FIH2Menu::MakeText(WidgetTree, FText::GetEmpty(), Theme.BodyFontSize, Theme.TextColor, false, TEXT("LabelText"));
		if (UHorizontalBoxSlot* LabelSlot = Row->AddChildToHorizontalBox(LabelText))
		{
			LabelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			LabelSlot->SetVerticalAlignment(VAlign_Center);
		}

		USizeBox* ControlSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("ControlSize"));
		ControlSize->SetWidthOverride(Theme.OptionControlWidth);
		if (UHorizontalBoxSlot* ControlSlot = Row->AddChildToHorizontalBox(ControlSize))
		{
			ControlSlot->SetVerticalAlignment(VAlign_Center);
		}

		ControlBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ControlBox"));
		ControlSize->SetContent(ControlBox);

		RefreshBackground();
	}
	return bSuperInitialized;
}

void UMenuOptionRowWidget::SetupSlider(const FText& Label, float MinValue, float MaxValue, float Step, float Value, TFunction<FText(float)> Format)
{
	if (!ControlBox || Slider || ValueButton)
	{
		return; // déjà configurée
	}
	const UMenuTheme& Theme = UMenuTheme::Get();
	LabelText->SetText(Label);
	SliderFormat = MoveTemp(Format);

	Slider = WidgetTree->ConstructWidget<USlider>(USlider::StaticClass(), TEXT("Slider"));
	// Gauche / droite ajustent directement la valeur, sans devoir « verrouiller » le curseur avec A / Entrée.
	FIH2Menu::SetBoolProperty(Slider, TEXT("RequiresControllerLock"), false);
	Slider->SetMinValue(MinValue);
	Slider->SetMaxValue(MaxValue);
	Slider->SetStepSize(Step);
	Slider->SetValue(FMath::Clamp(Value, MinValue, MaxValue));
	Slider->SetSliderBarColor(Theme.TextMutedColor);
	Slider->SetSliderHandleColor(Theme.FocusOutlineColor);
	Slider->OnValueChanged.AddUniqueDynamic(this, &UMenuOptionRowWidget::HandleSliderValueChanged);
	if (UHorizontalBoxSlot* SliderSlot = ControlBox->AddChildToHorizontalBox(Slider))
	{
		SliderSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		SliderSlot->SetVerticalAlignment(VAlign_Center);
	}

	ValueText = FIH2Menu::MakeText(WidgetTree, FText::GetEmpty(), Theme.BodyFontSize, Theme.TextColor, true, TEXT("ValueText"));
	ValueText->SetJustification(ETextJustify::Right);
	USizeBox* ValueSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("ValueSize"));
	ValueSize->SetWidthOverride(Theme.OptionControlWidth * 0.25f);
	ValueSize->SetContent(ValueText);
	if (UHorizontalBoxSlot* ValueSlot = ControlBox->AddChildToHorizontalBox(ValueSize))
	{
		ValueSlot->SetVerticalAlignment(VAlign_Center);
	}
	RefreshValueText();
}

void UMenuOptionRowWidget::SetupChoice(const FText& Label, const TArray<FText>& InOptions, int32 Index)
{
	if (!ControlBox || Slider || ValueButton)
	{
		return; // déjà configurée
	}
	const UMenuTheme& Theme = UMenuTheme::Get();
	LabelText->SetText(Label);

	const FButtonStyle FlatStyle = FIH2Menu::MakeButtonStyle(
		FIH2Menu::RoundedBrush(FLinearColor::Transparent, Theme.CornerRadius),
		FIH2Menu::RoundedBrush(Theme.ButtonHoverColor, Theme.CornerRadius),
		FIH2Menu::RoundedBrush(Theme.ButtonPressedColor, Theme.CornerRadius),
		FIH2Menu::RoundedBrush(FLinearColor::Transparent, Theme.CornerRadius));

	auto MakeArrow = [this, &Theme, &FlatStyle](const TCHAR* Name, const FText& Glyph) -> UButton*
	{
		UButton* Arrow = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), Name);
		// Flèches réservées à la souris : au clavier / à la manette, gauche et droite agissent sur la valeur.
		FIH2Menu::SetBoolProperty(Arrow, TEXT("IsFocusable"), false);
		Arrow->SetStyle(FlatStyle);
		Arrow->SetContent(FIH2Menu::MakeText(WidgetTree, Glyph, Theme.BodyFontSize, Theme.TextMutedColor, true));
		USizeBox* ArrowSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		ArrowSize->SetWidthOverride(Theme.OptionControlWidth * ArrowWidthRatio);
		ArrowSize->SetContent(Arrow);
		ControlBox->AddChildToHorizontalBox(ArrowSize);
		return Arrow;
	};

	PreviousButton = MakeArrow(TEXT("PreviousButton"), LOCTEXT("ChoicePrevious", "<"));

	ValueButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("ValueButton"));
	ValueButton->SetStyle(FlatStyle);
	ValueText = FIH2Menu::MakeText(WidgetTree, FText::GetEmpty(), Theme.BodyFontSize, Theme.TextColor, true, TEXT("ValueText"));
	ValueText->SetJustification(ETextJustify::Center);
	ValueButton->SetContent(ValueText);
	if (UHorizontalBoxSlot* ValueSlot = ControlBox->AddChildToHorizontalBox(ValueButton))
	{
		ValueSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}

	NextButton = MakeArrow(TEXT("NextButton"), LOCTEXT("ChoiceNext", ">"));

	PreviousButton->OnClicked.AddUniqueDynamic(this, &UMenuOptionRowWidget::HandlePreviousClicked);
	NextButton->OnClicked.AddUniqueDynamic(this, &UMenuOptionRowWidget::HandleNextClicked);
	ValueButton->OnClicked.AddUniqueDynamic(this, &UMenuOptionRowWidget::HandleNextClicked);
	ValueButton->OnHovered.AddUniqueDynamic(this, &UMenuOptionRowWidget::HandleControlHovered);

	SetChoiceOptions(InOptions, Index);
}

void UMenuOptionRowWidget::NativeConstruct()
{
	Super::NativeConstruct();
	RefreshBackground();
}

UWidget* UMenuOptionRowWidget::GetFocusTarget() const
{
	if (Slider)
	{
		return Slider.Get();
	}
	return ValueButton.Get();
}

void UMenuOptionRowWidget::SetSliderValue(float Value)
{
	if (Slider)
	{
		Slider->SetValue(Value); // SetValue n'émet pas OnValueChanged
		RefreshValueText();
	}
}

float UMenuOptionRowWidget::GetSliderValue() const
{
	return Slider ? Slider->GetValue() : 0.f;
}

void UMenuOptionRowWidget::SetChoiceOptions(const TArray<FText>& InOptions, int32 Index)
{
	Options = InOptions;
	SetChoiceIndex(Index);
}

void UMenuOptionRowWidget::SetChoiceIndex(int32 Index)
{
	ChoiceIndex = Options.Num() > 0 ? FMath::Clamp(Index, 0, Options.Num() - 1) : 0;
	RefreshValueText();
}

void UMenuOptionRowWidget::RefreshValueText()
{
	if (!ValueText)
	{
		return;
	}
	if (Slider)
	{
		ValueText->SetText(SliderFormat ? SliderFormat(Slider->GetValue()) : FText::AsNumber(Slider->GetValue()));
	}
	else
	{
		ValueText->SetText(Options.IsValidIndex(ChoiceIndex) ? Options[ChoiceIndex] : FText::GetEmpty());
	}
}

void UMenuOptionRowWidget::RefreshBackground()
{
	if (!Background)
	{
		return;
	}
	const UMenuTheme& Theme = UMenuTheme::Get();
	Background->SetBrush(bFocused
		? FIH2Menu::RoundedBrush(Theme.ButtonHoverColor, Theme.CornerRadius, Theme.FocusOutlineColor, Theme.FocusOutlineWidth)
		: FIH2Menu::RoundedBrush(Theme.ButtonColor, Theme.CornerRadius, Theme.PanelOutlineColor, Theme.PanelOutlineWidth));
}

void UMenuOptionRowWidget::StepChoice(int32 Direction)
{
	if (Options.Num() < 2)
	{
		return;
	}
	ChoiceIndex = (ChoiceIndex + Direction + Options.Num()) % Options.Num();
	RefreshValueText();
	OnChoiceChanged.Broadcast(ChoiceIndex);
}

void UMenuOptionRowWidget::NativeOnAddedToFocusPath(const FFocusEvent& InFocusEvent)
{
	Super::NativeOnAddedToFocusPath(InFocusEvent);
	bFocused = true;
	RefreshBackground();
}

void UMenuOptionRowWidget::NativeOnRemovedFromFocusPath(const FFocusEvent& InFocusEvent)
{
	Super::NativeOnRemovedFromFocusPath(InFocusEvent);
	bFocused = false;
	RefreshBackground();
}

FReply UMenuOptionRowWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	// Ligne de choix : gauche / droite changent la valeur au lieu de déplacer le focus.
	// (Le curseur gère déjà gauche / droite lui-même.)
	if (ValueButton)
	{
		const FKey Key = InKeyEvent.GetKey();
		if (Key == EKeys::Left || Key == EKeys::Gamepad_DPad_Left || Key == EKeys::Gamepad_LeftStick_Left)
		{
			StepChoice(-1);
			return FReply::Handled();
		}
		if (Key == EKeys::Right || Key == EKeys::Gamepad_DPad_Right || Key == EKeys::Gamepad_LeftStick_Right)
		{
			StepChoice(1);
			return FReply::Handled();
		}
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UMenuOptionRowWidget::HandleSliderValueChanged(float Value)
{
	RefreshValueText();
	OnSliderChanged.Broadcast(Value);
}

void UMenuOptionRowWidget::HandlePreviousClicked()
{
	StepChoice(-1);
}

void UMenuOptionRowWidget::HandleNextClicked()
{
	StepChoice(1);
}

void UMenuOptionRowWidget::HandleControlHovered()
{
	if (ValueButton)
	{
		ValueButton->SetKeyboardFocus();
	}
}

#undef LOCTEXT_NAMESPACE
