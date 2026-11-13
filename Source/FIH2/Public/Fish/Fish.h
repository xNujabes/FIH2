// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SphereComponent.h"
#include "GameFramework/Actor.h"
#include "System/Eatable.h"
#include "System/Interactable.h"
#include "Fish.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnHungerChangedDelegate);

UENUM(BlueprintType)
enum class EFoodPreferences : uint8
{
	Omnivore UMETA(DisplayName = "Omnivore"),
	Carnivore UMETA(DisplayName = "Carnivore"),
	Herbivore UMETA(DisplayName = "Herbivore"),
	Crab UMETA(DisplayName = "Only eats crabs")
};

class AFishEgg;

/**
 * Reproduction et cycle de vie propres à une espèce de poisson (réglés dans le Blueprint du poisson).
 * UEcosystemSubsystem gère la reproduction mais lit ces valeurs sur chaque poisson.
 */
USTRUCT(BlueprintType)
struct FFishReproductionSettings
{
	GENERATED_BODY()

	/** COOLDOWN_REPRO [s] : délai moyen entre deux pontes pour un adulte nourri. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reproduction", meta = (ClampMin = "5"))
	float ReproCooldown = 100.f;

	/** Variation aléatoire du délai (0,2 = ±20 %). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reproduction", meta = (ClampMin = "0", ClampMax = "0.9"))
	float ReproCooldownJitter = 0.2f;

	/** SEUIL_SATIETE_REPRO : Hunger minimal (0–3) pour qu'un adulte ponde. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reproduction", meta = (ClampMin = "0", ClampMax = "3"))
	int32 ReproMinHunger = 2;

	/** MATURATION_ALEVIN [s] : durée du stade alevin. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reproduction|Fry", meta = (ClampMin = "1"))
	float MaturationTime = 30.f;

	/** Échelle de l'alevin à la naissance, relative à l'adulte (grandit jusqu'à 1). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reproduction|Fry", meta = (ClampMin = "0.1", ClampMax = "1"))
	float FryStartScale = 0.5f;

	/** Hunger (0–3) de l'alevin à la naissance. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reproduction|Fry", meta = (ClampMin = "0", ClampMax = "3"))
	int32 FryStartHunger = 2;

	/** Hauteur [cm] au-dessus de l'œuf où apparaît l'alevin. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reproduction|Fry", meta = (ClampMin = "0"))
	float FrySpawnDistance = 60.f;

	/** Classe d'œuf pondu (vide = AFishEgg, sphère provisoire). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reproduction|Egg")
	TSubclassOf<AFishEgg> EggClass;

	/** INCUBATION_OEUF [s] */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reproduction|Egg", meta = (ClampMin = "1"))
	float EggIncubationTime = 20.f;

	/** D-10 : l'œuf meurt si la pollution est en zone rouge. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reproduction|Egg")
	bool bEggDiesInRedPollution = true;

	/** Échelle monde de l'œuf (sphère de base = 100 cm : 0,8 = 80 cm). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reproduction|Egg", meta = (ClampMin = "0.01"))
	float EggScale = 0.8f;

	/** Distance [cm] sous le parent où l'œuf est pondu (toujours remonté au-dessus du sol). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reproduction|Egg", meta = (ClampMin = "0"))
	float EggDropOffset = 60.f;
};

UCLASS()
class FIH2_API AFish : public APawn, public IInteractable
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AFish();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USphereComponent* CollisionSphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* Mesh;
	

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	virtual void PossessedBy(AController* NewController) override;

	UPROPERTY(BlueprintAssignable, BlueprintCallable, Category = "AI|Hunger")
	FOnHungerChangedDelegate OnHungerChanged;

	UPROPERTY(BlueprintReadOnly, VisibleInstanceOnly, Category = "Fish|Status")
	int Hunger = 1;

	UPROPERTY(BlueprintReadOnly, VisibleInstanceOnly, Category = "Fish|Status")
	bool bIsStimulated = false;

	UPROPERTY(BlueprintReadOnly, VisibleInstanceOnly, Category = "Fish|Movement")
	bool bIsHeldByPlayer = false;
	
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Fish|Status")
	EFoodPreferences FoodPreferences = EFoodPreferences::Herbivore;

public:
	/** Reproduction, œuf et alevin de cette espèce (lus par UEcosystemSubsystem). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fish|Reproduction")
	FFishReproductionSettings Reproduction;

protected:


public:

	UFUNCTION(BlueprintPure, Category = "Fish|Status")
	int GetHunger() const;

	UFUNCTION(BlueprintCallable, Category = "Fish|Status")
	void SetHunger(int32 NewHunger);

	UFUNCTION(BlueprintCallable, Category = "Fish|Status")
	void AddHunger(int32 AddedHunger);


	UFUNCTION(BlueprintPure, Category = "Fish|Status")
	bool IsStimulated() const;

	/**
	 * A fish is kinematic and moves ONLY through its UFishSteeringMovement.
	 * The player controller calls this while the fish is held with the physics handle: the body then simulates
	 * physics so it can be dragged (steering stands aside). When released, the fish is kinematic again.
	 */
	UFUNCTION(BlueprintCallable, Category = "Fish|Movement")
	void SetHeldByPlayer(bool bHeld);

	UFUNCTION(BlueprintPure, Category = "Fish|Movement")
	bool IsHeldByPlayer() const { return bIsHeldByPlayer; }

	UFUNCTION(BlueprintCallable, Category = "Fish|Status")
	void SetIsStimulated(bool bNewStimulated);
	
	virtual FText GetName_Implementation() override;
	virtual FText GetDescription_Implementation() override;
};
