// Fill out your copyright notice in the Description page of Project Settings.

#include "Bonus/Food.h"
#include "Components/BoxComponent.h"
#include "Fish/Fish.h"
#include "GeometryCollection/GeometryCollectionComponent.h"
// Sets default values
AFood::AFood()
{
    PhysicsProxy = CreateDefaultSubobject<UBoxComponent>(TEXT("FoodPhysicsProxy"));
    SetRootComponent(PhysicsProxy);
    PhysicsProxy->SetSimulatePhysics(true);
    PhysicsProxy->SetCollisionProfileName(TEXT("PhysicsActor"));
    Mesh->SetupAttachment(PhysicsProxy);
    Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    
    GeometryCollection = CreateDefaultSubobject<UGeometryCollectionComponent>(TEXT("GeometryCollection"));
    GeometryCollection->SetupAttachment(PhysicsProxy);
    GeometryCollection->SetSimulatePhysics(false);
    GeometryCollection->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    GeometryCollection->SetVisibility(false);
}

void AFood::TriggerEat_Implementation(AActor* EatInstigator, FVector HitLocation, float DamageAmount)
{
    AFish* FishInstigator = Cast<AFish>(EatInstigator);
    if (FishInstigator)
    {
        FishInstigator->AddHunger(1);
        Mesh->SetVisibility(false);
        PhysicsProxy->SetSimulatePhysics(false);
        PhysicsProxy->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Execute_TriggerBreak(this, EatInstigator, HitLocation, DamageAmount);
        
        SetLifeSpan(8.0f); 
    }
}

void AFood::TriggerBreak_Implementation(AActor* BreakInstigator, FVector HitLocation, float DamageAmount)
{
    if (GeometryCollection)
    {
        GeometryCollection->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
        GeometryCollection->SetVisibility(true);
        GeometryCollection->SetCollisionProfileName(TEXT("Destructible"));
        GeometryCollection->SetSimulatePhysics(true);
        
        TWeakObjectPtr<AFood> WeakThis(this);
        GetWorldTimerManager().SetTimerForNextTick([WeakThis, HitLocation, DamageAmount]()
        {
            UE_LOG(LogTemp, Warning, TEXT("TriggerBreak Timer expired"));
            if (!WeakThis.IsValid() || !WeakThis->GeometryCollection) return;
            WeakThis->GeometryCollection->AddRadialImpulse(
                HitLocation, 150.0f, DamageAmount,
                ERadialImpulseFalloff::RIF_Linear, true);
            WeakThis->GeometryCollection->ApplyExternalStrain(
                0,                  // item index (check the exact signature in your 5.8 header)
                HitLocation,
                150.0f,             // radius
                0,                  // propagation depth
                1.0f,               // propagation factor
                1000000.0f);        // strain, big enough to exceed the thresholds
            
            UE_LOG(LogTemp, Warning, TEXT("Impulses should have been done"));
        });
        
    }
}
