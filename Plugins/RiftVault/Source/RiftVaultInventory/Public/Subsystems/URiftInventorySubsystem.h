#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "URiftInventorySubsystem.generated.h"

class URiftInventoryComponent;

/**
 * World subsystem that acts as a registry for all active inventory components.
 *
 * Any system that needs to find a player's inventory without a direct reference
 * can query this subsystem. Also provides the entry point for cross-inventory
 * operations such as loot distribution and vendor transactions.
 */
UCLASS()
class RIFTVAULTINVENTORY_API URiftInventorySubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:

    // -- Begin UWorldSubsystem interface
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    // -- End UWorldSubsystem interface

    /**
     * Registers an inventory component with the subsystem.
     * Called automatically by URiftInventoryComponent on BeginPlay.
     */
    void RegisterInventory(URiftInventoryComponent* InventoryComponent);

    /**
     * Unregisters an inventory component.
     * Called automatically by URiftInventoryComponent on EndPlay.
     */
    void UnregisterInventory(URiftInventoryComponent* InventoryComponent);

    /**
     * Returns the inventory component for the given owner actor.
     * Returns null if no inventory is registered for that owner.
     */
    UFUNCTION(BlueprintPure, Category = "RiftVault|Inventory Subsystem")
    URiftInventoryComponent* GetInventoryForOwner(const AActor* Owner) const;

    /**
     * Returns all currently registered inventory components.
     */
    UFUNCTION(BlueprintPure, Category = "RiftVault|Inventory Subsystem")
    TArray<URiftInventoryComponent*> GetAllInventories() const;

private:

    /** Registry of all active inventory components keyed by their owner. */
    UPROPERTY()
    TMap<TObjectPtr<const AActor>, TObjectPtr<URiftInventoryComponent>> KnownInventories;
};
