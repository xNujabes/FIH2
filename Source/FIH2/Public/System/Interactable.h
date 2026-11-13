// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Interactable.generated.h"

/** Évènement natif (non exposé aux Blueprints) : « les infos affichables de cet acteur ont changé ». */
DECLARE_MULTICAST_DELEGATE_OneParam(FOnInteractableInfoChangedNative, AActor* /*Actor*/);

/**
 * Canal global des changements d'infos des interactables.
 * Chaque interactable appelle NotifyInfoChanged(this) quand ce qu'il affiche (nom, description, progression) change.
 * Personne n'écoute tant qu'aucun panneau d'infos n'est ouvert : un Broadcast sans abonné ne coûte presque rien.
 * AMouseController ne s'abonne que pendant qu'un acteur est sélectionné, et filtre sur cet acteur.
 */
struct FIH2_API FInteractableEvents
{
	static FOnInteractableInfoChangedNative& OnInfoChanged();
	static void NotifyInfoChanged(AActor* Actor);
};

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UInteractable : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class FIH2_API IInteractable
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Interaction")
	void OnHover();

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Interaction")
	void OnGrab();

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Interaction")
	void OnDropped();

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Interaction")
	void OnThrown(FVector ThrowVelocity);
	
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Interaction")
	FText GetName();
	
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Interaction")
	FText GetDescription();

	/**
	 * Jauge de progression à afficher dans le panneau d'infos (croissance, incubation...).
	 * Retourne false si l'acteur n'a rien à afficher (par défaut). OutProgress : 0 à 1.
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Interaction")
	bool GetInfoProgress(float& OutProgress, FText& OutLabel);
	
	
};
