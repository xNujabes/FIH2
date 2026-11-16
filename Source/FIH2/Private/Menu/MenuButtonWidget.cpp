#include "Menu/MenuButtonWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Menu/MenuTheme.h"
#include "Menu/MenuTweenSubsystem.h"
#include "MenuWidgetHelpers.h"

namespace
{
	const FName HoverChannel(TEXT("Hover"));
}

bool UMenuButtonWidget::Initialize()
{
	const bool bSuperInitialized = Super::Initialize();
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		const FVector2D Size = GetDefaultSize();
		SizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("SizeBox"));
		SizeBox->SetWidthOverride(Size.X);
		SizeBox->SetHeightOverride(Size.Y);
		WidgetTree->RootWidget = SizeBox;

		Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("Button"));
		SizeBox->SetContent(Button);

		if (UWidget* Content = BuildContent())
		{
			Button->SetContent(Content);
			if (UButtonSlot* ContentSlot = Cast<UButtonSlot>(Button->GetContentSlot()))
			{
				const bool bFill = ShouldContentFill();
				ContentSlot->SetHorizontalAlignment(bFill ? HAlign_Fill : HAlign_Center);
				ContentSlot->SetVerticalAlignment(bFill ? VAlign_Fill : VAlign_Center);
				ContentSlot->SetPadding(FMargin(0.f));
			}
		}
		RefreshStyle();
	}
	return bSuperInitialized;
}

UWidget* UMenuButtonWidget::BuildContent()
{
	const UMenuTheme& Theme = UMenuTheme::Get();
	LabelText = FIH2Menu::MakeText(WidgetTree, FText::GetEmpty(), Theme.ButtonFontSize, Theme.TextColor, true, TEXT("LabelText"));
	LabelText->SetJustification(ETextJustify::Center);
	return LabelText;
}

FVector2D UMenuButtonWidget::GetDefaultSize() const
{
	return UMenuTheme::Get().ButtonSize;
}

void UMenuButtonWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (Button)
	{
		Button->OnClicked.AddUniqueDynamic(this, &UMenuButtonWidget::HandleButtonClicked);
		Button->OnHovered.AddUniqueDynamic(this, &UMenuButtonWidget::HandleButtonHovered);
		Button->OnPressed.AddUniqueDynamic(this, &UMenuButtonWidget::HandleButtonPressed);
		Button->OnReleased.AddUniqueDynamic(this, &UMenuButtonWidget::HandleButtonReleased);
	}
}

void UMenuButtonWidget::Setup(const FText& Label, bool bInAccent)
{
	bAccent = bInAccent;
	SetLabel(Label);
	RefreshStyle();
}

void UMenuButtonWidget::SetLabel(const FText& Label)
{
	if (LabelText)
	{
		LabelText->SetText(Label);
	}
}

void UMenuButtonWidget::SetButtonSize(const FVector2D& Size)
{
	if (SizeBox)
	{
		SizeBox->SetWidthOverride(Size.X);
		SizeBox->SetHeightOverride(Size.Y);
	}
}

void UMenuButtonWidget::SetFontSize(int32 Size)
{
	FIH2Menu::SetFont(LabelText.Get(), Size, true);
}

UWidget* UMenuButtonWidget::GetFocusTarget() const
{
	return Button.Get();
}

void UMenuButtonWidget::RefreshStyle()
{
	if (!Button)
	{
		return;
	}
	const UMenuTheme& Theme = UMenuTheme::Get();
	const FLinearColor Normal = bAccent ? Theme.AccentColor : Theme.ButtonColor;
	const FLinearColor Hovered = bAccent ? Theme.AccentHoverColor : Theme.ButtonHoverColor;
	const FLinearColor Pressed = bAccent ? Theme.AccentPressedColor : Theme.ButtonPressedColor;
	// Focus : contour turquoise vif (la « lueur ») et fond de survol. Au repos : contour discret du panneau.
	const FLinearColor Outline = bFocused ? Theme.FocusOutlineColor : Theme.PanelOutlineColor;
	const float OutlineWidth = bFocused ? Theme.FocusOutlineWidth : Theme.PanelOutlineWidth;

	Button->SetStyle(FIH2Menu::MakeButtonStyle(
		FIH2Menu::RoundedBrush(bFocused ? Hovered : Normal, Theme.CornerRadius, Outline, OutlineWidth),
		FIH2Menu::RoundedBrush(Hovered, Theme.CornerRadius, Outline, OutlineWidth),
		FIH2Menu::RoundedBrush(Pressed, Theme.CornerRadius, Outline, OutlineWidth),
		FIH2Menu::RoundedBrush(Theme.ButtonDisabledColor, Theme.CornerRadius, Theme.PanelOutlineColor, Theme.PanelOutlineWidth)));

	if (LabelText)
	{
		LabelText->SetColorAndOpacity(FSlateColor(bAccent ? Theme.AccentTextColor : Theme.TextColor));
	}
}

void UMenuButtonWidget::TweenScale(const FVector2D& Target, float Duration)
{
	if (UMenuTweenSubsystem* Tweens = UMenuTweenSubsystem::Get(this))
	{
		FMenuTweenSpec Spec;
		Spec.Duration = Duration;
		Spec.Ease = EMenuEase::OutCubic;
		Spec.ToScale = Target;
		// On anime le bouton interne : le widget lui-même reste libre pour les animations d'entrée de l'écran.
		Tweens->Play(Button.Get(), Spec, HoverChannel);
	}
}

void UMenuButtonWidget::NativeOnAddedToFocusPath(const FFocusEvent& InFocusEvent)
{
	Super::NativeOnAddedToFocusPath(InFocusEvent);
	bFocused = true;
	RefreshStyle();
	const UMenuTheme& Theme = UMenuTheme::Get();
	TweenScale(FVector2D(Theme.HoverScale), Theme.HoverDuration);
}

void UMenuButtonWidget::NativeOnRemovedFromFocusPath(const FFocusEvent& InFocusEvent)
{
	Super::NativeOnRemovedFromFocusPath(InFocusEvent);
	bFocused = false;
	RefreshStyle();
	TweenScale(FVector2D::UnitVector, UMenuTheme::Get().HoverDuration);
}

void UMenuButtonWidget::HandleButtonHovered()
{
	// Le survol sélectionne : la souris et le clavier / la manette ne peuvent pas désigner deux boutons différents.
	if (Button && Button->GetIsEnabled())
	{
		Button->SetKeyboardFocus();
	}
}

void UMenuButtonWidget::HandleButtonPressed()
{
	const UMenuTheme& Theme = UMenuTheme::Get();
	TweenScale(Theme.PressScale, Theme.PressDuration);
}

void UMenuButtonWidget::HandleButtonReleased()
{
	const UMenuTheme& Theme = UMenuTheme::Get();
	TweenScale(bFocused ? FVector2D(Theme.HoverScale) : FVector2D::UnitVector, Theme.HoverDuration);
}

void UMenuButtonWidget::HandleButtonClicked()
{
	OnClicked.Broadcast();
}
