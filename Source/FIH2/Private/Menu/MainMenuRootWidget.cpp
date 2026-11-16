#include "Menu/MainMenuRootWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/CircularThrobber.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/World.h"
#include "Input/Events.h"
#include "InputCoreTypes.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Menu/MainMenuGameMode.h"
#include "Menu/MenuScreens.h"
#include "Menu/MenuTheme.h"
#include "Menu/MenuTweenSubsystem.h"
#include "MenuWidgetHelpers.h"
#include "System/FIH2ProgressionSubsystem.h"
#include "TimerManager.h"

#define LOCTEXT_NAMESPACE "FIH2Menu"

DEFINE_LOG_CATEGORY_STATIC(LogFIH2MenuUI, Log, All);

namespace
{
	const FName ScreenChannel(TEXT("Screen"));
	const FName FadeChannel(TEXT("Fade"));
	/** Ordre d'affichage du voile de fondu : au-dessus de tous les écrans. */
	const int32 FadeZOrder = 100;
}

bool UMainMenuRootWidget::Initialize()
{
	const bool bSuperInitialized = Super::Initialize();
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildTree();
	}
	return bSuperInitialized;
}

void UMainMenuRootWidget::BuildTree()
{
	const UMenuTheme& Theme = UMenuTheme::Get();

	UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
	WidgetTree->RootWidget = Canvas;

	auto FillCanvas = [](UCanvasPanelSlot* CanvasSlot, const FMargin& Offsets, int32 ZOrder)
	{
		if (CanvasSlot)
		{
			CanvasSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
			CanvasSlot->SetOffsets(Offsets);
			CanvasSlot->SetZOrder(ZOrder);
		}
	};

	// Fond invisible mais cliquable : un clic « dans le vide » revient à la racine, qui redonne le focus à l'écran
	// (sinon le clavier et la manette n'auraient plus de cible).
	UBorder* ClickCatcher = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("ClickCatcher"));
	ClickCatcher->SetBrushColor(FLinearColor::Transparent);
	ClickCatcher->SetVisibility(ESlateVisibility::Visible);
	FillCanvas(Canvas->AddChildToCanvas(ClickCatcher), FMargin(0.f), 0);

	ScreenHost = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("ScreenHost"));
	ScreenHost->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	FillCanvas(Canvas->AddChildToCanvas(ScreenHost), FMargin(Theme.ScreenPadding), 1);

	// Voile de fondu (ouverture du menu, lancement d'un niveau) avec l'indicateur de chargement.
	FadeOverlay = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("FadeOverlay"));
	FadeOverlay->SetBrushColor(Theme.FadeColor);
	FadeOverlay->SetHorizontalAlignment(HAlign_Center);
	FadeOverlay->SetVerticalAlignment(VAlign_Center);
	FadeOverlay->SetVisibility(ESlateVisibility::HitTestInvisible);
	FillCanvas(Canvas->AddChildToCanvas(FadeOverlay), FMargin(0.f), FadeZOrder);

	UVerticalBox* Loading = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("LoadingBox"));
	UCircularThrobber* Throbber = WidgetTree->ConstructWidget<UCircularThrobber>(UCircularThrobber::StaticClass(), TEXT("LoadingThrobber"));
	if (UVerticalBoxSlot* ThrobberSlot = Loading->AddChildToVerticalBox(Throbber))
	{
		ThrobberSlot->SetHorizontalAlignment(HAlign_Center);
	}
	UTextBlock* LoadingText = FIH2Menu::MakeText(WidgetTree, LOCTEXT("Loading", "Chargement…"), Theme.BodyFontSize, Theme.TextColor);
	if (UVerticalBoxSlot* TextSlot = Loading->AddChildToVerticalBox(LoadingText))
	{
		TextSlot->SetHorizontalAlignment(HAlign_Center);
		TextSlot->SetPadding(FMargin(0.f, Theme.ButtonSpacing, 0.f, 0.f));
	}
	Loading->SetVisibility(ESlateVisibility::Collapsed);
	FadeOverlay->SetContent(Loading);
	LoadingBox = Loading;
}

void UMainMenuRootWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (ScreenStack.Num() > 0 || !ScreenHost)
	{
		return;
	}

	// Premier écran, puis fondu depuis le noir : la cuve apparaît derrière le menu.
	if (UMenuScreenWidget* Main = GetOrCreateScreen(UMainMenuScreen::StaticClass()))
	{
		ScreenStack.Add(Main);
		TransitionScreens(nullptr, Main, true);
	}

	UMenuTweenSubsystem* Tweens = UMenuTweenSubsystem::Get(this);
	if (FadeOverlay && Tweens)
	{
		FMenuTweenSpec Fade;
		Fade.Duration = UMenuTheme::Get().IntroFadeDuration;
		Fade.Ease = EMenuEase::InOutSine;
		Fade.FromOpacity = 1.f;
		Fade.ToOpacity = 0.f;
		TWeakObjectPtr<UMainMenuRootWidget> WeakThis(this);
		Fade.OnFinished = [WeakThis]()
		{
			if (WeakThis.IsValid() && WeakThis->FadeOverlay && !WeakThis->bLaunching)
			{
				WeakThis->FadeOverlay->SetVisibility(ESlateVisibility::Collapsed);
			}
		};
		Tweens->Play(FadeOverlay.Get(), Fade, FadeChannel);
	}
	else if (FadeOverlay)
	{
		FadeOverlay->SetVisibility(ESlateVisibility::Collapsed);
	}
}

// ------------------------------------------------------------------ Pile d'écrans

UMenuScreenWidget* UMainMenuRootWidget::GetCurrentScreen() const
{
	return ScreenStack.Num() > 0 ? ScreenStack.Last().Get() : nullptr;
}

UMenuScreenWidget* UMainMenuRootWidget::GetOrCreateScreen(TSubclassOf<UMenuScreenWidget> ScreenClass)
{
	if (!ScreenClass || !ScreenHost)
	{
		return nullptr;
	}
	if (const TObjectPtr<UMenuScreenWidget>* Existing = ScreenInstances.Find(ScreenClass.Get()))
	{
		return Existing->Get();
	}

	UMenuScreenWidget* Screen = CreateWidget<UMenuScreenWidget>(GetOwningPlayer(), ScreenClass);
	if (!Screen)
	{
		return nullptr;
	}
	Screen->SetRoot(this);
	Screen->SetVisibility(ESlateVisibility::Collapsed);
	if (UOverlaySlot* ScreenSlot = ScreenHost->AddChildToOverlay(Screen))
	{
		ScreenSlot->SetHorizontalAlignment(HAlign_Fill);
		ScreenSlot->SetVerticalAlignment(VAlign_Fill);
	}
	ScreenInstances.Add(ScreenClass.Get(), Screen);
	return Screen;
}

UMenuScreenWidget* UMainMenuRootWidget::PushScreen(TSubclassOf<UMenuScreenWidget> ScreenClass)
{
	if (bInputLocked)
	{
		return nullptr; // anti double-clic : une transition est déjà en cours
	}
	UMenuScreenWidget* To = GetOrCreateScreen(ScreenClass);
	UMenuScreenWidget* From = GetCurrentScreen();
	if (!To || To == From)
	{
		return To;
	}
	ScreenStack.Add(To);
	TransitionScreens(From, To, true);
	return To;
}

void UMainMenuRootWidget::PopScreen()
{
	if (bInputLocked || ScreenStack.Num() < 2)
	{
		return;
	}
	UMenuScreenWidget* From = ScreenStack.Pop().Get();
	TransitionScreens(From, GetCurrentScreen(), false);
}

void UMainMenuRootWidget::SetInputLocked(bool bLocked)
{
	bInputLocked = bLocked;
	if (ScreenHost)
	{
		// Souris : plus aucun widget d'écran ne reçoit de clic ni de survol. Clavier / manette : NativeOnPreviewKeyDown.
		ScreenHost->SetVisibility(bLocked ? ESlateVisibility::HitTestInvisible : ESlateVisibility::SelfHitTestInvisible);
	}
}

void UMainMenuRootWidget::TransitionScreens(UMenuScreenWidget* From, UMenuScreenWidget* To, bool bForward)
{
	const UMenuTheme& Theme = UMenuTheme::Get();
	UMenuTweenSubsystem* Tweens = UMenuTweenSubsystem::Get(this);
	const float Direction = bForward ? 1.f : -1.f;
	const FVector2D Slide(Theme.ScreenSlideDistance * Direction, 0.f);
	TWeakObjectPtr<UMainMenuRootWidget> WeakThis(this);

	SetInputLocked(true);

	if (From)
	{
		From->OnScreenHidden();
		TWeakObjectPtr<UMenuScreenWidget> WeakFrom(From);
		auto HideFrom = [WeakFrom]()
		{
			if (WeakFrom.IsValid())
			{
				WeakFrom->SetVisibility(ESlateVisibility::Collapsed);
			}
		};
		if (Tweens)
		{
			FMenuTweenSpec Out;
			Out.Duration = Theme.ScreenTransitionDuration;
			Out.Ease = EMenuEase::InCubic;
			Out.ToOpacity = 0.f;
			Out.ToTranslation = -Slide;
			Out.OnFinished = HideFrom;
			Tweens->Play(From, Out, ScreenChannel);
		}
		else
		{
			HideFrom();
		}
	}

	auto FinishTransition = [WeakThis]()
	{
		if (WeakThis.IsValid() && !WeakThis->bLaunching)
		{
			WeakThis->SetInputLocked(false);
			WeakThis->FocusCurrentScreen();
		}
	};

	if (To)
	{
		To->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		To->OnScreenShown();
		if (Tweens)
		{
			FMenuTweenSpec In;
			In.Duration = Theme.ScreenTransitionDuration;
			In.Ease = EMenuEase::OutCubic;
			In.FromOpacity = 0.f;
			In.ToOpacity = 1.f;
			In.FromTranslation = Slide;
			In.ToTranslation = FVector2D::ZeroVector;
			In.OnFinished = FinishTransition;
			Tweens->Play(To, In, ScreenChannel);
			return;
		}
	}
	FinishTransition();
}

void UMainMenuRootWidget::FocusCurrentScreen()
{
	if (const UMenuScreenWidget* Screen = GetCurrentScreen())
	{
		if (UWidget* Target = Screen->GetDefaultFocusWidget())
		{
			Target->SetKeyboardFocus();
		}
	}
}

// ------------------------------------------------------------------ Entrées

FReply UMainMenuRootWidget::NativeOnPreviewKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	// Pendant une transition ou un lancement : aucune touche (clavier ou manette) n'atteint les écrans.
	if (bInputLocked)
	{
		return FReply::Handled();
	}
	return Super::NativeOnPreviewKeyDown(InGeometry, InKeyEvent);
}

FReply UMainMenuRootWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();
	if (Key == EKeys::Escape || Key == EKeys::Gamepad_FaceButton_Right)
	{
		if (!InKeyEvent.IsRepeat())
		{
			if (UMenuScreenWidget* Screen = GetCurrentScreen())
			{
				Screen->HandleBack();
			}
		}
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

FReply UMainMenuRootWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	// Clic hors de tout bouton : on garde une cible pour le clavier et la manette.
	const UMenuScreenWidget* Screen = GetCurrentScreen();
	if (!bInputLocked && Screen && !Screen->HasFocusedDescendants())
	{
		FocusCurrentScreen();
	}
	return FReply::Handled();
}

// ------------------------------------------------------------------ Actions

void UMainMenuRootWidget::RequestPlay()
{
	if (bInputLocked)
	{
		return;
	}
	const UFIH2ProgressionSubsystem* Progression = UFIH2ProgressionSubsystem::Get(this);
	const ULevelCatalog* Catalog = Progression ? Progression->GetCatalog() : nullptr;
	if (!Catalog)
	{
		UE_LOG(LogFIH2MenuUI, Warning, TEXT("Jouer : aucun catalogue de niveaux (Project Settings > FIH2 > Level Catalog)."));
		return;
	}

	// Tutoriel jamais complété : on y va directement. Sinon, le joueur choisit son niveau.
	if (!Progression->IsTutorialCompleted())
	{
		if (const FLevelCatalogEntry* Tutorial = Catalog->FindTutorialEntry())
		{
			LaunchLevel(*Tutorial);
			return;
		}
	}
	PushScreen(ULevelSelectScreen::StaticClass());
}

void UMainMenuRootWidget::RequestQuit()
{
	TWeakObjectPtr<UMainMenuRootWidget> WeakThis(this);
	ShowConfirm(
		LOCTEXT("QuitTitle", "Quitter le jeu ?"),
		LOCTEXT("QuitMessage", "Votre progression et vos réglages sont déjà enregistrés."),
		LOCTEXT("QuitConfirm", "Quitter"),
		LOCTEXT("QuitCancel", "Rester"),
		[WeakThis]()
		{
			if (WeakThis.IsValid())
			{
				UKismetSystemLibrary::QuitGame(WeakThis.Get(), WeakThis->GetOwningPlayer(), EQuitPreference::Quit, false);
			}
		});
}

void UMainMenuRootWidget::ShowConfirm(const FText& Title, const FText& Message, const FText& ConfirmLabel, const FText& CancelLabel,
	TFunction<void()> OnConfirm, TFunction<void()> OnCancel, float AutoCancelSeconds)
{
	if (bInputLocked)
	{
		return;
	}
	if (UConfirmDialogScreen* Dialog = Cast<UConfirmDialogScreen>(GetOrCreateScreen(UConfirmDialogScreen::StaticClass())))
	{
		Dialog->Configure(Title, Message, ConfirmLabel, CancelLabel, MoveTemp(OnConfirm), MoveTemp(OnCancel), AutoCancelSeconds);
		PushScreen(UConfirmDialogScreen::StaticClass());
	}
}

void UMainMenuRootWidget::LaunchLevel(const FLevelCatalogEntry& Entry)
{
	if (bLaunching || bInputLocked || Entry.Map.IsNull())
	{
		return;
	}
	bLaunching = true; // les entrées restent verrouillées jusqu'au changement de carte
	SetInputLocked(true);
	PendingMap = Entry.Map;

	UMenuTweenSubsystem* Tweens = UMenuTweenSubsystem::Get(this);
	if (!FadeOverlay || !Tweens)
	{
		HandleLaunchFadeFinished();
		return;
	}

	FadeOverlay->SetVisibility(ESlateVisibility::Visible);
	FMenuTweenSpec Fade;
	Fade.Duration = UMenuTheme::Get().LaunchFadeDuration;
	Fade.Ease = EMenuEase::InOutSine;
	Fade.FromOpacity = 0.f;
	Fade.ToOpacity = 1.f;
	TWeakObjectPtr<UMainMenuRootWidget> WeakThis(this);
	Fade.OnFinished = [WeakThis]()
	{
		if (WeakThis.IsValid())
		{
			WeakThis->HandleLaunchFadeFinished();
		}
	};
	Tweens->Play(FadeOverlay.Get(), Fade, FadeChannel);
}

void UMainMenuRootWidget::HandleLaunchFadeFinished()
{
	if (LoadingBox)
	{
		LoadingBox->SetVisibility(ESlateVisibility::HitTestInvisible);
	}

	// L'ouverture est repoussée au tick suivant : l'écran noir et l'indicateur sont dessinés avant le chargement.
	// À BRANCHER : OpenLevel charge la carte de façon bloquante (l'indicateur est figé pendant ce temps). Un
	// chargement réellement asynchrone demande un « seamless travel » avec une carte de transition ; précharger
	// le paquet ici ne sert à rien, le moteur libère tout avant de charger la nouvelle carte.
	if (UWorld* World = GetWorld())
	{
		TWeakObjectPtr<UMainMenuRootWidget> WeakThis(this);
		World->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this, [WeakThis]()
		{
			if (WeakThis.IsValid())
			{
				WeakThis->OpenPendingLevel();
			}
		}));
	}
}

void UMainMenuRootWidget::OpenPendingLevel()
{
	if (PendingMap.IsNull())
	{
		return;
	}
	// Le niveau doit retrouver ses entrées de jeu : le mode « UI seulement » du menu ne doit pas le suivre.
	if (AMainMenuPlayerController* MenuController = Cast<AMainMenuPlayerController>(GetOwningPlayer()))
	{
		MenuController->RestoreGameInput();
	}
	UGameplayStatics::OpenLevelBySoftObjectPtr(this, PendingMap);
}

#undef LOCTEXT_NAMESPACE
