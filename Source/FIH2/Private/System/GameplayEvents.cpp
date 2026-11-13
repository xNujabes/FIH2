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
