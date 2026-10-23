#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Engine/EngineTypes.h"
#include "SupportSurface.generated.h"

UINTERFACE(MinimalAPI, Blueprintable)
class USupportSurface : public UInterface
{
	GENERATED_BODY()
};

class FIH2_API ISupportSurface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Support")
	bool CanSupportAttachment(AActor* Attachee, const FHitResult& Hit) const;

	virtual bool CanSupportAttachment_Implementation(AActor* Attachee, const FHitResult& Hit) const { return true; }
};