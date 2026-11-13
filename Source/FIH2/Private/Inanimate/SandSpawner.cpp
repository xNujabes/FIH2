// Spawner de sable : crée les grains à l'exécution sur une grille sans recouvrement.

#include "Inanimate/SandSpawner.h"
#include "Components/BoxComponent.h"
#include "Engine/World.h"
#include "Math/RandomStream.h"
#include "System/FIH2Log.h"

ASandSpawner::ASandSpawner()
{
	PrimaryActorTick.bCanEverTick = false;

	SpawnArea = CreateDefaultSubobject<UBoxComponent>(TEXT("SpawnArea"));
	SpawnArea->SetBoxExtent(FVector(1000.f, 1000.f, 10.f));
	SpawnArea->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SpawnArea->SetHiddenInGame(true);
	RootComponent = SpawnArea;
}

void ASandSpawner::BeginPlay()
{
	Super::BeginPlay();
	SpawnSand();
}

int32 ASandSpawner::SpawnSand()
{
	UWorld* World = GetWorld();
	if (!World || !SandClass || SandCount <= 0)
	{
		UE_LOG(LogSand, Warning, TEXT("[SandSpawner %s] Rien à créer : SandClass non défini ou SandCount <= 0"), *GetNameSafe(this));
		return 0;
	}

	if (Spacing <= GrainDiameter)
	{
		UE_LOG(LogSand, Warning, TEXT("[SandSpawner %s] Spacing (%.0f) <= GrainDiameter (%.0f) : les grains vont se superposer"),
			*GetNameSafe(this), Spacing, GrainDiameter);
	}

	// Grille d'une couche, dans l'espace local de la boîte (taille réelle, échelle comprise).
	const FVector Extent = SpawnArea->GetScaledBoxExtent();
	const int32 Cols = FMath::Max(1, FMath::FloorToInt(2.f * Extent.X / Spacing));
	const int32 Rows = FMath::Max(1, FMath::FloorToInt(2.f * Extent.Y / Spacing));
	const int32 CellsPerLayer = Cols * Rows;

	// Le décalage aléatoire ne doit jamais rapprocher deux grains à moins d'un diamètre.
	const float MaxJitter = FMath::Clamp(Jitter, 0.f, FMath::Max(0.f, (Spacing - GrainDiameter) * 0.5f));

	FRandomStream Stream(RandomSeed != 0 ? RandomSeed : FMath::Rand());

	// Ordre des cases mélangé : avec moins de grains que de cases, ils se répartissent sur toute la zone.
	TArray<int32> CellOrder;
	CellOrder.Reserve(CellsPerLayer);
	for (int32 i = 0; i < CellsPerLayer; ++i)
	{
		CellOrder.Add(i);
	}
	for (int32 i = CellsPerLayer - 1; i > 0; --i)
	{
		CellOrder.Swap(i, Stream.RandRange(0, i));
	}

	const FTransform AreaXf(SpawnArea->GetComponentQuat(), SpawnArea->GetComponentLocation());

	// Les cases occupées par le décor (pierres, parois) sont sautées AVANT le spawn :
	// on évite ainsi FindTeleportSpot, qui échoue sur un grain dont la racine est un mesh (warning LogPhysics).
	FActorSpawnParameters Params;
	Params.Owner = this;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	FCollisionObjectQueryParams BlockingTypes;
	for (const TEnumAsByte<ECollisionChannel>& Channel : BlockingObjectTypes)
	{
		BlockingTypes.AddObjectTypesToQuery(Channel);
	}
	FCollisionQueryParams OverlapParams(SCENE_QUERY_STAT(SandSpawnerOverlap), false, this);
	const FCollisionShape GrainShape = FCollisionShape::MakeSphere(GrainDiameter * 0.5f);

	// Assez de candidats pour compenser les cases bloquées : les couches nécessaires + MaxExtraLayers.
	const int32 NeededLayers = FMath::DivideAndRoundUp(SandCount, CellsPerLayer);
	const int32 MaxCandidates = CellsPerLayer * (NeededLayers + MaxExtraLayers);

	int32 Spawned = 0;
	int32 Skipped = 0;
	for (int32 i = 0; i < MaxCandidates && Spawned < SandCount; ++i)
	{
		const int32 Layer = i / CellsPerLayer;
		const int32 Cell = CellOrder[i % CellsPerLayer];
		const int32 Col = Cell % Cols;
		const int32 Row = Cell / Cols;

		const FVector Local(
			-Extent.X + Spacing * (Col + 0.5f) + Stream.FRandRange(-MaxJitter, MaxJitter),
			-Extent.Y + Spacing * (Row + 0.5f) + Stream.FRandRange(-MaxJitter, MaxJitter),
			Layer * Spacing);
		FVector WorldLoc = AreaXf.TransformPosition(Local);

		// Le premier obstacle rencontré en descendant (sol, rocher ou grain déjà créé) fixe la hauteur minimale.
		if (bLiftAboveGround && GroundTraceHalfHeight > 0.f)
		{
			FCollisionQueryParams GroundParams(SCENE_QUERY_STAT(SandSpawnerGround), false, this);
			FHitResult GroundHit;
			const FVector Up = WorldLoc + FVector(0.f, 0.f, GroundTraceHalfHeight);
			const FVector Down = WorldLoc - FVector(0.f, 0.f, GroundTraceHalfHeight);
			if (World->LineTraceSingleByChannel(GroundHit, Up, Down, ECC_Visibility, GroundParams))
			{
				const float MinZ = GroundHit.ImpactPoint.Z + GrainDiameter * 0.5f + GroundClearance;
				WorldLoc.Z = FMath::Max(WorldLoc.Z, MinZ);
			}
		}

		if (BlockingObjectTypes.Num() > 0 &&
			World->OverlapAnyTestByObjectType(WorldLoc, FQuat::Identity, BlockingTypes, GrainShape, OverlapParams))
		{
			++Skipped;
			continue;
		}

		const FRotator Rot(0.f, Stream.FRandRange(0.f, 360.f), 0.f);
		if (AActor* Grain = World->SpawnActor<AActor>(SandClass, FTransform(Rot, WorldLoc), Params))
		{
			SpawnedGrains.Add(Grain);
			++Spawned;
		}
	}

	if (Spawned < SandCount)
	{
		UE_LOG(LogSand, Warning, TEXT("[SandSpawner %s] Seulement %d/%d grains : zone trop encombrée, agrandir SpawnArea ou MaxExtraLayers"),
			*GetNameSafe(this), Spawned, SandCount);
	}

	UE_LOG(LogSand, Verbose, TEXT("[SandSpawner %s] %d grains créés, %d cases sautées (%d x %d par couche, pas %.0f cm)"),
		*GetNameSafe(this), Spawned, Skipped, Cols, Rows, Spacing);
	return Spawned;
}
