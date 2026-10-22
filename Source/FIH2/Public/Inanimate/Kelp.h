// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Inanimate/Inanimate.h"
#include "Kelp.generated.h"

class AFood;
class UStaticMeshComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnKelpMaturedDelegate, AKelp*, Kelp);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnKelpGrowthProgressChangedDelegate, AKelp*, Kelp, float, NewProgress);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnKelpFruitSpawnedDelegate, AKelp*, Kelp, AFood*, SpawnedFruit);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnKelpFruitDetachedDelegate, AKelp*, Kelp, AFood*, DetachedFruit);


UCLASS()
class FIH2_API AKelp : public AInanimate
{
	GENERATED_BODY()

public:
	AKelp();
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kelp|Growth", meta = (ClampMin = "0.1"))
	float GrowthDuration = 20.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kelp|Growth", meta = (ClampMin = "0.01"))
	float GrowthUpdateInterval = 0.1f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Kelp|Growth")
	float GrowthProgress = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kelp|Growth")
	FVector InitialScale = FVector(0.2f, 0.2f, 0.05f);
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kelp|Growth")
	FVector MatureScale = FVector(1.0f, 1.0f, 1.0f);
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kelp|Growth")
	bool bScaleHeightOnly = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kelp|Growth")
	bool bStartMature = false;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Kelp|Growth")
	bool bIsMature = false;
	
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kelp|Spawning")
	TSubclassOf<AFood> FruitClass;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kelp|Spawning")
	TArray<TSubclassOf<AFood>> FruitClasses;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kelp|Spawning", meta = (ClampMin = "1"))
	int32 MaxFruitCount = 3;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kelp|Spawning", meta = (ClampMin = "0.1"))
	float SpawnInterval = 10.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kelp|Spawning", meta = (ClampMin = "0.0"))
	float InitialSpawnDelay = 1.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kelp|Spawning")
	TArray<FName> FruitSocketNames;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kelp|Spawning")
	TArray<FVector> FruitSpawnOffsets;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kelp|Spawning")
	FVector DefaultFruitSpawnOffset = FVector(0.0f, 0.0f, 150.0f);
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Kelp|Spawning")
	TArray<AFood*> ActiveFruits;


	UPROPERTY(BlueprintAssignable, Category = "Kelp|Events")
	FOnKelpMaturedDelegate OnKelpMatured;
	UPROPERTY(BlueprintAssignable, Category = "Kelp|Events")
	FOnKelpGrowthProgressChangedDelegate OnGrowthProgressChanged;
	UPROPERTY(BlueprintAssignable, Category = "Kelp|Events")
	FOnKelpFruitSpawnedDelegate OnFruitSpawned;
	UPROPERTY(BlueprintAssignable, Category = "Kelp|Events")
	FOnKelpFruitDetachedDelegate OnFruitDetached;


	UFUNCTION(BlueprintCallable, Category = "Kelp|Growth")
	void StartGrowth();
	UFUNCTION(BlueprintCallable, Category = "Kelp|Growth")
	void SetGrowthProgress(float InProgress);
	UFUNCTION(BlueprintPure, Category = "Kelp|Growth")
	float GetGrowthProgress() const { return GrowthProgress; }
	UFUNCTION(BlueprintPure, Category = "Kelp|Growth")
	bool IsMature() const { return bIsMature; }
	UFUNCTION(BlueprintPure, Category = "Kelp|Spawning")
	int32 GetActiveFruitCount() const;
	UFUNCTION(BlueprintPure, Category = "Kelp|Spawning")
	TSubclassOf<AFood> GetFruitClassToSpawn() const;
	UFUNCTION(BlueprintCallable, Category = "Kelp|Spawning")
	bool TrySpawnFruit();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	
	void UpdateGrowth();
	void OnMatured();
	void StartFruitSpawning();
	void HandleSpawnFruitTimer();
	bool GetAvailableSpawnTransform(FName& OutSocketName, FTransform& OutTransform);
	UFUNCTION()
	void HandleFruitDetached(AFood* DetachedFruit);
	UFUNCTION()
	void HandleFruitDestroyed(AActor* DestroyedActor);

private:
	FTimerHandle GrowthTimerHandle;
	FTimerHandle FruitSpawnTimerHandle;
	float CurrentGrowthTime = 0.0f;

	// TODO: Kelp produces oxygen (integrate with ecosystem / oxygen manager)
};
