# RiftVault — Technical Design Document

**Version:** 1.0.3  
**Status:** Active  
**Author:** RiftVault Development  
**Last Updated:** 2026-03-14  
**Engine:** Unreal Engine 5 (Mover-compatible)  
**Plugin Target:** Reusable across multiple game projects  

---

## Changelog

| Version | Date | Summary |
|---|---|---|
| 0.1.0 | 2026-03-12 | Initial draft |
| 1.0.1 | 2026-03-12 | Added Section 15 — Architecture Review post-mortem from implementation attempt 1 |
| 1.0.2 | 2026-03-14 | Renamed "Fragment Memory" → "Item State". Reduced to 10 modules. Closed all 7 open architecture questions. |
| 1.0.3 | 2026-03-14 | Added RiftVaultSave module. Moved IRiftPersistenceInterface and FRiftInventorySaveData from RiftVaultCore to RiftVaultSave. Added cross-module UHT rule to Section 18. Added async server fragment pattern note. Added gameplay attribute-driven container capacity note. Total modules now 11. |

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
   - 7.1 [Adding an Item](#71-adding-an-item)
   - 7.2 [Equipping an Item](#72-equipping-an-item)
   - 7.3 [Crafting an Item](#73-crafting-an-item)
   - 7.4 [Player Respawn Flow](#74-player-respawn-flow)
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
- **Client-side inventory prediction.** All mutations are server-authoritative.
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

**Server-Authoritative Replication**  
All inventory mutations go through the server. `URiftInventoryNetProxy` manages replication to clients.

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
| `URiftContainer` | UObject | Runtime container. Flat slot array up to capacity. Replicated. |
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
| `URiftEquipmentComponent` | UActorComponent | Lives on APawn. Tracks equipped items by slot tag. Grants/revokes GAS abilities. |
| `FRiftEquipmentSlotState` | Struct | Current state of one equipment slot. |
| `URiftAbility_Equip` | UGameplayAbility | Validates and executes equip. |
| `URiftAbility_Unequip` | UGameplayAbility | Revokes abilities, returns item to inventory. |
| `URiftMutableEquipmentComponent` | UActorComponent | Translates slot changes into Mutable parameter updates. |
| `ARiftWeaponActor` | AActor | Minimal actor hosting a weapon's Mutable instance. |
| `URiftMutableWeaponComponent` | UActorComponent | Manages weapon COI on ARiftWeaponActor. |
| `FRiftMutableParameter` | Struct | Name/value pair passed to a Mutable instance. |

---

### 4.5 RiftVaultLoot

**Type:** Runtime  
**Depends on:** RiftVaultCore, RiftVaultInventory  
**Purpose:** Loot table evaluation, item selection, and pickup actors.

**Key classes:**

| Class | Type | Role |
|---|---|---|
| `ARiftPickup` | AActor | World actor representing items on the ground. |
| `URiftPickupComponent` | UActorComponent | Reusable pickup capability for any actor. |
| `URiftLootSelector` | UObject (abstract) | Base for loot selection strategies. |
| `URiftLootSelector_DataTable` | URiftLootSelector | Weighted random selection from a DataTable. |
| `URiftLootHandler` | UObject (abstract) | Base for loot delivery strategies. |
| `URiftLootHandler_AddToInventory` | URiftLootHandler | Delivers items to URiftInventoryComponent. |
| `FRiftLootEntry` | Struct (FTableRowBase) | One loot DataTable row. |

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
    │   │       └── State/          ← FRiftFragmentState, FRiftStackState, FRiftConditionState
    │   └── Private/
    │       ├── Data/
    │       ├── Interfaces/
    │       ├── Tags/
    │       └── Types/
    │           └── State/
    │
    ├── RiftVaultSave/
    │   ├── Public/
    │   │   └── Interfaces/         ← IRiftPersistenceInterface, FRiftInventorySaveData
    │   └── Private/
    │       └── Interfaces/
    │
    ├── RiftVaultInventory/
    │   ├── Public/
    │   │   ├── Components/         ← URiftInventoryComponent, URiftInventoryNetProxy
    │   │   ├── Data/               ← URiftItemDefinition
    │   │   ├── GameFramework/
    │   │   │   ├── Containers/     ← URiftContainer
    │   │   │   ├── Fragments/      ← URiftItemFragment base + all fragment subclasses
    │   │   │   └── Items/          ← URiftItemInstance, URiftItemProcessor
    │   │   ├── Interfaces/         ← (Inventory-specific interfaces)
    │   │   └── Subsystems/         ← URiftInventorySubsystem
    │   └── Private/
    │       ├── Components/
    │       ├── Data/
    │       ├── GameFramework/
    │       │   ├── Containers/
    │       │   ├── Fragments/
    │       │   └── Items/
    │       ├── Interfaces/
    │       └── Subsystems/
    │
    ├── RiftVaultEquipment/
    │   ├── Public/
    │   │   ├── AbilitySystem/
    │   │   │   └── Abilities/
    │   │   ├── Components/
    │   │   ├── GameFramework/
    │   │   │   └── Actors/
    │   │   └── Interfaces/
    │   └── Private/
    │       ├── AbilitySystem/Abilities/
    │       ├── Components/
    │       ├── GameFramework/Actors/
    │       └── Interfaces/
    │
    ├── RiftVaultLoot/
    │   ├── Public/
    │   │   ├── Components/
    │   │   └── GameFramework/
    │   │       ├── Actors/
    │   │       ├── Handlers/
    │   │       └── Selectors/
    │   └── Private/
    │       ├── Components/
    │       └── GameFramework/
    │           ├── Actors/
    │           ├── Handlers/
    │           └── Selectors/
    │
    ├── RiftVaultCrafting/
    │   ├── Public/
    │   │   ├── AbilitySystem/Abilities/
    │   │   ├── Components/
    │   │   └── Data/
    │   └── Private/
    │       ├── AbilitySystem/Abilities/
    │       ├── Components/
    │       └── Data/
    │
    ├── RiftVaultEconomy/
    │   ├── Public/
    │   │   ├── AbilitySystem/
    │   │   │   ├── Abilities/
    │   │   │   └── Attributes/
    │   │   ├── Components/
    │   │   └── GameFramework/Evaluators/
    │   └── Private/
    │       ├── AbilitySystem/
    │       │   ├── Abilities/
    │       │   └── Attributes/
    │       ├── Components/
    │       └── GameFramework/Evaluators/
    │
    ├── RiftVaultDurability/
    │   ├── Public/
    │   │   ├── AbilitySystem/
    │   │   │   ├── Abilities/
    │   │   │   └── Effects/
    │   │   └── Types/
    │   └── Private/
    │       ├── AbilitySystem/
    │       │   ├── Abilities/
    │       │   └── Effects/
    │       └── Types/
    │
    ├── RiftVaultUI/
    │   ├── Public/
    │   │   ├── DragDrop/
    │   │   ├── Settings/
    │   │   ├── Subsystems/
    │   │   ├── ViewModels/
    │   │   └── Widgets/
    │   └── Private/
    │       ├── DragDrop/
    │       ├── Settings/
    │       ├── Subsystems/
    │       ├── ViewModels/
    │       └── Widgets/
    │
    ├── RiftVaultEditor/
    │   ├── Public/
    │   │   ├── AssetTypeActions/
    │   │   ├── Customizations/
    │   │   └── Factories/
    │   └── Private/
    │       ├── AssetTypeActions/
    │       ├── Customizations/
    │       └── Factories/
    │
    └── RiftVaultTests/
        ├── Public/Tests/
        │   ├── Fixtures/
        │   └── Support/
        └── Private/Specs/
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

---

### 6.4 Inventory Component and Processing Queue

`URiftInventoryComponent` lives on `APlayerState`. In-session mutations are synchronous. Initial persistence load is async — the callback feeds items into the synchronous queue when data arrives.

---

### 6.5 Replication Strategy

Server-authoritative. `URiftInventoryNetProxy` replicates to the owning client. Other clients receive equipment state via `URiftEquipmentComponent`, not full inventory state.

---

### 6.6 Equipment and Mutable Integration

Equipment flow: input → `URiftAbility_Equip` → move item to equipment container → `URiftEquipmentComponent` grants abilities → `FRiftEquipmentSlotChanged` → `URiftMutableEquipmentComponent` updates COI parameters → Mutable async regeneration.

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

### 7.1 Adding an Item

```
Source (loot, pickup, admin)
    → URiftInventoryComponent::AddItem() [Server]
    → CreateItemInstance()
    → FindBestContainerForItem()
    → URiftContainer::AddItem()
    → InitializeFragmentStates() → ActivateFragments()
    → OnItemAdded broadcast
        → URiftInventoryNetProxy → replicates to client
        → URiftViewModel_Container → widget updates
        → SaveInventory() (debounced)
```

### 7.2 Equipping an Item

```
Player input → URiftAbility_Equip [Server]
    → Validate URiftFragment_Equippable present
    → Validate condition > 0
    → URiftInventoryComponent::MoveItemToContainer(EquipmentContainer)
    → URiftEquipmentComponent::OnSlotChanged()
        → Grant abilities from URiftFragment_Equippable
    → FRiftEquipmentSlotChanged broadcast
        → URiftMutableEquipmentComponent → update COI → Mutable async regen
        → URiftViewModel_Equipment → widget updates
```

### 7.3 Crafting an Item

```
Player selects recipe → URiftAbility_Craft [Server]
    → Validate prerequisites
    → Check ingredients present
    → Consume each ingredient (RemoveItem)
    → Add each result (AddItem)
    → OnCraftComplete broadcast → UI via ViewModel
```

### 7.4 Player Respawn Flow

```
APawn destroyed (URiftInventoryComponent on PlayerState survives)
    → New APawn spawned
    → Game calls URiftEquipmentComponent::OnPawnReady()
    → Query PlayerState→URiftInventoryComponent for equipment container
    → Re-apply slot states → re-grant abilities
    → FRiftEquipmentSlotChanged → URiftMutableEquipmentComponent rebuilds COIs
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

### URiftFragment_Equippable
**State:** None. Equipment state tracked by `URiftEquipmentComponent`.  
**Properties:** `EquipmentSlotTag`, `GrantedAbilities`, `ActiveMutableParameters`, `HolsteredMutableParameters`, `WeaponSocketName`

### URiftFragment_Condition
**State:** `FRiftConditionState` — `CurrentCondition`, `bIsBroken`, `LastCondition`  
**Properties:** `MaxCondition`, `DegradationTags`, `ConditionPerEvent`, `BrokenMutableParameters`

### URiftFragment_Display
**State:** None.  
**Properties:** `DisplayName`, `ShortDescription`, `FullDescription`, `Icon`, `RarityTag`

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
| **No client prediction** | Inventory is too valuable to rollback. |
| **Persistence behind interface in RiftVaultSave** | Every game has a different backend. Interface lives in RiftVaultSave to avoid cross-module UHT violations. |
| **Currency as GAS attribute** | Replication, clamping, and modification already handled by GAS. |
| **Synchronous mutations, async persistence load** | In-session ops are fast. Only backend I/O justifies async. |
| **ViewModels owned by subsystem** | Widget teardown must not destroy shared ViewModel state. |
| **RiftVaultEquipmentMutable merged into Equipment** | Mutable is required for the target game. Split when pain is real. |
| **Fragments and URiftItemDefinition in Inventory not Core** | Fragment methods take URiftItemInstance* — placing in Core creates circular dependency. |
| **Item State replaces Fragment Memory** | "State" is more expressive game language. |
| **Container capacity defaults to 0** | Forces GAS attribute to always be configured. Never silently falls back to a hardcoded number. |
| **RiftVaultSave is a dedicated module** | IRiftPersistenceInterface and FRiftInventorySaveData must be referenceable in UFUNCTION signatures in RiftVaultInventory without cross-module UHT violations. |

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

---

*End of RiftVault Technical Design Document v1.0.3*
