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
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnKelpSupportChangedDelegate, AKelp*, Kelp);

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
	
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Kelp|Support")
	UPhysicsConstraintComponent* AttachConstraint;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kelp|Support")
	FVector BaseLocalOffset = FVector::ZeroVector;
	UPROPERTY(EditAnywhere, Category = "Kelp|Support")
	TEnumAsByte<ECollisionChannel> AttachTraceChannel = ECC_Visibility;
	UPROPERTY(EditAnywhere, Category = "Kelp|Support")
	float AttachTraceUp = 10.f;
	UPROPERTY(EditAnywhere, Category = "Kelp|Support")
	float AttachTraceDown = 30.f;
	UPROPERTY(EditAnywhere, Category = "Kelp|Support")
	float DetachStrainThreshold = 40.f;
	UPROPERTY(EditAnywhere, Category = "Kelp|Support")
	float DetachHoldTime = 0.4f;
	UPROPERTY(EditAnywhere, Category = "Kelp|Support")
	float StrainRecoveryMultiplier = 2.f;
	/** Délai (s) après le relâchement avant une nouvelle tentative d'attache, si le kelp n'est toujours pas attaché. 0 = désactivé. */
	UPROPERTY(EditAnywhere, Category = "Kelp|Support", meta = (ClampMin = "0.0"))
	float ReattachRetryDelay = 3.f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Kelp|Support")
	bool bIsAttached = false;
	/** Tag de composant qui rend une surface d'attache valide sans rendre tout l'acteur support
	 *  (ex. le composant SM_SeabedTank de BP_Tank, mais pas ses murs). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kelp|Support")
	FName SupportComponentTag = TEXT("SupportSurface");
	/** Tentatives d'attache après l'apparition, espacées de ReattachRetryDelay : un kelp posé au-dessus du sol tombe puis s'attache. 0 = aucune. */
	UPROPERTY(EditAnywhere, Category = "Kelp|Support", meta = (ClampMin = "0"))
	int32 SpawnAttachRetries = 10;
	/** Au démarrage : hauteur [cm] au-dessus de la base d'où part le tracé (rattrape un kelp placé légèrement sous le relief). */
	UPROPERTY(EditAnywhere, Category = "Kelp|Support", meta = (ClampMin = "0"))
	float SpawnAttachTraceUp = 500.f;
	/** Au démarrage : profondeur [cm] du tracé sous la base. Le kelp est posé directement sur le sol trouvé, sans tomber
	 *  (un corps qui tombe sur une pente peut traverser une collision en triangles). */
	UPROPERTY(EditAnywhere, Category = "Kelp|Support", meta = (ClampMin = "0"))
	float SpawnAttachTraceDown = 5000.f;


	UPROPERTY(BlueprintAssignable, Category = "Kelp|Events")
	FOnKelpMaturedDelegate OnKelpMatured;
	UPROPERTY(BlueprintAssignable, Category = "Kelp|Events")
	FOnKelpGrowthProgressChangedDelegate OnGrowthProgressChanged;
	UPROPERTY(BlueprintAssignable, Category = "Kelp|Events")
	FOnKelpFruitSpawnedDelegate OnFruitSpawned;
	UPROPERTY(BlueprintAssignable, Category = "Kelp|Events")
	FOnKelpFruitDetachedDelegate OnFruitDetached;
	UPROPERTY(BlueprintAssignable, Category = "Kelp|Events")
	FOnKelpSupportChangedDelegate OnAttachedToSupport;
	UPROPERTY(BlueprintAssignable, Category = "Kelp|Events")
	FOnKelpSupportChangedDelegate OnDetachedFromSupport;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kelp|Upright", meta = (ClampMin = "0.0"))
	float UprightStrength = 20.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kelp|Upright", meta = (ClampMin = "0.0"))
	float UprightDamping = 9.f;

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
	
	
	UFUNCTION(BlueprintCallable, Category = "Kelp|Support")
	bool TryAttach();
	UFUNCTION(BlueprintCallable, Category = "Kelp|Support")
	void Detach();
	UFUNCTION(BlueprintPure, Category = "Kelp|Support")
	bool IsAttached() const { return bIsAttached; }

	/** Called by the controller every frame while the kelp is grabbed. */
	void ReportGrabStrain(float Strain, float DeltaTime);
	/** Called by the controller when the player lets go. */
	void NotifyReleased();
	
	virtual FText GetName_Implementation() override;
	virtual FText GetDescription_Implementation() override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaTime) override;
	
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
	FTimerHandle ReattachTimerHandle;
	void HandleReattachRetry();
	/** Tracé d'attache avec des distances explicites [cm, avant échelle du mesh pour TryAttach()]. bScaleBySize : multiplier par l'échelle Z du mesh. */
	bool TryAttachWithTrace(float TraceUp, float TraceDown, bool bScaleBySize);
	float CurrentGrowthTime = 0.0f;

	float StrainTimer = 0.f;
	TWeakObjectPtr<AActor> AttachedSupport;
	/** Tentatives d'attache restantes depuis l'apparition (voir SpawnAttachRetries). */
	int32 SpawnAttachAttemptsLeft = 0;
	
	// TODO: Kelp produces oxygen (integrate with ecosystem / oxygen manager)
};
