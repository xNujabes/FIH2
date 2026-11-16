#include "System/TutorialSubsystem.h"

#include "Bonus/Food.h"
#include "Components/MeshComponent.h"
#include "EnhancedInputComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Fish/Fish.h"
#include "Fish/FishEgg.h"
#include "InputAction.h"
#include "Materials/MaterialInterface.h"
#include "System/FIH2ProgressionSubsystem.h"
#include "System/GameplayEvents.h"
#include "System/LevelConfig.h"
#include "System/MouseController.h"
#include "System/TutorialPanelWidget.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogTutorial, Log, All);

UTutorialSubsystem* UTutorialSubsystem::Get(const UObject* WorldContext)
{
	const UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::ReturnNull) : nullptr;
	return World ? World->GetSubsystem<UTutorialSubsystem>() : nullptr;
}

bool UTutorialSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

void UTutorialSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	// L'ordre des OnWorldBeginPlay entre sous-systèmes n'est pas garanti, et le contrôleur du joueur doit avoir
	// son InputComponent : on démarre au tick suivant, quand l'écosystème a chargé sa config.
	InWorld.GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this, [this]()
	{
		UEcosystemSubsystem* Eco = UEcosystemSubsystem::Get(this);
		const ULevelConfig* Config = Eco ? Eco->GetLevelConfig() : nullptr;
		if (Config && Config->TutorialSequence && Config->TutorialSequence->bStartAutomatically)
		{
			StartTutorial(Config->TutorialSequence);
		}
	}));
}

void UTutorialSubsystem::Deinitialize()
{
	if (IsRunning())
	{
		LeaveCurrentStep();
	}
	FGameplayEvents::OnActorSelected().Remove(SelectedHandle);
	FGameplayEvents::OnFocusUsed().Remove(FocusHandle);
	FGameplayEvents::OnActorReleased().Remove(ReleasedHandle);
	FGameplayEvents::OnFishAte().Remove(FishAteHandle);
	FGameplayEvents::OnEggLaid().Remove(EggLaidHandle);
	FGameplayEvents::OnEggHatched().Remove(EggHatchedHandle);
	FGameplayEvents::OnEggDied().Remove(EggDiedHandle);
	Super::Deinitialize();
}

// ------------------------------------------------------------------ Déroulé

void UTutorialSubsystem::StartTutorial(UTutorialSequence* InSequence)
{
	UWorld* World = GetWorld();
	if (!World || !InSequence || InSequence->Steps.Num() == 0 || IsRunning())
	{
		return;
	}
	Sequence = InSequence;
	bWasSkipped = false;

	// Avant l'étape de maintien : ni défaite, ni mort par la faim (décision du 13/11).
	Ecosystem = UEcosystemSubsystem::Get(this);
	if (UEcosystemSubsystem* Eco = Ecosystem.Get())
	{
		Eco->SetDefeatEnabled(false);
		Eco->SetStarvationEnabled(false);
		Eco->OnUpdated.AddUniqueDynamic(this, &UTutorialSubsystem::HandleEcosystemUpdated);
		Eco->OnStateChanged.AddUniqueDynamic(this, &UTutorialSubsystem::HandleEcosystemStateChanged);
	}

	SelectedHandle = FGameplayEvents::OnActorSelected().AddUObject(this, &UTutorialSubsystem::HandleActorSelected);
	FocusHandle = FGameplayEvents::OnFocusUsed().AddUObject(this, &UTutorialSubsystem::HandleFocusUsed);
	ReleasedHandle = FGameplayEvents::OnActorReleased().AddUObject(this, &UTutorialSubsystem::HandleActorReleased);
	FishAteHandle = FGameplayEvents::OnFishAte().AddUObject(this, &UTutorialSubsystem::HandleFishAte);
	EggLaidHandle = FGameplayEvents::OnEggLaid().AddUObject(this, &UTutorialSubsystem::HandleEggLaid);
	EggHatchedHandle = FGameplayEvents::OnEggHatched().AddUObject(this, &UTutorialSubsystem::HandleEggHatched);
	EggDiedHandle = FGameplayEvents::OnEggDied().AddUObject(this, &UTutorialSubsystem::HandleEggDied);

	if (APlayerController* PC = World->GetFirstPlayerController())
	{
		const TSubclassOf<UTutorialPanelWidget> PanelClass = Sequence->PanelWidgetClass
			? Sequence->PanelWidgetClass : TSubclassOf<UTutorialPanelWidget>(UTutorialPanelWidget::StaticClass());
		Panel = CreateWidget<UTutorialPanelWidget>(PC, PanelClass);
		if (Panel)
		{
			Panel->AddToViewport(50);
		}
	}

	UE_LOG(LogTutorial, Log, TEXT("Tutoriel démarré (%d étapes)."), Sequence->Steps.Num());
	EnterStep(0);
}

const FTutorialStep* UTutorialSubsystem::CurrentStep() const
{
	return (Sequence && Sequence->Steps.IsValidIndex(CurrentIndex)) ? &Sequence->Steps[CurrentIndex] : nullptr;
}

void UTutorialSubsystem::EnterStep(int32 Index)
{
	UWorld* World = GetWorld();
	if (!World || !Sequence || !Sequence->Steps.IsValidIndex(Index))
	{
		return;
	}

	LeaveCurrentStep();
	CurrentIndex = Index;
	StepEnterTime = World->GetTimeSeconds();
	bStepCompleting = false;
	PlayerReleasedFood.Reset();

	const FTutorialStep& Step = Sequence->Steps[Index];
	UE_LOG(LogTutorial, Log, TEXT("Étape %d : %s"), Index, *UEnum::GetValueAsString(Step.Condition));

	ApplyHighlight(Step.HighlightTag);

	if (!Step.CameraFocusTag.IsNone())
	{
		if (AMouseController* PC = Cast<AMouseController>(World->GetFirstPlayerController()))
		{
			PC->FocusOnActors(GetActorsWithTag(Step.CameraFocusTag));
		}
	}

	if (Step.bFocusEventActor && LastEventActor.IsValid())
	{
		TArray<AActor*> EventActors = { LastEventActor.Get() };
		if (AMouseController* PC = Cast<AMouseController>(World->GetFirstPlayerController()))
		{
			PC->FocusOnActors(EventActors);
		}
		HighlightActors(EventActors);
	}

	if (Step.ShortenReproTimersTo > 0.f)
	{
		if (UEcosystemSubsystem* Eco = Ecosystem.Get())
		{
			Eco->ShortenReproTimers(Step.ShortenReproTimersTo);
		}
	}

	if (Step.SetFishHungerOnEnter >= 0)
	{
		for (TActorIterator<AFish> It(World); It; ++It)
		{
			It->SetHunger(Step.SetFishHungerOnEnter);
		}
	}

	FTimerManager& Timers = World->GetTimerManager();
	switch (Step.Condition)
	{
	case ETutorialCondition::Delay:
		Timers.SetTimer(StepTimer, FTimerDelegate::CreateUObject(this, &UTutorialSubsystem::CompleteCurrentStep),
			FMath::Max(0.01f, Step.Duration), false);
		break;
	case ETutorialCondition::InputAction:
		BindInputActions(Step);
		break;
	default:
		break;
	}

	if (Step.Timeout > 0.f && Step.Condition != ETutorialCondition::Maintain)
	{
		Timers.SetTimer(TimeoutTimer, FTimerDelegate::CreateUObject(this, &UTutorialSubsystem::CompleteCurrentStep),
			Step.Timeout, false);
	}

	if (Panel)
	{
		Panel->ShowStep(Step.Message, Index, Sequence->Steps.Num(),
			Step.Condition == ETutorialCondition::NextButton, Step.Condition == ETutorialCondition::Maintain);
	}
	OnStepChanged.Broadcast(Index);

	if (Step.Condition == ETutorialCondition::Maintain)
	{
		EnterMaintain();
	}
	else
	{
		EvaluateWorldConditions(); // condition peut-être déjà remplie
	}
}

void UTutorialSubsystem::LeaveCurrentStep()
{
	if (UWorld* World = GetWorld())
	{
		FTimerManager& Timers = World->GetTimerManager();
		Timers.ClearTimer(StepTimer);
		Timers.ClearTimer(TimeoutTimer);
		Timers.ClearTimer(CompleteTimer);
	}
	UnbindInputActions();
	RemoveHighlight();
}

void UTutorialSubsystem::CompleteCurrentStep()
{
	const FTutorialStep* Step = CurrentStep();
	UWorld* World = GetWorld();
	if (!Step || !World || bStepCompleting)
	{
		return;
	}

	// Le message reste affiché au moins MinDisplayTime, même si le joueur a déjà fait l'action.
	const float Remaining = Step->MinDisplayTime - static_cast<float>(World->GetTimeSeconds() - StepEnterTime);
	bStepCompleting = true;
	if (Remaining > 0.f)
	{
		World->GetTimerManager().SetTimer(CompleteTimer, FTimerDelegate::CreateUObject(this, &UTutorialSubsystem::AdvanceStep),
			Remaining, false);
	}
	else
	{
		AdvanceStep();
	}
}

void UTutorialSubsystem::AdvanceStep()
{
	if (!Sequence)
	{
		return;
	}
	if (Sequence->Steps.IsValidIndex(CurrentIndex + 1))
	{
		EnterStep(CurrentIndex + 1);
	}
	else
	{
		FinishTutorial(false); // séquence sans étape de maintien : fin du guidage, la partie continue
	}
}

void UTutorialSubsystem::Next()
{
	const FTutorialStep* Step = CurrentStep();
	if (Step && Step->Condition == ETutorialCondition::NextButton)
	{
		CompleteCurrentStep();
	}
}

void UTutorialSubsystem::Skip()
{
	const FTutorialStep* Step = CurrentStep();
	if (!Step || Step->Condition == ETutorialCondition::Maintain)
	{
		return; // pendant le maintien, « passer » finirait le tutoriel sans victoire possible (partie sans fin)
	}
	bWasSkipped = true; // point d'accroche de la progression : un tutoriel passé ne compte pas comme complété
	for (int32 i = CurrentIndex + 1; i < Sequence->Steps.Num(); ++i)
	{
		if (Sequence->Steps[i].Condition == ETutorialCondition::Maintain)
		{
			EnterStep(i);
			return;
		}
	}
	FinishTutorial(false);
}

void UTutorialSubsystem::FinishTutorial(bool bWon)
{
	if (!IsRunning())
	{
		return;
	}
	LeaveCurrentStep();
	CurrentIndex = INDEX_NONE;

	FGameplayEvents::OnActorSelected().Remove(SelectedHandle);
	FGameplayEvents::OnFocusUsed().Remove(FocusHandle);
	FGameplayEvents::OnActorReleased().Remove(ReleasedHandle);
	FGameplayEvents::OnFishAte().Remove(FishAteHandle);
	FGameplayEvents::OnEggLaid().Remove(EggLaidHandle);
	FGameplayEvents::OnEggHatched().Remove(EggHatchedHandle);
	FGameplayEvents::OnEggDied().Remove(EggDiedHandle);

	if (UEcosystemSubsystem* Eco = Ecosystem.Get())
	{
		Eco->OnUpdated.RemoveDynamic(this, &UTutorialSubsystem::HandleEcosystemUpdated);
		Eco->OnStateChanged.RemoveDynamic(this, &UTutorialSubsystem::HandleEcosystemStateChanged);
		Eco->SetDefeatEnabled(true);
		Eco->SetStarvationEnabled(true);
	}
	if (Panel)
	{
		Panel->RemoveFromParent();
		Panel = nullptr;
	}

	UE_LOG(LogTutorial, Log, TEXT("Tutoriel terminé (%s)."), bWon ? TEXT("victoire") : TEXT("sans victoire"));
	// Point d'accroche de la progression (sauvegarde) : aucune logique de tutoriel ici.
	if (UFIH2ProgressionSubsystem* Progression = UFIH2ProgressionSubsystem::Get(this))
	{
		Progression->NotifyTutorialFinished(this, bWon, bWasSkipped);
	}
	OnTutorialFinished.Broadcast(bWon);
	Sequence = nullptr;
}

// ------------------------------------------------------------------ Conditions

void UTutorialSubsystem::EvaluateWorldConditions()
{
	const FTutorialStep* Step = CurrentStep();
	if (!Step)
	{
		return;
	}
	if (Step->Condition == ETutorialCondition::GaugeZoneReached)
	{
		const UEcosystemSubsystem* Eco = Ecosystem.Get();
		if (Eco && IsGaugeConditionMet(Eco->GetSnapshot()))
		{
			CompleteCurrentStep();
		}
	}
	else if (Step->Condition == ETutorialCondition::TaggedActorsNear && AreTaggedActorsNear())
	{
		CompleteCurrentStep();
	}
}

bool UTutorialSubsystem::IsGaugeConditionMet(const FEcosystemSnapshot& Snapshot) const
{
	const FTutorialStep* Step = CurrentStep();
	const UEcosystemSubsystem* Eco = Ecosystem.Get();
	const ULevelConfig* Config = Eco ? Eco->GetLevelConfig() : nullptr;
	if (!Step || !Config)
	{
		return false;
	}

	EEcosystemZone Zone = EEcosystemZone::Green;
	switch (Step->GaugeVariable)
	{
	case ETutorialGaugeVariable::Pollution: Zone = Snapshot.ZonePollution; break;
	case ETutorialGaugeVariable::Oxygen: Zone = Snapshot.ZoneOxygen; break;
	case ETutorialGaugeVariable::Hunger: Zone = Snapshot.ZoneHunger; break;
	case ETutorialGaugeVariable::Global:
		// Même lecture que le HUD : rouge à partir du seuil de défaite.
		Zone = Snapshot.Gauge >= Config->DefeatGaugeThreshold ? EEcosystemZone::Red
			: (Snapshot.Gauge >= Config->YellowThreshold ? EEcosystemZone::Yellow : EEcosystemZone::Green);
		break;
	}
	return static_cast<uint8>(Zone) >= static_cast<uint8>(Step->Zone);
}

bool UTutorialSubsystem::AreTaggedActorsNear() const
{
	const FTutorialStep* Step = CurrentStep();
	UWorld* World = GetWorld();
	if (!Step || !World || Step->TargetTag.IsNone())
	{
		return false;
	}

	TArray<const AActor*> Targets;
	TArray<const AActor*> References;
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		const AActor* Actor = *It;
		if (Actor->ActorHasTag(Step->TargetTag))
		{
			Targets.Add(Actor);
		}
		else if (Step->ReferenceTag.IsNone() ? Actor->IsA<AFish>() : Actor->ActorHasTag(Step->ReferenceTag))
		{
			References.Add(Actor);
		}
	}
	if (Targets.Num() == 0)
	{
		return true; // rien à rapprocher (cibles détruites) : ne pas bloquer le joueur
	}
	if (References.Num() == 0)
	{
		return false;
	}

	for (const AActor* Target : Targets)
	{
		const bool bNear = References.ContainsByPredicate([Target, Step](const AActor* Reference)
		{
			return FVector::Dist2D(Target->GetActorLocation(), Reference->GetActorLocation()) <= Step->NearRadius;
		});
		if (!bNear)
		{
			return false;
		}
	}
	return true;
}

// ------------------------------------------------------------------ Maintien

void UTutorialSubsystem::EnterMaintain()
{
	const FTutorialStep* Step = CurrentStep();
	UEcosystemSubsystem* Eco = Ecosystem.Get();
	if (!Step || !Eco || !Eco->GetLevelConfig())
	{
		return;
	}

	Eco->SetDefeatEnabled(true);
	Eco->SetStarvationEnabled(true);

	// QA : un poisson à 0/3 mourrait dès le prochain retrait de faim ; on lui laisse une marge.
	for (TActorIterator<AFish> It(GetWorld()); It; ++It)
	{
		if (It->GetHunger() < Sequence->MinHungerOnMaintain)
		{
			It->SetHunger(Sequence->MinHungerOnMaintain);
		}
	}

	const FEcosystemSnapshot& Snapshot = Eco->GetSnapshot();
	MaintainRemaining = Step->Duration;
	LastSnapshotTime = Snapshot.ElapsedTime;
	// Déjà au rouge en entrant : le compte à rebours attend, sans message de remise à zéro.
	bMaintainWaiting = Snapshot.Gauge >= Eco->GetLevelConfig()->DefeatGaugeThreshold;

	if (Panel)
	{
		Panel->ShowTimer(MaintainRemaining, bMaintainWaiting);
	}
	OnMaintainUpdated.Broadcast(MaintainRemaining, bMaintainWaiting);
}

void UTutorialSubsystem::UpdateMaintain(const FEcosystemSnapshot& Snapshot)
{
	const FTutorialStep* Step = CurrentStep();
	UEcosystemSubsystem* Eco = Ecosystem.Get();
	if (!Step || !Eco || !Eco->GetLevelConfig() || Snapshot.State != EEcosystemState::Playing)
	{
		return;
	}

	const float Dt = FMath::Max(0.f, Snapshot.ElapsedTime - LastSnapshotTime);
	LastSnapshotTime = Snapshot.ElapsedTime;

	const bool bRed = Snapshot.Gauge >= Eco->GetLevelConfig()->DefeatGaugeThreshold;
	if (bRed)
	{
		// Le compte à rebours revient à 1:00 et ne repart qu'une fois la jauge sortie du rouge.
		if (!bMaintainWaiting)
		{
			bMaintainWaiting = true;
			if (Panel)
			{
				Panel->ShowResetMessage(Sequence->MaintainResetMessage, Sequence->MaintainResetMessageDuration);
			}
			OnMaintainReset.Broadcast();
		}
		MaintainRemaining = Step->Duration;
	}
	else
	{
		bMaintainWaiting = false;
		MaintainRemaining -= Dt;
	}

	if (Panel)
	{
		Panel->ShowTimer(MaintainRemaining, bMaintainWaiting);
	}
	OnMaintainUpdated.Broadcast(MaintainRemaining, bMaintainWaiting);

	if (MaintainRemaining <= 0.f)
	{
		Eco->FinishLevel(true); // déclenche OnStateChanged(Won) → FinishTutorial
		FinishTutorial(true);
	}
}

// ------------------------------------------------------------------ Surbrillance

void UTutorialSubsystem::ApplyHighlight(FName Tag)
{
	if (!Tag.IsNone())
	{
		HighlightActors(GetActorsWithTag(Tag));
	}
}

void UTutorialSubsystem::HighlightActors(const TArray<AActor*>& Actors)
{
	UWorld* World = GetWorld();
	if (!World || !Sequence)
	{
		return;
	}
	// La surbrillance passe par le contrôleur, qui la concilie avec celle de la sélection (sinon un kelp attrapé
	// pendant l'étape restait surligné après elle : test du 13/11).
	AMouseController* PC = Cast<AMouseController>(World->GetFirstPlayerController());
	UMaterialInterface* Material = Sequence->HighlightMaterial ? Sequence->HighlightMaterial.Get()
		: (PC ? PC->SelectionOverlayMaterial.Get() : nullptr);
	if (!PC || !Material)
	{
		return;
	}
	for (AActor* Actor : Actors)
	{
		if (Actor)
		{
			PC->SetPersistentHighlight(Actor, Material);
			HighlightedActors.Add(Actor);
		}
	}
}

void UTutorialSubsystem::RemoveHighlight()
{
	UWorld* World = GetWorld();
	if (AMouseController* PC = World ? Cast<AMouseController>(World->GetFirstPlayerController()) : nullptr)
	{
		for (const TWeakObjectPtr<AActor>& Actor : HighlightedActors)
		{
			PC->SetPersistentHighlight(Actor.Get(), nullptr);
		}
	}
	HighlightedActors.Reset();
}

TArray<AActor*> UTutorialSubsystem::GetActorsWithTag(FName Tag) const
{
	TArray<AActor*> Result;
	if (UWorld* World = GetWorld(); World && !Tag.IsNone())
	{
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			if (It->ActorHasTag(Tag))
			{
				Result.Add(*It);
			}
		}
	}
	return Result;
}

// ------------------------------------------------------------------ Entrées

void UTutorialSubsystem::BindInputActions(const FTutorialStep& Step)
{
	UWorld* World = GetWorld();
	APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
	UEnhancedInputComponent* Input = PC ? Cast<UEnhancedInputComponent>(PC->InputComponent) : nullptr;
	if (!Input)
	{
		UE_LOG(LogTutorial, Warning, TEXT("Pas d'EnhancedInputComponent sur le contrôleur : l'étape %d passera par son timeout."), CurrentIndex);
		return;
	}
	for (const TSoftObjectPtr<UInputAction>& SoftAction : Step.InputActions)
	{
		if (const UInputAction* Action = SoftAction.LoadSynchronous())
		{
			InputBindingHandles.Add(Input->BindAction(Action, ETriggerEvent::Triggered, this, &UTutorialSubsystem::HandleInputAction).GetHandle());
		}
	}
}

void UTutorialSubsystem::UnbindInputActions()
{
	if (InputBindingHandles.Num() == 0)
	{
		return;
	}
	UWorld* World = GetWorld();
	APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
	if (UEnhancedInputComponent* Input = PC ? Cast<UEnhancedInputComponent>(PC->InputComponent) : nullptr)
	{
		for (const uint32 Handle : InputBindingHandles)
		{
			Input->RemoveBindingByHandle(Handle);
		}
	}
	InputBindingHandles.Reset();
}

void UTutorialSubsystem::HandleInputAction(const FInputActionInstance& Instance)
{
	const FTutorialStep* Step = CurrentStep();
	if (Step && Step->Condition == ETutorialCondition::InputAction)
	{
		CompleteCurrentStep();
	}
}

// ------------------------------------------------------------------ Évènements de jeu

void UTutorialSubsystem::HandleActorSelected(AActor* Actor)
{
	const FTutorialStep* Step = CurrentStep();
	if (!Step || !Actor)
	{
		return;
	}
	if (Step->Condition == ETutorialCondition::SelectActorOfClass && (!Step->TargetClass || Actor->IsA(Step->TargetClass)))
	{
		CompleteCurrentStep();
	}
	else if (Step->Condition == ETutorialCondition::SelectFry)
	{
		// L'alevin qui vient de naître compte même s'il est déjà devenu adulte (le joueur a pu tarder).
		const AFish* Fish = Cast<AFish>(Actor);
		if (Fish && (Actor == LastEventActor.Get() || !Fish->IsAdult()))
		{
			CompleteCurrentStep();
		}
	}
}

void UTutorialSubsystem::HandleEggLaid(AFishEgg* Egg)
{
	const FTutorialStep* Step = CurrentStep();
	if (Step && Egg && Step->Condition == ETutorialCondition::EggLaid)
	{
		LastEventActor = Egg; // l'étape suivante (incubation) suit cet œuf
		CompleteCurrentStep();
	}
}

void UTutorialSubsystem::HandleEggHatched(AFishEgg* Egg, AFish* Fry)
{
	const FTutorialStep* Step = CurrentStep();
	if (Step && Step->Condition == ETutorialCondition::EggHatched)
	{
		LastEventActor = Fry; // l'étape suivante (alevin) suit cet alevin
		CompleteCurrentStep();
	}
}

void UTutorialSubsystem::HandleEggDied(AFishEgg* Egg)
{
	const FTutorialStep* Step = CurrentStep();
	if (!Step || !Egg || Step->Condition != ETutorialCondition::EggHatched || Egg != LastEventActor.Get())
	{
		return;
	}
	// Décision du 13/11 : l'œuf suivi meurt → retour à l'étape de ponte précédente, avec un message.
	for (int32 i = CurrentIndex - 1; i >= 0; --i)
	{
		if (Sequence->Steps[i].Condition == ETutorialCondition::EggLaid)
		{
			LastEventActor.Reset();
			EnterStep(i);
			if (Panel)
			{
				Panel->ShowResetMessage(Sequence->EggLostMessage, 4.f);
			}
			return;
		}
	}
}

void UTutorialSubsystem::HandleFocusUsed(AActor* Actor)
{
	const FTutorialStep* Step = CurrentStep();
	if (Step && Step->Condition == ETutorialCondition::FocusUsed)
	{
		CompleteCurrentStep();
	}
}

void UTutorialSubsystem::HandleActorReleased(AActor* Actor)
{
	if (Actor && Actor->IsA<AFood>())
	{
		PlayerReleasedFood.Add(Actor);
	}
}

void UTutorialSubsystem::HandleFishAte(AFish* Fish, AActor* Food)
{
	const FTutorialStep* Step = CurrentStep();
	if (!Step || Step->Condition != ETutorialCondition::FishAte)
	{
		return;
	}
	// Un poisson mange un fruit dès qu'il le touche, souvent pendant que le joueur le tient encore :
	// le fruit tenu compte comme nourriture du joueur (sinon le lâcher arrive après le repas : test du 13/11).
	const UWorld* World = GetWorld();
	const AMouseController* PC = World ? Cast<AMouseController>(World->GetFirstPlayerController()) : nullptr;
	const bool bHeldByPlayer = PC && Food && PC->GrabbedActor == Food;
	if (!Step->bRequirePlayerFood || bHeldByPlayer || PlayerReleasedFood.Contains(Food))
	{
		CompleteCurrentStep();
	}
}

void UTutorialSubsystem::HandleEcosystemUpdated(const FEcosystemSnapshot& Snapshot)
{
	const FTutorialStep* Step = CurrentStep();
	if (!Step)
	{
		return;
	}
	if (Step->Condition == ETutorialCondition::Maintain)
	{
		UpdateMaintain(Snapshot);
	}
	else
	{
		// Un pas de simulation (SIM_TICK = 0,5 s) : la jauge ou la position des kelps ont pu changer.
		EvaluateWorldConditions();
	}
}

void UTutorialSubsystem::HandleEcosystemStateChanged(EEcosystemState NewState)
{
	if (NewState == EEcosystemState::Lost)
	{
		FinishTutorial(false);
	}
	else if (NewState == EEcosystemState::Won)
	{
		FinishTutorial(true);
	}
}
