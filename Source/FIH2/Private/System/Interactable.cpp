// Fill out your copyright notice in the Description page of Project Settings.


#include "System/Interactable.h"

// Add default functionality here for any IInteractable functions that are not pure virtual.

FOnInteractableInfoChangedNative& FInteractableEvents::OnInfoChanged()
{
	static FOnInteractableInfoChangedNative Delegate;
	return Delegate;
}

void FInteractableEvents::NotifyInfoChanged(AActor* Actor)
{
	FOnInteractableInfoChangedNative& Delegate = OnInfoChanged();
	if (Actor && Delegate.IsBound())
	{
		Delegate.Broadcast(Actor);
	}
}
