#include "System/EcosystemSubsystem.h"

#include "Bonus/Food.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Fish/Fish.h"
#include "Fish/FishEgg.h"
#include "GameFramework/PlayerController.h"
#include "Fish/PredatorFish.h"
#include "Inanimate/Kelp.h"
#include "PhysicsEngine/PhysicsHandleComponent.h"
#include "Stats/Stats.h"
#include "System/LevelConfig.h"

DEFINE_LOG_CATEGORY_STATIC(LogEcosystem, Log, All);

const TCHAR* UEcosystemSubsystem::DefaultConfigPath = TEXT("/Game/Data/DA_LevelConfig_Tutorial.DA_LevelConfig_Tutorial");

UEcosystemSubsystem* UEcosystemSubsystem::Get(const UObject* WorldContext)
{
	const UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::ReturnNull) : nullptr;
	return World ? World->GetSubsystem<UEcosystemSubsystem>() : nullptr;
}

bool UEcosystemSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	// Uniquement en jeu (ou PIE) : pas dans le monde de l'éditeur.
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

void UEcosystemSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UEcosystemSubsystem::Deinitialize()
{
	if (UWorld* World = GetWorld())
	{
		if (ActorSpawnedHandle.IsValid())
		{
			World->RemoveOnActorSpawnedHandler(ActorSpawnedHandle);
			ActorSpawnedHandle.Reset();
		}
	}
	Fish.Empty();
	Kelps.Empty();
	Foods.Empty();
	Eggs.Empty();
	Super::Deinitialize();
}

TStatId UEcosystemSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UEcosystemSubsystem, STATGROUP_Tickables);
}

void UEcosystemSubsystem::EnsureConfig()
{
	if (Config)
	{
		return;
	}

	// Data Asset du niveau s'il existe, sinon valeurs par défaut du code (déjà calibrées pour le niveau 1).
	Config = LoadObject<ULevelConfig>(nullptr, DefaultConfigPath, nullptr, LOAD_NoWarn);
	if (!Config)
	{
		Config = NewObject<ULevelConfig>(this);
		UE_LOG(LogEcosystem, Log, TEXT("Aucun Data Asset %s : valeurs par défaut du code utilisées."), DefaultConfigPath);
	}
}

void UEcosystemSubsystem::SetLevelConfig(ULevelConfig* NewConfig)
{
	Config = NewConfig;
	EnsureConfig();
	if (!bLevelStarted)
	{
		Oxygen = Config->StartOxygen;
	}
}

void UEcosystemSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	EnsureConfig();
	ResetEcosystem();

	// Découverte unique des acteurs déjà présents dans le niveau, puis suivi des nouveaux.
	for (TActorIterator<AFish> It(&InWorld); It; ++It) { TrackActor(*It); }
	for (TActorIterator<AKelp> It(&InWorld); It; ++It) { TrackActor(*It); }
	for (TActorIterator<AFood> It(&InWorld); It; ++It) { TrackActor(*It); }
	for (TActorIterator<AFishEgg> It(&InWorld); It; ++It) { TrackActor(*It); }

	ActorSpawnedHandle = InWorld.AddOnActorSpawnedHandler(
		FOnActorSpawned::FDelegate::CreateUObject(this, &UEcosystemSubsystem::HandleActorSpawned));

	UE_LOG(LogEcosystem, Log, TEXT("Écosystème prêt : %d poissons, %d kelps, %d fruits, %d œufs suivis."), Fish.Num(), Kelps.Num(), Foods.Num(), Eggs.Num());
}

void UEcosystemSubsystem::ResetEcosystem()
{
	EnsureConfig();
	State = EEcosystemState::Playing;
	EndReason = EEcosystemEndReason::None;
	Pollution = 0.f;
	Oxygen = Config->StartOxygen;
	Elapsed = 0.f;
	DefeatTimer = 0.f;
	ExtinctionTimer = 0.f;
	Accumulator = 0.f;
	CorpsePulses.Reset();
	EggsLost = 0;
	bLevelStarted = false;
}

void UEcosystemSubsystem::SetDefeatEnabled(bool bEnabled)
{
	bDefeatEnabled = bEnabled;
	if (!bEnabled)
	{
		DefeatTimer = 0.f;
		ExtinctionTimer = 0.f;
	}
}

void UEcosystemSubsystem::SetStarvationEnabled(bool bEnabled)
{
	bStarvationEnabled = bEnabled;
}

void UEcosystemSubsystem::FinishLevel(bool bWon)
{
	if (State != EEcosystemState::Playing)
	{
		return;
	}
	SetState(bWon ? EEcosystemState::Won : EEcosystemState::Lost,
		bWon ? EEcosystemEndReason::None : EEcosystemEndReason::ObjectiveMissed);
}

// ------------------------------------------------------------------ Suivi des acteurs

void UEcosystemSubsystem::TrackActor(AActor* Actor)
{
	if (!Actor || !Config)
	{
		return;
	}

	if (AFish* FishActor = Cast<AFish>(Actor))
	{
		if (FishActor->IsA<APredatorFish>())
		{
			return; // Prédateurs : hors périmètre du niveau 1 (ni comptés, ni reproduits).
		}

		const TWeakObjectPtr<AFish> Key(FishActor);
		if (Fish.Contains(Key))
		{
			return;
		}

		FFishRecord Record;
		Record.BornTime = GetWorld()->GetTimeSeconds();
		if (bSpawningFry)
		{
			Record.bAdult = false;
			Record.AdultScale = PendingFryAdultScale;
		}
		else
		{
			// Poisson placé dans le niveau ou posé par le joueur : adulte, avec un timer décalé pour éviter les naissances groupées.
			Record.bAdult = true;
			Record.AdultScale = FishActor->GetActorScale3D();
			Record.ReproTimer = FMath::FRandRange(0.f, FishActor->Reproduction.ReproCooldown);
		}
		Fish.Add(Key, Record);
		FishActor->OnEndPlay.AddUniqueDynamic(this, &UEcosystemSubsystem::HandleActorEndPlay);
	}
	else if (AKelp* KelpActor = Cast<AKelp>(Actor))
	{
		Kelps.AddUnique(TWeakObjectPtr<AKelp>(KelpActor));
		KelpActor->OnEndPlay.AddUniqueDynamic(this, &UEcosystemSubsystem::HandleActorEndPlay);
	}
	else if (AFood* FoodActor = Cast<AFood>(Actor))
	{
		Foods.AddUnique(TWeakObjectPtr<AFood>(FoodActor));
		FoodActor->OnEndPlay.AddUniqueDynamic(this, &UEcosystemSubsystem::HandleActorEndPlay);
	}
	else if (AFishEgg* EggActor = Cast<AFishEgg>(Actor))
	{
		// Œuf placé à la main dans le niveau : incubation par défaut (la ponte appelle InitEgg ensuite, qui l'écrase).
		if (EggActor->IncubationDuration <= 0.f)
		{
			// Réglages de l'espèce si l'œuf en a une, sinon valeurs par défaut de la structure.
			const FFishReproductionSettings Species = EggActor->FishClass
				? EggActor->FishClass->GetDefaultObject<AFish>()->Reproduction : FFishReproductionSettings();
			EggActor->InitEgg(EggActor->FishClass, EggActor->AdultScale, Species.EggIncubationTime, Species.bEggDiesInRedPollution);
		}
		Eggs.AddUnique(TWeakObjectPtr<AFishEgg>(EggActor));
		EggActor->OnEndPlay.AddUniqueDynamic(this, &UEcosystemSubsystem::HandleActorEndPlay);
	}
}

void UEcosystemSubsystem::HandleActorSpawned(AActor* Actor)
{
	TrackActor(Actor);
}

void UEcosystemSubsystem::HandleActorEndPlay(AActor* Actor, EEndPlayReason::Type EndPlayReason)
{
	if (AFish* FishActor = Cast<AFish>(Actor))
	{
		const TWeakObjectPtr<AFish> Key(FishActor);
		if (Fish.Contains(Key))
		{
			// Un poisson détruit à Hunger 0 est mort de faim (BP_Fish) : il pollue un moment. Une simple suppression par le joueur, non.
			if (Config && bLevelStarted && EndPlayReason == EEndPlayReason::Destroyed
				&& FishActor->GetHunger() <= Config->StarvingHungerMax)
			{
				CorpsePulses.Add(Config->CorpsePollutionDuration);
			}
			Fish.Remove(Key);
		}
	}
	else if (AKelp* KelpActor = Cast<AKelp>(Actor))
	{
		Kelps.RemoveSingleSwap(TWeakObjectPtr<AKelp>(KelpActor));
	}
	else if (AFood* FoodActor = Cast<AFood>(Actor))
	{
		Foods.RemoveSingleSwap(TWeakObjectPtr<AFood>(FoodActor));
	}
	else if (AFishEgg* EggActor = Cast<AFishEgg>(Actor))
	{
		Eggs.RemoveSingleSwap(TWeakObjectPtr<AFishEgg>(EggActor));
	}
}

// ------------------------------------------------------------------ Boucle

void UEcosystemSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!Config || !GetWorld())
	{
		return;
	}

	if (!bLevelStarted)
	{
		if (Fish.Num() == 0)
		{
			return; // Pas de poissons : ce niveau n'utilise pas l'écosystème.
		}
		bLevelStarted = true;
	}

	if (State != EEcosystemState::Lost)
	{
		UpdateFryScales();

		const float Tick = FMath::Max(0.05f, Config->SimTick);
		Accumulator = FMath::Min(Accumulator + DeltaTime, 5.f);
		while (Accumulator >= Tick)
		{
			Accumulator -= Tick;
			Step(Tick);
			if (State == EEcosystemState::Lost)
			{
				break;
			}
		}
	}

	DrawDebug();
}

EEcosystemZone UEcosystemSubsystem::ZoneOf(float Stress) const
{
	if (Stress < Config->YellowThreshold) { return EEcosystemZone::Green; }
	if (Stress < Config->RedThreshold) { return EEcosystemZone::Yellow; }
	return EEcosystemZone::Red;
}

float UEcosystemSubsystem::CoefOf(float Stress) const
{
	switch (ZoneOf(Stress))
	{
	case EEcosystemZone::Green: return Config->CoefGreen;
	case EEcosystemZone::Yellow: return Config->CoefYellow;
	default: return Config->CoefRed;
	}
}

float UEcosystemSubsystem::NextReproCooldown(const FFishReproductionSettings& Settings)
{
	const float J = Settings.ReproCooldownJitter;
	return Settings.ReproCooldown * FMath::FRandRange(1.f - J, 1.f + J);
}

void UEcosystemSubsystem::Step(float Dt)
{
	const ULevelConfig& C = *Config;
	const double Now = GetWorld()->GetTimeSeconds();

	// 0. Purge des références invalides
	for (auto It = Fish.CreateIterator(); It; ++It)
	{
		if (!It.Key().IsValid()) { It.RemoveCurrent(); }
	}
	Kelps.RemoveAllSwap([](const TWeakObjectPtr<AKelp>& K) { return !K.IsValid(); });
	Foods.RemoveAllSwap([](const TWeakObjectPtr<AFood>& F) { return !F.IsValid(); });
	Eggs.RemoveAllSwap([](const TWeakObjectPtr<AFishEgg>& E) { return !E.IsValid(); });

	// 1. Comptages
	int32 Adults = 0, Fry = 0, Starving = 0, MatureKelp = 0, DecayingFood = 0;
	auto CountFish = [&]()
	{
		Adults = 0; Fry = 0; Starving = 0;
		for (const auto& Pair : Fish)
		{
			const AFish* F = Pair.Key.Get();
			if (!F) { continue; }
			(Pair.Value.bAdult ? Adults : Fry)++;
			if (F->GetHunger() <= C.StarvingHungerMax) { ++Starving; }
		}
	};
	CountFish();
	for (const TWeakObjectPtr<AKelp>& K : Kelps)
	{
		if (K.IsValid() && K->IsMature()) { ++MatureKelp; }
	}
	for (const TWeakObjectPtr<AFood>& F : Foods)
	{
		if (F.IsValid() && F->GetGrowthState() == EFoodGrowthState::Decaying) { ++DecayingFood; }
	}

	// 2. Pollution : fruits qui pourrissent + cadavres − épuration (base + kelps mûrs)
	for (float& T : CorpsePulses) { T -= Dt; }
	CorpsePulses.RemoveAll([](float T) { return T <= 0.f; });
	const float PollutionSource = DecayingFood * C.PollutionPerDecayingFood + CorpsePulses.Num() * C.PollutionPerCorpse;
	const float PollutionSink = C.BasePurification + MatureKelp * C.PurificationPerKelp;
	Pollution = FMath::Clamp(Pollution + (PollutionSource - PollutionSink) * Dt, 0.f, 100.f);

	// 3. Oxygène : production (base + kelps mûrs) − consommation des poissons − décomposition
	const float OxygenProduction = C.BaseOxygenProduction + MatureKelp * C.OxygenPerMatureKelp;
	const float OxygenConsumption = (Adults + Fry * C.FryOxygenFactor) * C.AdultOxygenConsumption
		+ C.OxygenConsumptionPerPollution * Pollution;
	Oxygen = FMath::Clamp(Oxygen + (OxygenProduction - OxygenConsumption) * Dt, 0.f, 100.f);

	// 4. Asphyxie : mortalité par poisson sous le seuil. Le poisson est retiré du suivi avant d'être détruit (le cadavre est ajouté ici).
	if (Oxygen < C.AsphyxiaThreshold)
	{
		TArray<AFish*> Victims;
		for (const auto& Pair : Fish)
		{
			AFish* F = Pair.Key.Get();
			if (F && FMath::FRand() < C.AsphyxiaDeathRate * Dt) { Victims.Add(F); }
		}
		for (AFish* Victim : Victims)
		{
			Fish.Remove(TWeakObjectPtr<AFish>(Victim));
			CorpsePulses.Add(C.CorpsePollutionDuration);
			Victim->Destroy();
		}
		if (Victims.Num() > 0) { CountFish(); }
	}

	// 4 bis. Œufs (D-09, D-10) : incubation en pause tant que le joueur tient l'œuf ; mort si la pollution est en zone rouge.
	{
		const AActor* HeldActor = GetHeldActor();
		const bool bPollutionRed = Pollution >= C.RedThreshold;
		TArray<AFishEgg*> ToKill;
		TArray<AFishEgg*> ToHatch;
		for (const TWeakObjectPtr<AFishEgg>& WeakEgg : Eggs)
		{
			AFishEgg* Egg = WeakEgg.Get();
			if (!Egg || Egg == HeldActor) { continue; }
			if (bPollutionRed && Egg->bDiesInRedPollution) { ToKill.Add(Egg); continue; }
			Egg->AdvanceIncubation(Dt);
			if (Egg->IncubationRemaining <= 0.f) { ToHatch.Add(Egg); }
		}
		for (AFishEgg* Egg : ToKill)
		{
			Eggs.RemoveSingleSwap(TWeakObjectPtr<AFishEgg>(Egg));
			Egg->Destroy();
			++EggsLost;
		}
		for (AFishEgg* Egg : ToHatch) { HatchEgg(Egg); }
		if (ToHatch.Num() > 0) { CountFish(); }
	}
	const int32 EggCount = Eggs.Num();

	// 5. Stress par variable, puis jauge globale = moyenne pondérée par zone (D-03)
	const int32 Total = Adults + Fry;
	const float StressPollution = Pollution;
	const float StressOxygen = 100.f - Oxygen;
	const float StressHunger = Total > 0 ? 100.f * static_cast<float>(Starving) / static_cast<float>(Total) : 0.f;
	const float CoefP = CoefOf(StressPollution);
	const float CoefO = CoefOf(StressOxygen);
	const float CoefH = CoefOf(StressHunger);
	const float Gauge = (CoefP * StressPollution + CoefO * StressOxygen + CoefH * StressHunger) / (CoefP + CoefO + CoefH);

	// 6. Maturation et reproduction (modulée par la jauge : un bac déréglé ne se reproduit plus)
	const float ReproFactor = FMath::Max(0.f, 1.f - Gauge / FMath::Max(1.f, C.DefeatGaugeThreshold));
	TArray<AFish*> Parents;
	for (auto& Pair : Fish)
	{
		AFish* F = Pair.Key.Get();
		if (!F) { continue; }
		FFishRecord& Record = Pair.Value;

		if (!Record.bAdult)
		{
			if (Now - Record.BornTime >= F->Reproduction.MaturationTime)
			{
				Record.bAdult = true;
				Record.ReproTimer = NextReproCooldown(F->Reproduction);
				F->SetActorScale3D(Record.AdultScale);
				F->SetGrowthProgress(1.f);
			}
			continue;
		}

		if (!C.bReproductionEnabled || Adults < 2 || F->GetHunger() < F->Reproduction.ReproMinHunger)
		{
			continue;
		}

		Record.ReproTimer -= Dt * ReproFactor;
		if (Record.ReproTimer <= 0.f)
		{
			Record.ReproTimer = NextReproCooldown(F->Reproduction);
			if (Total + EggCount + Parents.Num() < C.MaxFishTotal) { Parents.Add(F); }
		}
	}
	for (AFish* Parent : Parents) { LayEgg(Parent); }

	// 7. Timers et conditions de fin (seulement tant que la partie est en cours)
	if (State == EEcosystemState::Playing)
	{
		Elapsed += Dt;
		if (bDefeatEnabled)
		{
			DefeatTimer = (Gauge >= C.DefeatGaugeThreshold)
				? DefeatTimer + Dt
				: FMath::Max(0.f, DefeatTimer - C.DefeatTimerDecay * Dt);
			ExtinctionTimer = (Adults < 2 && Fry == 0 && EggCount == 0) ? ExtinctionTimer + Dt : 0.f; // D-11
		}
		else
		{
			// Défaite désactivée (tutoriel) : timers à zéro, donc pas d'alerte de défaite au HUD.
			DefeatTimer = 0.f;
			ExtinctionTimer = 0.f;
		}

		if (bDefeatEnabled && DefeatTimer >= C.DefeatDuration)
		{
			SetState(EEcosystemState::Lost, EEcosystemEndReason::GaugeTooHigh);
		}
		else if (bDefeatEnabled && ExtinctionTimer >= C.ExtinctionGraceTime)
		{
			SetState(EEcosystemState::Lost, EEcosystemEndReason::Extinction);
		}
		else if (C.bUseTimedObjective && Elapsed >= C.LevelDuration)
		{
			if (Adults >= C.ObjectiveAdultFish)
			{
				SetState(EEcosystemState::Won, EEcosystemEndReason::None);
			}
			else if (bDefeatEnabled)
			{
				SetState(EEcosystemState::Lost, EEcosystemEndReason::ObjectiveMissed);
			}
		}
	}

	// 8. Photographie pour le HUD
	Snapshot.State = State;
	Snapshot.EndReason = EndReason;
	Snapshot.ElapsedTime = Elapsed;
	Snapshot.TimeRemaining = FMath::Max(0.f, C.LevelDuration - Elapsed);
	Snapshot.bChronoCountsDown = C.bUseTimedObjective;
	Snapshot.ChronoTime = C.bUseTimedObjective ? Snapshot.TimeRemaining : Elapsed;
	Snapshot.AdultFish = Adults;
	Snapshot.FryFish = Fry;
	Snapshot.Eggs = EggCount;
	Snapshot.EggsLost = EggsLost;
	Snapshot.ObjectiveAdultFish = C.ObjectiveAdultFish;
	Snapshot.Pollution = Pollution;
	Snapshot.Oxygen = Oxygen;
	Snapshot.StarvingPercent = StressHunger;
	Snapshot.StressPollution = StressPollution;
	Snapshot.StressOxygen = StressOxygen;
	Snapshot.StressHunger = StressHunger;
	Snapshot.ZonePollution = ZoneOf(StressPollution);
	Snapshot.ZoneOxygen = ZoneOf(StressOxygen);
	Snapshot.ZoneHunger = ZoneOf(StressHunger);
	Snapshot.Gauge = Gauge;
	Snapshot.DefeatTimer = DefeatTimer;
	Snapshot.DefeatDuration = C.DefeatDuration;
	OnUpdated.Broadcast(Snapshot);
}

void UEcosystemSubsystem::UpdateFryScales()
{
	const double Now = GetWorld()->GetTimeSeconds();
	for (auto& Pair : Fish)
	{
		if (Pair.Value.bAdult) { continue; }
		AFish* F = Pair.Key.Get();
		if (!F) { continue; }
		const FFishReproductionSettings& R = F->Reproduction;
		const float Alpha = FMath::Clamp(static_cast<float>((Now - Pair.Value.BornTime) / R.MaturationTime), 0.f, 1.f);
		F->SetActorScale3D(Pair.Value.AdultScale * FMath::Lerp(R.FryStartScale, 1.f, Alpha));
		F->SetGrowthProgress(FMath::Min(Alpha, 0.999f)); // 1 (adulte) est posé par la maturation, pas ici
	}
}

const AActor* UEcosystemSubsystem::GetHeldActor() const
{
	const APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	const UPhysicsHandleComponent* Handle = PC ? PC->FindComponentByClass<UPhysicsHandleComponent>() : nullptr;
	const UPrimitiveComponent* Grabbed = Handle ? Handle->GetGrabbedComponent() : nullptr;
	return Grabbed ? Grabbed->GetOwner() : nullptr;
}

void UEcosystemSubsystem::LayEgg(AFish* Parent)
{
	UWorld* World = GetWorld();
	if (!Parent || !World || !Config)
	{
		return;
	}

	const FFishRecord* ParentRecord = Fish.Find(TWeakObjectPtr<AFish>(Parent));
	const FVector AdultScale = ParentRecord ? ParentRecord->AdultScale : Parent->GetActorScale3D();
	const FFishReproductionSettings& R = Parent->Reproduction;
	UClass* EggClass = R.EggClass ? R.EggClass.Get() : AFishEgg::StaticClass();

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	FVector Location = Parent->GetActorLocation() - FVector(0.f, 0.f, R.EggDropOffset);

	// Un poisson qui nage près du fond pondrait sous le relief, et l'œuf traverserait la collision en triangles.
	// On cherche le sol sous le parent et on garde l'œuf au-dessus (rayon de la sphère de base = 50 × EggScale).
	{
		const FVector TraceStart = Parent->GetActorLocation();
		const FVector TraceEnd = TraceStart - FVector(0.f, 0.f, 10000.f);
		FCollisionQueryParams Query(SCENE_QUERY_STAT(EcosystemEggGround), false, Parent);
		FHitResult GroundHit;
		if (World->LineTraceSingleByChannel(GroundHit, TraceStart, TraceEnd, ECC_Visibility, Query))
		{
			const float MinZ = GroundHit.ImpactPoint.Z + 50.f * R.EggScale + 10.f;
			Location.Z = FMath::Max(Location.Z, MinZ);
		}
	}

	AFishEgg* Egg = World->SpawnActor<AFishEgg>(EggClass, Location, FRotator::ZeroRotator, Params);
	if (!Egg)
	{
		return;
	}
	Egg->SetActorScale3D(FVector(R.EggScale));
	Egg->InitEgg(Parent->GetClass(), AdultScale, R.EggIncubationTime, R.bEggDiesInRedPollution);
	TrackActor(Egg); // sans effet si HandleActorSpawned l'a déjà enregistré
}

void UEcosystemSubsystem::HatchEgg(AFishEgg* Egg)
{
	if (!Egg)
	{
		return;
	}

	TSubclassOf<AFish> FishClass = Egg->FishClass;
	FVector AdultScale = Egg->AdultScale;
	if (!FishClass)
	{
		// Œuf placé à la main sans parent : il prend l'espèce d'un poisson suivi.
		for (const auto& Pair : Fish)
		{
			if (const AFish* F = Pair.Key.Get())
			{
				FishClass = F->GetClass();
				AdultScale = Pair.Value.AdultScale;
				break;
			}
		}
	}

	const float FrySpawnDistance = FishClass ? FishClass->GetDefaultObject<AFish>()->Reproduction.FrySpawnDistance
		: FFishReproductionSettings().FrySpawnDistance;
	const FVector Location = Egg->GetActorLocation() + FVector(0.f, 0.f, FrySpawnDistance);
	Eggs.RemoveSingleSwap(TWeakObjectPtr<AFishEgg>(Egg));
	Egg->Destroy();

	if (FishClass)
	{
		SpawnFry(FishClass, Location, FRotator(0.f, FMath::FRandRange(0.f, 360.f), 0.f), AdultScale);
	}
}

void UEcosystemSubsystem::SpawnFry(TSubclassOf<AFish> FishClass, const FVector& Location, const FRotator& Rotation, const FVector& AdultScale)
{
	UWorld* World = GetWorld();
	if (!FishClass || !World || !Config)
	{
		return;
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	bSpawningFry = true;
	PendingFryAdultScale = AdultScale;
	AFish* Newborn = World->SpawnActor<AFish>(FishClass, Location, Rotation, Params);
	TrackActor(Newborn); // sans effet si HandleActorSpawned l'a déjà enregistré
	bSpawningFry = false;

	if (!Newborn)
	{
		return;
	}

	Newborn->SetActorScale3D(AdultScale * Newborn->Reproduction.FryStartScale);
	Newborn->SetGrowthProgress(0.f);
	Newborn->SetHunger(Newborn->Reproduction.FryStartHunger);
	if (!Newborn->GetController())
	{
		Newborn->SpawnDefaultController(); // garantit l'IA même si le BP n'est pas en AutoPossess à l'apparition
	}
}

void UEcosystemSubsystem::SetState(EEcosystemState NewState, EEcosystemEndReason Reason)
{
	if (State == NewState)
	{
		return;
	}

	State = NewState;
	EndReason = Reason;
	UE_LOG(LogEcosystem, Log, TEXT("Partie : %s (raison : %s) à %.1f s"),
		*UEnum::GetValueAsString(NewState), *UEnum::GetValueAsString(Reason), Elapsed);
	OnStateChanged.Broadcast(NewState);
}

void UEcosystemSubsystem::DrawDebug() const
{
#if !UE_BUILD_SHIPPING
	if (!GEngine || !Config || !Config->bShowDebugHUD)
	{
		return;
	}

	auto ZoneColor = [](EEcosystemZone Zone) -> FColor
	{
		switch (Zone)
		{
		case EEcosystemZone::Green: return FColor::Green;
		case EEcosystemZone::Yellow: return FColor::Yellow;
		default: return FColor::Red;
		}
	};

	const FEcosystemSnapshot& S = Snapshot;
	const FColor StateColor = S.State == EEcosystemState::Won ? FColor::Green : (S.State == EEcosystemState::Lost ? FColor::Red : FColor::White);
	const float Life = 0.25f;

	GEngine->AddOnScreenDebugMessage(94100, Life, StateColor,
		FString::Printf(TEXT("[Ecosysteme] %s%s | reste %.0f s | adultes %d / %d (+%d alevins, %d oeufs, %d oeufs perdus)"),
			*UEnum::GetValueAsString(S.State), S.State == EEcosystemState::Lost ? *FString::Printf(TEXT(" (%s)"), *UEnum::GetValueAsString(S.EndReason)) : TEXT(""),
			S.TimeRemaining, S.AdultFish, S.ObjectiveAdultFish, S.FryFish, S.Eggs, S.EggsLost));
	GEngine->AddOnScreenDebugMessage(94101, Life, ZoneColor(S.ZonePollution),
		FString::Printf(TEXT("  Pollution : %.0f (stress %.0f)"), S.Pollution, S.StressPollution));
	GEngine->AddOnScreenDebugMessage(94102, Life, ZoneColor(S.ZoneOxygen),
		FString::Printf(TEXT("  Oxygene : %.0f (stress %.0f)"), S.Oxygen, S.StressOxygen));
	GEngine->AddOnScreenDebugMessage(94103, Life, ZoneColor(S.ZoneHunger),
		FString::Printf(TEXT("  Faim : %.0f %% affames (stress %.0f)"), S.StarvingPercent, S.StressHunger));
	GEngine->AddOnScreenDebugMessage(94104, Life, S.Gauge >= Config->DefeatGaugeThreshold ? FColor::Red : FColor::White,
		FString::Printf(TEXT("  Jauge D : %.0f (defaite a %.0f) | timer defaite %.0f / %.0f s"),
			S.Gauge, Config->DefeatGaugeThreshold, S.DefeatTimer, S.DefeatDuration));
#endif
}
