// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "InputMappingContext.h"
#include "MouseController.generated.h"

// AI Given solution 
class UPhysicsHandleComponent;
class UInputAction;
class UMaterialInterface;
class UMeshComponent;
class USpringArmComponent;
/**
 * 
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnActorClicked, AActor*, ClickedActor);
UCLASS()
class FIH2_API AMouseController : public APlayerController
{
	GENERATED_BODY()

public:
	AMouseController();

	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputMappingContext* InputMapping;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* ClickAction;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* LookAction;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Interaction")
	UPhysicsHandleComponent* PhysicsHandle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	float TraceRange = 2500.f;
	
	UPROPERTY(BlueprintAssignable, BlueprintCallable, Category = "Interaction")
	FOnActorClicked OnActorClicked;

	// ---------------------------------------------------------------- Sélection
	/** Émis quand la sélection change. NewSelection = nullptr : plus rien de sélectionné (fermer le panneau). */
	UPROPERTY(BlueprintAssignable, Category = "Selection")
	FOnActorClicked OnSelectionChanged;

	/** Émis quand les infos de l'acteur sélectionné changent (jamais quand rien n'est sélectionné). */
	UPROPERTY(BlueprintAssignable, Category = "Selection")
	FOnActorClicked OnSelectedInfoChanged;

	/** Matériau de surbrillance appliqué en « overlay » sur les meshes de l'acteur sélectionné (vide = pas de surbrillance). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Selection")
	TObjectPtr<UMaterialInterface> SelectionOverlayMaterial;

	/** FOCUS_DUREE [s] : durée du déplacement de caméra de la loupe. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Selection|Focus", meta = (ClampMin = "0"))
	float FocusDuration = 0.6f;

	/** FOCUS_DISTANCE_FACTEUR : distance caméra = rayon des bornes de l'acteur × ce facteur. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Selection|Focus", meta = (ClampMin = "0.5"))
	float FocusDistanceFactor = 4.f;

	/** FOCUS_DISTANCE_MIN [cm] : distance caméra minimale (petits objets). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Selection|Focus", meta = (ClampMin = "0"))
	float FocusMinArmLength = 300.f;

	UFUNCTION(BlueprintCallable, Category = "Selection")
	void SelectActor(AActor* Actor);

	UFUNCTION(BlueprintCallable, Category = "Selection")
	void ClearSelection();

	UFUNCTION(BlueprintPure, Category = "Selection")
	AActor* GetSelectedActor() const { return SelectedActor; }

	/** Loupe : amène la caméra (bras de caméra du pion) juste devant l'acteur sélectionné, sans changer l'angle de vue. */
	UFUNCTION(BlueprintCallable, Category = "Selection")
	void FocusOnSelection();

	/** Cadre la caméra sur un groupe d'acteurs, sans changer l'angle (tutoriel). N'émet pas OnFocusUsed, réservé à la loupe. */
	void FocusOnActors(const TArray<AActor*>& Actors);

	/**
	 * Surbrillance posée par un autre système (tutoriel). Material nullptr = la retirer.
	 * Si l'acteur est sélectionné, la surbrillance de sélection reste et c'est ce qui sera restauré à la désélection
	 * qui est mis à jour : la sélection ne peut plus « figer » une surbrillance de tutoriel.
	 */
	void SetPersistentHighlight(AActor* Actor, UMaterialInterface* Material);
	
	UPROPERTY(EditAnywhere, Category="Interaction") 
	float GrabScrollSpeed = 0.05f;
	
	UPROPERTY(EditAnywhere, Category="Interaction")
	float MinGrabAlpha = 0.02f;
	
	UPROPERTY(EditAnywhere, Category="Interaction")
	float BaseInterpolationSpeed = 50.f;
	
	// index 0 : linear, index 1 : angular
	UPROPERTY(EditAnywhere, Category="Interaction")
	FVector2D BaseStifness = FVector2D(750, 1500);

	float GrabAlpha = 0.5f;
	bool bIsDragging;
	bool bClickHeld;
	AActor* GrabbedActor;
	
	FHitResult ClickHitResult;
	
	FVector GrabLocalPoint = FVector::ZeroVector;
	
	UFUNCTION(BlueprintCallable) void AdjustGrabDistance(float AxisValue);
	UFUNCTION(BlueprintPure) bool GetIsDragging() const { return bIsDragging; }
	
	void UpdateDragTarget(float DeltaTime);
	void UpdateFocus(float DeltaTime);
	void OnMouseClick();
	void OnMouseRelease();
	void OnMouseMoved();
	virtual void PlayerTick(float DeltaTime) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UPROPERTY(Transient)
	TObjectPtr<AActor> SelectedActor;

	/** Overlays d'origine des meshes surlignés, restaurés à la désélection. */
	TMap<TWeakObjectPtr<UMeshComponent>, TWeakObjectPtr<UMaterialInterface>> SavedOverlays;

	FDelegateHandle InfoChangedHandle;

	// Interpolation de la loupe
	TWeakObjectPtr<USpringArmComponent> FocusArm;
	FVector FocusStartLocation = FVector::ZeroVector;
	FVector FocusEndLocation = FVector::ZeroVector;
	float FocusStartArmLength = 0.f;
	float FocusEndArmLength = 0.f;
	float FocusElapsed = -1.f; // < 0 : pas de focus en cours

	void ApplyHighlight(AActor* Actor);
	void RemoveHighlight();
	/** Lance l'interpolation de la caméra vers Origin. Retourne false sans bras de caméra. */
	bool StartFocus(const FVector& Origin, float Radius);
	void HandleInteractableInfoChanged(AActor* Actor);

	UFUNCTION()
	void HandleSelectedEndPlay(AActor* Actor, EEndPlayReason::Type EndPlayReason);
};
