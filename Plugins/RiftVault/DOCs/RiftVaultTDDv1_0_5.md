# RiftVault — Technical Design Document

**Version:** 1.0.5
**Status:** Active
**Author:** RiftVault Development
**Last Updated:** 2026-03-21
**Engine:** Unreal Engine 5 (Mover-compatible)
**Plugin Target:** Reusable across multiple game projects

---

## Changelog

| Version | Date | Summary |
|---|---|---|
| 0.1.0 | 2026-03-12 | Initial draft |
| 1.0.1 | 2026-03-12 | Added Section 15 — Architecture Review post-mortem from implementation attempt 1 |
| 1.0.2 | 2026-03-14 | Renamed "Fragment Memory" → "Item State". Merged RiftVaultEquipmentMutable into RiftVaultEquipment. Reduced to 10 modules. Closed all 7 open architecture questions. |
| 1.0.3 | 2026-03-14 | Added RiftVaultSave module. Moved IRiftPersistenceInterface and FRiftInventorySaveData from RiftVaultCore to RiftVaultSave. Added cross-module UHT rule to Section 18. Added async server fragment pattern note. Added gameplay attribute-driven container capacity note. Total modules now 11. |
| 1.0.4 | 2026-03-17 | Implemented RiftVaultEquipment and RiftVaultLoot. Added per-slot container design. Added FRiftMutableParameter and ERiftMutableParameterType to Core. Added Tag_Rift_Component_BodyMesh and ability activation tags. Updated folder structure, data flow, closed decisions D1–D5, and Architecture Review lessons 18.8–18.12. |
| 1.0.5 | 2026-03-21 | Multiplayer client-side fixes. Added OnRep_Items with PreviousItems diffing to broadcast OnItemAdded/OnItemRemoved on the owning client (pickup flow). Added Server_MoveItemToContainerAtSlot RPC and client-side prediction pattern for drag-drop. Added deferred Mutable UpdateSkeletalMeshAsync via FTimerHandle to coalesce swap operations. Updated Section 6.5 (replication), Section 6.6 (equip flow), Section 7.1 (add item flow), Section 7.2 (equip flow diagram). Added Architecture Review lessons 18.13–18.16. |

---

## Table of Contents

1. [Overview](#1-overview)
2. [Goals and Non-Goals](#2-goals-and-non-goals)
3. [Architecture Overview](#3-architecture-overview)
4. [Module Breakdown](#4-module-breakdown)
   - 4.1 [RiftVaultCore](#41-riftvaultcore)
   - 4.2 [RiftVaultSave](#42-riftvaultsave)
   - 4.3 [RiftVaultInventory](#43-riftvaultinventory)
   - 4.4 [RiftVaultEquipment](#44-riftvaultequipment)
   - 4.5 [RiftVaultLoot](#45-riftvaultloot)
   - 4.6 [RiftVaultCrafting](#46-riftvaultcrafting)
   - 4.7 [RiftVaultEconomy](#47-riftvaulteconomy)
   - 4.8 [RiftVaultDurability](#48-riftvaultdurability)
   - 4.9 [RiftVaultUI](#49-riftvaultui)
   - 4.10 [RiftVaultEditor](#410-riftvaulteditor)
   - 4.11 [RiftVaultTests](#411-riftvaulttests)
5. [Folder Structure](#5-folder-structure)
6. [Core Systems Deep Dive](#6-core-systems-deep-dive)
   - 6.1 [Item Definition and Fragment System](#61-item-definition-and-fragment-system)
   - 6.2 [Item Instance and Item State](#62-item-instance-and-item-state)
   - 6.3 [Container System](#63-container-system)
   - 6.4 [Inventory Component and Processing Queue](#64-inventory-component-and-processing-queue)
   - 6.5 [Replication Strategy](#65-replication-strategy)
   - 6.6 [Equipment and Mutable Integration](#66-equipment-and-mutable-integration)
   - 6.7 [GAS Integration](#67-gas-integration)
   - 6.8 [Persistence Strategy](#68-persistence-strategy)
   - 6.9 [MVVM UI Architecture](#69-mvvm-ui-architecture)
7. [Data Flow Diagrams](#7-data-flow-diagrams)
   - 7.1 [Adding an Item (Pickup Flow)](#71-adding-an-item-pickup-flow)
   - 7.2 [Equipping an Item](#72-equipping-an-item)
   - 7.3 [Drag-Drop Cross-Container Move](#73-drag-drop-cross-container-move)
   - 7.4 [Crafting an Item](#74-crafting-an-item)
   - 7.5 [Player Respawn Flow](#75-player-respawn-flow)
8. [Naming Conventions](#8-naming-conventions)
9. [Dependency Graph](#9-dependency-graph)
10. [Fragment Reference](#10-fragment-reference)
11. [Fragment Implementation Patterns](#11-fragment-implementation-patterns)
12. [Known Constraints and Decisions](#12-known-constraints-and-decisions)
13. [Closed Architecture Decisions](#13-closed-architecture-decisions)
14. [Build Order](#14-build-order)
15. [Testing Strategy](#15-testing-strategy)
16. [Future Considerations](#16-future-considerations)
17. [Glossary](#17-glossary)
18. [Architecture Review — Lessons from Implementation](#18-architecture-review--lessons-from-implementation)

---

## 1. Overview

RiftVault is a modular, data-driven inventory and equipment plugin for Unreal Engine 5. It is designed to be game-agnostic — meaning it ships no assumptions about genre, camera perspective, or control scheme — and can be dropped into any UE5 C++ project as a plugin.

The first game it will power is a **sci-fi multiplayer title** using **Epic's Mover plugin** for character movement. This context has directly influenced several architectural decisions, most notably:

- All equipment and attachment logic operates at the **APawn level**, not ACharacter, because Mover replaces UCharacterMovementComponent and ACharacter carries too many assumptions about movement that conflict with Mover. **This is a closed, permanent decision — it is never revisited.**
- **Mutable** (Unreal's procedural mesh plugin) is the **sole visual system** for both character equipment and weapons. There are no legacy skeletal mesh swap systems. Mutable integration lives directly inside `RiftVaultEquipment` — there is no separate Mutable module.
- The **URiftInventoryComponent lives on APlayerState**, not APawn, so inventory survives pawn death and respawn cleanly. **This is a closed, permanent decision — it is never revisited.**
- Persistence is handled via a **server-side backend interface** defined in `RiftVaultSave`, not local save files, to support a dedicated server multiplayer setup.

---

## 2. Goals and Non-Goals

### Goals

- **Modular by design.** Each system (inventory, equipment, loot, crafting, economy, durability, UI, save) lives in its own module with explicit dependencies.
- **Data-driven.** Item types, container rules, recipes, loot tables, and vendor offers are all defined in DataAssets and DataTables — no hardcoded item logic.
- **Fragment-based extensibility.** Items are defined by composing fragments rather than subclassing.
- **GAS-native.** The plugin is built assuming the Gameplay Ability System is present.
- **Replication-first.** All inventory mutations are server-authoritative.
- **Mover-compatible.** No code assumes ACharacter or UCharacterMovementComponent.
- **Mutable-first equipment.** No legacy SkeletalMesh swap systems exist.
- **Reusable across games.** Persistence, UI, and game-specific logic are behind interfaces.
- **Verbose and well-commented.** All code is written with learning in mind.

### Non-Goals

- **Spatial/grid inventory.** All items are 1x1. No Diablo/Tarkov-style placement.
- **Legacy mesh attachment.** No skeletal mesh swap systems.
- **Client-side inventory prediction.** All mutations are server-authoritative. (UI uses local-call-then-RPC for immediate feedback only — see Section 6.5.)
- **Built-in UI assets.** Blueprint widget assets are game-specific.
- **Built-in item definitions.** RiftVault ships the framework, not the content.
- **ACharacter assumptions.** All pawn-level code uses APawn only.

---

## 3. Architecture Overview

RiftVault is structured as a **layered plugin** with 11 modules organized into 4 tiers.

`RiftVaultSave` was added in v1.0.3 to cleanly own all persistence types (`IRiftPersistenceInterface`, `FRiftInventorySaveData`, serialization structs). This prevents cross-module UHT boundary violations that would occur if these types were referenced in UFUNCTION signatures across modules.

```
┌─────────────────────────────────────────────────────┐
│  Tier 4 — Tooling                                   │
│  RiftVaultUI · RiftVaultEditor · RiftVaultTests     │
├─────────────────────────────────────────────────────┤
│  Tier 3 — Higher-Level Systems                      │
│  RiftVaultCrafting · RiftVaultEconomy               │
│  RiftVaultDurability                                │
├─────────────────────────────────────────────────────┤
│  Tier 2 — Core Gameplay                             │
│  RiftVaultInventory · RiftVaultEquipment            │
│  RiftVaultLoot · RiftVaultSave                      │
├─────────────────────────────────────────────────────┤
│  Tier 1 — Foundation                                │
│  RiftVaultCore                                      │
└─────────────────────────────────────────────────────┘
```

### Key Architectural Patterns

**Fragment Pattern**
Items are defined by composing `URiftItemFragment` subclasses onto a `URiftItemDefinition` DataAsset. No subclassing of items is required.

**Item State Pattern**
Fragments define type-level data. Item States store per-instance runtime data via `TInstancedStruct<FRiftFragmentState>` on `URiftItemInstance`.

**Strategy Pattern**
Loot selectors, attach finders, and loot handlers are interchangeable strategy objects.

**Interface-Driven Boundaries**
Modules communicate via interfaces. Cross-module interface usage in UFUNCTION signatures requires the interface to live in a shared dependency module — see Section 18.

**Server-Authoritative Replication with UI Prediction**
All inventory mutations execute on the server. For drag-drop operations, the client first calls the mutation locally (immediate UI feedback), then sends the Server RPC for authority. The server's replicated result will overwrite any incorrect local state. See Section 6.5.

---

## 4. Module Breakdown

### 4.1 RiftVaultCore

**Type:** Runtime
**Depends on:** Nothing (UE5 engine modules only)
**Purpose:** Pure foundation. Enums, tags, base state structs, interfaces that reference no Inventory types.

**Key classes:**

| Class | Type | Role |
|---|---|---|
| `URiftContainerDefinition` | UDataAsset | Rules for a container — capacity, acceptance tags. |
| `FRiftFragmentState` | Struct (abstract) | Base for all per-instance item state structs. |
| `FRiftStackState` | Struct | Per-instance stack quantity. |
| `FRiftConditionState` | Struct | Per-instance condition and broken flag. |
| `ERiftMutableParameterType` | Enum | Int32, Float, Color — controls which Mutable API is called. |
| `FRiftMutableParameter` | Struct | One Mutable parameter name/value pair. Used in URiftFragment_Equippable. |
| `IRiftInventoryInterface` | UInterface | Advertises that an actor owns a URiftInventoryComponent. |
| `IRiftEquipmentInterface` | UInterface | Advertises that an actor owns a URiftEquipmentComponent. |
| `EItemLifecycle` | Enum | Initializing → Active → PendingRemoval → Removed. |
| `EContainerType` | Enum | PlayerInventory, Equipment, Stash, Vendor, Loot. |
| `FRiftVaultTags` | Tags | All native GameplayTags for RiftVault. |

> **Note:** `IRiftPersistenceInterface` and `FRiftInventorySaveData` live in `RiftVaultSave`, not Core. See Section 18.3 for the reasoning.

---

### 4.2 RiftVaultSave

**Type:** Runtime
**Depends on:** RiftVaultCore
**Purpose:** Owns all persistence types. Exists as a dedicated module so that `IRiftPersistenceInterface` and `FRiftInventorySaveData` can be safely used in UFUNCTION signatures in `RiftVaultInventory` without cross-module UHT violations.

**Key classes:**

| Class | Type | Role |
|---|---|---|
| `FRiftInventorySaveData` | Struct | Backend-agnostic serialized inventory payload. Contains PlayerId and opaque InventoryData bytes. |
| `IRiftPersistenceInterface` | UInterface | Contract between RiftVault and the game's backend. Defines SaveInventory and LoadInventory. |
| `FOnSaveComplete` | Delegate | Broadcast when a save operation completes. |
| `FOnLoadComplete` | Delegate | Broadcast when a load operation completes. |

**Why a separate module?**
`URiftInventoryComponent` in `RiftVaultInventory` needs to reference `FRiftInventorySaveData` and `IRiftPersistenceInterface` in UFUNCTION and UPROPERTY declarations. UHT requires that any type used in a UFUNCTION parameter lives in the same module or a declared dependency. Placing these types in a dedicated `RiftVaultSave` module that both `RiftVaultInventory` and game projects depend on cleanly resolves the boundary.

---

### 4.3 RiftVaultInventory

**Type:** Runtime
**Depends on:** RiftVaultCore, RiftVaultSave
**Purpose:** The heart of the system. Item definitions, fragments, item instances, containers, inventory component, replication, and persistence calls.

`URiftInventoryComponent` lives on `APlayerState`. Permanent, closed decision.

**Key classes:**

| Class | Type | Role |
|---|---|---|
| `URiftItemDefinition` | UDataAsset | Item type blueprint. Holds fragment array and fragment cache. |
| `URiftItemFragment` | UObject (abstract) | Base for all fragments. Lives in Inventory because fragment methods take URiftItemInstance* parameters. |
| `URiftItemInstance` | UObject | Runtime item. Holds definition reference and fragment state list. Replicated. |
| `URiftContainer` | UObject | Runtime container. Flat slot array up to capacity. Replicated. Uses `ReplicatedUsing = OnRep_Items` to broadcast add/remove delegates on the owning client. |
| `URiftInventoryComponent` | UActorComponent | Main manager on APlayerState. Owns all instances and containers. |
| `URiftInventoryNetProxy` | UActorComponent | Handles replication concerns separately from logic. |
| `URiftInventorySubsystem` | UWorldSubsystem | World-level registry and cross-inventory operations. |

**Container slot capacity:**
Container capacity will be driven by a GAS gameplay attribute (`URiftAttributeSet_Inventory`). The `URiftContainerDefinition` capacity defaults to 0 — a zero value forces the attribute to always be set up correctly rather than silently falling back to a hardcoded number. This will be implemented alongside the attribute set.

---

### 4.4 RiftVaultEquipment

**Type:** Runtime
**Depends on:** RiftVaultCore, RiftVaultSave, RiftVaultInventory
**Purpose:** Equipment state management and Mutable visual layer. APawn only — never ACharacter.

**Key classes:**

| Class | Type | Role |
|---|---|---|
| `URiftEquipmentComponent` | UActorComponent | Lives on APawn. Tracks equipped items by slot tag. Grants/revokes GAS abilities. Spawns weapon actors. |
| `FRiftEquipmentSlotEntry` | Struct (FFastArraySerializerItem) | One occupied equipment slot — slot tag + item pointer. Replicated. |
| `FRiftEquipmentSlotList` | Struct (FFastArraySerializer) | Replicated array of all occupied slot entries. Owns FastArray callbacks. |
| `URiftAbility_Equip` | UGameplayAbility | Thin GAS wrapper. Reads item from OptionalObject and slot tag from TargetTags. ServerOnly. |
| `URiftAbility_Unequip` | UGameplayAbility | Thin GAS wrapper. Reads slot tag from TargetTags. ServerOnly. |
| `URiftMutableEquipmentComponent` | UActorComponent | Finds body mesh by Rift.Component.BodyMesh ComponentTag. Applies Mutable parameters on equip/unequip. Uses deferred timer to coalesce multiple parameter changes into one UpdateSkeletalMeshAsync call. |
| `ARiftWeaponActor` | AActor | Replicated actor hosting a weapon's UCustomizableSkeletalComponent. Spawned by URiftEquipmentComponent. |
| `URiftMutableWeaponComponent` | UActorComponent | Applies ActiveMutableParameters to ARiftWeaponActor's weapon mesh instance. |
| `ERiftMutableParameterType` | Enum (in Core) | Identifies Mutable parameter value type for URiftMutableEquipmentComponent dispatch. |
| `FRiftMutableParameter` | Struct (in Core) | Name/value pair passed to UCustomizableObjectInstance at equip/unequip. |

#### Per-Slot Container Design

Each equipment slot has its own `URiftContainerDefinition` with capacity 1. The container tag **equals the slot tag** — `URiftEquipmentComponent::EquipItem` calls `GetContainerByTag(SlotTag)` to find and move items.

Container item compatibility queries use the slot tag directly (e.g. `ANY(Rift.Slot.Armor.Head)`). `URiftFragment_Equippable::AppendFragmentTags` automatically emits the `EquipmentSlotTag` so items carry their slot tag and pass the query.

**To add a new equipment slot:**
1. Create a `URiftContainerDefinition` asset — ContainerTag = slot tag, Capacity = 1, Query = `ANY(<SlotTag>)`.
2. Add the asset to `DefaultContainers` on `URiftInventoryComponent`.
3. Add the slot tag to `SupportedSlots` on `URiftEquipmentComponent`.

#### Body Mesh Component Identification (Decision D1)

`URiftMutableEquipmentComponent` finds the `UCustomizableSkeletalComponent` that represents the character body by scanning all components on the owning pawn and checking `ComponentTags` for the FName `"Rift.Component.BodyMesh"` (the string form of `Tag_Rift_Component_BodyMesh`).

In the pawn Blueprint, add `"Rift.Component.BodyMesh"` to the `ComponentTags` array of the `UCustomizableSkeletalComponent` that is the body mesh.

#### Deferred Mutable Update (v1.0.5)

Equip and unequip each call `ScheduleMutableUpdate()` rather than calling `UpdateSkeletalMeshAsync()` directly. `ScheduleMutableUpdate` sets a one-shot timer (`KINDA_SMALL_NUMBER` seconds) that calls `FlushMutableUpdate` — which calls `UpdateSkeletalMeshAsync` once. This means a swap (unequip old item → equip new item = two Mutable parameter writes in the same tick) produces exactly one `UpdateSkeletalMeshAsync` call rather than two competing calls where the first (reset) might shadow the second (apply).

---

### 4.5 RiftVaultLoot

**Type:** Runtime
**Depends on:** RiftVaultCore, RiftVaultInventory
**Purpose:** Loot table evaluation, item selection, and pickup actors.

**Key classes:**

| Class | Type | Role |
|---|---|---|
| `ARiftPickup` | AActor | Placeable/spawnable world pickup. USphereComponent overlap triggers AddItem. Replicated. |
| `URiftPickupComponent` | UActorComponent | Composable pickup logic. Auto-binds to first overlap-generating PrimitiveComponent. Walks to PlayerState for inventory. |
| `URiftLootSelector` | UObject (abstract) | Planned — base for loot selection strategies. Not yet implemented. |
| `URiftLootHandler` | UObject (abstract) | Planned — base for loot delivery strategies. Not yet implemented. |

> **Status:** RiftVaultLoot is currently a minimal stub sufficient to test equipment pickup. Selectors, handlers, and loot tables are planned for a future implementation pass.

---

### 4.6 RiftVaultCrafting

**Type:** Runtime
**Depends on:** RiftVaultCore, RiftVaultInventory
**Purpose:** Recipe-based item crafting.

**Key classes:**

| Class | Type | Role |
|---|---|---|
| `URiftRecipeDefinition` | UDataAsset | Ingredients, results, station tag, prerequisites. |
| `URiftCraftingComponent` | UActorComponent | Manages crafting queue. |
| `URiftAbility_Craft` | UGameplayAbility | Validates, consumes ingredients, spawns results. |
| `FRiftCraftIngredient` | Struct | One input requirement. |
| `FRiftCraftResult` | Struct | One output. |

---

### 4.7 RiftVaultEconomy

**Type:** Runtime
**Depends on:** RiftVaultCore, RiftVaultInventory
**Purpose:** Vendor interactions and currency management via GAS.

**Key classes:**

| Class | Type | Role |
|---|---|---|
| `URiftVendorComponent` | UActorComponent | NPC vendor stock and transaction handling. |
| `URiftOfferEvaluator` | UObject | Calculates final buy/sell prices. |
| `URiftAbility_Buy` | UGameplayAbility | Deducts Wealth attribute, adds item to inventory. |
| `URiftAbility_Sell` | UGameplayAbility | Removes item, adds sell value to Wealth. |
| `URiftAttributeSet_Wealth` | UAttributeSet | GAS attribute set for currency. |
| `FRiftVendorOffer` | Struct | One vendor stock entry. |

---

### 4.8 RiftVaultDurability

**Type:** Runtime
**Depends on:** RiftVaultCore, RiftVaultInventory, RiftVaultEquipment
**Purpose:** Wear and repair via GAS effects.

**Key classes:**

| Class | Type | Role |
|---|---|---|
| `URiftEffect_Wear` | UGameplayEffect | Reduces FRiftConditionState.CurrentCondition. |
| `URiftEffect_Repair` | UGameplayEffect | Restores condition state. |
| `URiftAbility_Repair` | UGameplayAbility | Handles repair action. |
| `FRiftConditionPayload` | Struct | Event payload for condition changes. |

---

### 4.9 RiftVaultUI

**Type:** Runtime
**Depends on:** RiftVaultCore, RiftVaultInventory, RiftVaultEquipment, RiftVaultEconomy
**Purpose:** Full MVVM UI stack. No Blueprint widget assets shipped.

**Three rules, no exceptions:**
1. ViewModels owned by subsystem, not widgets.
2. All ViewModel updates are event-driven — no polling.
3. Cross-object references inside ViewModels use `TWeakObjectPtr`.

---

### 4.10 RiftVaultEditor

**Type:** Editor
**Depends on:** RiftVaultCore, RiftVaultInventory, RiftVaultEquipment
**Purpose:** Designer tooling — factories, asset actions, Details customizations.

---

### 4.11 RiftVaultTests

**Type:** DeveloperTool
**Depends on:** All runtime modules
**Purpose:** Automated tests for silent-failure-risk systems using `DEFINE_SPEC`.

---

## 5. Folder Structure

```
RiftVault/
├── RiftVault.uplugin
└── Source/
    ├── RiftVaultCore/
    │   ├── Public/
    │   │   ├── Data/               ← URiftContainerDefinition
    │   │   ├── Interfaces/         ← IRiftInventoryInterface, IRiftEquipmentInterface
    │   │   ├── Tags/               ← RiftVaultTags.h
    │   │   └── Types/
    │   │       ├── ERiftMutableParameterType.h
    │   │       ├── FRiftMutableParameter.h
    │   │       └── State/          ← FRiftFragmentState, FRiftStackState, FRiftConditionState
    │   └── Private/
    │
    ├── RiftVaultSave/
    │   ├── Public/
    │   │   └── Interfaces/         ← IRiftPersistenceInterface, FRiftInventorySaveData
    │   └── Private/
    │
    ├── RiftVaultInventory/
    │   ├── Public/
    │   │   ├── Components/         ← URiftInventoryComponent, URiftInventoryNetProxy
    │   │   ├── Data/               ← URiftItemDefinition
    │   │   ├── GameFramework/
    │   │   │   ├── Containers/     ← URiftContainer
    │   │   │   ├── Fragments/      ← URiftItemFragment base + all fragment subclasses
    │   │   │   └── Items/          ← URiftItemInstance
    │   │   ├── Interfaces/
    │   │   └── Subsystems/         ← URiftInventorySubsystem
    │   └── Private/
    │
    ├── RiftVaultEquipment/
    │   ├── Public/
    │   │   ├── Abilities/          ← URiftAbility_Equip, URiftAbility_Unequip
    │   │   ├── Actors/             ← ARiftWeaponActor
    │   │   ├── Components/         ← URiftEquipmentComponent, URiftMutableEquipmentComponent, URiftMutableWeaponComponent
    │   │   └── Data/               ← FRiftEquipmentSlotState
    │   └── Private/
    │
    ├── RiftVaultLoot/
    │   ├── Public/
    │   │   ├── Actors/             ← ARiftPickup
    │   │   └── Components/         ← URiftPickupComponent
    │   └── Private/
    │
    ├── RiftVaultCrafting/
    ├── RiftVaultEconomy/
    ├── RiftVaultDurability/
    │
    ├── RiftVaultUI/
    │   ├── Public/
    │   │   ├── DragDrop/
    │   │   ├── Settings/
    │   │   ├── Subsystems/
    │   │   ├── ViewModels/
    │   │   └── Widgets/
    │   └── Private/
    │
    ├── RiftVaultEditor/
    └── RiftVaultTests/
```

---

## 6. Core Systems Deep Dive

### 6.1 Item Definition and Fragment System

`URiftItemDefinition` is a `UDataAsset` subclass that defines what an item *type* is. Both `URiftItemDefinition` and all `URiftItemFragment` subclasses live in `RiftVaultInventory` — fragment methods take `URiftItemInstance*` parameters, placing them in Core would create a circular dependency.

**Finding a fragment:**
```cpp
// Always use the StaticClass form to avoid MSVC C2275/C2059 template parse errors
Cast<URiftFragment_Stack>(Definition->FindFragmentByClass(URiftFragment_Stack::StaticClass()))
```

---

### 6.2 Item Instance and Item State

`URiftItemInstance` holds a definition reference and a `FRiftFragmentStateList` — per-instance state data stored via `TInstancedStruct<FRiftFragmentState>` keyed by fragment class.

**Reading state:**
```cpp
FRiftConditionState* State = ItemInstance->FindState<FRiftConditionState>();
if (State) { float Condition = State->CurrentCondition; }
```

**Writing state (server only):**
```cpp
FRiftConditionState* State = ItemInstance->FindOrAddState<FRiftConditionState>();
State->CurrentCondition = NewValue;
ItemInstance->MarkStateDirty();
```

---

### 6.3 Container System

`URiftContainer` holds a flat pre-allocated array of `URiftItemInstance` pointers. All items are 1x1. Slot index = position. Moving an item to slot 24 from slot 0 is an array swap — a container pre-allocated to 30 slots supports this regardless of how many items are in it.

**Capacity:** Driven by a GAS gameplay attribute. `URiftContainerDefinition` defaults to 0 to force the attribute to be configured. See Future Considerations.

**Null slots:** `RemoveItem` nulls the slot rather than compacting the array, preserving slot identity. `AddItem` scans for null entries first before appending new items.

---

### 6.4 Inventory Component and Processing Queue

`URiftInventoryComponent` lives on `APlayerState`. In-session mutations are synchronous. Initial persistence load is async — the callback feeds items into the synchronous queue when data arrives.

---

### 6.5 Replication Strategy

**Overview:** All inventory mutations are server-authoritative. The owning client receives replicated state only.

**Subobject replication:**
`URiftInventoryComponent::ReplicateSubobjects` manually replicates each `URiftContainer` and `URiftItemInstance` to the owning client. This uses `COND_OwnerOnly` on the properties — other clients receive no inventory data.

**Container items (OnRep_Items — v1.0.5):**
`URiftContainer.Items` is declared with `ReplicatedUsing = OnRep_Items`. When the server adds an item (e.g. via a world pickup), the Items array replicates to the owning client and `OnRep_Items` fires. `OnRep_Items` diffs the current `Items` array against a `PreviousItems` snapshot to determine which items are new (call `OnItemAdded`) and which are gone (call `OnItemRemoved`). This is the mechanism that drives client-side UI updates after pickups — without it, `OnItemAdded` only broadcasts on the server.

```
Key design note: PreviousItems is not replicated — it exists only on the owning client
to enable the diff. It is updated at the end of each OnRep_Items call.
```

**Equipment slots (FastArray):**
`FRiftEquipmentSlotList` on `URiftEquipmentComponent` uses FFastArraySerializer. `PostReplicatedAdd` and `PreReplicatedRemove` callbacks fire on clients when slot entries change, broadcasting `OnItemEquipped` and `OnItemUnequipped` without requiring extra RPCs.

**Drag-drop cross-container moves (client-side prediction — v1.0.5):**
When the player drags an item from one container to another via the UI, the widget:
1. Calls `MoveItemToContainerAtSlot` locally (immediate feedback — updates the local Items array and broadcasts `OnItemMoved` for the UI to respond).
2. Calls `Server_MoveItemToContainerAtSlot` (Server RPC) to execute the authoritative mutation on the server.

The server then replicates the result back. If the local call was wrong (e.g. the item was already moved by another action), the server's replicated state overwrites the local result.

`Server_MoveItemToContainerAtSlot` is declared `UFUNCTION(Server, Reliable)` in `URiftInventoryComponent`. Its implementation simply calls `MoveItemToContainerAtSlot`. It is intentionally thin — all validation lives in `MoveItemToContainerAtSlot`.

> **Important:** `BlueprintAuthorityOnly` on a UFUNCTION only blocks Blueprint callers from calling it on a non-authority machine. C++ can still call a `BlueprintAuthorityOnly` function from anywhere — this is a Blueprint-only guard. See lesson 18.15.

---

### 6.6 Equipment and Mutable Integration

Equipment flow:

```
Player input
  → Send gameplay event Tag_Rift_Ability_Equip (OptionalObject = item, TargetTags = slot tag)
  → URiftAbility_Equip [ServerOnly]
  → URiftEquipmentComponent::EquipItem(Item, SlotTag)
      → Validate: slot supported, slot empty, fragment present, slot tag matches
      → Record FRiftEquipmentSlotEntry in FRiftEquipmentSlotList (replicated)
      → MoveItemToContainer(Item, GetContainerByTag(SlotTag))
      → BroadcastItemEvent(Item, Tag_Rift_Event_Item_Equipped)
      → GrantItemAbilities (ASC->GiveAbility for each ability in fragment)
      → If WeaponSocketName set: SpawnActor<ARiftWeaponActor>, attach to body mesh socket
          → ARiftWeaponActor::SetupForItem → URiftMutableWeaponComponent::ApplyForItem
  → OnItemEquipped broadcast (server + client via FastArray PostReplicatedAdd)
      → URiftMutableEquipmentComponent::OnItemEquipped
          → ApplyMutableParameters(ActiveMutableParameters)
              → ScheduleMutableUpdate()  ← deferred, coalesces multi-write swaps
          → [timer fires next tick] FlushMutableUpdate()
              → UCustomizableObjectInstance::UpdateSkeletalMeshAsync()
```

Drag-drop equip flow (item dragged directly into slot container via UI):
```
Client widget drag-drop
  → MoveItemToContainerAtSlot(Item, SlotContainer, SlotIndex) [local, client-side prediction]
  → Server_MoveItemToContainerAtSlot(Item, SlotContainer, SlotIndex) [Server RPC]
  → [on server] MoveItemToContainerAtSlot
      → OnItemMoved.Broadcast
          → URiftEquipmentComponent::OnInventoryItemMoved
              → Detects move into/out of slot container
              → Calls FinishEquip or FinishUnequip accordingly
```

Body armor and helmets flow through `URiftMutableEquipmentComponent` (pawn's body mesh).
Weapons flow through `URiftMutableWeaponComponent` on `ARiftWeaponActor`.

---

### 6.7 GAS Integration

- Equipping grants abilities from `URiftFragment_Equippable`.
- Wear and repair are `UGameplayEffect` applications.
- Currency is a `URiftAttributeSet_Wealth` attribute.
- `Tag_Rift_Status_Inventory_Busy` gates abilities during queue processing.
- `Tag_Rift_Item_Trait_Broken` is applied when condition reaches zero.

---

### 6.8 Persistence Strategy

`IRiftPersistenceInterface` lives in `RiftVaultSave`. The game implements it against its own backend. `URiftInventoryComponent` calls `LoadInventory` async on init and `SaveInventory` debounced on mutation.

```cpp
// IRiftPersistenceInterface — defined in RiftVaultSave
void SaveInventory(const FRiftInventorySaveData& Data, const FOnSaveComplete& OnComplete);
void LoadInventory(const FString& PlayerId, const FOnLoadComplete& OnComplete);
```

---

### 6.9 MVVM UI Architecture

Three rules govern all ViewModel design:
1. ViewModels owned by `URiftInventoryUISubsystem`, not widgets.
2. All updates are event-driven — no polling.
3. Cross-object references use `TWeakObjectPtr`.

---

## 7. Data Flow Diagrams

### 7.1 Adding an Item (Pickup Flow)

```
World pickup overlap → URiftPickupComponent::TryCollect [Server]
    → URiftInventoryComponent::AddItem(Definition, Quantity) [Server]
    → CreateItemInstance()
    → FindBestContainerForItem()
    → URiftContainer::AddItem()       ← Items array modified on server
    → InitializeFragmentStates() → ActivateFragments()
    → OnItemAdded.Broadcast()         ← server delegates fire here
    → SaveInventory() (debounced)
    → [Items array replicates to owning client]
    → URiftContainer::OnRep_Items() fires on client
        → Diffs Items vs PreviousItems
        → OnItemAdded.Broadcast(NewItem, Container)  ← client UI updates here
        → URiftInventoryGridWidget::OnItemAdded → widget slot filled
```

> **Why OnRep_Items?** Server-side broadcasts of `OnItemAdded` do not reach client widgets. Without `ReplicatedUsing = OnRep_Items`, items added via world pickups (which run entirely server-side) would never appear in the inventory UI on the client. The OnRep diff is the mechanism that bridges the gap.

### 7.2 Equipping an Item

```
Player input → Send gameplay event Tag_Rift_Ability_Equip [Server]
    → URiftAbility_Equip reads item from TriggerEventData.OptionalObject
    → URiftAbility_Equip reads slot tag from TriggerEventData.TargetTags[0]
    → URiftEquipmentComponent::EquipItem(Item, SlotTag)
        → Validate fragment, slot support, slot vacancy
        → MoveItemToContainer(Item, GetContainerByTag(SlotTag))  ← per-slot container
        → BroadcastItemEvent → Tag_Rift_Event_Item_Equipped
        → GrantItemAbilities from URiftFragment_Equippable
        → Spawn ARiftWeaponActor if WeaponSocketName set
    → OnItemEquipped.Broadcast(Item, SlotTag) [server + client via FastArray replication]
        → URiftMutableEquipmentComponent → ApplyMutableParameters → ScheduleMutableUpdate
        → [next tick] FlushMutableUpdate → UCustomizableObjectInstance::UpdateSkeletalMeshAsync
        → ViewModel notified via OnItemEvent → widget updates
```

### 7.3 Drag-Drop Cross-Container Move

```
Player drags item to slot in a different container (e.g. backpack → equipment slot)
    → URiftItemSlotWidget::NativeOnDrop
    → OnItemDropped_Implementation
        → InventoryComponent->MoveItemToContainerAtSlot(Item, TargetContainer, SlotIndex)
            [local, client-side prediction — UI updates immediately]
        → InventoryComponent->Server_MoveItemToContainerAtSlot(Item, TargetContainer, SlotIndex)
            [Server RPC — server authority executes and replicates result]
    → [on server] MoveItemToContainerAtSlot
        → OnItemMoved.Broadcast(Item, FromContainer, ToContainer)
            → URiftEquipmentComponent::OnInventoryItemMoved
                → If ToContainer is a slot container: FinishEquip(Item, SlotTag)
                    → Record FRiftEquipmentSlotEntry → GrantAbilities → SpawnWeaponActor
                    → OnItemEquipped.Broadcast
                → If FromContainer was a slot container: FinishUnequip(Item, SlotTag)
                    → RevokeAbilities → DestroyWeaponActors → OnItemUnequipped.Broadcast
    → [EquipmentSlots replicates to client via FastArray]
    → PostReplicatedAdd/PreReplicatedRemove fire on client
        → OnItemEquipped/OnItemUnequipped broadcast on client
        → URiftMutableEquipmentComponent applies/resets Mutable parameters
```

### 7.4 Crafting an Item

```
Player selects recipe → URiftAbility_Craft [Server]
    → Validate prerequisites
    → Check ingredients present
    → Consume each ingredient (RemoveItem)
    → Add each result (AddItem)
    → OnCraftComplete broadcast → UI via ViewModel
```

### 7.5 Player Respawn Flow

```
APawn destroyed (URiftInventoryComponent on PlayerState survives)
    → New APawn spawned and possessed
    → URiftEquipmentComponent::BeginPlay [Server]
        → OnPawnReady() — checks APawn::GetPlayerState()
        → If PlayerState null: SetTimerForNextTick to retry (possession race guard)
        → Finds URiftInventoryComponent via PlayerState->FindComponentByClass
        → Calls WaitForInitialized (handles both already-initialized and pending cases)
    → OnInventoryReady: binds OnInventoryItemMoved and OnInventoryItemEvent delegates
    → NOTE: Re-equipping items after respawn is game-specific — query the equipment
      container (GetContainerByTag with each slot tag) and call EquipItem for each.
```

---

## 8. Naming Conventions

| Prefix | Meaning | Examples |
|---|---|---|
| `A` | AActor | `ARiftPickup`, `ARiftWeaponActor` |
| `U` | UObject | `URiftItemDefinition`, `URiftInventoryComponent` |
| `F` | Struct | `FRiftLootEntry`, `FRiftEquipmentSlotState` |
| `E` | Enum | `EItemLifecycle`, `EContainerType` |
| `I` | UInterface | `IRiftInventoryInterface` |

**Fragments:** `URiftFragment_<Capability>` — `URiftFragment_Stack`, `URiftFragment_Equippable`
**Item States:** `FRift<Capability>State` — `FRiftStackState`, `FRiftConditionState`
**Abilities:** `URiftAbility_<Verb>` — `URiftAbility_Equip`, `URiftAbility_Craft`
**Effects:** `URiftEffect_<Noun>` — `URiftEffect_Wear`, `URiftEffect_Repair`
**ViewModels:** `URiftViewModel_<Subject>` — `URiftViewModel_Item`, `URiftViewModel_Container`

**Tags:** Declared with `UE_DEFINE_GAMEPLAY_TAG` / `UE_DECLARE_GAMEPLAY_TAG_EXTERN`. Accessed directly as global variables (`Tag_Rift_X`). Never use a singleton struct.

---

## 9. Dependency Graph

```
RiftVaultTests
    └── all runtime modules

RiftVaultUI
    └── RiftVaultEconomy → RiftVaultInventory → RiftVaultSave → RiftVaultCore
    └── RiftVaultEquipment → RiftVaultInventory
    └── RiftVaultInventory
    └── RiftVaultCore

RiftVaultEditor
    └── RiftVaultEquipment → RiftVaultInventory
    └── RiftVaultInventory
    └── RiftVaultCore

RiftVaultDurability
    └── RiftVaultEquipment
    └── RiftVaultInventory
    └── RiftVaultCore

RiftVaultEconomy
    └── RiftVaultInventory
    └── RiftVaultCore

RiftVaultCrafting
    └── RiftVaultInventory
    └── RiftVaultCore

RiftVaultLoot
    └── RiftVaultInventory
    └── RiftVaultCore

RiftVaultEquipment
    └── RiftVaultInventory
    └── RiftVaultSave
    └── RiftVaultCore

RiftVaultInventory
    └── RiftVaultSave
    └── RiftVaultCore

RiftVaultSave
    └── RiftVaultCore

RiftVaultCore
    └── (UE5 engine modules only)
```

---

## 10. Fragment Reference

### URiftFragment_Stack
**State:** `FRiftStackState` — `CurrentQuantity`, `LastQuantity`
**Design note:** `RemoveQuantity` floors at 1. The inventory component must call `RemoveItem` explicitly to destroy the item. This prevents ghost items with zero quantity from lingering in containers.

### URiftFragment_Equippable
**State:** None. Equipment state tracked by `URiftEquipmentComponent`.
**Properties:** `EquipmentSlotTag`, `ActiveMutableParameters`, `HolsteredMutableParameters`
**Auto-emits slot tag:** `AppendFragmentTags` automatically adds `EquipmentSlotTag` to the item's runtime tag container so per-slot container queries (e.g. `ANY(Rift.Slot.Armor.Head)`) accept the item without manual tag configuration.

### URiftFragment_Condition
**State:** `FRiftConditionState` — `CurrentCondition`, `bIsBroken`, `LastCondition`
**Properties:** `MaxCondition`, `DegradationTags`, `ConditionPerEvent`, `BrokenMutableParameters`

### URiftFragment_Display
**State:** None.
**Properties:** `DisplayName`, `Description`, `Icon`, `RarityTag`, `Quality`

### URiftFragment_Drop
**State:** None.
**Properties:** `DropMesh`, `bDroppable`, `bDeletable`
**Behaviour:** If absent, item cannot be dropped but can be deleted. If `bDroppable`, `URiftDropLibrary::DropItem` spawns or finds a nearby `ARiftPickup`. `OnDragCancelledOutside` in the slot widget calls this when a drag ends outside any widget.

### URiftFragment_Value
**State:** None. Prices modified at runtime by `URiftOfferEvaluator`.
**Properties:** `BaseBuyPrice`, `BaseSellPrice`, `CurrencyTag`, `bCanBeSold`

---

## 11. Fragment Implementation Patterns

### Async Server Lookup in a Fragment

Some fragments may need to call a remote server to fetch item data (e.g. dynamic pricing, server-authoritative stat values) and write the result back into item state.

**The choice you must make when building such a fragment:**

**Option A — Defer activation (clean data, more complexity)**
Keep the item in `EItemLifecycle::Initializing` state until the async response arrives. Only advance to `Active` once the server data is written into state. The processing queue must support deferred activation for this item.

- Pro: Item is never `Active` with incomplete data.
- Con: Adds complexity to the queue. Other systems cannot interact with the item until the server responds.

**Option B — Post-initialize update (simpler, eventual consistency)**
Let the item go `Active` with default state values immediately. When the server responds, update the state via `SaveStateForFragment()`. Systems that read the state get the default until the update arrives.

- Pro: Simple. Queue unchanged. Item is immediately usable.
- Con: There is a window where state data is incomplete. Systems must tolerate default values.

**Recommendation:** Use Option B unless the item is non-functional with default values (e.g. a weapon with 0 damage). Option A is appropriate only when shipping an item with incomplete state would cause gameplay errors.

**The decision is made entirely in the fragment subclass** — `URiftItemFragment`, `URiftItemInstance`, and `URiftInventoryComponent` require no changes for either option.

---

## 12. Known Constraints and Decisions

| Decision | Rationale |
|---|---|
| **No spatial inventory** | Complexity without benefit for most games. Optional module path exists. |
| **Mutable-only visuals** | Eliminates legacy mesh swap edge cases. |
| **APawn not ACharacter** | Mover compatibility. Permanent, closed. |
| **Inventory on APlayerState** | Survives pawn death. Permanent, closed. |
| **No client prediction for mutations** | Inventory is too valuable to rollback. UI uses local-call-then-RPC for immediate visual feedback only — the server result is authoritative. |
| **Persistence behind interface in RiftVaultSave** | Every game has a different backend. Interface lives in RiftVaultSave to avoid cross-module UHT violations. |
| **Currency as GAS attribute** | Replication, clamping, and modification already handled by GAS. |
| **Synchronous mutations, async persistence load** | In-session ops are fast. Only backend I/O justifies async. |
| **ViewModels owned by subsystem** | Widget teardown must not destroy shared ViewModel state. |
| **RiftVaultEquipmentMutable merged into Equipment** | Mutable is required for the target game. Split when pain is real. |
| **Fragments and URiftItemDefinition in Inventory not Core** | Fragment methods take URiftItemInstance* — placing in Core creates circular dependency. |
| **Item State replaces Fragment Memory** | "State" is more expressive game language. |
| **Container capacity defaults to 0** | Forces GAS attribute to always be configured. Never silently falls back to a hardcoded number. |
| **RiftVaultSave is a dedicated module** | IRiftPersistenceInterface and FRiftInventorySaveData must be referenceable in UFUNCTION signatures in RiftVaultInventory without cross-module UHT violations. |
| **Per-slot containers use slot tag as container tag** | Eliminates a separate tag namespace. Slot tag and container tag are the same value — zero duplication. |
| **Body mesh found by ComponentTags FName** | UActorComponent::ComponentTags (TArray<FName>) is the built-in mechanism. Tag value "Rift.Component.BodyMesh" bridges the Gameplay Tag system to component lookup without a wrapper component. |
| **Loot selectors/handlers deferred** | RiftVaultLoot ships as a minimal stub (ARiftPickup + URiftPickupComponent) sufficient for equipment testing. Full loot table evaluation is a future pass. |
| **OnRep_Items with PreviousItems diff** | Server-side OnItemAdded broadcasts do not reach client widgets. OnRep enables the owning client to receive pickup and add events without extra RPCs. |
| **Deferred Mutable UpdateSkeletalMeshAsync** | Equip and unequip in the same tick (swap) would produce two competing UpdateSkeletalMeshAsync calls. Mutable ignores a second call while the first is processing, causing the reset to shadow the apply. Deferring to a timer coalesces both into one call. |

---

## 13. Closed Architecture Decisions

| # | Question | Decision |
|---|---|---|
| 1 | Fragment module placement | **Option B — Fragments in RiftVaultInventory** |
| 2 | URiftItemDefinition direct ref vs interface | **Tabled** |
| 3 | Fragment state storage | **Strongly typed TInstancedStruct** |
| 4 | Equipment on APawn vs ACharacter | **APawn — permanent, closed** |
| 5 | Inventory Component location | **APlayerState — permanent, closed** |
| 6 | Processing queue sync vs async | **Synchronous mutations, async persistence load** |
| 7 | RiftVaultEquipmentMutable separate or merged | **Merged into RiftVaultEquipment** |
| 8 | IRiftPersistenceInterface location | **RiftVaultSave — dedicated module to avoid cross-module UHT violations** |
| D1 | Body mesh component identification | **ComponentTags FName lookup using Tag_Rift_Component_BodyMesh.GetTag().GetTagName()** |
| D2 | Equipment slot container granularity | **Per-slot containers — one URiftContainerDefinition per slot, capacity 1, slot tag = container tag** |
| D3 | Equip/unequip activation mechanism | **Gameplay event (SendGameplayEventToActor) → URiftAbility_Equip / URiftAbility_Unequip, ServerOnly** |
| D4 | Mutable parameter type for equipment | **FRiftMutableParameter in RiftVaultCore — Int32/Float/Color enum dispatch** |
| D5 | Weapon visual layer | **ARiftWeaponActor + URiftMutableWeaponComponent — separate replicated actor per weapon slot** |
| D6 | Client pickup inventory update | **ReplicatedUsing = OnRep_Items with PreviousItems diff — no extra RPC needed** |
| D7 | Drag-drop cross-container move authority | **Client-side prediction (local call + Server RPC) — server result is authoritative** |
| D8 | Mutable UpdateSkeletalMeshAsync timing | **Deferred single-flush via FTimerHandle — coalesces multi-write swaps into one call** |

---

## 14. Build Order

| Step | Module | Key deliverable |
|---|---|---|
| 1 | `RiftVaultCore` | Enums, tags, state structs, interfaces, URiftContainerDefinition |
| 2 | `RiftVaultSave` | IRiftPersistenceInterface, FRiftInventorySaveData, delegates |
| 3 | `RiftVaultInventory` | URiftItemFragment, URiftItemDefinition, URiftItemInstance, URiftContainer, URiftInventoryComponent, URiftInventoryNetProxy |
| 4 | `RiftVaultEquipment` | URiftEquipmentComponent, FRiftEquipmentSlotState, Mutable components, respawn flow |
| 5 | `RiftVaultLoot` | ARiftPickup, URiftPickupComponent, selectors, handlers |
| 6 | `RiftVaultDurability` | Effects, repair ability, broken item flow |
| 7 | `RiftVaultCrafting` | URiftRecipeDefinition, URiftCraftingComponent, URiftAbility_Craft |
| 8 | `RiftVaultEconomy` | URiftVendorComponent, buy/sell abilities, URiftAttributeSet_Wealth |
| 9 | `RiftVaultUI` | ViewModels, subsystem, widget base classes |
| 10 | `RiftVaultEditor` | Factories, customizations |
| 11 | `RiftVaultTests` | Spec files consolidated and run |

---

## 15. Testing Strategy

### High Priority — Silent failure possible

**Stacking:** merge, overflow, reject, split, move preserves quantity.
**Serialization:** full round-trip, state survival, empty inventory, all fragment types.
**Crafting:** success consumes ingredients, failure consumes nothing, bConsumed=false respected.
**Condition:** wear reduces state, zero sets bIsBroken, broken blocks equip, repair restores, persists through serialization.

### Medium Priority

**Economy:** buy/sell correct amounts, insufficient wealth blocked, offer evaluator base case.
**Loot:** DataTable selector produces valid definition, empty table no crash, pickup delivers correctly.
**Replication:** OnRep_Items diff correctly identifies adds and removes. Client UI updates after server-side AddItem.

### Low Priority

**Equipment:** equip grants abilities, unequip removes them, broken item blocked.

---

## 16. Future Considerations

- **Spatial inventory** — `URiftSpatialContainer` subclass, no core changes needed.
- **Item modifiers/enchantments** — `URiftFragment_Modifiers` + GAS attribute application.
- **Crafting stations** — station actor grants tag to nearby ASCs.
- **Rarity stat scaling** — processor step scales fragment values at instance creation.
- **Player trading** — escrow session object, existing move operations sufficient.
- **Mutable cosmetic UI** — `RiftVaultCustomization` module exposing COI parameters.
- **Split RiftVaultEquipmentMutable** — when a game needs equipment without Mutable.
- **Gameplay attribute-driven container capacity** — `URiftAttributeSet_Inventory` with `MaxInventorySlots`. Container queries ASC for capacity. Definition capacity of 0 is the current placeholder.
- **Drop world RPC** — `OnDragCancelledOutside` currently calls `URiftDropLibrary::DropItem` directly from the client. This is correct for standalone/listen-server but needs a Server RPC for dedicated server projects.

---

## 17. Glossary

| Term | Definition |
|---|---|
| **ASC** | Ability System Component (`UAbilitySystemComponent`). |
| **COI** | Customizable Object Instance. Mutable's runtime unique mesh. |
| **DataAsset** | `UDataAsset` subclass. Designer-authored data, no logic. |
| **Fragment** | `URiftItemFragment` subclass. Adds capability to an item by composition. Lives in RiftVaultInventory. |
| **Item State** | `FRiftFragmentState` subclass. Per-instance runtime data. Named `FRift<Capability>State`. |
| **GAS** | Gameplay Ability System. |
| **Item Definition** | `URiftItemDefinition`. DataAsset describing an item type. Lives in RiftVaultInventory. |
| **Item Instance** | `URiftItemInstance`. Runtime UObject for one item in inventory. |
| **Mover** | Epic's movement plugin replacing `UCharacterMovementComponent`. APawn-level. |
| **Mutable** | Epic's procedural mesh plugin. Generates unique skeletal meshes at runtime. |
| **Net Proxy** | `URiftInventoryNetProxy`. Handles replication separately from inventory logic. |
| **OnRep_Items** | RepNotify on `URiftContainer.Items`. Fires on owning client to diff and broadcast add/remove events. |
| **PlayerState** | `APlayerState`. Where `URiftInventoryComponent` lives. Persists across respawns. |
| **Processing Queue** | Serialized operation queue in `URiftInventoryComponent`. Mutations synchronous, persistence load async. |
| **RiftVaultSave** | Dedicated module owning `IRiftPersistenceInterface` and `FRiftInventorySaveData`. |
| **ViewModel** | `UMVVMViewModelBase` subclass. Exposes inventory data to UMG via field notifications. |
| **Widget Controller** | `UObject` mediating widget intent and system operations. |

---

## 18. Architecture Review — Lessons from Implementation

### 18.1 Module Boundary Problem — Fragments vs Items

**What went wrong:** `URiftItemFragment` placed in Core needed `URiftItemInstance*` parameters — circular dependency.
**Resolution:** Fragments and `URiftItemDefinition` both live in `RiftVaultInventory`.

### 18.2 UHT Cross-Module Type Rules

**Rule:** A UFUNCTION in module A cannot take a parameter of type defined in module B unless A's Build.cs declares B as a dependency. **The module dependency graph must be fully designed before writing any UFUNCTION signature.**

**Before writing any UFUNCTION:** confirm every parameter type's home module. If it crosses a boundary not declared in Build.cs, either move the type or create a new module to own it.

### 18.3 IRiftPersistenceInterface Cross-Module Violation

**What went wrong:** `IRiftPersistenceInterface` and `FRiftInventorySaveData` were placed in `RiftVaultCore`. `URiftInventoryComponent` in `RiftVaultInventory` needed to reference them in UFUNCTION signatures and delegate declarations. UHT could not resolve the types at header parse time.

**Resolution:** Dedicated `RiftVaultSave` module owns all persistence types. `RiftVaultInventory` declares `RiftVaultSave` as a dependency. No cross-module boundary in UFUNCTION signatures.

**The pattern going forward:** Any type that needs to appear in a UFUNCTION signature or DECLARE_DYNAMIC_MULTICAST_DELEGATE across multiple modules must live in a shared dependency module that all referencing modules declare in their Build.cs.

### 18.4 BlueprintNativeEvent Wrapper Methods

**Rule:** Only define `FunctionName_Implementation` in `.cpp`. Never define the UHT-generated wrapper — LNK2005 duplicate symbol.

### 18.5 Tag Access Pattern

**Rule:** Use `UE_DEFINE_GAMEPLAY_TAG` / `UE_DECLARE_GAMEPLAY_TAG_EXTERN` globals. No singleton struct — causes `C2653`.

### 18.6 DeveloperSettings Module Dependency

`UDeveloperSettings` requires `"DeveloperSettings"` in Build.cs `PrivateDependencyModuleNames`.

### 18.7 FindFragmentByClass Template Syntax

Use `Cast<UMyFragment>(Definition->FindFragmentByClass(UMyFragment::StaticClass()))` — template form causes MSVC C2275/C2059 when type is only forward declared.

### 18.8 FNativeGameplayTag Does Not Have GetTagName()

**What went wrong:** Called `Tag_Rift_Component_BodyMesh.GetTagName()` — C2039 identifier not found.
**Resolution:** `FNativeGameplayTag` is not `FGameplayTag`. Use `.GetTag()` to get the `FGameplayTag` first: `Tag_Rift_Component_BodyMesh.GetTag().GetTagName()`.

### 18.9 UActorComponent Has No HasAuthority()

**What went wrong:** Called `HasAuthority()` directly inside `URiftInventoryComponent::BroadcastItemEvent` — C3861 identifier not found.
**Resolution:** `HasAuthority()` lives on `AActor`. From a component, use `GetOwner()->HasAuthority()`.

### 18.10 Mutable Headers Require MuCO/ Path Prefix

**What went wrong:** `#include "CustomizableSkeletalComponent.h"` → C1083 cannot open include file.
**Resolution:** Mutable headers live under a `MuCO/` subdirectory. Always use:
- `#include "MuCO/CustomizableSkeletalComponent.h"`
- `#include "MuCO/CustomizableObjectInstance.h"`

The Build.cs module name remains `CustomizableObject`.

### 18.11 const TArray<T>& Return Type Not Supported in UFUNCTION

**What went wrong:** `UFUNCTION(BlueprintPure) FORCEINLINE const TArray<FRiftMutableParameter>& GetActive...` → UHT error: "Inappropriate keyword 'const' on variable of type 'TArray'".
**Resolution:** UHT does not support `const TArray<T>&` as a UFUNCTION return type. Remove the `UFUNCTION` macro. All C++ callers work fine. If Blueprint access is needed, return by value `TArray<T>` (makes a copy).

### 18.12 FGameplayEventData::OptionalObject is const UObject*

**What went wrong:** `Cast<URiftItemInstance>(TriggerEventData->OptionalObject)` returns `const URiftItemInstance*` — cannot assign to `URiftItemInstance*`.
**Resolution:** `OptionalObject` is `TObjectPtr<const UObject>`. Cast preserves const. Use `const_cast` on the Cast result:
```cpp
URiftItemInstance* Item = const_cast<URiftItemInstance*>(Cast<URiftItemInstance>(TriggerEventData->OptionalObject));
```

### 18.13 UFunctions Cannot Take TObjectPtr as a Parameter

**What went wrong:** `OnRep_Items` was declared with `const TArray<TObjectPtr<URiftItemInstance>>& OldItems` as a parameter — UHT error: "UFunctions cannot take a TObjectPtr as a function parameter or return value."
**Resolution:** RepNotify functions that need to compare old vs new state must use the no-parameter form. Store the previous state in a non-replicated `UPROPERTY()` member (e.g. `TArray<TObjectPtr<URiftItemInstance>> PreviousItems`) and update it manually at the end of the RepNotify function.

```cpp
// WRONG — UHT error
UFUNCTION()
void OnRep_Items(const TArray<TObjectPtr<URiftItemInstance>>& OldItems);

// CORRECT — no-parameter form, previous state stored as member
UPROPERTY()
TArray<TObjectPtr<URiftItemInstance>> PreviousItems;

UFUNCTION()
void OnRep_Items();
```

### 18.14 OnInventoryItemMoved Delegate Fires Only on the Machine That Called the Function

**What went wrong:** Cross-container drag-drop was calling `MoveItemToContainerAtSlot` only on the client-side widget. `OnItemMoved` broadcast in that function fired on the client. `URiftEquipmentComponent::OnInventoryItemMoved` was bound on the server only — it never received the client broadcast, so the equip setup never ran.
**Resolution:** Client widgets must send a Server RPC (`Server_MoveItemToContainerAtSlot`) so the mutation runs on the server, where the equipment component is bound. The client also calls the function locally for immediate UI feedback (client-side prediction pattern).

### 18.15 BlueprintAuthorityOnly Does NOT Prevent C++ Calls from Non-Authority

**What went wrong:** Assumption that `BlueprintAuthorityOnly` would prevent erroneous client calls.
**The rule:** `UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)` only prevents Blueprint graph callers from calling the function on a non-authority machine. C++ can still call a `BlueprintAuthorityOnly` function from anywhere — the specifier is a Blueprint-only guard. For C++ authority gating, explicitly check `GetOwner()->HasAuthority()`.

### 18.16 Mutable UpdateSkeletalMeshAsync Race on Item Swap

**What went wrong:** Equipping a new item while an old one was equipped (swap) calls `ResetMutableParameters` then `ApplyMutableParameters` in the same tick — two `UpdateSkeletalMeshAsync` calls. Mutable ignores the second call while the first is still generating, so only the reset (first call) completes. The visual never updates to the new item.
**Resolution:** Both `ApplyMutableParameters` and `ResetMutableParameters` now call `ScheduleMutableUpdate()` rather than `UpdateSkeletalMeshAsync()` directly. `ScheduleMutableUpdate` sets a one-shot timer (`KINDA_SMALL_NUMBER` seconds). Multiple calls within the same tick all reset the same timer handle, so only one `FlushMutableUpdate` fires — producing exactly one `UpdateSkeletalMeshAsync` call per tick regardless of how many parameter writes occurred.

---

*End of RiftVault Technical Design Document v1.0.5*
