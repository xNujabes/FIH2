// Spawner de sable : crée les grains à l'exécution sur une grille sans recouvrement.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SandSpawner.generated.h"

class UBoxComponent;

/**
 * Remplace les grains de sable placés à la main.
 * Au BeginPlay, répartit SandCount grains dans la boîte SpawnArea sur une grille de pas Spacing
 * (couches empilées si la grille d'une couche est pleine), avec un léger décalage aléatoire.
 * Le pas doit rester supérieur au diamètre d'un grain pour éviter toute superposition.
 */
UCLASS()
class FIH2_API ASandSpawner : public AActor
{
	GENERATED_BODY()

public:
	ASandSpawner();

	/** Zone de dépôt : seul le plan XY de la boîte compte, les couches montent depuis son centre. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sand")
	TObjectPtr<UBoxComponent> SpawnArea;

	/** Classe du grain (BP_Sand). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sand")
	TSubclassOf<AActor> SandClass;

	/** SAND_COUNT : nombre de grains créés [20–200]. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sand", meta = (ClampMin = "0", UIMax = "200"))
	int32 SandCount = 65;

	/** SAND_SPACING (cm) : pas de la grille [110–200]. Doit dépasser GrainDiameter. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sand", meta = (ClampMin = "1.0"))
	float Spacing = 120.f;

	/** Diamètre d'un grain (cm) : la sphère de base du moteur fait 100 cm à l'échelle 1. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sand", meta = (ClampMin = "1.0"))
	float GrainDiameter = 100.f;

	/** Décalage aléatoire max (cm) autour de chaque case ; borné automatiquement pour ne jamais créer de recouvrement. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sand", meta = (ClampMin = "0.0"))
	float Jitter = 8.f;

	/** Graine aléatoire : 0 = différente à chaque partie, autre valeur = disposition reproductible. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sand")
	int32 RandomSeed = 0;

	/** Types d'objets qui bloquent une case (décor : pierres, parois). Vide = aucun test. */
	UPROPERTY(EditAnywhere, Category = "Sand")
	TArray<TEnumAsByte<ECollisionChannel>> BlockingObjectTypes = { ECC_WorldStatic };

	/** Remonte chaque grain au-dessus du sol trouvé sous lui : un grain créé sous le relief (collision en triangles)
	 *  n'est pas repoussé et le traverse. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sand|Ground")
	bool bLiftAboveGround = true;

	/** Demi-hauteur (cm) du tracé vertical qui cherche le sol, au-dessus et au-dessous de chaque case. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sand|Ground", meta = (ClampMin = "0.0"))
	float GroundTraceHalfHeight = 5000.f;

	/** Marge (cm) entre le bas du grain et le sol (ou le grain déjà posé dessous). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sand|Ground", meta = (ClampMin = "0.0"))
	float GroundClearance = 5.f;

	/** Couches supplémentaires autorisées pour remplacer les cases bloquées [0–5]. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sand", meta = (ClampMin = "0", UIMax = "5"))
	int32 MaxExtraLayers = 1;

	/** Crée les grains (appelé au BeginPlay). Renvoie le nombre de grains créés. */
	UFUNCTION(BlueprintCallable, Category = "Sand")
	int32 SpawnSand();

protected:
	virtual void BeginPlay() override;

private:
	/** Grains créés par ce spawner (suivis pour le garbage collector). */
	UPROPERTY(Transient)
	TArray<TObjectPtr<AActor>> SpawnedGrains;
};
