#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Components/ActorComponent.h"
#include "GameFramework/PlayerState.h"
#include "Data/URiftItemDefinition.h"
#include "Data/URiftContainerDefinition.h"
#include "GameFramework/Containers/URiftContainer.h"
#include "GameFramework/Items/URiftItemInstance.h"
#include "Interfaces/IRiftPersistenceInterface.h"
#include "URiftInventoryComponent.generated.h"

class URiftItemProcessor;

// ------------------------------------------------------------------
// Delegates
// ------------------------------------------------------------------

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRiftContainerAdded, URiftContainer*, Container);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRiftContainerRemoved, URiftContainer*, Container);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnRiftItemAdded, URiftItemInstance*, Item, URiftContainer*, Container);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnRiftItemRemoved, URiftItemInstance*, Item, URiftContainer*, Container);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnRiftItemMoved, URiftItemInstance*, Item, URiftContainer*, FromContainer, URiftContainer*, ToContainer);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRiftInventoryInitialized, bool, bSuccess);

/** Non-dynamic delegate for C++ callers using WaitForInitialized. */
DECLARE_DELEGATE_OneParam(FOnRiftInventoryInitializedDelegate, bool);

/**
 * Manages all inventory containers and items for one player.
 *
 * Lives on APlayerState — not APawn. Permanent, closed decision.
 * Inventory survives pawn death and respawn.
 *
 * All mutations are server-authoritative. In-session operations are
 * synchronous. The initial persistence load is async.
 */
UCLASS(Blueprintable, BlueprintType, ClassGroup=(RiftVault), meta=(BlueprintSpawnableComponent))
class RIFTVAULTINVENTORY_API URiftInventoryComponent : public UActorComponent
{
    GENERATED_BODY()

public:

    UPROPERTY(BlueprintAssignable, Category = "RiftVault|Inventory|Delegates")
    FOnRiftContainerAdded OnContainerAdded;

    UPROPERTY(BlueprintAssignable, Category = "RiftVault|Inventory|Delegates")
    FOnRiftContainerRemoved OnContainerRemoved;

    UPROPERTY(BlueprintAssignable, Category = "RiftVault|Inventory|Delegates")
    FOnRiftItemAdded OnItemAdded;

    UPROPERTY(BlueprintAssignable, Category = "RiftVault|Inventory|Delegates")
    FOnRiftItemRemoved OnItemRemoved;

    UPROPERTY(BlueprintAssignable, Category = "RiftVault|Inventory|Delegates")
    FOnRiftItemMoved OnItemMoved;

    UPROPERTY(BlueprintAssignable, Category = "RiftVault|Inventory|Delegates")
    FOnRiftInventoryInitialized OnInventoryInitialized;

    URiftInventoryComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

    virtual void InitializeComponent() override;
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    virtual bool ReplicateSubobjects(UActorChannel* Channel, FOutBunch* Bunch, FReplicationFlags* RepFlags) override;

    UFUNCTION(BlueprintPure, Category = "RiftVault|Inventory")
    bool IsInitialized() const;

    /**
     * Calls the delegate immediately if the inventory is already initialized.
     * Otherwise binds the delegate to fire when initialization completes.
     * This is the correct way for any system to wait for inventory readiness
     * regardless of whether it registers before or after initialization fires.
     */
    void WaitForInitialized(FOnRiftInventoryInitializedDelegate&& Delegate);

    /**
     * Sets the persistence backend. Must implement IRiftPersistenceInterface.
     * Call before BeginPlay — typically in the owning PlayerState constructor.
     */
    UFUNCTION(BlueprintCallable, Category = "RiftVault|Inventory")
    void SetPersistenceObject(UObject* NewPersistenceObject);

    // ------------------------------------------------------------------
    // Container API
    // ------------------------------------------------------------------

    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "RiftVault|Inventory|Containers")
    FGuid AddContainer(URiftContainerDefinition* ContainerDefinition);

    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "RiftVault|Inventory|Containers")
    void RemoveContainer(URiftContainer* Container);

    UFUNCTION(BlueprintPure, Category = "RiftVault|Inventory|Containers")
    URiftContainer* GetContainerById(const FGuid& ContainerId) const;

    UFUNCTION(BlueprintPure, Category = "RiftVault|Inventory|Containers")
    URiftContainer* GetContainerByTag(FGameplayTag ContainerTag) const;

    UFUNCTION(BlueprintPure, Category = "RiftVault|Inventory|Containers")
    TArray<URiftContainer*> GetAllContainers() const;

    UFUNCTION(BlueprintPure, Category = "RiftVault|Inventory|Containers")
    int32 CountContainers() const;

    // ------------------------------------------------------------------
    // Item API
    // ------------------------------------------------------------------

    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "RiftVault|Inventory|Items")
    FGuid AddItem(URiftItemDefinition* Definition, int32 Quantity = 1);

    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "RiftVault|Inventory|Items")
    FGuid AddItemToContainer(URiftItemDefinition* Definition, URiftContainer* TargetContainer, int32 Quantity = 1);

    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "RiftVault|Inventory|Items")
    void RemoveItem(URiftItemInstance* Item);

    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "RiftVault|Inventory|Items")
    bool MoveItemToSlot(URiftItemInstance* Item, int32 TargetSlotIndex);

    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "RiftVault|Inventory|Items")
    bool MoveItemToContainer(URiftItemInstance* Item, URiftContainer* TargetContainer);

    UFUNCTION(BlueprintPure, Category = "RiftVault|Inventory|Items")
    URiftItemInstance* GetItemById(const FGuid& ItemId) const;

    UFUNCTION(BlueprintPure, Category = "RiftVault|Inventory|Items")
    URiftContainer* GetContainerForItem(const URiftItemInstance* Item) const;

    UFUNCTION(BlueprintPure, Category = "RiftVault|Inventory|Items")
    bool HasItem(const URiftItemDefinition* Definition) const;

    UFUNCTION(BlueprintPure, Category = "RiftVault|Inventory|Items")
    TArray<URiftItemInstance*> GetItemsByDefinition(const URiftItemDefinition* Definition) const;

    UFUNCTION(BlueprintPure, Category = "RiftVault|Inventory|Items")
    TArray<URiftItemInstance*> GetAllItems() const;

    UFUNCTION(BlueprintPure, Category = "RiftVault|Inventory|Items")
    int32 CountItems() const;

protected:

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory")
    TArray<TObjectPtr<URiftContainerDefinition>> DefaultContainers;

    virtual void InitializeInventory();
    virtual void OnInventoryLoaded(bool bSuccess, const FRiftInventorySaveData& SaveData);
    virtual void SaveInventory();

private:

    UPROPERTY()
    TArray<TObjectPtr<URiftContainer>> Containers;

    UPROPERTY()
    TArray<TObjectPtr<URiftItemInstance>> Items;

    UPROPERTY()
    TObjectPtr<UObject> PersistenceObject;

    bool bIsInitialized = false;
    FTimerHandle SaveDebounceHandle;
    TArray<FOnRiftInventoryInitializedDelegate> PendingInitializedDelegates;

    void BroadcastInitialized(bool bSuccess);

    URiftItemInstance* CreateItemInstance(URiftItemDefinition* Definition);
    URiftContainer* CreateContainer(URiftContainerDefinition* Definition);
    URiftContainer* FindBestContainerForItem(const URiftItemInstance* Item) const;
};
