#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "FRiftEquipmentSlotState.generated.h"

class URiftItemInstance;
class URiftItemDefinition;
class URiftEquipmentComponent;

/**
 * One occupied equipment slot entry, stored inside FRiftEquipmentSlotList.
 *
 * Inherits from FFastArraySerializerItem so Unreal's Fast TArray Replication
 * can efficiently delta-serialize only changed entries rather than the whole array.
 *
 * Lifecycle:
 *   - An entry is created when URiftEquipmentComponent::EquipItem succeeds on the server.
 *   - The entry is removed when URiftEquipmentComponent::UnequipItem succeeds on the server.
 *   - Clients receive added/removed entries through the FastArray replication callbacks below.
 *
 * Replication note:
 *   The three callback methods (PreReplicatedRemove, PostReplicatedAdd, PostReplicatedChange)
 *   are deliberately defined in URiftEquipmentComponent.cpp rather than a separate .cpp for
 *   this struct. That avoids a circular include: the callbacks need to access the full
 *   URiftEquipmentComponent declaration to fire its delegates, which in turn includes this header.
 */
USTRUCT(BlueprintType)
struct RIFTVAULTEQUIPMENT_API FRiftEquipmentSlotEntry : public FFastArraySerializerItem
{
    GENERATED_BODY()

    /** Default constructor required by TArray and GENERATED_BODY machinery. */
    FRiftEquipmentSlotEntry() = default;

    /**
     * Convenience constructor used by URiftEquipmentComponent::EquipItem to build
     * a new entry in one line before pushing it into FRiftEquipmentSlotList::Entries.
     *
     * @param InSlotTag  The gameplay tag identifying the equipment slot (e.g. Rift.Slot.Weapon.Primary).
     * @param InItem     The item instance that now occupies the slot.
     */
    FRiftEquipmentSlotEntry(FGameplayTag InSlotTag, URiftItemInstance* InItem)
        : SlotTag(InSlotTag), Item(InItem)
    {
    }

    /**
     * Gameplay tag that identifies which equipment slot this entry occupies.
     * Must match one of the pawn's URiftEquipmentComponent::SupportedSlots entries.
     * BlueprintReadOnly — slot tag is set at equip time and never changes for a given entry.
     */
    UPROPERTY(BlueprintReadOnly)
    FGameplayTag SlotTag;

    /**
     * The item instance currently occupying this slot.
     * NOT replicated — items are no longer network subobjects.
     * On the owning client: resolved via TryResolveEquipmentSlotItem (container lookup) after
     *   PostReplicatedAdd fires. May be null briefly if the inventory container has not yet replicated.
     * On non-owning clients: always null — use ItemDefinition for visual data instead.
     * On the server: the live URiftItemInstance pointer.
     */
    UPROPERTY(BlueprintReadOnly, NotReplicated)
    TObjectPtr<URiftItemInstance> Item = nullptr;

    /**
     * The item's definition asset. Replicated to ALL clients (not just the owner) because
     * data assets are always available in content on every machine.
     * Used by URiftMutableEquipmentComponent on simulated proxies where Item is null.
     */
    UPROPERTY(BlueprintReadOnly)
    TObjectPtr<URiftItemDefinition> ItemDefinition = nullptr;

    // ------------------------------------------------------------------
    // FastArray replication callbacks
    // Called automatically by the Unreal replication system on CLIENTS
    // when the server's EquipmentSlots array changes.
    // Implementations live in URiftEquipmentComponent.cpp.
    // ------------------------------------------------------------------

    /**
     * Called on clients just BEFORE a slot entry is removed from the local array
     * due to the server having removed it (i.e., the item was unequipped server-side).
     * We fire the OnItemUnequipped delegate here so client visuals react immediately.
     */
    void PreReplicatedRemove(const struct FRiftEquipmentSlotList& InArraySerializer);

    /**
     * Called on clients just AFTER a new slot entry is added to the local array
     * because the server equipped a new item into this slot.
     * We fire the OnItemEquipped delegate here so client visuals (Mutable, UI, VFX) react.
     */
    void PostReplicatedAdd(const struct FRiftEquipmentSlotList& InArraySerializer);

    /**
     * Called on clients when an existing slot entry changes in-place.
     * In practice equipment entries are add/remove only, so this is a no-op.
     * Retained to satisfy the FastArray interface contract.
     */
    void PostReplicatedChange(const struct FRiftEquipmentSlotList& InArraySerializer);
};

/**
 * Replicated Fast TArray that holds all active equipment slot entries for one pawn.
 *
 * Lives as a UPROPERTY on URiftEquipmentComponent and is registered in
 * GetLifetimeReplicatedProps with DOREPLIFETIME. The FFastArraySerializer base
 * provides efficient delta serialization: only entries that changed since the
 * last server update are sent to each client.
 *
 * FRiftEquipmentSlotEntry's three callbacks fire automatically when the network layer
 * detects additions, removals, or changes — no explicit RPC plumbing needed.
 */
USTRUCT()
struct RIFTVAULTEQUIPMENT_API FRiftEquipmentSlotList : public FFastArraySerializer
{
    GENERATED_BODY()

    FRiftEquipmentSlotList() : OwnerComponent(nullptr) {}

    /**
     * The flat list of currently occupied slot entries.
     * Replicated via NetDeltaSerialize below. Treat as the authoritative slot state.
     */
    UPROPERTY()
    TArray<FRiftEquipmentSlotEntry> Entries;

    /**
     * Non-replicated back-pointer to the URiftEquipmentComponent that owns this list.
     * Required so the FastArray callbacks (PreReplicatedRemove, PostReplicatedAdd) can
     * reach the component's delegates without needing to resolve the outer object each time.
     * Set in URiftEquipmentComponent's constructor; never null on a properly initialised component.
     */
    UPROPERTY(NotReplicated)
    TObjectPtr<URiftEquipmentComponent> OwnerComponent;

    /**
     * Linear search for the entry matching the given slot tag.
     * Expected Entries count is small (typically 2–6 slots), so linear scan is fine.
     *
     * @param SlotTag  The slot tag to look up.
     * @return Pointer into Entries array, or nullptr if the slot is empty.
     */
    FRiftEquipmentSlotEntry*       FindEntryForSlot(FGameplayTag SlotTag);
    const FRiftEquipmentSlotEntry* FindEntryForSlot(FGameplayTag SlotTag) const;

    /**
     * Called by the Unreal replication system to delta-serialize this array.
     * Delegates to FFastArraySerializer::FastArrayDeltaSerialize, which handles
     * tracking item IDs, computing diffs, and invoking the per-entry callbacks.
     * Do not call this manually.
     */
    bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
    {
        return FFastArraySerializer::FastArrayDeltaSerialize<FRiftEquipmentSlotEntry, FRiftEquipmentSlotList>(
            Entries, DeltaParms, *this);
    }
};

/**
 * Tells the Unreal type system that FRiftEquipmentSlotList participates in
 * Fast TArray delta serialization. Without this specialization the DOREPLIFETIME
 * macro would fall back to slow full-array replication on every net update.
 */
template<>
struct TStructOpsTypeTraits<FRiftEquipmentSlotList> : public TStructOpsTypeTraitsBase2<FRiftEquipmentSlotList>
{
    enum { WithNetDeltaSerializer = true };
};
