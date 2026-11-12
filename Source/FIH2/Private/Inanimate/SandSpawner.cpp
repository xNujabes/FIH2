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

	FActorSpawnParameters Params;
	Params.Owner = this;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	int32 Spawned = 0;
	for (int32 i = 0; i < SandCount; ++i)
	{
		const int32 Layer = i / CellsPerLayer;
		const int32 Cell = CellOrder[i % CellsPerLayer];
		const int32 Col = Cell % Cols;
		const int32 Row = Cell / Cols;

		const FVector Local(
			-Extent.X + Spacing * (Col + 0.5f) + Stream.FRandRange(-MaxJitter, MaxJitter),
			-Extent.Y + Spacing * (Row + 0.5f) + Stream.FRandRange(-MaxJitter, MaxJitter),
			Layer * Spacing);

		const FRotator Rot(0.f, Stream.FRandRange(0.f, 360.f), 0.f);
		const FTransform SpawnXf(Rot, AreaXf.TransformPosition(Local));

		if (AActor* Grain = World->SpawnActor<AActor>(SandClass, SpawnXf, Params))
		{
			SpawnedGrains.Add(Grain);
			++Spawned;
		}
	}

	UE_LOG(LogSand, Verbose, TEXT("[SandSpawner %s] %d grains créés (%d x %d par couche, pas %.0f cm)"),
		*GetNameSafe(this), Spawned, Cols, Rows, Spacing);
	return Spawned;
}
