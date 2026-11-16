#include "Menu/MenuScreens.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "GameFramework/GameUserSettings.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Menu/MainMenuRootWidget.h"
#include "Menu/MenuOptionRowWidget.h"
#include "Menu/MenuTheme.h"
#include "Menu/MenuTweenSubsystem.h"
#include "MenuWidgetHelpers.h"
#include "TimerManager.h"

#define LOCTEXT_NAMESPACE "FIH2Menu"

namespace
{
	const FName EnterChannel(TEXT("Enter"));
	const FName FloatChannel(TEXT("Float"));

	/** Ombre portée des textes posés directement sur la cuve (lisibilité). */
	void AddTextShadow(UTextBlock* Text)
	{
		if (Text)
		{
			Text->SetShadowOffset(FVector2D(0.f, 2.f));
			Text->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.6f));
		}
	}

	FText FormatPercent(float Value)
	{
		return FText::Format(LOCTEXT("PercentFormat", "{0} %"), FText::AsNumber(FMath::RoundToInt(Value * 100.f)));
	}

	FText FormatMultiplier(float Value)
	{
		FNumberFormattingOptions Options;
		Options.MinimumFractionalDigits = 1;
		Options.MaximumFractionalDigits = 1;
		return FText::Format(LOCTEXT("MultiplierFormat", "× {0}"), FText::AsNumber(Value, &Options));
	}
}

// ====================================================================== Base

bool UMenuScreenWidget::Initialize()
{
	const bool bSuperInitialized = Super::Initialize();
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildScreen();
	}
	return bSuperInitialized;
}

void UMenuScreenWidget::OnScreenShown()
{
	PlayStagger(FVector2D(-UMenuTheme::Get().EntryOffset, 0.f));
}

bool UMenuScreenWidget::HandleBack()
{
	if (UMainMenuRootWidget* RootWidget = GetRoot())
	{
		RootWidget->PopScreen();
		return true;
	}
	return false;
}

UBorder* UMenuScreenWidget::MakePanel(FName Name)
{
	const UMenuTheme& Theme = UMenuTheme::Get();
	UBorder* Panel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), Name);
	Panel->SetBrush(FIH2Menu::RoundedBrush(Theme.PanelColor, Theme.CornerRadius, Theme.PanelOutlineColor, Theme.PanelOutlineWidth));
	Panel->SetPadding(FMargin(Theme.PanelPadding));
	return Panel;
}

UMenuButtonWidget* UMenuScreenWidget::MakeButton(const FText& Label, bool bAccent, FName Name)
{
	UMenuButtonWidget* Button = WidgetTree->ConstructWidget<UMenuButtonWidget>(UMenuButtonWidget::StaticClass(), Name);
	Button->Setup(Label, bAccent);
	return Button;
}

void UMenuScreenWidget::PlayStagger(const FVector2D& FromOffset)
{
	UMenuTweenSubsystem* Tweens = UMenuTweenSubsystem::Get(this);
	if (!Tweens)
	{
		return;
	}
	const UMenuTheme& Theme = UMenuTheme::Get();
	int32 Order = 0;
	for (const TWeakObjectPtr<UWidget>& WeakWidget : StaggerWidgets)
	{
		UWidget* Widget = WeakWidget.Get();
		if (!Widget)
		{
			continue;
		}
		FMenuTweenSpec Spec;
		Spec.Duration = Theme.EntryDuration;
		Spec.Delay = Theme.StaggerDelay * Order++;
		Spec.Ease = EMenuEase::OutBack;
		Spec.FromOpacity = 0.f;
		Spec.ToOpacity = 1.f;
		Spec.FromTranslation = FromOffset;
		Spec.ToTranslation = FVector2D::ZeroVector;
		Tweens->Play(Widget, Spec, EnterChannel);
	}
}

// ====================================================================== Écran principal

void UMainMenuScreen::BuildScreen()
{
	const UMenuTheme& Theme = UMenuTheme::Get();

	UOverlay* RootOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Root"));
	WidgetTree->RootWidget = RootOverlay;

	// Colonne à gauche, centrée verticalement : la cuve reste visible sur les deux tiers droits (16:9 comme 21:9).
	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Column"));
	if (UOverlaySlot* ColumnSlot = RootOverlay->AddChildToOverlay(Column))
	{
		ColumnSlot->SetHorizontalAlignment(HAlign_Left);
		ColumnSlot->SetVerticalAlignment(VAlign_Center);
	}

	TitleText = FIH2Menu::MakeText(WidgetTree, Theme.GameTitle, Theme.TitleFontSize, Theme.TextColor, true, TEXT("TitleText"));
	AddTextShadow(TitleText);
	Column->AddChildToVerticalBox(TitleText);

	UTextBlock* Subtitle = FIH2Menu::MakeText(WidgetTree, Theme.GameSubtitle, Theme.SubtitleFontSize, Theme.TextMutedColor, false, TEXT("SubtitleText"));
	AddTextShadow(Subtitle);
	if (UVerticalBoxSlot* SubtitleSlot = Column->AddChildToVerticalBox(Subtitle))
	{
		SubtitleSlot->SetPadding(FMargin(0.f, 0.f, 0.f, Theme.PanelPadding));
	}
	StaggerWidgets.Add(Subtitle);

	auto AddButton = [this, Column, &Theme](const FText& Label, bool bAccent, FName Name) -> UMenuButtonWidget*
	{
		UMenuButtonWidget* Button = MakeButton(Label, bAccent, Name);
		if (UVerticalBoxSlot* ButtonSlot = Column->AddChildToVerticalBox(Button))
		{
			ButtonSlot->SetPadding(FMargin(0.f, 0.f, 0.f, Theme.ButtonSpacing));
			ButtonSlot->SetHorizontalAlignment(HAlign_Left);
		}
		StaggerWidgets.Add(Button);
		return Button;
	};

	UMenuButtonWidget* PlayButton = AddButton(LOCTEXT("Play", "Jouer"), true, TEXT("PlayButton"));
	UMenuButtonWidget* SettingsButton = AddButton(LOCTEXT("Settings", "Réglages"), false, TEXT("SettingsButton"));
	UMenuButtonWidget* QuitButton = AddButton(LOCTEXT("Quit", "Quitter"), false, TEXT("QuitButton"));

	PlayButton->OnClicked.AddWeakLambda(this, [this]()
	{
		if (UMainMenuRootWidget* RootWidget = GetRoot()) { RootWidget->RequestPlay(); }
	});
	SettingsButton->OnClicked.AddWeakLambda(this, [this]()
	{
		if (UMainMenuRootWidget* RootWidget = GetRoot()) { RootWidget->PushScreen(USettingsScreen::StaticClass()); }
	});
	QuitButton->OnClicked.AddWeakLambda(this, [this]()
	{
		if (UMainMenuRootWidget* RootWidget = GetRoot()) { RootWidget->RequestQuit(); }
	});

	DefaultFocus = PlayButton->GetFocusTarget();
}

void UMainMenuScreen::OnScreenShown()
{
	Super::OnScreenShown();

	// Titre : simple fondu à l'entrée (sa translation est réservée au flottement), puis flottement continu.
	UMenuTweenSubsystem* Tweens = UMenuTweenSubsystem::Get(this);
	if (!Tweens || !TitleText)
	{
		return;
	}
	const UMenuTheme& Theme = UMenuTheme::Get();

	FMenuTweenSpec Fade;
	Fade.Duration = Theme.EntryDuration;
	Fade.FromOpacity = 0.f;
	Fade.ToOpacity = 1.f;
	Tweens->Play(TitleText.Get(), Fade, EnterChannel);

	FMenuTweenSpec Float;
	Float.Duration = Theme.TitleFloatHalfPeriod;
	Float.Ease = EMenuEase::InOutSine;
	Float.bPingPongLoop = true;
	Float.FromTranslation = FVector2D(0.f, -Theme.TitleFloatAmplitude);
	Float.ToTranslation = FVector2D(0.f, Theme.TitleFloatAmplitude);
	Tweens->Play(TitleText.Get(), Float, FloatChannel);
}

void UMainMenuScreen::OnScreenHidden()
{
	// Le flottement est la seule animation sans fin : on l'arrête dès que l'écran n'est plus affiché.
	if (UMenuTweenSubsystem* Tweens = UMenuTweenSubsystem::Get(this))
	{
		Tweens->Stop(TitleText.Get(), FloatChannel);
	}
}

bool UMainMenuScreen::HandleBack()
{
	// Échap sur l'écran principal : proposer de quitter.
	if (UMainMenuRootWidget* RootWidget = GetRoot())
	{
		RootWidget->RequestQuit();
		return true;
	}
	return false;
}

// ====================================================================== Carte de niveau

FVector2D ULevelCardWidget::GetDefaultSize() const
{
	return UMenuTheme::Get().CardSize;
}

UWidget* ULevelCardWidget::BuildContent()
{
	const UMenuTheme& Theme = UMenuTheme::Get();

	UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("CardBox"));

	USizeBox* ThumbnailSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("ThumbnailSize"));
	ThumbnailSize->SetHeightOverride(Theme.CardThumbnailHeight);
	if (UVerticalBoxSlot* ThumbnailSlot = Box->AddChildToVerticalBox(ThumbnailSize))
	{
		ThumbnailSlot->SetPadding(FMargin(Theme.ButtonSpacing));
	}

	ThumbnailFrame = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("ThumbnailFrame"));
	ThumbnailFrame->SetBrush(FIH2Menu::RoundedBrush(Theme.ButtonPressedColor, Theme.CornerRadius));
	ThumbnailFrame->SetHorizontalAlignment(HAlign_Fill);
	ThumbnailFrame->SetVerticalAlignment(VAlign_Fill);
	ThumbnailSize->SetContent(ThumbnailFrame);

	UOverlay* ThumbnailOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("ThumbnailOverlay"));
	ThumbnailFrame->SetContent(ThumbnailOverlay);

	ThumbnailImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("ThumbnailImage"));
	ThumbnailImage->SetVisibility(ESlateVisibility::Collapsed);
	if (UOverlaySlot* ImageSlot = ThumbnailOverlay->AddChildToOverlay(ThumbnailImage))
	{
		ImageSlot->SetHorizontalAlignment(HAlign_Fill);
		ImageSlot->SetVerticalAlignment(VAlign_Fill);
	}

	NumberText = FIH2Menu::MakeText(WidgetTree, FText::GetEmpty(), Theme.TitleFontSize, Theme.TextMutedColor, true, TEXT("NumberText"));
	if (UOverlaySlot* NumberSlot = ThumbnailOverlay->AddChildToOverlay(NumberText))
	{
		NumberSlot->SetHorizontalAlignment(HAlign_Center);
		NumberSlot->SetVerticalAlignment(VAlign_Center);
	}

	const FMargin TextPadding(Theme.ButtonSpacing, 0.f, Theme.ButtonSpacing, Theme.ButtonSpacing * 0.5f);

	NameText = FIH2Menu::MakeText(WidgetTree, FText::GetEmpty(), Theme.SubtitleFontSize, Theme.TextColor, true, TEXT("NameText"));
	NameText->SetAutoWrapText(true);
	if (UVerticalBoxSlot* NameSlot = Box->AddChildToVerticalBox(NameText))
	{
		NameSlot->SetPadding(TextPadding);
	}

	DescriptionText = FIH2Menu::MakeText(WidgetTree, FText::GetEmpty(), Theme.SmallFontSize, Theme.TextMutedColor, false, TEXT("DescriptionText"));
	DescriptionText->SetAutoWrapText(true);
	if (UVerticalBoxSlot* DescriptionSlot = Box->AddChildToVerticalBox(DescriptionText))
	{
		DescriptionSlot->SetPadding(TextPadding);
		DescriptionSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}

	StateText = FIH2Menu::MakeText(WidgetTree, FText::GetEmpty(), Theme.SmallFontSize, Theme.TextColor, true, TEXT("StateText"));
	if (UVerticalBoxSlot* StateSlot = Box->AddChildToVerticalBox(StateText))
	{
		StateSlot->SetPadding(FMargin(Theme.ButtonSpacing));
	}
	return Box;
}

void ULevelCardWidget::SetupCard(const FLevelCatalogEntry& Entry, ELevelCardState State, int32 DisplayIndex)
{
	const UMenuTheme& Theme = UMenuTheme::Get();
	const bool bLocked = State == ELevelCardState::Locked;

	if (NameText)
	{
		NameText->SetText(Entry.DisplayName);
		NameText->SetColorAndOpacity(FSlateColor(bLocked ? Theme.LockedColor : Theme.TextColor));
	}
	if (DescriptionText)
	{
		DescriptionText->SetText(Entry.Description);
	}
	if (NumberText)
	{
		NumberText->SetText(FText::AsNumber(DisplayIndex));
	}

	// Vignette si le catalogue en fournit une, sinon le numéro d'ordre reste affiché.
	if (ThumbnailImage && NumberText && !Entry.Thumbnail.IsNull())
	{
		if (UTexture2D* Texture = Entry.Thumbnail.LoadSynchronous())
		{
			ThumbnailImage->SetBrushFromTexture(Texture);
			ThumbnailImage->SetVisibility(ESlateVisibility::HitTestInvisible);
			ThumbnailImage->SetColorAndOpacity(bLocked ? Theme.LockedColor : FLinearColor::White);
			NumberText->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	if (StateText)
	{
		switch (State)
		{
		case ELevelCardState::Completed:
			// « ● » : même pastille que le HUD (présente dans la police par défaut, contrairement à ✔).
			StateText->SetText(LOCTEXT("CardCompleted", "● Terminé — rejouer"));
			StateText->SetColorAndOpacity(FSlateColor(Theme.CompletedColor));
			break;
		case ELevelCardState::Locked:
			StateText->SetText(Entry.bComingSoon ? LOCTEXT("CardComingSoon", "Bientôt") : LOCTEXT("CardLocked", "Verrouillé"));
			StateText->SetColorAndOpacity(FSlateColor(Theme.LockedColor));
			break;
		default:
			StateText->SetText(LOCTEXT("CardAvailable", "Disponible"));
			StateText->SetColorAndOpacity(FSlateColor(Theme.FocusOutlineColor));
			break;
		}
	}

	// Une carte verrouillée ne se clique pas et ne prend pas le focus.
	if (Button)
	{
		Button->SetIsEnabled(!bLocked);
	}
}

// ====================================================================== Sélecteur de niveau

void ULevelSelectScreen::BuildScreen()
{
	const UMenuTheme& Theme = UMenuTheme::Get();

	UOverlay* RootOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Root"));
	WidgetTree->RootWidget = RootOverlay;

	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Column"));
	if (UOverlaySlot* ColumnSlot = RootOverlay->AddChildToOverlay(Column))
	{
		ColumnSlot->SetHorizontalAlignment(HAlign_Center);
		ColumnSlot->SetVerticalAlignment(VAlign_Center);
	}

	HeadingText = FIH2Menu::MakeText(WidgetTree, LOCTEXT("LevelSelectHeading", "Choisir un niveau"), Theme.HeadingFontSize, Theme.TextColor, true, TEXT("HeadingText"));
	AddTextShadow(HeadingText);
	if (UVerticalBoxSlot* HeadingSlot = Column->AddChildToVerticalBox(HeadingText))
	{
		HeadingSlot->SetHorizontalAlignment(HAlign_Center);
		HeadingSlot->SetPadding(FMargin(0.f, 0.f, 0.f, Theme.CardSpacing));
	}

	// Défilement horizontal : la rangée reste utilisable quand le catalogue dépasse la largeur de l'écran.
	UScrollBox* Scroll = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("CardScroll"));
	Scroll->SetOrientation(Orient_Horizontal);
	if (UVerticalBoxSlot* ScrollSlot = Column->AddChildToVerticalBox(Scroll))
	{
		ScrollSlot->SetHorizontalAlignment(HAlign_Center);
	}

	CardRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("CardRow"));
	Scroll->AddChild(CardRow);

	BackButton = MakeButton(LOCTEXT("Back", "Retour"), false, TEXT("BackButton"));
	if (UVerticalBoxSlot* BackSlot = Column->AddChildToVerticalBox(BackButton))
	{
		BackSlot->SetHorizontalAlignment(HAlign_Center);
		BackSlot->SetPadding(FMargin(0.f, Theme.CardSpacing, 0.f, 0.f));
	}
	BackButton->OnClicked.AddWeakLambda(this, [this]() { HandleBack(); });
}

void ULevelSelectScreen::RebuildCards()
{
	if (!CardRow)
	{
		return;
	}
	const UMenuTheme& Theme = UMenuTheme::Get();
	CardRow->ClearChildren();
	StaggerWidgets.Reset();
	StaggerWidgets.Add(HeadingText.Get());
	DefaultFocus = BackButton ? BackButton->GetFocusTarget() : nullptr;

	const UFIH2ProgressionSubsystem* Progression = UFIH2ProgressionSubsystem::Get(this);
	const ULevelCatalog* Catalog = Progression ? Progression->GetCatalog() : nullptr;
	if (Catalog)
	{
		bool bFocusChosen = false;
		int32 DisplayIndex = 1;
		for (const FLevelCatalogEntry& Entry : Catalog->GetSortedEntries())
		{
			const ELevelCardState State = Progression->GetLevelState(Entry);
			ULevelCardWidget* Card = WidgetTree->ConstructWidget<ULevelCardWidget>(ULevelCardWidget::StaticClass());
			Card->SetupCard(Entry, State, DisplayIndex++);
			if (UHorizontalBoxSlot* CardSlot = CardRow->AddChildToHorizontalBox(Card))
			{
				// La marge laisse la place à l'agrandissement au survol (sinon coupé par le défilement).
				CardSlot->SetPadding(FMargin(Theme.CardSpacing * 0.5f));
			}
			Card->OnClicked.AddWeakLambda(this, [this, Entry]()
			{
				if (UMainMenuRootWidget* RootWidget = GetRoot()) { RootWidget->LaunchLevel(Entry); }
			});
			StaggerWidgets.Add(Card);

			// Focus d'arrivée : la première carte jouable.
			if (!bFocusChosen && State != ELevelCardState::Locked)
			{
				DefaultFocus = Card->GetFocusTarget();
				bFocusChosen = true;
			}
		}
	}
	StaggerWidgets.Add(BackButton.Get());
}

void ULevelSelectScreen::OnScreenShown()
{
	// Les états (disponible / verrouillé / terminé) sont relus à chaque affichage.
	RebuildCards();
	PlayStagger(FVector2D(0.f, UMenuTheme::Get().EntryOffset));
}

// ====================================================================== Réglages

UMenuOptionRowWidget* USettingsScreen::AddRow(UVerticalBox* List, FName Name)
{
	UMenuOptionRowWidget* Row = WidgetTree->ConstructWidget<UMenuOptionRowWidget>(UMenuOptionRowWidget::StaticClass(), Name);
	if (UVerticalBoxSlot* RowSlot = List->AddChildToVerticalBox(Row))
	{
		RowSlot->SetPadding(FMargin(0.f, 0.f, 0.f, UMenuTheme::Get().ButtonSpacing * 0.5f));
	}
	return Row;
}

void USettingsScreen::AddSectionTitle(UVerticalBox* List, const FText& Title)
{
	const UMenuTheme& Theme = UMenuTheme::Get();
	UTextBlock* Text = FIH2Menu::MakeText(WidgetTree, Title, Theme.SmallFontSize, Theme.TextMutedColor, true);
	if (UVerticalBoxSlot* TitleSlot = List->AddChildToVerticalBox(Text))
	{
		TitleSlot->SetPadding(FMargin(0.f, Theme.ButtonSpacing, 0.f, Theme.ButtonSpacing * 0.5f));
	}
}

void USettingsScreen::BuildScreen()
{
	const UMenuTheme& Theme = UMenuTheme::Get();

	UOverlay* RootOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Root"));
	WidgetTree->RootWidget = RootOverlay;

	USizeBox* PanelSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("PanelSize"));
	PanelSize->SetWidthOverride(Theme.SettingsPanelWidth);
	if (UOverlaySlot* PanelSlot = RootOverlay->AddChildToOverlay(PanelSize))
	{
		PanelSlot->SetHorizontalAlignment(HAlign_Center);
		PanelSlot->SetVerticalAlignment(VAlign_Center);
	}

	UBorder* Panel = MakePanel(TEXT("Panel"));
	PanelSize->SetContent(Panel);

	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Column"));
	Panel->SetContent(Column);

	UTextBlock* Heading = FIH2Menu::MakeText(WidgetTree, LOCTEXT("SettingsHeading", "Réglages"), Theme.HeadingFontSize, Theme.TextColor, true, TEXT("HeadingText"));
	Column->AddChildToVerticalBox(Heading);

	// Liste défilante : reste utilisable sur les petites hauteurs d'écran.
	USizeBox* ListSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("ListSize"));
	ListSize->SetMaxDesiredHeight(Theme.SettingsListHeight);
	Column->AddChildToVerticalBox(ListSize);

	UScrollBox* Scroll = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("Scroll"));
	ListSize->SetContent(Scroll);

	UVerticalBox* List = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("List"));
	Scroll->AddChild(List);

	const TArray<FText> NoOptions;
	auto BindUserSetting = [this](UMenuOptionRowWidget* Row)
	{
		Row->OnSliderChanged.AddWeakLambda(this, [this](float) { HandleUserSettingChanged(); });
	};

	// ---- Audio
	AddSectionTitle(List, LOCTEXT("SectionAudio", "AUDIO"));
	MasterRow = AddRow(List, TEXT("MasterRow"));
	MasterRow->SetupSlider(LOCTEXT("MasterVolume", "Volume général"), 0.f, 1.f, Theme.VolumeStep, 1.f, &FormatPercent);
	BindUserSetting(MasterRow);
	MusicRow = AddRow(List, TEXT("MusicRow"));
	MusicRow->SetupSlider(LOCTEXT("MusicVolume", "Musique"), 0.f, 1.f, Theme.VolumeStep, 1.f, &FormatPercent);
	BindUserSetting(MusicRow);
	SfxRow = AddRow(List, TEXT("SfxRow"));
	SfxRow->SetupSlider(LOCTEXT("SfxVolume", "Effets"), 0.f, 1.f, Theme.VolumeStep, 1.f, &FormatPercent);
	BindUserSetting(SfxRow);

	// ---- Vidéo (appliquée par le bouton « Appliquer »)
	AddSectionTitle(List, LOCTEXT("SectionVideo", "VIDÉO"));
	WindowModeRow = AddRow(List, TEXT("WindowModeRow"));
	WindowModeRow->SetupChoice(LOCTEXT("WindowMode", "Mode d'affichage"), NoOptions, 0);
	ResolutionRow = AddRow(List, TEXT("ResolutionRow"));
	ResolutionRow->SetupChoice(LOCTEXT("Resolution", "Résolution"), NoOptions, 0);
	VSyncRow = AddRow(List, TEXT("VSyncRow"));
	VSyncRow->SetupChoice(LOCTEXT("VSync", "Synchronisation verticale"), NoOptions, 0);
	QualityRow = AddRow(List, TEXT("QualityRow"));
	QualityRow->SetupChoice(LOCTEXT("Quality", "Qualité graphique"), NoOptions, 0);
	FrameRateRow = AddRow(List, TEXT("FrameRateRow"));
	FrameRateRow->SetupChoice(LOCTEXT("FrameRate", "Limite d'images par seconde"), NoOptions, 0);

	// ---- Jeu
	AddSectionTitle(List, LOCTEXT("SectionGame", "JEU"));
	SensitivityRow = AddRow(List, TEXT("SensitivityRow"));
	SensitivityRow->SetupSlider(LOCTEXT("CameraSensitivity", "Sensibilité de la caméra"),
		Theme.CameraSensitivityRange.X, Theme.CameraSensitivityRange.Y, Theme.CameraSensitivityStep, 1.f, &FormatMultiplier);
	BindUserSetting(SensitivityRow);

	// ---- Boutons
	UHorizontalBox* Buttons = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("Buttons"));
	if (UVerticalBoxSlot* ButtonsSlot = Column->AddChildToVerticalBox(Buttons))
	{
		ButtonsSlot->SetPadding(FMargin(0.f, Theme.PanelPadding * 0.5f, 0.f, 0.f));
		ButtonsSlot->SetHorizontalAlignment(HAlign_Fill);
	}

	auto AddButton = [this, Buttons, &Theme](const FText& Label, bool bAccent, FName Name) -> UMenuButtonWidget*
	{
		UMenuButtonWidget* Button = MakeButton(Label, bAccent, Name);
		if (UHorizontalBoxSlot* ButtonSlot = Buttons->AddChildToHorizontalBox(Button))
		{
			ButtonSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			ButtonSlot->SetPadding(FMargin(Theme.ButtonSpacing * 0.5f, 0.f));
		}
		return Button;
	};
	UMenuButtonWidget* BackButton = AddButton(LOCTEXT("Back", "Retour"), false, TEXT("BackButton"));
	UMenuButtonWidget* ResetButton = AddButton(LOCTEXT("ResetSettings", "Réinitialiser"), false, TEXT("ResetButton"));
	UMenuButtonWidget* ApplyButton = AddButton(LOCTEXT("ApplySettings", "Appliquer"), true, TEXT("ApplyButton"));

	BackButton->OnClicked.AddWeakLambda(this, [this]() { HandleBack(); });
	ResetButton->OnClicked.AddWeakLambda(this, [this]() { HandleReset(); });
	ApplyButton->OnClicked.AddWeakLambda(this, [this]() { HandleApply(); });

	StaggerWidgets.Add(Panel);
	DefaultFocus = MasterRow->GetFocusTarget();
}

void USettingsScreen::OnScreenShown()
{
	RefreshFromSettings();
	// Un seul bloc à faire entrer : le panneau glisse déjà avec l'écran.
	PlayStagger(FVector2D(0.f, UMenuTheme::Get().EntryOffset));
}

void USettingsScreen::RefreshFromSettings()
{
	const UMenuTheme& Theme = UMenuTheme::Get();

	// ---- Audio + jeu (sauvegarde)
	if (const UFIH2ProgressionSubsystem* Progression = UFIH2ProgressionSubsystem::Get(this))
	{
		const FFIH2UserSettings User = Progression->GetUserSettings();
		MasterRow->SetSliderValue(User.MasterVolume);
		MusicRow->SetSliderValue(User.MusicVolume);
		SfxRow->SetSliderValue(User.SfxVolume);
		SensitivityRow->SetSliderValue(User.CameraSensitivity);
	}

	// ---- Vidéo (UGameUserSettings)
	UGameUserSettings* Video = UGameUserSettings::GetGameUserSettings();
	if (!Video)
	{
		return;
	}

	WindowModeRow->SetChoiceOptions(
		{ LOCTEXT("ModeFullscreen", "Plein écran"), LOCTEXT("ModeBorderless", "Fenêtré sans bordure"), LOCTEXT("ModeWindowed", "Fenêtré") },
		FMath::Clamp(static_cast<int32>(Video->GetFullscreenMode()), 0, 2));

	// Résolutions proposées par l'écran, plus la résolution courante si elle n'y figure pas.
	Resolutions.Reset();
	UKismetSystemLibrary::GetSupportedFullscreenResolutions(Resolutions);
	const FIntPoint CurrentResolution = Video->GetScreenResolution();
	if (CurrentResolution.X > 0 && CurrentResolution.Y > 0)
	{
		Resolutions.AddUnique(CurrentResolution);
	}
	Resolutions.Sort([](const FIntPoint& A, const FIntPoint& B) { return A.X != B.X ? A.X < B.X : A.Y < B.Y; });
	TArray<FText> ResolutionOptions;
	for (const FIntPoint& Resolution : Resolutions)
	{
		ResolutionOptions.Add(FText::FromString(FString::Printf(TEXT("%d × %d"), Resolution.X, Resolution.Y)));
	}
	ResolutionRow->SetChoiceOptions(ResolutionOptions, FMath::Max(0, Resolutions.IndexOfByKey(CurrentResolution)));

	VSyncRow->SetChoiceOptions({ LOCTEXT("Off", "Non"), LOCTEXT("On", "Oui") }, Video->IsVSyncEnabled() ? 1 : 0);

	// Qualité : les 5 niveaux du moteur ; « Personnalisée » n'apparaît que si les réglages actuels ne correspondent à aucun.
	TArray<FText> QualityOptions = {
		LOCTEXT("QualityLow", "Basse"), LOCTEXT("QualityMedium", "Moyenne"), LOCTEXT("QualityHigh", "Haute"),
		LOCTEXT("QualityEpic", "Épique"), LOCTEXT("QualityCinematic", "Cinématique") };
	QualityLevels = { 0, 1, 2, 3, 4 };
	const int32 CurrentQuality = Video->GetOverallScalabilityLevel();
	if (!QualityLevels.Contains(CurrentQuality))
	{
		QualityOptions.Add(LOCTEXT("QualityCustom", "Personnalisée"));
		QualityLevels.Add(INDEX_NONE);
	}
	QualityRow->SetChoiceOptions(QualityOptions, QualityLevels.Contains(CurrentQuality) ? QualityLevels.IndexOfByKey(CurrentQuality) : QualityLevels.Num() - 1);

	// Limite de FPS : liste du thème (0 = illimitée), plus la valeur courante si elle n'y figure pas.
	FrameRates = Theme.FrameRateLimits;
	const int32 CurrentLimit = FMath::Max(0, FMath::RoundToInt(Video->GetFrameRateLimit()));
	FrameRates.AddUnique(CurrentLimit);
	TArray<FText> FrameRateOptions;
	for (const int32 Limit : FrameRates)
	{
		FrameRateOptions.Add(Limit > 0
			? FText::Format(LOCTEXT("FrameRateFormat", "{0} FPS"), FText::AsNumber(Limit))
			: LOCTEXT("FrameRateUnlimited", "Illimitée"));
	}
	FrameRateRow->SetChoiceOptions(FrameRateOptions, FMath::Max(0, FrameRates.IndexOfByKey(CurrentLimit)));
}

void USettingsScreen::HandleUserSettingChanged()
{
	// Audio et sensibilité : effet immédiat (pré-écoute), écriture de la sauvegarde à « Appliquer » ou « Retour ».
	if (UFIH2ProgressionSubsystem* Progression = UFIH2ProgressionSubsystem::Get(this))
	{
		FFIH2UserSettings User = Progression->GetUserSettings();
		User.MasterVolume = MasterRow->GetSliderValue();
		User.MusicVolume = MusicRow->GetSliderValue();
		User.SfxVolume = SfxRow->GetSliderValue();
		User.CameraSensitivity = SensitivityRow->GetSliderValue();
		Progression->SetUserSettings(this, User, false);
	}
}

void USettingsScreen::HandleApply()
{
	if (UFIH2ProgressionSubsystem* Progression = UFIH2ProgressionSubsystem::Get(this))
	{
		Progression->SaveNow();
	}
	ApplyVideoSettings();
}

void USettingsScreen::HandleReset()
{
	if (UFIH2ProgressionSubsystem* Progression = UFIH2ProgressionSubsystem::Get(this))
	{
		Progression->SetUserSettings(this, Progression->GetDefaultUserSettings(), true);
	}
	if (UGameUserSettings* Video = UGameUserSettings::GetGameUserSettings())
	{
		Video->SetToDefaults();
		const FIntPoint Resolution = Video->GetScreenResolution();
		if (Resolution.X <= 0 || Resolution.Y <= 0)
		{
			Video->SetScreenResolution(Video->GetDesktopResolution());
		}
	}
	RefreshFromSettings();
	ApplyVideoSettings(); // même chemin que « Appliquer » : confirmation si le mode vidéo change
}

void USettingsScreen::ApplyVideoSettings()
{
	UGameUserSettings* Video = UGameUserSettings::GetGameUserSettings();
	if (!Video)
	{
		return;
	}

	const EWindowMode::Type PreviousMode = Video->GetLastConfirmedFullscreenMode();
	const FIntPoint PreviousResolution = Video->GetLastConfirmedScreenResolution();

	const EWindowMode::Type NewMode = EWindowMode::ConvertIntToWindowMode(WindowModeRow->GetChoiceIndex());
	const FIntPoint NewResolution = Resolutions.IsValidIndex(ResolutionRow->GetChoiceIndex())
		? Resolutions[ResolutionRow->GetChoiceIndex()] : Video->GetScreenResolution();

	Video->SetFullscreenMode(NewMode);
	Video->SetScreenResolution(NewResolution);
	Video->SetVSyncEnabled(VSyncRow->GetChoiceIndex() == 1);
	if (QualityLevels.IsValidIndex(QualityRow->GetChoiceIndex()) && QualityLevels[QualityRow->GetChoiceIndex()] != INDEX_NONE)
	{
		Video->SetOverallScalabilityLevel(QualityLevels[QualityRow->GetChoiceIndex()]);
	}
	if (FrameRates.IsValidIndex(FrameRateRow->GetChoiceIndex()))
	{
		Video->SetFrameRateLimit(static_cast<float>(FrameRates[FrameRateRow->GetChoiceIndex()]));
	}

	Video->ApplyNonResolutionSettings();
	Video->ApplyResolutionSettings(false);

	const bool bVideoModeChanged = NewMode != PreviousMode || NewResolution != PreviousResolution;
	UMainMenuRootWidget* RootWidget = GetRoot();
	if (!bVideoModeChanged || !RootWidget)
	{
		Video->ConfirmVideoMode();
		Video->SaveSettings();
		return;
	}

	// Nouveau mode vidéo : le joueur doit le confirmer. Sans réponse (écran noir, résolution refusée...),
	// l'ancien mode est rétabli automatiquement à la fin du compte à rebours.
	TWeakObjectPtr<USettingsScreen> WeakThis(this);
	RootWidget->ShowConfirm(
		LOCTEXT("VideoConfirmTitle", "Conserver ce mode d'affichage ?"),
		LOCTEXT("VideoConfirmMessage", "Sans confirmation, l'ancien mode d'affichage sera rétabli."),
		LOCTEXT("VideoConfirmKeep", "Conserver"),
		LOCTEXT("VideoConfirmRevert", "Rétablir"),
		[]()
		{
			if (UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings())
			{
				Settings->ConfirmVideoMode();
				Settings->SaveSettings();
			}
		},
		[WeakThis]()
		{
			if (UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings())
			{
				Settings->RevertVideoMode();
				Settings->ApplyResolutionSettings(false);
				Settings->SaveSettings();
			}
			if (WeakThis.IsValid())
			{
				WeakThis->RefreshFromSettings();
			}
		},
		UMenuTheme::Get().VideoConfirmSeconds);
}

bool USettingsScreen::HandleBack()
{
	// Audio / jeu : enregistrés en sortant. Vidéo non appliquée : abandonnée (relue au prochain affichage).
	if (UFIH2ProgressionSubsystem* Progression = UFIH2ProgressionSubsystem::Get(this))
	{
		Progression->SaveNow();
	}
	return Super::HandleBack();
}

// ====================================================================== Confirmation

void UConfirmDialogScreen::BuildScreen()
{
	const UMenuTheme& Theme = UMenuTheme::Get();

	UOverlay* RootOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Root"));
	WidgetTree->RootWidget = RootOverlay;

	USizeBox* DialogSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("DialogSize"));
	DialogSize->SetWidthOverride(Theme.DialogWidth);
	if (UOverlaySlot* DialogSlot = RootOverlay->AddChildToOverlay(DialogSize))
	{
		DialogSlot->SetHorizontalAlignment(HAlign_Center);
		DialogSlot->SetVerticalAlignment(VAlign_Center);
	}

	UBorder* Panel = MakePanel(TEXT("Panel"));
	DialogSize->SetContent(Panel);

	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Column"));
	Panel->SetContent(Column);

	TitleText = FIH2Menu::MakeText(WidgetTree, FText::GetEmpty(), Theme.SubtitleFontSize, Theme.TextColor, true, TEXT("TitleText"));
	TitleText->SetAutoWrapText(true);
	Column->AddChildToVerticalBox(TitleText);

	MessageText = FIH2Menu::MakeText(WidgetTree, FText::GetEmpty(), Theme.BodyFontSize, Theme.TextMutedColor, false, TEXT("MessageText"));
	MessageText->SetAutoWrapText(true);
	if (UVerticalBoxSlot* MessageSlot = Column->AddChildToVerticalBox(MessageText))
	{
		MessageSlot->SetPadding(FMargin(0.f, Theme.ButtonSpacing, 0.f, 0.f));
	}

	CountdownText = FIH2Menu::MakeText(WidgetTree, FText::GetEmpty(), Theme.BodyFontSize, Theme.AlertColor, true, TEXT("CountdownText"));
	CountdownText->SetVisibility(ESlateVisibility::Collapsed);
	if (UVerticalBoxSlot* CountdownSlot = Column->AddChildToVerticalBox(CountdownText))
	{
		CountdownSlot->SetPadding(FMargin(0.f, Theme.ButtonSpacing, 0.f, 0.f));
	}

	UHorizontalBox* Buttons = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("Buttons"));
	if (UVerticalBoxSlot* ButtonsSlot = Column->AddChildToVerticalBox(Buttons))
	{
		ButtonsSlot->SetPadding(FMargin(0.f, Theme.PanelPadding * 0.75f, 0.f, 0.f));
	}
	auto AddButton = [this, Buttons, &Theme](bool bAccent, FName Name) -> UMenuButtonWidget*
	{
		UMenuButtonWidget* Button = MakeButton(FText::GetEmpty(), bAccent, Name);
		if (UHorizontalBoxSlot* ButtonSlot = Buttons->AddChildToHorizontalBox(Button))
		{
			ButtonSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			ButtonSlot->SetPadding(FMargin(Theme.ButtonSpacing * 0.5f, 0.f));
		}
		return Button;
	};
	CancelButton = AddButton(false, TEXT("CancelButton"));
	ConfirmButton = AddButton(true, TEXT("ConfirmButton"));

	CancelButton->OnClicked.AddWeakLambda(this, [this]() { Resolve(false); });
	ConfirmButton->OnClicked.AddWeakLambda(this, [this]() { Resolve(true); });

	StaggerWidgets.Add(Panel);
	// Focus d'arrivée sur « annuler » : une pression machinale sur Entrée ne confirme rien.
	DefaultFocus = CancelButton->GetFocusTarget();
}

void UConfirmDialogScreen::Configure(const FText& Title, const FText& Message, const FText& ConfirmLabel, const FText& CancelLabel,
	TFunction<void()> InOnConfirm, TFunction<void()> InOnCancel, float AutoCancelSeconds)
{
	if (TitleText) { TitleText->SetText(Title); }
	if (MessageText) { MessageText->SetText(Message); }
	if (ConfirmButton) { ConfirmButton->SetLabel(ConfirmLabel); }
	if (CancelButton) { CancelButton->SetLabel(CancelLabel); }
	OnConfirm = MoveTemp(InOnConfirm);
	OnCancel = MoveTemp(InOnCancel);
	SecondsRemaining = AutoCancelSeconds > 0.f ? FMath::CeilToInt(AutoCancelSeconds) : 0;
	bResolved = false;
}

void UConfirmDialogScreen::OnScreenShown()
{
	PlayStagger(FVector2D(0.f, UMenuTheme::Get().EntryOffset));

	if (CountdownText)
	{
		CountdownText->SetVisibility(SecondsRemaining > 0 ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
	if (SecondsRemaining > 0)
	{
		RefreshCountdownText();
		if (UWorld* World = GetWorld())
		{
			// Une minuterie d'une seconde (unité du compte à rebours), pas de Tick.
			World->GetTimerManager().SetTimer(CountdownTimer,
				FTimerDelegate::CreateUObject(this, &UConfirmDialogScreen::HandleCountdownTick), 1.f, true);
		}
	}
}

void UConfirmDialogScreen::OnScreenHidden()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(CountdownTimer);
	}
}

void UConfirmDialogScreen::RefreshCountdownText()
{
	if (CountdownText)
	{
		CountdownText->SetText(FText::Format(LOCTEXT("CountdownFormat", "Retour automatique dans {0} s"), FText::AsNumber(FMath::Max(0, SecondsRemaining))));
	}
}

void UConfirmDialogScreen::HandleCountdownTick()
{
	SecondsRemaining = FMath::Max(0, SecondsRemaining - 1);
	RefreshCountdownText();
	// Si une transition est en cours, on réessaie à la seconde suivante.
	const UMainMenuRootWidget* RootWidget = GetRoot();
	if (SecondsRemaining <= 0 && RootWidget && !RootWidget->IsInputLocked())
	{
		Resolve(false);
	}
}

bool UConfirmDialogScreen::HandleBack()
{
	Resolve(false);
	return true;
}

void UConfirmDialogScreen::Resolve(bool bConfirmed)
{
	UMainMenuRootWidget* RootWidget = GetRoot();
	if (bResolved || !RootWidget || RootWidget->IsInputLocked())
	{
		return;
	}
	bResolved = true;

	// Les rappels sont copiés avant de refermer : ils peuvent rouvrir une boîte et écraser ceux-ci.
	TFunction<void()> Callback = bConfirmed ? MoveTemp(OnConfirm) : MoveTemp(OnCancel);
	OnConfirm = nullptr;
	OnCancel = nullptr;

	RootWidget->PopScreen();
	if (Callback)
	{
		Callback();
	}
}

#undef LOCTEXT_NAMESPACE
