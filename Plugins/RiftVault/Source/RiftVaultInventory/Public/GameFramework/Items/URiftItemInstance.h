#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UObject/Object.h"
#include "Net/Serialization/FastArraySerializer.h"
#if ENGINE_MINOR_VERSION < 5
#include "InstancedStruct.h"
#else
#include "StructUtils/InstancedStruct.h"
#endif
#include "Types/State/FRiftFragmentState.h"
#include "Data/URiftItemDefinition.h"
#include "GameFramework/Fragments/URiftItemFragment.h"
#include "URiftItemInstance.generated.h"

class URiftInventoryComponent;

/**
 * Stores a fragment's item state and replication metadata for one item instance.
 *
 * This is the entry stored in the FragmentStates fast array on URiftItemInstance.
 * Each entry maps a fragment class to its TInstancedStruct state data.
 */
USTRUCT()
struct FRiftStoredFragmentStateEntry : public FFastArraySerializerItem
{
    GENERATED_BODY()

    /** The fragment class that owns this state. */
    UPROPERTY()
    TSubclassOf<URiftItemFragment> FragmentClass;

    /** The per-instance state data for this fragment. */
    UPROPERTY()
    TInstancedStruct<FRiftFragmentState> State;

    /** Last time this state was changed. Used for replication change detection. */
    UPROPERTY()
    float StateTimestamp = 0.f;

    /** Last timestamp that was replicated. Not replicated itself. */
    UPROPERTY(NotReplicated)
    float LastReplicatedTimestamp = 0.f;
};

/**
 * Fast array container for all fragment states on one item instance.
 * Handles replication via FFastArraySerializer.
 */
USTRUCT()
struct FRiftFragmentStateList : public FFastArraySerializer
{
    GENERATED_BODY()

    FRiftFragmentStateList() : ItemOwner(nullptr) { }
    explicit FRiftFragmentStateList(URiftItemInstance* InOwner) : ItemOwner(InOwner) { }

    /** Returns true if a state exists for the given fragment class. */
    bool HasStateForFragment(const TSubclassOf<URiftItemFragment>& FragmentClass) const;

    /** Finds and returns the state for the given fragment class. Returns false if not found. */
    bool GetStateForFragment(const TSubclassOf<URiftItemFragment>& FragmentClass, TInstancedStruct<FRiftFragmentState>& OutState) const;

    /** Saves or updates the state for the given fragment class. Returns the entry index. */
    int32 SaveStateForFragment(const TSubclassOf<URiftItemFragment>& FragmentClass, const TInstancedStruct<FRiftFragmentState>& State);

    /** Removes the state for the given fragment class. */
    void RemoveStateForFragment(const TSubclassOf<URiftItemFragment>& FragmentClass);

    // -- Begin FFastArraySerializer implementation
    void PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize);
    void PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize);
    void PreReplicatedRemove(const TArrayView<int32> RemovedIndices, int32 FinalSize);
    bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParams);
    // -- End FFastArraySerializer implementation

private:

    UPROPERTY()
    TObjectPtr<URiftItemInstance> ItemOwner;

    UPROPERTY()
    TArray<FRiftStoredFragmentStateEntry> Entries;

    /** Cached index map for O(1) lookup by fragment class. */
    UPROPERTY(Transient, NotReplicated)
    TMap<TSubclassOf<URiftItemFragment>, int32> FragmentClassMap;
};

template<>
struct TStructOpsTypeTraits<FRiftFragmentStateList> : TStructOpsTypeTraitsBase2<FRiftFragmentStateList>
{
    enum { WithNetDeltaSerializer = true };
};

/**
 * Represents one item currently held in a RiftVault inventory.
 *
 * URiftItemInstance is the runtime object for an item. It holds:
 *   - A reference to its URiftItemDefinition (the type — shared across all instances)
 *   - A FRiftFragmentStateList of per-instance states (unique to this instance)
 *   - A unique FGuid identifying this specific item
 *   - Its current EItemLifecycle state
 *
 * Item instances are owned by URiftInventoryComponent on APlayerState.
 * All mutations are server-authoritative — clients receive replicated state only.
 *
 * URiftInventoryComponent is the only class that should create or destroy instances.
 */
UCLASS(BlueprintType, Blueprintable)
class RIFTVAULTINVENTORY_API URiftItemInstance : public UObject
{
    GENERATED_BODY()

public:

    friend URiftInventoryComponent;

    URiftItemInstance(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

    // -- Begin UObject interface
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    virtual bool IsSupportedForNetworking() const override { return true; }
    virtual void GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const;
    // -- End UObject interface

    /** Returns the unique ID for this item instance. */
    UFUNCTION(BlueprintPure, Category = "RiftVault|Item")
    FORCEINLINE FGuid GetItemId() const { return ItemId; }

    /** Returns the definition (type data) for this item. */
    UFUNCTION(BlueprintPure, Category = "RiftVault|Item")
    FORCEINLINE URiftItemDefinition* GetDefinition() const { return ItemDefinition; }

    /** Returns true if this item's definition matches the given definition. */
    UFUNCTION(BlueprintPure, Category = "RiftVault|Item")
    bool IsSameType(const URiftItemInstance* OtherItem) const;

    /** Returns true if this object has network authority (server or standalone). */
    UFUNCTION(BlueprintPure, Category = "RiftVault|Item")
    bool HasAuthority() const;

    // ------------------------------------------------------------------
    // Fragment queries
    // ------------------------------------------------------------------

    /** Returns true if this item's definition contains a fragment of the given class. */
    UFUNCTION(BlueprintPure, Category = "RiftVault|Item")
    bool HasFragmentOfClass(TSubclassOf<URiftItemFragment> FragmentClass) const;

    /**
     * Finds and returns a fragment by class from this item's definition.
     * Returns null if the fragment is not present.
     * Use the StaticClass form to avoid MSVC template parse errors:
     *   Cast<UMyFragment>(Item->FindFragmentByClass(UMyFragment::StaticClass()))
     */
    UFUNCTION(BlueprintCallable, Category = "RiftVault|Item", meta = (DeterminesOutputType = FragmentClass))
    URiftItemFragment* FindFragmentByClass(TSubclassOf<URiftItemFragment> FragmentClass) const;

    /** Typed template wrapper around FindFragmentByClass for C++ callers. */
    template<typename T>
    T* FindFragment() const
    {
        return Cast<T>(FindFragmentByClass(T::StaticClass()));
    }

    // ------------------------------------------------------------------
    // Fragment state access
    // ------------------------------------------------------------------

    /** Returns true if this item has a state entry for the given fragment class. */
    UFUNCTION(BlueprintPure, Category = "RiftVault|Item")
    bool HasStateForFragment(TSubclassOf<URiftItemFragment> FragmentClass) const;

    /**
     * Retrieves the state for the given fragment class.
     * Returns false if no state exists for that fragment.
     *
     * @param FragmentClass     Fragment whose state to retrieve.
     * @param OutState          The state data if found.
     */
    UFUNCTION(BlueprintPure, Category = "RiftVault|Item")
    bool GetStateForFragment(TSubclassOf<URiftItemFragment> FragmentClass, TInstancedStruct<FRiftFragmentState>& OutState) const;

    /**
     * Saves or updates the state for the given fragment class.
     * Must only be called server-side — gate all calls with HasAuthority().
     *
     * @param FragmentClass     Fragment that owns this state.
     * @param State             The state data to save.
     * @return                  Index of the saved entry.
     */
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "RiftVault|Item")
    int32 SaveStateForFragment(TSubclassOf<URiftItemFragment> FragmentClass, const TInstancedStruct<FRiftFragmentState>& State);

    /**
     * Removes the state for the given fragment class.
     * Must only be called server-side.
     */
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "RiftVault|Item")
    void RemoveStateForFragment(TSubclassOf<URiftItemFragment> FragmentClass);

    // ------------------------------------------------------------------
    // Fragment lifecycle (called by URiftInventoryComponent only)
    // ------------------------------------------------------------------

    /** Calls InitializeState on all fragments in the definition. Server only. */
    void InitializeFragmentStates();

    /**
     * Calls InitializeFragmentStates on the client during local item reconstruction.
     * Must be called while bReconstructingLocally is true (set by URiftInventoryComponent
     * friend via ReconstructItemInstance) so SaveStateForFragment bypasses the authority check.
     */
    void InitializeFragmentStatesLocally();

    /** Calls ActivateForItem on all fragments. Called after initialization. Server only. */
    void ActivateFragments();

    /** Calls DeactivateForItem on all fragments. Called before removal. Server only. */
    void DeactivateFragments();

    /**
     * Returns the current stack quantity for this item.
     * Reads from FRiftStackState if the item has URiftFragment_Stack; returns 1 otherwise.
     * Safe to call on both server and client (reads local fragment state).
     */
    int32 GetCurrentQuantity() const;

private:

    /** Unique identifier for this item instance. Server-side only — never sent over the wire. */
    UPROPERTY()
    FGuid ItemId;

    /** The definition describing this item's type. Set once at creation, never changes. */
    UPROPERTY()
    TObjectPtr<URiftItemDefinition> ItemDefinition;

    /** Per-instance state data for each fragment that needs runtime state. Server-authoritative. */
    UPROPERTY()
    FRiftFragmentStateList FragmentStates;

    /**
     * When true, SaveStateForFragment bypasses the HasAuthority() check.
     * Set exclusively by URiftInventoryComponent::ReconstructItemInstance (friend) to allow
     * fragment state initialization during client-side item reconstruction without authority.
     */
    bool bReconstructingLocally = false;

    /** Sets the definition. Called once by URiftInventoryComponent during creation. */
    void SetDefinition(const URiftItemDefinition* NewDefinition);

    /** Sets the unique ID. Called once by URiftInventoryComponent during creation. */
    void SetItemId(const FGuid& NewItemId);
};