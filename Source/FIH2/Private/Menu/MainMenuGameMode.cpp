#include "Menu/MainMenuGameMode.h"

#include "Components/BoxComponent.h"
#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "Framework/Application/SlateApplication.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "Menu/MainMenuRootWidget.h"
#include "Menu/MenuAmbientConfig.h"
#include "Menu/MenuCameraRig.h"
#include "System/EcosystemSubsystem.h"
#include "System/LevelConfig.h"

DEFINE_LOG_CATEGORY_STATIC(LogFIH2Menu, Log, All);

AMainMenuGameMode::AMainMenuGameMode()
{
	DefaultPawnClass = nullptr; // pas de pion : la vue vient de AMenuCameraRig
	PlayerControllerClass = AMainMenuPlayerController::StaticClass();
}

void AMainMenuGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);

	// InitGame passe avant OnWorldBeginPlay des sous-systèmes : la config ambiante est en place avant que
	// l'écosystème ne charge celle du tutoriel, et UTutorialSubsystem ne trouve aucune séquence à lancer.
	UEcosystemSubsystem* Ecosystem = GetWorld() ? GetWorld()->GetSubsystem<UEcosystemSubsystem>() : nullptr;
	if (!Ecosystem)
	{
		return;
	}

	// Copie : on ne modifie jamais le Data Asset chargé.
	const ULevelConfig* Source = AmbientConfig ? AmbientConfig->EcosystemConfig.Get() : nullptr;
	ULevelConfig* Ambient = Source ? DuplicateObject<ULevelConfig>(Source, this) : NewObject<ULevelConfig>(this);

	// Garde-fous du mode ambiant, quelles que soient les données.
	Ambient->TutorialSequence = nullptr;      // pas de tutoriel dans le menu
	Ambient->bUseTimedObjective = false;      // pas de victoire / défaite au chrono
	Ambient->bReproductionEnabled = false;    // effectif stable, pas d'œufs
	Ambient->AsphyxiaThreshold = 0.f;         // l'oxygène ne passe jamais sous 0 : aucune asphyxie
	Ambient->bShowDebugHUD = false;

	Ecosystem->SetLevelConfig(Ambient);
	Ecosystem->SetDefeatEnabled(false);       // ni jauge, ni extinction
	Ecosystem->SetStarvationEnabled(false);   // BP_Fish ne détruit pas un poisson à 0 de faim
}

void AMainMenuGameMode::BeginPlay()
{
	Super::BeginPlay();
	SpawnPopulation();
}

void AMainMenuGameMode::SpawnPopulation()
{
	UWorld* World = GetWorld();
	if (!World || !AmbientConfig || AmbientConfig->Population.Num() == 0)
	{
		return;
	}

	// Volume d'eau de la cuve placée dans la carte.
	FBox Water(ForceInit);
	if (AmbientConfig->TankClass)
	{
		for (TActorIterator<AActor> It(World, AmbientConfig->TankClass); It && !Water.IsValid; ++It)
		{
			TInlineComponentArray<UBoxComponent*> Boxes(*It);
			for (const UBoxComponent* Box : Boxes)
			{
				if (Box && Box->GetFName() == AmbientConfig->WaterComponentName)
				{
					Water = Box->Bounds.GetBox();
					break;
				}
			}
		}
	}
	if (!Water.IsValid)
	{
		UE_LOG(LogFIH2Menu, Warning, TEXT("Fond du menu : aucun volume d'eau « %s » trouvé, pas de peuplement."),
			*AmbientConfig->WaterComponentName.ToString());
		return;
	}

	const FVector Size = Water.GetSize();
	const float Margin = FMath::Clamp(AmbientConfig->HorizontalMargin, 0.f, 0.45f);
	const float HeightMin = FMath::Clamp(FMath::Min(AmbientConfig->HeightRange.X, AmbientConfig->HeightRange.Y), 0.f, 1.f);
	const float HeightMax = FMath::Clamp(FMath::Max(AmbientConfig->HeightRange.X, AmbientConfig->HeightRange.Y), 0.f, 1.f);

	FRandomStream Random;
	if (AmbientConfig->RandomSeed != 0)
	{
		Random.Initialize(AmbientConfig->RandomSeed);
	}
	else
	{
		Random.GenerateNewSeed();
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	int32 Spawned = 0;
	for (const FMenuPopulationEntry& Entry : AmbientConfig->Population)
	{
		if (!Entry.ActorClass)
		{
			continue;
		}
		const float ScaleMin = FMath::Min(Entry.ScaleRange.X, Entry.ScaleRange.Y);
		const float ScaleMax = FMath::Max(Entry.ScaleRange.X, Entry.ScaleRange.Y);

		for (int32 Index = 0; Index < Entry.Count; ++Index)
		{
			const FVector Location(
				Water.Min.X + Size.X * Random.FRandRange(Margin, 1.f - Margin),
				Water.Min.Y + Size.Y * Random.FRandRange(Margin, 1.f - Margin),
				Water.Min.Z + Size.Z * Random.FRandRange(HeightMin, HeightMax));
			const FRotator Rotation(0.f, Random.FRandRange(0.f, 360.f), 0.f);

			AActor* Actor = World->SpawnActor<AActor>(Entry.ActorClass, Location, Rotation, Params);
			if (!Actor)
			{
				continue;
			}
			Actor->SetActorScale3D(FVector(Random.FRandRange(ScaleMin, ScaleMax)));
			if (APawn* Pawn = Cast<APawn>(Actor))
			{
				if (!Pawn->GetController())
				{
					Pawn->SpawnDefaultController(); // l'IA existante de l'espèce prend le relais
				}
			}
			++Spawned;
		}
	}
	UE_LOG(LogFIH2Menu, Log, TEXT("Fond du menu : %d acteurs vivants apparus."), Spawned);
}

// ------------------------------------------------------------------ PlayerController

AMainMenuPlayerController::AMainMenuPlayerController()
{
	bShowMouseCursor = true;
}

void AMainMenuPlayerController::BeginPlay()
{
	Super::BeginPlay();
	if (!IsLocalController())
	{
		return;
	}

	// Vue : la caméra du menu placée dans la carte.
	for (TActorIterator<AMenuCameraRig> It(GetWorld()); It; ++It)
	{
		SetViewTarget(*It);
		break;
	}

	bShowMouseCursor = true;
	const TSubclassOf<UMainMenuRootWidget> WidgetClass = MenuWidgetClass
		? MenuWidgetClass : TSubclassOf<UMainMenuRootWidget>(UMainMenuRootWidget::StaticClass());
	MenuWidget = CreateWidget<UMainMenuRootWidget>(this, WidgetClass);
	if (MenuWidget)
	{
		MenuWidget->AddToViewport();
	}

	// État du viewport avant le menu, pour le rendre tel quel au niveau lancé (voir RestoreGameInput).
	if (const ULocalPlayer* LocalPlayer = GetLocalPlayer())
	{
		if (UGameViewportClient* Viewport = LocalPlayer->ViewportClient.Get())
		{
			bPreviousIgnoreInput = Viewport->IgnoreInput();
			PreviousCaptureMode = Viewport->GetMouseCaptureMode();
			PreviousLockMode = Viewport->GetMouseLockMode();
			bInputModeOverridden = true;
		}
	}

	// Entrées vers l'UI uniquement : aucune action de jeu ne peut partir depuis le menu.
	FInputModeUIOnly InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(InputMode);
}

void AMainMenuPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	RestoreGameInput();
	Super::EndPlay(EndPlayReason);
}

void AMainMenuPlayerController::RestoreGameInput()
{
	if (!bInputModeOverridden)
	{
		return;
	}
	bInputModeOverridden = false;

	const ULocalPlayer* LocalPlayer = GetLocalPlayer();
	UGameViewportClient* Viewport = LocalPlayer ? LocalPlayer->ViewportClient.Get() : nullptr;
	if (!Viewport)
	{
		return;
	}
	Viewport->SetIgnoreInput(bPreviousIgnoreInput);
	Viewport->SetMouseCaptureMode(PreviousCaptureMode);
	Viewport->SetMouseLockMode(PreviousLockMode);
	// Le focus clavier était sur un bouton du menu : il revient au viewport de jeu.
	FSlateApplication::Get().SetAllUserFocusToGameViewport();
}
