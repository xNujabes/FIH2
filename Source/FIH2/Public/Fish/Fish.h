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

UCLASS()
class FIH2_API AFish : public APawn, public IInteractable, public IEatable
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
