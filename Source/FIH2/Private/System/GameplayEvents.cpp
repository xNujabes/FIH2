#include "System/GameplayEvents.h"

FOnFishAteNative& FGameplayEvents::OnFishAte()
{
	static FOnFishAteNative Delegate;
	return Delegate;
}

FOnPlayerActorNative& FGameplayEvents::OnActorSelected()
{
	static FOnPlayerActorNative Delegate;
	return Delegate;
}

FOnPlayerActorNative& FGameplayEvents::OnActorReleased()
{
	static FOnPlayerActorNative Delegate;
	return Delegate;
}

FOnPlayerActorNative& FGameplayEvents::OnFocusUsed()
{
	static FOnPlayerActorNative Delegate;
	return Delegate;
}

FOnEggNative& FGameplayEvents::OnEggLaid()
{
	static FOnEggNative Delegate;
	return Delegate;
}

FOnEggHatchedNative& FGameplayEvents::OnEggHatched()
{
	static FOnEggHatchedNative Delegate;
	return Delegate;
}

FOnEggNative& FGameplayEvents::OnEggDied()
{
	static FOnEggNative Delegate;
	return Delegate;
}
