// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SphereComponent.h"
#include "GameFramework/Actor.h"
#include "System/Interactable.h"
#include "Fish.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnHungerChangedDelegate);

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

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintPure, Category = "Fish|Status")
	int GetHunger() const;

	UFUNCTION(BlueprintCallable, Category = "Fish|Status")
	void SetHunger(float NewHunger);

	UFUNCTION(BlueprintCallable, Category = "Fish|Status")
	void AddHunger(float AddedHunger);


	UFUNCTION(BlueprintPure, Category = "Fish|Status")
	bool IsStimulated() const;

	UFUNCTION(BlueprintCallable, Category = "Fish|Status")
	void SetIsStimulated(bool bNewStimulated);
};
