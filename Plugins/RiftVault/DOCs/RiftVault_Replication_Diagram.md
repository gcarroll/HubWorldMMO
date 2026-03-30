# RiftVault Replication Architecture

## Data Flow Overview

```
                            +----------------------------------+
                            |           SERVER                 |
                            |                                  |
                            |  URiftInventoryComponent         |
                            |  +-- Containers (FastArray)      |
                            |  +-- Items (FastArray) <-- NOT REPLICATED (server-only stack index)
                            |  +-- bIsInitialized              |
                            |       |                          |
                            |  URiftContainer x N (subobject)  |
                            |  +-- ContainerDefinition         |
                            |  +-- Slots (FastArray)           |
                            |      +-- ItemDefinition [R]      |
                            |      +-- Quantity [R]            |
                            |      +-- Item <-- NOT REPLICATED |
                            |                                  |
                            |  URiftItemInstance <-- ENTIRE OBJECT SERVER-ONLY
                            |  +-- ItemId (FGuid)              |
                            |  +-- ItemDefinition              |
                            |  +-- FragmentStates (FastArray)  |
                            |                                  |
                            |  URiftEquipmentComponent         |
                            |  +-- EquipmentSlots (FastArray)  |
                            |      +-- SlotTag [R]             |
                            |      +-- ItemDefinition [R]      |
                            |      +-- Item <-- NOT REPLICATED |
                            |                                  |
                            |  ARiftWeaponActor                |
                            |  +-- ItemInstance [R]            |
                            |                                  |
                            |  ARiftPickup                     |
                            |  +-- ReplicatedDropMesh [R]      |
                            +--------+----------------+--------+
                                     |                |
                        COND_OwnerOnly|                | All Clients
                                     |                |
                    +----------------v--+    +--------v-----------------+
                    |   OWNING CLIENT    |    |   ALL CLIENTS            |
                    |                    |    |                          |
                    |  Containers -----> |    |  EquipmentSlots ------> |
                    |   +-- Slots -----> |    |   +-- ItemDefinition    |
                    |      +-- (Def+Qty) |    |   +-- SlotTag           |
                    |           |        |    |        |                |
                    |    +------v------+ |    |  +-----v---------+     |
                    |    | Reconstruct | |    |  | OnEquipped    |     |
                    |    | ItemInstance| |    |  | Visual Sync   |     |
                    |    | locally     | |    |  | (Mutable)     |     |
                    |    +-------------+ |    |  +---------------+     |
                    |                    |    |                          |
                    |  bIsInitialized -> |    |  WeaponActor ---------->|
                    |   +-- OnRep drains |    |   +-- OnRep updates     |
                    |     pending waits  |    |     weapon visuals      |
                    |                    |    |                          |
                    |  NetProxy -------> |    |  Pickup --------------->|
                    |   (RPC surface)    |    |   +-- OnRep updates     |
                    |                    |    |     mesh visual         |
                    +--------------------+    +--------------------------+
```

`[R]` = Replicated property

---

## Replicated Properties

### URiftInventoryComponent (RiftVaultInventory)

| Property | Type | Condition | OnRep | Notes |
|----------|------|-----------|-------|-------|
| `Containers` | `FRiftContainerList` (FastArray) | `COND_OwnerOnly` | -- | Delta-serialized container list |
| `bIsInitialized` | `bool` | `COND_OwnerOnly` | `OnRep_bIsInitialized()` | Signals async persistence load complete; drains queued WaitForInitialized delegates |
| `Items` | `FRiftItemList` (FastArray) | **Not replicated** | -- | Server-only stack-fill index |

Containers are registered as subobjects via `AddReplicatedSubObject(Container, COND_OwnerOnly)` with `bReplicateUsingRegisteredSubObjectList = true` (Iris-compatible).

### URiftContainer (RiftVaultInventory)

| Property | Type | Condition | Notes |
|----------|------|-----------|-------|
| `ContainerDefinition` | `URiftContainerDefinition*` | `COND_OwnerOnly` | Shared asset reference |
| `Slots` | `FRiftSlotList` (FastArray) | `COND_OwnerOnly` | Delta-serialized slot descriptors |

Slot entry (`FRiftSlotEntry`) contents:
- `ItemDefinition` -- replicated
- `Quantity` -- replicated
- `Item` (`URiftItemInstance*`) -- **NOT replicated** (reconstructed locally on client)

`IsSupportedForNetworking()` returns true (required for subobject replication).

### URiftEquipmentComponent (RiftVaultEquipment)

| Property | Type | Condition | Notes |
|----------|------|-----------|-------|
| `EquipmentSlots` | `FRiftEquipmentSlotList` (FastArray) | All clients | Visible to everyone for Mutable visuals |

Equipment slot entry (`FRiftEquipmentSlotEntry`) contents:
- `SlotTag` -- replicated
- `ItemDefinition` -- replicated (all clients need this for Mutable)
- `Item` (`URiftItemInstance*`) -- **NOT replicated** (owning client resolves via container lookup)

### ARiftWeaponActor (RiftVaultEquipment)

| Property | Type | Condition | OnRep |
|----------|------|-----------|-------|
| `ItemInstance` | `URiftItemInstance*` | All clients | `OnRep_ItemInstance()` -- updates weapon visuals |

Spawned and owned by server, attached to pawn socket.

### ARiftPickup (RiftVaultInventory)

| Property | Type | Condition | OnRep |
|----------|------|-----------|-------|
| `ReplicatedDropMesh` | `UStaticMesh*` | All clients | `OnRep_DropMesh()` -- updates mesh component |

### URiftItemInstance (RiftVaultInventory)

**Not replicated.** Entire object is server-authoritative.

| Property | Type | Notes |
|----------|------|-------|
| `ItemId` | `FGuid` | Server-only identifier |
| `ItemDefinition` | `URiftItemDefinition*` | Server-only |
| `FragmentStates` | `FRiftFragmentStateList` (FastArray) | Server-only persistent state |

Clients reconstruct item instances locally from `(ItemDefinition, Quantity)` slot descriptors via `URiftInventoryComponent::ReconstructItemInstance()`.

### URiftInventoryNetProxy (RiftVaultInventory)

| Property | Type | Condition | Notes |
|----------|------|-----------|-------|
| `OwningInventory` | `URiftInventoryComponent*` | All clients | RPC surface actor |

`bOnlyRelevantToOwner = true`, `NetDormancy = DORM_DormantAll`. Spawned by PlayerController.

---

## Fast Array Serializers

All use `NetDeltaSerialize()` with `TStructOpsTypeTraits<T>::WithNetDeltaSerializer = true`.

| FastArray | Owner | Condition | Callbacks |
|-----------|-------|-----------|-----------|
| `FRiftContainerList` | URiftInventoryComponent | COND_OwnerOnly | -- |
| `FRiftItemList` | URiftInventoryComponent | **Not replicated** | Server-only index |
| `FRiftSlotList` | URiftContainer | COND_OwnerOnly | PostAdd: reconstruct item locally. PostChange: detect swap/qty change. PreRemove: cleanup |
| `FRiftEquipmentSlotList` | URiftEquipmentComponent | All clients | PostAdd: fire OnItemEquipped. PreRemove: fire OnItemUnequipped |
| `FRiftFragmentStateList` | URiftItemInstance | **Not replicated** | Server-only fragment state |

---

## Server RPCs

All on `URiftInventoryComponent`, all `Server, Reliable`.

| RPC | Parameters | Purpose |
|-----|-----------|---------|
| `Server_MoveItemToSlot` | ContainerTag, SourceSlot, TargetSlot | Move within same container (by tag) |
| `Server_MoveItemToContainerAtSlot` | SourceTag, SourceSlot, TargetTag, TargetSlot | Cross-container move (by tag) |
| `Server_MoveItemToSlotByObject` | Container*, SourceSlot, TargetSlot | Move within container (by ptr) |
| `Server_MoveItemToContainerAtSlotByObject` | Source*, SourceSlot, Target*, TargetSlot | Cross-container move (by ptr) |
| `Server_MergeStacks` | SourceTag, SourceSlot, TargetTag, TargetSlot, Quantity | Merge quantity between stacks |
| `Server_DropItem` | ContainerTag, SlotIndex, Radius, PickupClass | Remove + spawn ARiftPickup (by tag) |
| `Server_DropItemByObject` | Container*, SlotIndex, Radius, PickupClass | Remove + spawn ARiftPickup (by ptr) |
| `Server_DeleteItem` | ContainerTag, SlotIndex | Permanently destroy (by tag) |
| `Server_DeleteItemByObject` | Container*, SlotIndex | Permanently destroy (by ptr) |

Dual RPC variants (tag vs object pointer) exist because tag-based is convenient for UI, while object-pointer eliminates ambiguity when multiple containers share the same tag.

---

## Replication Conditions Summary

| Condition | Used For | Effect |
|-----------|----------|--------|
| `COND_OwnerOnly` | Containers, Slots, bIsInitialized | Only owning client receives data |
| `COND_None` (default) | EquipmentSlots, DropMesh, WeaponItemInstance | All clients receive data |

---

## Key Design Decisions

1. **Items never replicate** -- `URiftItemInstance` is fully server-authoritative. Clients reconstruct from `(ItemDefinition, Quantity)` slot descriptors via `ReconstructItemInstance()`. This keeps bandwidth minimal and avoids replicating mutable fragment state.

2. **Inventory is owner-only** -- Your bags are private. Only you see your containers and slots. All inventory data uses `COND_OwnerOnly`.

3. **Equipment is public** -- All clients see what's equipped. This is required for Mutable visual sync and weapon actor spawning. Equipment slots replicate `ItemDefinition` to all clients.

4. **Subobject registration** -- Containers use `AddReplicatedSubObject(Container, COND_OwnerOnly)` with `bReplicateUsingRegisteredSubObjectList = true` (Iris-compatible pattern).

5. **Client-side item reconstruction** -- When a slot replicates with a new `ItemDefinition + Quantity`, the Fast Array `PostReplicatedAdd` callback creates a local `URiftItemInstance` on the owning client. This gives the client a usable item object without ever sending the full instance over the wire.

6. **NetProxy as RPC surface** -- `URiftInventoryNetProxy` is a dormant actor owned by the PlayerController. It exists solely to provide a network channel for Server RPCs, keeping the inventory component's replication clean.
