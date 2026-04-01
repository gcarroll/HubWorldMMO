#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "URiftPickupComponent.generated.h"

class URiftItemDefinition;
class URiftInventoryComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRiftPickupCollected, AActor*, Collector);

/**
 * Composable component that makes any actor a pickup.
 *
 * On server-side overlap: finds URiftInventoryComponent on the overlapping actor
 * (or their PlayerState), calls AddItem, then optionally destroys the pickup actor.
 *
 * Binds to the first PrimitiveComponent on the owner that has overlap events enabled.
 * ARiftPickup provides a ready-made actor with the right collision setup.
 *
 * Add to custom actors (e.g. chests, item drops) instead of using ARiftPickup when
 * you need full control over the actor's other components.
 */
UCLASS(Blueprintable, BlueprintType, ClassGroup=(RiftVault), meta=(BlueprintSpawnableComponent))
class RIFTVAULTINVENTORY_API URiftPickupComponent : public UActorComponent
{
    GENERATED_BODY()

public:

    /** Fired on the server after the item has been successfully added to the collector's inventory. */
    UPROPERTY(BlueprintAssignable, Category = "RiftVault|Inventory|Delegates")
    FOnRiftPickupCollected OnPickupCollected;

    URiftPickupComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

    virtual void BeginPlay() override;

    UFUNCTION(BlueprintPure, Category = "RiftVault|Inventory")
    URiftItemDefinition* GetItemDefinition() const { return ItemDefinition; }

    UFUNCTION(BlueprintPure, Category = "RiftVault|Inventory")
    int32 GetQuantity() const { return Quantity; }

protected:

    /** The item definition this pickup grants when collected. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pickup")
    TObjectPtr<URiftItemDefinition> ItemDefinition;

    /** Quantity to grant. Defaults to 1. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pickup", meta = (ClampMin = 1))
    int32 Quantity = 1;

    /**
     * If true, the owning actor is destroyed after a successful collect.
     * Set to false for respawning sources (chests, resource nodes).
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pickup")
    bool bDestroyOnCollect = true;

protected:

    /**
     * Called when a valid collector overlaps the pickup trigger.
     * Default implementation finds the collector's URiftInventoryComponent,
     * calls AddItem, fires OnPickupCollected, and optionally destroys the owner.
     *
     * Override in Blueprint or C++ to replace the default collect logic entirely.
     *
     * @param Collector  The actor that triggered the overlap.
     * @return  true if the item was successfully collected.
     */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "RiftVault|Inventory")
    bool TryCollect(AActor* Collector);
    virtual bool TryCollect_Implementation(AActor* Collector);

private:

    UFUNCTION()
    void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
        bool bFromSweep, const FHitResult& SweepResult);
};
