// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Inanimate/Inanimate.h"
#include "System/Breakable.h"
#include "System/Eatable.h"
#include "Components/BoxComponent.h"
#include "Food.generated.h"

class AKelp;
class UGeometryCollectionComponent;

/**
 * Growth states for food/fruits
 */
UENUM(BlueprintType)
enum class EFoodGrowthState : uint8
{
	Growing UMETA(DisplayName = "Growing"),
	FullyGrown UMETA(DisplayName = "Fully Grown"),
	Detached UMETA(DisplayName = "Detached"),
	Decaying UMETA(DisplayName = "Decaying")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnFoodFullyGrownDelegate, AFood*, Food);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnFoodDetachedDelegate, AFood*, Food);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnFoodDecayStartedDelegate, AFood*, Food);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnFoodGrowthStateChangedDelegate, AFood*, Food, EFoodGrowthState, NewState);
class UAIPerceptionStimuliSourceComponent;

UCLASS()
class FIH2_API AFood : public AInanimate, public IBreakable, public IEatable
{
	GENERATED_BODY()

public:
	AFood();

	/** IEatable: not edible anymore once eaten (the actor lives a few seconds more for the break effect). */
	virtual bool IsEdible() const override;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Physics")
	UBoxComponent* PhysicsProxy;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Destruction")
	UGeometryCollectionComponent* GeometryCollection;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	UAIPerceptionStimuliSourceComponent* StimuliSource;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Food|State")
	EFoodGrowthState CurrentGrowthState = EFoodGrowthState::Growing;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Food|State")
	EFoodGrowthState InitialGrowthState = EFoodGrowthState::Growing;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Food|State")
	bool bRequireAttachParentToGrow = true;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Food|Growth", meta = (ClampMin = "0.1"))
	float GrowthDuration = 10.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Food|Growth", meta = (ClampMin = "0.01"))
	float GrowthUpdateInterval = 0.05f;
	FVector InitialGrowthScale = FVector(0.1f, 0.1f, 0.1f);
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Food|Growth")
	FVector MatureGrowthScale = FVector(1.0f, 1.0f, 1.0f);
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Food|Growth", meta = (ClampMin = "0.1"))
	float FullyGrownDuration = 30.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Food|Decay")
	bool bDecayStartsOnLanding = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Food|Decay", meta = (ClampMin = "0.1"))
	float DecayDuration = 15.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Food|Decay", meta = (ClampMin = "0.0"))
	float DetachedFallbackDecayDelay = 15.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Food|Eating")
	bool bCanBeEatenWhileGrowing = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Food|Interaction")
	bool bInteractableOnlyWhenFullyGrown = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Food|Eating")
	float HungerRestoreAmount = 1.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Food|Eating")
	float EatenLifeSpan = 8.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Food|Destruction")
	float BreakImpulseRadius = 150.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Food|Destruction")
	float BreakExternalStrain = 1000000.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Food|Destruction")
	float BreakPropagationFactor = 1.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Food|Destruction")
	int32 StrainItemIndex = 0;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Food|Growth")
	AKelp* AttachParentKelp = nullptr;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Food|Growth")
	FName AttachedSocketName = NAME_None;

	UPROPERTY(BlueprintAssignable, Category = "Food|Events")
	FOnFoodFullyGrownDelegate OnFullyGrown;
	UPROPERTY(BlueprintAssignable, Category = "Food|Events")
	FOnFoodDetachedDelegate OnDetached;
	UPROPERTY(BlueprintAssignable, Category = "Food|Events")
	FOnFoodDecayStartedDelegate OnDecayStarted;
	UPROPERTY(BlueprintAssignable, Category = "Food|Events")
	FOnFoodGrowthStateChangedDelegate OnGrowthStateChanged;

	UFUNCTION(BlueprintCallable, Category = "Food|Growth")
	void AttachToKelp(AKelp* InKelp, FName InSocketName = NAME_None);
	UFUNCTION(BlueprintCallable, Category = "Food|Growth")
	void StartGrowth();
	UFUNCTION(BlueprintCallable, Category = "Food|Growth")
	void DetachFruit();
	UFUNCTION(BlueprintCallable, Category = "Food|Decay")
	void StartDecay();
	UFUNCTION(BlueprintPure, Category = "Food|State")
	EFoodGrowthState GetGrowthState() const { return CurrentGrowthState; }
	UFUNCTION(BlueprintPure, Category = "Food|Growth")
	FName GetAttachedSocketName() const { return AttachedSocketName; }
	UFUNCTION(BlueprintPure, Category = "Food|Growth")
	AKelp* GetAttachParentKelp() const;

	virtual void TriggerEat_Implementation(AActor* EatInstigator, FVector HitLocation, float DamageAmount) override;
	virtual void TriggerBreak_Implementation(AActor* BreakInstigator, FVector HitLocation, float DamageAmount) override;
	virtual void OnHover_Implementation() override;
	virtual void OnGrab_Implementation() override;
	virtual void OnDropped_Implementation() override;
	virtual void OnThrown_Implementation(FVector ThrowVelocity) override;
	virtual FText GetName_Implementation() override;
	virtual FText GetDescription_Implementation() override;
	/** Jauge « Growth » tant que le fruit pousse sur le kelp (comme l'alevin) ; rien une fois mûr. */
	virtual bool GetInfoProgress_Implementation(float& OutProgress, FText& OutLabel) override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	void UpdateGrowth();
	void HandleDecayExpired();
	void SetGrowthState(EFoodGrowthState NewState);
	UFUNCTION()
	void OnPhysicsProxyHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

private:
	FTimerHandle GrowthTimerHandle;
	FTimerHandle FullyGrownTimerHandle;
	FTimerHandle DetachedFallbackTimerHandle;
	FTimerHandle DecayTimerHandle;

	float CurrentGrowthTime = 0.0f;
	bool bIsEaten = false;

	// TODO: Decaying food increases environment pollution (notify pollution manager / subsystem)
};
