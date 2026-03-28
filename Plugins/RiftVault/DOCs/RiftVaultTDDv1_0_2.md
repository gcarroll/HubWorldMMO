# RiftVault — Technical Design Document

**Version:** 1.0.2  
**Status:** Active  
**Author:** RiftVault Development  
**Last Updated:** 2026-03-14  
**Engine:** Unreal Engine 5 (Mover-compatible)  
**Plugin Target:** Reusable across multiple game projects

\---

## Changelog

|Version|Date|Summary|
|-|-|-|
|0.1.0|2026-03-12|Initial draft|
|1.0.1|2026-03-12|Added Section 15 — Architecture Review post-mortem from implementation attempt 1|
|1.0.2|2026-03-14|Merged RiftVaultEquipmentMutable into RiftVaultEquipment. Reduced to 10 modules. Closed all 7 open architecture questions from Section 15. Removed Epic copyright boilerplate placeholders.|

\---

## Table of Contents

1. [Overview](#1-overview)
2. [Goals and Non-Goals](#2-goals-and-non-goals)
3. [Architecture Overview](#3-architecture-overview)
4. [Module Breakdown](#4-module-breakdown)

   * 4.1 [RiftVaultCore](#41-riftvaultcore)
   * 4.2 [RiftVaultInventory](#42-riftvaultinventory)
   * 4.3 [RiftVaultEquipment](#43-riftvaultequipment)
   * 4.4 [RiftVaultLoot](#44-riftvaultloot)
   * 4.5 [RiftVaultCrafting](#45-riftvaultcrafting)
   * 4.6 [RiftVaultEconomy](#46-riftvaulteconomy)
   * 4.7 [RiftVaultDurability](#47-riftvaultdurability)
   * 4.8 [RiftVaultUI](#48-riftvaultui)
   * 4.9 [RiftVaultEditor](#49-riftvaulteditor)
   * 4.10 [RiftVaultTests](#410-riftvaulttests)
5. [Folder Structure](#5-folder-structure)
6. [Core Systems Deep Dive](#6-core-systems-deep-dive)

   * 6.1 [Item Definition and Fragment System](#61-item-definition-and-fragment-system)
   * 6.2 [Item Instance and Item State](#62-item-instance-and-item-state)
   * 6.3 [Container System](#63-container-system)
   * 6.4 [Inventory Component and Processing Queue](#64-inventory-component-and-processing-queue)
   * 6.5 [Replication Strategy](#65-replication-strategy)
   * 6.6 [Equipment and Mutable Integration](#66-equipment-and-mutable-integration)
   * 6.7 [GAS Integration](#67-gas-integration)
   * 6.8 [Persistence Strategy](#68-persistence-strategy)
   * 6.9 [MVVM UI Architecture](#69-mvvm-ui-architecture)
7. [Data Flow Diagrams](#7-data-flow-diagrams)

   * 7.1 [Adding an Item](#71-adding-an-item)
   * 7.2 [Equipping an Item](#72-equipping-an-item)
   * 7.3 [Crafting an Item](#73-crafting-an-item)
   * 7.4 [Player Respawn Flow](#74-player-respawn-flow)
8. [Naming Conventions](#8-naming-conventions)
9. [Dependency Graph](#9-dependency-graph)
10. [Fragment Reference](#10-fragment-reference)
11. [Known Constraints and Decisions](#11-known-constraints-and-decisions)
12. [Closed Architecture Decisions](#12-closed-architecture-decisions)
13. [Build Order](#13-build-order)
14. [Testing Strategy](#14-testing-strategy)
15. [Future Considerations](#15-future-considerations)
16. [Glossary](#16-glossary)
17. [Architecture Review — Lessons from Implementation Attempt 1](#17-architecture-review--lessons-from-implementation-attempt-1)

\---

## 1\. Overview

RiftVault is a modular, data-driven inventory and equipment plugin for Unreal Engine 5. It is designed to be game-agnostic — meaning it ships no assumptions about genre, camera perspective, or control scheme — and can be dropped into any UE5 C++ project as a plugin.

The first game it will power is a **sci-fi multiplayer title** using **Epic's Mover plugin** for character movement. This context has directly influenced several architectural decisions, most notably:

* All equipment and attachment logic operates at the **APawn level**, not ACharacter, because Mover replaces UCharacterMovementComponent and ACharacter carries too many assumptions about movement that conflict with Mover. **This is a closed, permanent decision — it is never revisited.**
* **Mutable** (Unreal's procedural mesh plugin) is the **sole visual system** for both character equipment and weapons. There are no legacy skeletal mesh swap systems. Mutable integration lives directly inside `RiftVaultEquipment` — there is no separate Mutable module.
* The **URiftInventoryComponent lives on APlayerState**, not APawn, so inventory survives pawn death and respawn cleanly. **This is a closed, permanent decision — it is never revisited.**
* Persistence is handled via a **server-side backend interface**, not local save files, to support a dedicated server multiplayer setup.



\---

## 2\. Goals and Non-Goals

### Goals

* **Modular by design.** Each system (inventory, equipment, loot, crafting, economy, durability, UI) lives in its own module with explicit dependencies. A project that only needs inventory and loot can include just those two modules.
* **Data-driven.** Item types, container rules, recipes, loot tables, and vendor offers are all defined in DataAssets and DataTables — no hardcoded item logic.
* **Fragment-based extensibility.** Items are defined by composing fragments rather than subclassing. Adding a new property to an item (e.g. "radiation level") means creating a new fragment, not modifying the item definition class.
* **GAS-native.** The plugin is built assuming the Gameplay Ability System is present. Equipping an item grants abilities. Durability wear is a GameplayEffect. Crafting and buying are GameplayAbilities. Currency is a GameplayAttribute.
* **Replication-first.** All inventory mutations are server-authoritative. Clients receive replicated state and never speculatively mutate inventory data.
* **Mover-compatible.** No code assumes ACharacter or UCharacterMovementComponent. All pawn-level code uses APawn interfaces only.
* **Mutable-first equipment.** Character suits, armor, helmets, and weapons are all driven through Unreal's Mutable plugin. No legacy SkeletalMesh swap systems exist.
* **Reusable across games.** Persistence, UI, and game-specific logic are behind interfaces so the plugin core never has to change between projects.
* **Verbose and well-commented.** All code is written with learning in mind. Every Unreal-specific pattern is explained in comments.

### Non-Goals

* **Spatial/grid inventory.** All items occupy a single 1x1 slot. There is no Diablo-style or Tarkov-style shape-based placement. The complexity of occupancy masks, grid layouts, and spatial queries is explicitly out of scope.
* **Legacy mesh attachment.** No `AttachActorToComponent` equipment actor spawning outside of the `ARiftWeaponActor` (which is itself just a Mutable host). No skeletal mesh swap systems.
* **Client-side inventory prediction.** Inventory changes are not predicted on the client. The authoritative server processes all mutations. This is a deliberate trade-off: inventory data is too valuable to trust client predictions.
* **Built-in UI assets.** RiftVaultUI provides the C++ widget and view model infrastructure. Actual `.uasset` Blueprint widgets are the responsibility of the consuming project.
* **Built-in item definitions.** RiftVault ships no item DataAssets. It ships the framework to create them.
* **ACharacter assumptions.** If code would only work correctly with ACharacter, it does not belong in this plugin.

\---

## 3\. Architecture Overview

RiftVault is structured as a **layered plugin** with 10 modules organized into 4 tiers. Each tier can only depend on tiers below it, never above. This prevents circular dependencies and ensures any module can be included independently.

`RiftVaultEquipmentMutable` has been merged into `RiftVaultEquipment`. Mutable is a required dependency for the target game and the additional module boundary provided no benefit for a single-team project. If a future project needs equipment without Mutable, the split can be made then.

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
│  RiftVaultLoot                                      │
├─────────────────────────────────────────────────────┤
│  Tier 1 — Foundation                                │
│  RiftVaultCore                                      │
└─────────────────────────────────────────────────────┘
```

### Key Architectural Patterns

**Fragment Pattern**  
Items are defined by composing `URiftItemFragment` subclasses onto a `URiftItemDefinition` DataAsset. A sword might have `URiftFragment\_Equippable`, `URiftFragment\_Display`, `URiftFragment\_Value`, and `URiftFragment\_Condition`. A consumable potion might have `URiftFragment\_Stack` and `URiftFragment\_Display`. No subclassing of items is required or encouraged — new item behaviours are new fragments.

Fragments and `URiftItemDefinition` both live in `RiftVaultInventory`. This is intentional — fragments require `URiftItemInstance\*` parameters, which lives in Inventory, so placing them in Core would create a circular dependency. Core holds only pure data types: enums, interfaces, tags, and item state structs.

**Item State Pattern**  
Fragments define the *type-level* data (e.g. "this item type has a max condition of 100"). Item States store the *instance-level* runtime data (e.g. "this specific sword instance currently has 73 condition"). Item States are strongly-typed structs stored via `TInstancedStruct` on `URiftItemInstance` and serialized for persistence. The base struct is `FRiftFragmentState`. Each fragment that needs per-instance data defines its own state subclass following the pattern `FRift<Capability>State` (e.g. `FRiftStackState`, `FRiftConditionState`).

**Strategy Pattern (Selectors, Finders, Handlers)**  
Loot selectors, attach finders, and loot handlers are all interchangeable strategy objects. This means you can swap from a DataTable loot selector to a procedurally-weighted one without changing the loot system. New strategies are new classes, not modifications to existing ones.

**Interface-Driven Boundaries**  
Modules communicate across boundaries via interfaces rather than direct class references wherever possible. This keeps modules independently usable and makes mocking for tests straightforward.

**Server-Authoritative Replication**  
All inventory mutations go through the server. `URiftInventoryNetProxy` manages the replication of inventory state to clients. Clients display what the server tells them — they never speculatively add or remove items.

\---

## 4\. Module Breakdown

### 4.1 RiftVaultCore

**Type:** Runtime  
**Depends on:** Nothing (pure UE5 engine modules only)  
**Purpose:** The foundation layer. Contains pure data types, enums, interfaces, and tags. Has zero knowledge of managers, components, fragments, or item instances. The rule is strict: if a class in RiftVaultCore needs to reference a class from any other RiftVault module, it does not belong here.

**Key classes and their roles:**

|Class|Type|Role|
|-|-|-|
|`URiftContainerDefinition`|UDataAsset|Defines the rules for a container — its max capacity, what tags items must have to be accepted, etc. Does not reference fragments.|
|`FRiftFragmentState`|Struct (abstract)|Base struct for all per-instance item state. Subclassed by each fragment that needs runtime data. Stored via `TInstancedStruct` on `URiftItemInstance`.|
|`IRiftInventoryInterface`|UInterface|Interface any actor or component can implement to advertise that it has an inventory. Used by loot handlers and pickup actors.|
|`IRiftEquipmentInterface`|UInterface|Interface for actors/components that manage equipment state.|
|`IRiftPersistenceInterface`|UInterface|The boundary between RiftVault and the game's backend. Defines `SaveInventory` / `LoadInventory`. The game implements this against its own backend.|
|`EItemLifecycle`|Enum|States an item instance moves through: Initializing → Active → PendingRemoval → Removed.|
|`EContainerType`|Enum|Categorizes containers: PlayerInventory, Equipment, Stash, Vendor, Loot, etc.|
|`FRiftVaultTags`|Tags|Central declaration point for all GameplayTags used by RiftVault. Tags are declared with `UE\_DEFINE\_GAMEPLAY\_TAG` / `UE\_DECLARE\_GAMEPLAY\_TAG\_EXTERN` and accessed directly as global variables — not through a singleton struct.|

**Item State structs defined in Core (pure data, no fragment dependency):**

|Struct|Role|
|-|-|
|`FRiftStackState`|Stores `CurrentQuantity` per item instance.|
|`FRiftConditionState`|Stores `CurrentCondition` and `bIsBroken` per item instance.|

> \*\*Note:\*\* `URiftItemDefinition` and all `URiftItemFragment` subclasses live in `RiftVaultInventory`, not Core. See Section 12 — Closed Architecture Decisions for the reasoning.

\---

### 4.2 RiftVaultInventory

**Type:** Runtime  
**Depends on:** RiftVaultCore  
**Purpose:** The heart of the system. Contains item definitions, all fragment classes, item instances, containers, the inventory component, the processing queue, replication, and the persistence interface calls.

`URiftInventoryComponent` lives on `APlayerState`. This is a permanent, closed decision — inventory survives pawn death and respawn. When a new pawn spawns, it queries the PlayerState's inventory component to re-apply equipment state. The equipment component on the Pawn is ephemeral; the inventory component on PlayerState is persistent for the duration of a session.

**Key classes:**

|Class|Type|Role|
|-|-|-|
|`URiftItemDefinition`|UDataAsset|The "blueprint" for an item type. Authored by designers. Contains a list of fragments that describe the item's capabilities. Lives in Inventory (not Core) because it references `URiftItemFragment`.|
|`URiftItemFragment`|UObject (abstract)|Base class for all fragments. Subclassed to add new item capabilities without modifying `URiftItemDefinition`. Lives in Inventory because fragment methods take `URiftItemInstance\*` parameters.|
|`URiftInventoryComponent`|UActorComponent|The main manager. Lives on `APlayerState`. Owns all `URiftItemInstance` and `URiftContainer` objects. Processes all add/remove/move operations via the async-load / synchronous-mutation queue.|
|`URiftInventorySubsystem`|UWorldSubsystem|World-level subsystem for operations spanning multiple inventories (loot distribution, vendor transactions). Registry so any system can find an inventory by player.|
|`URiftItemInstance`|UObject|A runtime object representing one item in the world. Holds its definition reference and a `TInstancedStruct`-based map of item states keyed by fragment class. Replicated via `URiftInventoryNetProxy`.|
|`URiftContainer`|UObject|A runtime container holding an ordered flat array of item instances. Enforces capacity and acceptance rules from its `URiftContainerDefinition`.|
|`URiftItemProcessor`|UObject|Processes items during initialization — runs fragments in a defined order to apply initial state (e.g. rolling random starting condition).|
|`URiftInventoryNetProxy`|UActorComponent|Handles the replication concern. The inventory component focuses on logic; the net proxy focuses on what data to replicate and how. Lives alongside `URiftInventoryComponent` on PlayerState.|

**The Processing Queue**  
Item operations (add, remove, move, split stack) execute synchronously within the same frame — they are fast and the one-frame ordering guarantee prevents race conditions. The **exception** is the initial inventory load from persistence: loading a saved inventory (potentially 100+ items) is async to avoid blocking the game thread. The async callback feeds items into the synchronous queue once the data arrives. The queue itself is always synchronous; only the persistence boundary is async.

\---

### 4.3 RiftVaultEquipment

**Type:** Runtime  
**Depends on:** RiftVaultCore, RiftVaultInventory  
**Purpose:** Manages the logical state of what is equipped on a Pawn **and** all Mutable-driven visual updates for equipment and weapons. This module absorbed `RiftVaultEquipmentMutable` — Mutable integration is a required dependency for the target game and the separate module boundary provided no benefit.

**Critical design note — Pawn not Character:**  
Every class in this module is written against `APawn` only. There is no `Cast<ACharacter>()`, no assumption of `GetMesh()` returning a `USkeletalMeshComponent`. This is a permanent, closed decision required for Mover compatibility. Every equipment header carries a comment to this effect so it is never relitigated.

**Key classes — Equipment Logic:**

|Class|Type|Role|
|-|-|-|
|`URiftEquipmentComponent`|UActorComponent|Lives on APawn. Tracks what `URiftItemInstance` is in each equipment slot (identified by GameplayTag). Applies and removes GAS abilities when slots change. Notifies `URiftMutableEquipmentComponent` of slot changes.|
|`FRiftEquipmentSlotState`|Struct|Represents the current state of a single equipment slot: which item is equipped, which ability handles are active, and what the visual state is (Active, Holstered, Stowed).|
|`URiftAbility\_Equip`|UGameplayAbility|GAS ability that handles the equip action — validates the item, moves it from inventory into an equipment slot, triggers the Mutable update.|
|`URiftAbility\_Unequip`|UGameplayAbility|GAS ability for unequipping — returns item to inventory, removes granted abilities, triggers the Mutable update.|

**Key classes — Mutable Visual Layer:**

|Class|Type|Role|
|-|-|-|
|`URiftMutableEquipmentComponent`|UActorComponent|Lives on APawn alongside `URiftEquipmentComponent`. Holds a reference to the character's `UCustomizableObjectInstance`. Responds to equipment slot change events and translates them into Mutable parameter updates.|
|`ARiftWeaponActor`|AActor|A minimal actor spawned to hold a weapon's Mutable instance. Has a `UCustomizableSkeletalComponent` and is attached to the pawn's hand socket. All logic lives in `URiftMutableWeaponComponent`.|
|`URiftMutableWeaponComponent`|UActorComponent|Lives on `ARiftWeaponActor`. Holds the weapon's `UCustomizableObjectInstance`. Updates weapon appearance when item condition or cosmetic parameters change.|
|`FRiftMutableParameter`|Struct|A name/value pair passed to a Mutable instance. Abstracts int, float, colour, and mesh parameters so the equipment component does not need to know Mutable's API directly.|

**Respawn handling:**  
`URiftEquipmentComponent::OnPawnReady()` is called by the game when a new pawn is fully initialized. It queries the PlayerState's `URiftInventoryComponent` for items previously in equipment slots and re-applies them. This is the bridge between the ephemeral pawn and the persistent inventory.

\---

### 4.4 RiftVaultLoot

**Type:** Runtime  
**Depends on:** RiftVaultCore, RiftVaultInventory  
**Purpose:** Manages loot table evaluation, item selection, and pickup actors in the world. Fully data-driven via DataTables.

**Key classes:**

|Class|Type|Role|
|-|-|-|
|`ARiftPickup`|AActor|A world actor representing items on the ground. Holds one or more item definitions. Triggers pickup when a player interacts.|
|`URiftPickupComponent`|UActorComponent|Reusable component adding pickup capability to any actor. Handles server-side logic of finding a valid inventory and delivering items.|
|`URiftLootSelector`|UObject (abstract)|Base class for loot selection strategies.|
|`URiftLootSelector\_DataTable`|URiftLootSelector|Selects items from a UDataTable of `FRiftLootEntry` rows. Supports weighted random selection.|
|`URiftLootSelector\_InventoryItems`|URiftLootSelector|Selects items from an existing inventory (e.g. looting a dead enemy's equipped items).|
|`URiftLootHandler`|UObject (abstract)|Base class for loot delivery strategies.|
|`URiftLootHandler\_AddToInventory`|URiftLootHandler|Delivers selected items by adding them to the player's `URiftInventoryComponent`.|
|`FRiftLootEntry`|Struct (FTableRowBase)|A single loot DataTable row: item definition reference, min/max quantity, weight, conditions.|

\---

### 4.5 RiftVaultCrafting

**Type:** Runtime  
**Depends on:** RiftVaultCore, RiftVaultInventory  
**Purpose:** Recipe-based item crafting. Recipes are DataAssets. The crafting ability checks prerequisites, consumes ingredients via `URiftInventoryComponent`, and delivers results.

**Key classes:**

|Class|Type|Role|
|-|-|-|
|`URiftRecipeDefinition`|UDataAsset|Defines a crafting recipe: list of `FRiftCraftIngredient` inputs, list of `FRiftCraftResult` outputs, required station tag, prerequisites.|
|`URiftCraftingComponent`|UActorComponent|Lives on APawn or a crafting station actor. Manages the crafting queue and communicates with `URiftInventoryComponent` to consume/produce items.|
|`URiftAbility\_Craft`|UGameplayAbility|GAS ability that executes a craft. Validates prerequisites, checks inventory for ingredients, consumes them, and spawns results.|
|`FRiftCraftIngredient`|Struct|One input: item definition reference, required quantity, whether consumed or just required.|
|`FRiftCraftResult`|Struct|One output: item definition reference, quantity, optional fragment state overrides.|

\---

### 4.6 RiftVaultEconomy

**Type:** Runtime  
**Depends on:** RiftVaultCore, RiftVaultInventory  
**Purpose:** Vendor interactions and currency management. Currency is a GAS attribute. Transactions are GAS abilities.

**Key classes:**

|Class|Type|Role|
|-|-|-|
|`URiftVendorComponent`|UActorComponent|Lives on an NPC actor. Exposes a list of `FRiftVendorOffer` structs. Can accept items for sale from the player.|
|`URiftOfferEvaluator`|UObject|Calculates final buy/sell prices. Takes reputation, bulk discounts, item condition, and other modifiers into account. Subclassable for game-specific pricing logic.|
|`URiftAbility\_Buy`|UGameplayAbility|Verifies the player has sufficient Wealth attribute, deducts it, and adds the item to inventory.|
|`URiftAbility\_Sell`|UGameplayAbility|Removes item from inventory and adds sell value to the Wealth attribute.|
|`URiftAttributeSet\_Wealth`|UAttributeSet|GAS attribute set containing the Wealth attribute. Lives on the player's AbilitySystemComponent.|
|`FRiftVendorOffer`|Struct|One vendor stock entry: item definition, quantity available (-1 for infinite), base price, price curve reference.|

\---

### 4.7 RiftVaultDurability

**Type:** Runtime  
**Depends on:** RiftVaultCore, RiftVaultInventory, RiftVaultEquipment  
**Purpose:** Drives wear and repair of items through GAS effects. `URiftFragment\_Condition` data lives in RiftVaultInventory, but the GAS effects that modify it live here to keep Inventory free of the full GAS gameplay module dependency.

**Key classes:**

|Class|Type|Role|
|-|-|-|
|`URiftEffect\_Wear`|UGameplayEffect|Applied when equipped items take damage. Modifies the `FRiftConditionState` of relevant item instances.|
|`URiftEffect\_Repair`|UGameplayEffect|Applied during repair. Restores condition state up to the item's maximum.|
|`URiftAbility\_Repair`|UGameplayAbility|Handles the repair action. Can require materials, time, or a repair station tag.|
|`FRiftConditionPayload`|Struct|Event payload broadcast when an item's condition changes. Consumed by `URiftEquipmentComponent` to disable broken item abilities, and by `URiftMutableEquipmentComponent` to update visual wear on the Mutable mesh.|

**Broken item flow:**  
When an item's condition reaches zero, `URiftEquipmentComponent` receives a `FRiftConditionPayload` with `bIsBroken = true`. It immediately removes all abilities granted by that item. The item cannot be re-equipped until repaired. This crosses module boundaries cleanly via the payload event — RiftVaultDurability does not reference `URiftEquipmentComponent` directly.

\---

### 4.8 RiftVaultUI

**Type:** Runtime  
**Depends on:** RiftVaultCore, RiftVaultInventory, RiftVaultEquipment, RiftVaultEconomy  
**Purpose:** The full MVVM UI stack. Provides C++ ViewModels, Widget base classes, Widget Controllers, and drag-and-drop infrastructure. Does not ship any Blueprint widget assets — those are game-specific.

**MVVM Architecture Rules (three rules, no exceptions):**

1. **ViewModels are owned by the subsystem, not by widgets.** `URiftInventoryUISubsystem` (a UGameInstanceSubsystem) owns all ViewModels. Widgets request a ViewModel from the subsystem. A ViewModel survives widget teardown and is shared between multiple widgets showing the same data.
2. **All ViewModel updates are event-driven.** ViewModels do not poll. They bind to delegates on `URiftInventoryComponent` at initialization and update only when relevant events fire. `UE\_MVVM\_SET\_PROPERTY\_VALUE` is used for every property update.
3. **Cross-object references inside ViewModels use `TWeakObjectPtr`.** A ViewModel holding a `TObjectPtr<URiftItemInstance>` keeps that item alive after removal, causing stale display data. `TWeakObjectPtr` allows `IsValid()` checks and clean empty-state display.

**Key classes:**

|Class|Type|Role|
|-|-|-|
|`URiftWidget`|UUserWidget|Base class for all RiftVault UMG widgets. Helpers for finding the ViewModel subsystem and requesting controllers.|
|`URiftContainerWidget`|URiftWidget|Displays one container's item slots. Binds to `URiftViewModel\_Container`.|
|`URiftItemWidget`|URiftWidget|Displays one item slot. Handles click and drag initiation.|
|`URiftDragOperation`|UDragDropOperation|Carries item move data during UMG drag-and-drop. Holds source container tag, source slot index, and item instance reference.|
|`URiftViewModel`|UMVVMViewModelBase|Base class for all RiftVault ViewModels. Common helpers for field notification and weak reference management.|
|`URiftViewModel\_Item`|URiftViewModel|Exposes a single item's display data: name, icon, stack count, condition, rarity.|
|`URiftViewModel\_Container`|URiftViewModel|Exposes a container's slot array as a list of `URiftViewModel\_Item`.|
|`URiftViewModel\_Equipment`|URiftViewModel|Exposes current equipment state by slot — what's equipped, condition, active abilities.|
|`URiftViewModel\_Vendor`|URiftViewModel|Exposes a vendor's offer list and the player's current wealth.|
|`URiftWidgetController`|UObject|Mediates between widget intent and system operations. Keeps widgets free of business logic.|
|`URiftInventoryUISubsystem`|UGameInstanceSubsystem|Owns all ViewModel lifetime. ViewModels are shared across widgets showing the same data.|

\---

### 4.9 RiftVaultEditor

**Type:** Editor  
**Depends on:** RiftVaultCore, RiftVaultInventory, RiftVaultEquipment  
**Purpose:** Improves the authoring experience for designers. Provides right-click-to-create for item and recipe definitions, and custom Details panel customizations for complex properties.

**Key classes:**

|Class|Type|Role|
|-|-|-|
|`URiftFactory\_ItemDefinition`|UFactory|Right-click → New → RiftVault → Item Definition in the Content Browser.|
|`URiftFactory\_RecipeDefinition`|UFactory|Same for crafting recipe DataAssets.|
|`URiftAssetAction\_ItemDefinition`|FAssetTypeActions\_Base|Registers Item Definitions with the Asset Manager. Friendly category name and thumbnail color in the Content Browser.|
|`URiftConditionCustomization`|IPropertyTypeCustomization|Renders condition range as a min-max slider rather than two separate float fields.|
|`URiftEquipmentStateCustomization`|IPropertyTypeCustomization|Renders `FRiftEquipmentSlotState` with a visual slot diagram in the Details panel.|

\---

### 4.10 RiftVaultTests

**Type:** DeveloperTool  
**Depends on:** All runtime modules  
**Purpose:** Automated testing for systems where silent failure is most dangerous. Uses Unreal's `DEFINE\_SPEC` automation framework.

Tests are not written for everything. The guiding principle: write a test where a bug could exist for days before being noticed. Visual bugs do not qualify. Logic bugs in stacking math, save/load serialization, crafting ingredient consumption, and condition state persistence absolutely do.

**Key infrastructure:**

|Class|Role|
|-|-|
|`FRiftWorldFixture`|Spins up a minimal UWorld with a GameInstance. Handles teardown. Foundation all other fixtures build on.|
|`FRiftInventoryFixture`|Builds on FRiftWorldFixture. Adds a test APawn with a PlayerState, ASC, and fully initialized `URiftInventoryComponent`.|
|`FRiftItemFixture`|Helpers for creating test `URiftItemDefinition` assets in-memory without on-disk DataAssets.|
|`ARiftTestPawn`|Minimal APawn subclass with an ASC, used as the test subject.|

\---

## 5\. Folder Structure

RiftVault folder convention: files within a module are organised by **type**, not by feature. This keeps related base classes and implementations discoverable by anyone familiar with the pattern.

```
RiftVault/
├── RiftVault.uplugin
│
├── Source/
│   │
│   ├── RiftVaultCore/
│   │   ├── RiftVaultCore.Build.cs
│   │   ├── Public/
│   │   │   ├── Interfaces/         ← IRiftInventoryInterface, IRiftEquipmentInterface, IRiftPersistenceInterface
│   │   │   ├── State/              ← FRiftFragmentState (base), FRiftStackState, FRiftConditionState
│   │   │   ├── Types/              ← Enums (EItemLifecycle, EContainerType), small payload structs
│   │   │   ├── Data/               ← URiftContainerDefinition
│   │   │   └── Tags/               ← RiftVaultTags.h
│   │   └── Private/
│   │       └── Tags/               ← RiftVaultTags.cpp
│   │
│   ├── RiftVaultInventory/
│   │   ├── RiftVaultInventory.Build.cs
│   │   ├── Public/
│   │   │   ├── Data/               ← URiftItemDefinition
│   │   │   ├── Fragments/          ← URiftItemFragment (base), URiftFragment\_Stack, URiftFragment\_Equippable, URiftFragment\_Condition, URiftFragment\_Display, URiftFragment\_Value
│   │   │   ├── Items/              ← URiftItemInstance, URiftItemProcessor
│   │   │   ├── Containers/         ← URiftContainer
│   │   │   ├── Components/         ← URiftInventoryComponent, URiftInventoryNetProxy
│   │   │   ├── Subsystems/         ← URiftInventorySubsystem
│   │   │   └── Types/              ← Queue entry structs, operation result enums
│   │   └── Private/
│   │
│   ├── RiftVaultEquipment/
│   │   ├── RiftVaultEquipment.Build.cs
│   │   ├── Public/
│   │   │   ├── Abilities/          ← URiftAbility\_Equip, URiftAbility\_Unequip
│   │   │   ├── Actors/             ← ARiftWeaponActor
│   │   │   ├── Components/         ← URiftEquipmentComponent, URiftMutableEquipmentComponent, URiftMutableWeaponComponent
│   │   │   └── Types/              ← FRiftEquipmentSlotState, FRiftMutableParameter, FRiftEquipmentSlotChanged event
│   │   └── Private/
│   │
│   ├── RiftVaultLoot/
│   │   ├── RiftVaultLoot.Build.cs
│   │   ├── Public/
│   │   │   ├── Actors/             ← ARiftPickup
│   │   │   ├── Components/         ← URiftPickupComponent
│   │   │   ├── Selectors/          ← URiftLootSelector (base), URiftLootSelector\_DataTable, URiftLootSelector\_InventoryItems
│   │   │   ├── Handlers/           ← URiftLootHandler (base), URiftLootHandler\_AddToInventory
│   │   │   └── Types/              ← FRiftLootEntry
│   │   └── Private/
│   │
│   ├── RiftVaultCrafting/
│   │   ├── RiftVaultCrafting.Build.cs
│   │   ├── Public/
│   │   │   ├── Abilities/          ← URiftAbility\_Craft
│   │   │   ├── Components/         ← URiftCraftingComponent
│   │   │   ├── Data/               ← URiftRecipeDefinition
│   │   │   └── Types/              ← FRiftCraftIngredient, FRiftCraftResult
│   │   └── Private/
│   │
│   ├── RiftVaultEconomy/
│   │   ├── RiftVaultEconomy.Build.cs
│   │   ├── Public/
│   │   │   ├── Abilities/          ← URiftAbility\_Buy, URiftAbility\_Sell
│   │   │   ├── Attributes/         ← URiftAttributeSet\_Wealth
│   │   │   ├── Components/         ← URiftVendorComponent
│   │   │   ├── Evaluators/         ← URiftOfferEvaluator
│   │   │   └── Types/              ← FRiftVendorOffer
│   │   └── Private/
│   │
│   ├── RiftVaultDurability/
│   │   ├── RiftVaultDurability.Build.cs
│   │   ├── Public/
│   │   │   ├── Abilities/          ← URiftAbility\_Repair
│   │   │   ├── Effects/            ← URiftEffect\_Wear, URiftEffect\_Repair
│   │   │   └── Types/              ← FRiftConditionPayload
│   │   └── Private/
│   │
│   ├── RiftVaultUI/
│   │   ├── RiftVaultUI.Build.cs
│   │   ├── Public/
│   │   │   ├── Controllers/        ← URiftWidgetController
│   │   │   ├── Subsystems/         ← URiftInventoryUISubsystem
│   │   │   ├── ViewModels/         ← URiftViewModel (base), URiftViewModel\_Item, URiftViewModel\_Container, URiftViewModel\_Equipment, URiftViewModel\_Vendor
│   │   │   └── Widgets/            ← URiftWidget (base), URiftContainerWidget, URiftItemWidget, URiftDragOperation
│   │   └── Private/
│   │
│   ├── RiftVaultEditor/
│   │   ├── RiftVaultEditor.Build.cs
│   │   ├── Public/
│   │   │   ├── Customizations/     ← URiftConditionCustomization, URiftEquipmentStateCustomization
│   │   │   └── Factories/          ← URiftFactory\_ItemDefinition, URiftFactory\_RecipeDefinition, URiftAssetAction\_ItemDefinition
│   │   └── Private/
│   │
│   └── RiftVaultTests/
│       ├── RiftVaultTests.Build.cs
│       ├── Public/
│       │   └── Fixtures/           ← FRiftWorldFixture, FRiftInventoryFixture, FRiftItemFixture, ARiftTestPawn
│       └── Private/
│           └── Specs/              ← \*\_spec.cpp test files, one per system
```

\---

## 6\. Core Systems Deep Dive

### 6.1 Item Definition and Fragment System

A `URiftItemDefinition` is a `UDataAsset` subclass — a designer-authored asset that defines what an item *type* is. It is never instantiated directly at runtime; instead, a `URiftItemInstance` is created that holds a reference to its definition.

The definition holds a `TArray<TObjectPtr<URiftItemFragment>>` — an array of fragment objects. Each fragment is a `UObject` subclass that adds a specific capability or data group to the item. The philosophy is **composition over inheritance**: rather than `USwordItem : UWeaponItem : URiftItemDefinition`, you have a `URiftItemDefinition` with `URiftFragment\_Equippable` and `URiftFragment\_Condition` attached.

Both `URiftItemDefinition` and all `URiftItemFragment` subclasses live in `RiftVaultInventory`. This is because fragment methods take `URiftItemInstance\*` as parameters — placing them in Core would require Core to know about Inventory, creating a circular dependency.

**Finding a fragment:**

```cpp
const URiftFragment\_Stack\* StackFragment = ItemDefinition->FindFragment<URiftFragment\_Stack>();
if (StackFragment)
{
    // Item supports stacking
}
```

If the fragment isn't found, the feature simply doesn't apply. A sword without `URiftFragment\_Stack` cannot be stacked — no special case handling required.

\---

### 6.2 Item Instance and Item State

A `URiftItemInstance` is a `UObject` representing one item currently in an inventory. It holds:

1. A `TObjectPtr<URiftItemDefinition>` — a reference to its definition (the type).
2. A `TInstancedStruct`-based collection of **Item States** — the per-instance runtime data, keyed by fragment class.

**Item State** is the separation between type-level data and instance-level data. The definition says "this sword type has a max condition of 100." The instance's `FRiftConditionState` says "this specific sword is currently at 73 condition."

Not all fragments need a state. `URiftFragment\_Display` has no state — the name and icon are the same for every instance of that item type. Only fragments that track per-instance data (condition, stack count, dynamic attributes) define a state struct.

**Reading item state:**

```cpp
FRiftConditionState\* State = ItemInstance->FindState<FRiftConditionState>();
if (State)
{
    float CurrentCondition = State->CurrentCondition;
}
```

**Writing item state (always server-side):**

```cpp
FRiftConditionState\* State = ItemInstance->FindOrAddState<FRiftConditionState>();
State->CurrentCondition = NewValue;
// Notify the replication system that this instance's data has changed
ItemInstance->MarkStateDirty();
```

**Item State naming convention:**  
All state structs follow `FRift<Capability>State`:

* `FRiftStackState` — current stack quantity
* `FRiftConditionState` — current condition and broken flag
* `FRiftEquipmentState` — current equipment visual state (Active/Holstered/Stowed)

The base struct `FRiftFragmentState` provides the `SynchronizeReplicationState()` virtual, called by the Fast Array replication system after broadcasting updates.

\---

### 6.3 Container System

A `URiftContainer` holds an ordered flat array of `URiftItemInstance` pointers up to the capacity defined by its `URiftContainerDefinition`. Containers are owned by `URiftInventoryComponent` and identified by a `FGameplayTag` (e.g. `RiftVault.Container.Backpack`, `RiftVault.Container.Hotbar`).

**Container acceptance rules:**  
Before adding an item, the container checks:

* Does the item have a required tag?
* Is there space?
* If stackable, is there an existing partial stack to merge into?

These checks run during the enqueue phase, before the operation hits the processing queue. A rejected operation returns a result enum immediately.

**Item layout:**  
All items are 1x1. A container with capacity 20 holds up to 20 item instances in a flat array. Moving an item from slot 3 to slot 7 is an array swap — nothing more.

\---

### 6.4 Inventory Component and Processing Queue

`URiftInventoryComponent` lives on `APlayerState`. It is the single source of truth for all items a player owns.

**Queue behaviour:**

* **In-session mutations** (add, remove, move, split stack) are **synchronous**. They are fast single-frame operations. The queue serializes them to prevent race conditions when multiple systems mutate inventory in the same frame.
* **Initial persistence load** is **async**. Loading a saved inventory with 100+ items cannot block the game thread. The async persistence callback delivers items into the synchronous queue once data arrives, so all downstream logic (fragment initialization, replication, UI) remains unchanged.

Each queued operation is:

1. **Validated** — can this operation succeed given the current state?
2. **Executed** — the state change is applied.
3. **Broadcast** — delegates fire to notify ViewModels, equipment component, persistence layer.

\---

### 6.5 Replication Strategy

RiftVault uses a **server-authoritative, client-display** model.

* All inventory mutations originate on the server via Server RPCs from GAS abilities.
* `URiftInventoryNetProxy` replicates inventory state to the owning client.
* Other clients receive equipment state via `URiftEquipmentComponent` replication, not full inventory state.
* Item instances replicate their definition reference and item state data to the owning client only.

**Why a separate NetProxy component?**  
Separating replication concerns from inventory logic keeps `URiftInventoryComponent` focused on logic and makes it easier to adjust replication strategy without touching the core system.

\---

### 6.6 Equipment and Mutable Integration

The flow when an item is equipped:

```
Player triggers equip input
    → URiftAbility\_Equip activates (server)
    → Validates item has URiftFragment\_Equippable
    → Calls URiftInventoryComponent to move item to equipment container
    → URiftEquipmentComponent receives slot-changed event
    → Grants GAS abilities from URiftFragment\_Equippable
    → Broadcasts FRiftEquipmentSlotChanged event
        → URiftMutableEquipmentComponent receives event
        → Extracts FRiftMutableParameter list from URiftFragment\_Equippable
        → Updates UCustomizableObjectInstance parameters
        → Mutable regenerates mesh asynchronously
        → OnMeshUpdated callback fires when complete
```

For weapons specifically:

```
Weapon slot filled
    → URiftEquipmentComponent spawns ARiftWeaponActor
    → Attaches to socket defined in URiftFragment\_Equippable
    → URiftMutableWeaponComponent on weapon actor initializes its own COI
    → Sets initial Mutable parameters from item definition and instance state
    → Mutable generates the weapon mesh
```

\---

### 6.7 GAS Integration

RiftVault assumes a fully configured Ability System Component exists on the player. It does not create or manage the ASC — that is the game's responsibility. RiftVault interacts with the ASC in these ways:

* **Equipping grants abilities.** `URiftFragment\_Equippable` holds a `TArray<TSubclassOf<UGameplayAbility>>`. When equipped, these are granted. When unequipped, the grants are removed using stored `FGameplayAbilitySpecHandle` values.
* **GAS effects drive system state.** Wear, repair, and wealth changes are all `UGameplayEffect` applications.
* **Currency is a GAS attribute.** `URiftAttributeSet\_Wealth` lives on the player's ASC. Buy and sell abilities modify this attribute via effects.
* **Tags gate operations.** `FRiftVaultTags` declares tags like `RiftVault.Status.Inventory.Busy` (set while the queue is processing) and `RiftVault.Item.Broken` (set on items at zero condition).

\---

### 6.8 Persistence Strategy

RiftVault does not ship a persistence implementation. It ships `IRiftPersistenceInterface` in `RiftVaultCore`:

```cpp
class IRiftPersistenceInterface
{
    GENERATED\_BODY()
public:
    virtual void SaveInventory(const FRiftInventorySaveData\& Data,
                               FOnSaveComplete OnComplete) = 0;
    virtual void LoadInventory(const FString\& PlayerId,
                               FOnLoadComplete OnComplete) = 0;
};
```

`URiftInventoryComponent` calls `SaveInventory` when inventory changes (debounced) and `LoadInventory` during PlayerState initialization. `LoadInventory` is async — the callback feeds items into the synchronous processing queue when data arrives.

`FRiftInventorySaveData` is a serializable struct containing all item instances, their definitions, and their item state data. It is backend-agnostic — JSON serializable, binary serializable, or however the game's backend expects it.

\---

### 6.9 MVVM UI Architecture

Three rules govern all ViewModel design. See Section 4.8 for the rules. The underlying reason they exist: the previous `ModularInventory` implementation had repeated failures in the MVVM layer — stale references, over-eager polling, and ViewModels destroyed by widget teardown. These rules exist to prevent every one of those failure modes.

\---

## 7\. Data Flow Diagrams

### 7.1 Adding an Item

```
Source (loot, pickup, admin, etc.)
    │
    ▼
URiftLootHandler::DeliverLoot()  \[Server]
    │
    ▼
URiftInventoryComponent::EnqueueAddItem(Definition, Quantity)
    │
    ▼
URiftItemProcessor::ProcessNewItem()
    ├── Create URiftItemInstance
    ├── Run each fragment's InitializeState()
    └── Return initialized instance
    │
    ▼
URiftContainer::TryAcceptItem()
    ├── Check capacity
    ├── Check acceptance tags
    └── Check for existing partial stack (if stackable)
    │
    ▼
Operation queued → Executed synchronously (next in queue)
    │
    ▼
Item added to container array
    │
    ▼
OnItemAdded delegate broadcast
    ├── URiftInventoryNetProxy  → replicates to client
    ├── URiftViewModel\_Container → field notification → widget updates
    └── IRiftPersistenceInterface::SaveInventory() (debounced)
```

### 7.2 Equipping an Item

```
Player input → URiftAbility\_Equip activates \[Server]
    │
    ▼
Validate: item has URiftFragment\_Equippable?
Validate: target slot tag matches fragment's slot tag?
Validate: item condition > 0 (not broken)?
    │
    ▼
URiftInventoryComponent::MoveItemToContainer(Item, EquipmentContainer)
    │
    ▼
URiftEquipmentComponent::OnSlotChanged(SlotTag, ItemInstance)
    ├── Grant abilities from URiftFragment\_Equippable
    └── Update FRiftEquipmentSlotState
    │
    ▼
FRiftEquipmentSlotChanged event broadcast
    ├── URiftMutableEquipmentComponent
    │       ├── Build FRiftMutableParameter list from fragment + instance state
    │       ├── Apply parameters to UCustomizableObjectInstance
    │       └── Mutable async regeneration begins
    └── URiftViewModel\_Equipment → field notification → widget updates
```

### 7.3 Crafting an Item

```
Player selects recipe → URiftAbility\_Craft activates \[Server]
    │
    ▼
Validate prerequisites (tags, station, level requirements)
    │
    ▼
For each FRiftCraftIngredient:
    └── URiftInventoryComponent::HasItem(Definition, Quantity)?
    │
    ▼
All ingredients present → consume each (EnqueueRemoveItem)
    │
    ▼
For each FRiftCraftResult:
    └── URiftInventoryComponent::EnqueueAddItem(Definition, Quantity)
    │
    ▼
OnCraftComplete delegate broadcast
    └── UI notified via ViewModel
```

### 7.4 Player Respawn Flow

```
APawn destroyed
    │ (URiftInventoryComponent on PlayerState survives)
    ▼
New APawn spawned and possessed by APlayerController
    │
    ▼
Game calls URiftEquipmentComponent::OnPawnReady()
    │
    ▼
Query PlayerState→URiftInventoryComponent for equipment container contents
    │
    ▼
For each occupied equipment slot:
    ├── URiftEquipmentComponent re-applies slot state
    ├── GAS abilities re-granted
    └── FRiftEquipmentSlotChanged broadcast
            └── URiftMutableEquipmentComponent rebuilds Mutable instances
```

\---

## 8\. Naming Conventions

### Class Prefixes

|Prefix|Meaning|Examples|
|-|-|-|
|`A`|AActor subclass|`ARiftPickup`, `ARiftWeaponActor`|
|`U`|UObject subclass|`URiftItemDefinition`, `URiftInventoryComponent`|
|`F`|Struct or plain C++ class|`FRiftLootEntry`, `FRiftEquipmentSlotState`|
|`E`|Enum|`EItemLifecycle`, `EContainerType`|
|`I`|UInterface|`IRiftInventoryInterface`|

### Module Naming

All modules are prefixed `RiftVault`. Classes within modules use `Rift` as the class prefix (not `RiftVault`) to keep class names readable.

### Fragment Naming

All fragment classes follow `URiftFragment\_<Capability>`:

* `URiftFragment\_Stack`
* `URiftFragment\_Equippable`
* `URiftFragment\_Condition`
* `URiftFragment\_Display`
* `URiftFragment\_Value`

### Item State Naming

All item state structs follow `FRift<Capability>State`:

* `FRiftStackState`
* `FRiftConditionState`
* `FRiftEquipmentSlotState`

The base struct is `FRiftFragmentState`.

### Ability Naming

`URiftAbility\_<Verb>`:

* `URiftAbility\_Equip`, `URiftAbility\_Craft`, `URiftAbility\_Buy`, `URiftAbility\_Repair`

### GAS Effect Naming

`URiftEffect\_<Noun>`:

* `URiftEffect\_Wear`, `URiftEffect\_Repair`

### ViewModel Naming

`URiftViewModel\_<Subject>`:

* `URiftViewModel\_Item`, `URiftViewModel\_Container`, `URiftViewModel\_Equipment`, `URiftViewModel\_Vendor`

### Tag Access Pattern

Tags are declared with `UE\_DEFINE\_GAMEPLAY\_TAG` / `UE\_DECLARE\_GAMEPLAY\_TAG\_EXTERN` and accessed directly as global variables (`Tag\_Rift\_X`). There is no singleton struct with a `Get()` method — that pattern causes `C2653` errors with native tags.

\---

## 9\. Dependency Graph

```
RiftVaultTests
    └── RiftVaultUI
    └── RiftVaultEditor
    └── RiftVaultDurability
    └── RiftVaultEconomy
    └── RiftVaultCrafting
    └── RiftVaultLoot
    └── RiftVaultEquipment      ← includes Mutable integration
    └── RiftVaultInventory
    └── RiftVaultCore

RiftVaultUI
    └── RiftVaultEconomy
    └── RiftVaultEquipment
    └── RiftVaultInventory
    └── RiftVaultCore

RiftVaultEditor
    └── RiftVaultEquipment
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

RiftVaultEquipment               ← Mutable merged in here
    └── RiftVaultInventory
    └── RiftVaultCore

RiftVaultInventory
    └── RiftVaultCore

RiftVaultCore
    └── (UE5 engine modules only)
```

\---

## 10\. Fragment Reference

Complete reference of all fragments shipping in the initial version.

### URiftFragment\_Stack

**Module:** RiftVaultInventory  
**Purpose:** Enables an item to stack.

|Property|Type|Description|
|-|-|-|
|`MaxStackSize`|int32|Maximum items sharing one slot. 1 = not stackable.|
|`InitialQuantity`|int32|Quantity when a new instance is created (default 1).|

**Item State:** `FRiftStackState`

|Property|Type|Description|
|-|-|-|
|`CurrentQuantity`|int32|How many of this item are in this stack right now.|

\---

### URiftFragment\_Equippable

**Module:** RiftVaultInventory  
**Purpose:** Marks an item as equippable and defines what happens when equipped.

|Property|Type|Description|
|-|-|-|
|`EquipmentSlotTag`|FGameplayTag|The slot this item occupies when equipped.|
|`GrantedAbilities`|TArray<TSubclassOf<UGameplayAbility>>|Abilities granted to the owner's ASC when equipped.|
|`ActiveMutableParameters`|TArray<FRiftMutableParameter>|Mutable parameters for the Active visual state.|
|`HolsteredMutableParameters`|TArray<FRiftMutableParameter>|Mutable parameters for the Holstered visual state.|
|`WeaponSocketName`|FName|Socket to attach ARiftWeaponActor to. Empty for body equipment.|

**Item State:** None. Equipment state is tracked by `URiftEquipmentComponent`.

\---

### URiftFragment\_Condition

**Module:** RiftVaultInventory  
**Purpose:** Gives an item a wear value that degrades and can be repaired.

|Property|Type|Description|
|-|-|-|
|`MaxCondition`|float|Maximum condition value (e.g. 100.0).|
|`DegradationTags`|TArray<FGameplayTag>|Gameplay event tags that trigger condition loss.|
|`ConditionPerEvent`|float|Condition lost per degradation event.|
|`BrokenMutableParameters`|TArray<FRiftMutableParameter>|Mutable parameters applied when condition reaches zero.|

**Item State:** `FRiftConditionState`

|Property|Type|Description|
|-|-|-|
|`CurrentCondition`|float|Item's current condition. 0 = broken.|
|`bIsBroken`|bool|True when CurrentCondition <= 0.|

\---

### URiftFragment\_Display

**Module:** RiftVaultInventory  
**Purpose:** Display metadata — name, icon, description, rarity.

|Property|Type|Description|
|-|-|-|
|`DisplayName`|FText|Player-facing item name. FText supports localization.|
|`ShortDescription`|FText|One-line tooltip description.|
|`FullDescription`|FText|Longer lore/stat description for the item detail panel.|
|`Icon`|TSoftObjectPtr<UTexture2D>|Soft reference to the item icon. Soft to avoid loading all icons at startup.|
|`RarityTag`|FGameplayTag|Rarity tag for UI color coding.|

**Item State:** None. Display data is identical for all instances of an item type.

\---

### URiftFragment\_Value

**Module:** RiftVaultInventory  
**Purpose:** Defines the economic value of an item for vendor interactions.

|Property|Type|Description|
|-|-|-|
|`BaseBuyPrice`|float|Base price to buy from a vendor.|
|`BaseSellPrice`|float|Base price a vendor pays for this item.|
|`CurrencyTag`|FGameplayTag|Which currency attribute this item trades in.|
|`bCanBeSold`|bool|Whether this item can be sold to vendors.|

**Item State:** None. Prices are modified at runtime by `URiftOfferEvaluator`.

\---

## 11\. Known Constraints and Decisions

|Decision|Rationale|
|-|-|
|**No spatial inventory (all items 1x1)**|Spatial placement adds significant complexity for minimal gameplay benefit in most games. Can be added as an optional module later without changing the core system.|
|**Mutable-only for equipment visuals**|Eliminates the entire legacy mesh swap system. Mutable handles LODs, cloth, physics, and cosmetic customization better than manual mesh attachment.|
|**APawn not ACharacter throughout**|Mover replaces UCharacterMovementComponent and breaks ACharacter assumptions. Building against APawn from the start costs nothing and ensures Mover compatibility.|
|**URiftInventoryComponent on APlayerState**|Inventory must survive pawn death. PlayerState is the correct place for persistent per-player data in a dedicated server setup.|
|**No client-side prediction for inventory**|Inventory data is authoritative and valuable. Rollbacks on misprediction would be jarring. The synchronous processing delay is imperceptible.|
|**Persistence behind an interface**|Every game has a different backend. Hardcoding a save implementation would make the plugin useless for online projects.|
|**Currency as GAS attribute**|GAS already provides replication, clamping, and effect-based modification. A parallel currency system would duplicate all of that.|
|**Synchronous mutations, async persistence load**|In-session operations are fast and need ordering guarantees. Only the initial load from a remote backend justifies async to avoid blocking the game thread.|
|**ViewModels owned by subsystem not widgets**|Widget teardown should not destroy shared ViewModel state. The subsystem ensures ViewModel lifetime matches the game session.|
|**RiftVaultEquipmentMutable merged into RiftVaultEquipment**|Mutable is a required dependency for the target game. The extra module boundary added cost without benefit for a single-team project.|
|**Fragments and URiftItemDefinition in Inventory, not Core**|Fragment methods take URiftItemInstance\* parameters (an Inventory type). Placing fragments in Core would create a Core→Inventory circular dependency.|
|**"Item State" replaces "Fragment Memory"**|State is more expressive game language. "What state is this item instance in?" is the correct mental model. Consistent with FRiftEquipmentSlotState already in the design.|
|**Selective tests only**|Full test coverage for a plugin of this scale is not achievable for a solo project. Targeting only high-risk silent-failure scenarios maximizes value per test written.|

\---

## 12\. Closed Architecture Decisions

These questions were open at the end of implementation attempt 1. They are now permanently closed. Do not relitigate them.

|#|Question|Decision|Rationale|
|-|-|-|-|
|1|Fragment module placement — Option A (dedicated module) or Option B (in Inventory)?|**Option B — Fragments in RiftVaultInventory**|Eliminates the circular dependency cleanly. Core stays pure.|
|2|Should `URiftItemDefinition` reference `URiftItemFragment` directly, or via interface?|**Tabled**|Revisit at the start of RiftVaultInventory implementation when the full impact is visible.|
|3|Should item states be strongly typed per fragment, or a generic key-value store?|**Strongly typed `TInstancedStruct`**|Type safety, clean replication via FFastArraySerializer, clean serialization. Complexity is earned.|
|4|Equipment on APawn vs ACharacter?|**APawn — permanent, closed**|Mover compatibility. Every equipment header carries a comment to this effect.|
|5|Inventory Component location — APawn or APlayerState?|**APlayerState — permanent, closed**|Inventory must survive pawn death and respawn.|
|6|Should the processing queue be synchronous or async?|**Synchronous mutations, async persistence load**|In-session mutations are fast — synchronous is simpler and safe. Only the initial load from a remote backend is async to avoid blocking the game thread.|
|7|Is RiftVaultEquipmentMutable a separate module or part of Equipment?|**Merged into RiftVaultEquipment**|Reduces module count to 10. Mutable is a required dep for the target game. Split it when the pain is real, not theoretical.|

\---

## 13\. Build Order

Modules must be built and validated in this order. Each module's tests should pass before moving to the next.

|Step|Module|Key deliverable|
|-|-|-|
|1|`RiftVaultCore`|`URiftContainerDefinition`, `FRiftFragmentState` base, all item state structs (`FRiftStackState`, `FRiftConditionState`), `FRiftVaultTags`, all interfaces and enums|
|2|`RiftVaultInventory`|`URiftItemDefinition`, all 5 initial fragments, `URiftItemInstance`, `URiftContainer`, `URiftInventoryComponent`, processing queue, `URiftInventoryNetProxy`|
|3|`RiftVaultEquipment`|`URiftEquipmentComponent`, `FRiftEquipmentSlotState`, `URiftMutableEquipmentComponent`, `ARiftWeaponActor`, respawn flow|
|4|`RiftVaultLoot`|`ARiftPickup`, `URiftPickupComponent`, `URiftLootSelector\_DataTable`, `URiftLootHandler\_AddToInventory`|
|5|`RiftVaultDurability`|`URiftEffect\_Wear`, `URiftEffect\_Repair`, `URiftAbility\_Repair`, broken item flow|
|6|`RiftVaultCrafting`|`URiftRecipeDefinition`, `URiftCraftingComponent`, `URiftAbility\_Craft`|
|7|`RiftVaultEconomy`|`URiftVendorComponent`, `URiftAbility\_Buy`, `URiftAbility\_Sell`, `URiftAttributeSet\_Wealth`|
|8|`RiftVaultUI`|`URiftViewModel` base, item and container ViewModels, `URiftInventoryUISubsystem`|
|9|`RiftVaultEditor`|Factories and customizations|
|10|`RiftVaultTests`|Written alongside steps 2–7, consolidated and run here|

\---

## 14\. Testing Strategy

Tests live in `RiftVaultTests` and use Unreal's `DEFINE\_SPEC` automation framework. Each spec file corresponds to one system.

### High Priority — Silent failure possible

**Stacking (RiftVaultInventory)**

* Adding items to a partial stack merges correctly
* Adding items exceeding stack max creates overflow into a new stack
* Adding items to a full container with a full stack correctly rejects
* Splitting a stack produces two valid stacks summing to the original quantity
* Moving a stack between containers preserves quantity

**Serialization (RiftVaultInventory)**

* A full inventory round-trips through `FRiftInventorySaveData` without data loss
* Item states survive serialization (condition, stack count)
* An empty inventory serializes and deserializes without errors
* An inventory with items of every fragment type serializes correctly

**Crafting (RiftVaultCrafting)**

* Exact ingredients present → craft succeeds, ingredients consumed, result added
* Missing one ingredient → craft fails, no ingredients consumed
* Insufficient ingredient quantity → craft fails, no ingredients consumed
* `bConsumed = false` ingredient → ingredient not consumed after craft
* Craft result correctly spawns item with expected item state values

**Condition (RiftVaultDurability)**

* Wear effect reduces `FRiftConditionState.CurrentCondition`
* Condition reaching zero sets `bIsBroken = true`
* Broken item triggers ability removal on `URiftEquipmentComponent`
* Repair effect restores condition and clears broken flag
* Condition state persists through serialization

### Medium Priority — Detectable but investigation required

**Economy (RiftVaultEconomy)**

* Buy deducts correct Wealth attribute amount
* Buy with insufficient Wealth is blocked by ability tag requirements
* Sell adds correct Wealth attribute amount
* `URiftOfferEvaluator` price calculation is correct for base case

**Loot (RiftVaultLoot)**

* DataTable loot selector produces a valid item definition
* Empty loot table returns no results without crashing
* Pickup delivers items to inventory correctly via `URiftLootHandler\_AddToInventory`

### Low Priority — Verifiable by inspection

**Equipment (RiftVaultEquipment)**

* Equipping an item grants the abilities from `URiftFragment\_Equippable`
* Unequipping removes those ability grants
* Equipping a broken item is blocked

\---

## 15\. Future Considerations

These features are deliberately out of scope for the initial version but the architecture supports without modification.

* **Spatial inventory as an optional module.** The container system is a flat array. A `URiftSpatialContainer` subclass could override slot management to add occupancy mask logic without touching the base system.
* **Item modifiers / enchantments.** A new fragment `URiftFragment\_Modifiers` could hold a list of modifier objects. GAS attribute system handles application.
* **Crafting stations.** `URiftCraftingComponent` already accepts a required station tag on recipes. A crafting station actor that grants a tag to nearby players' ASCs is a trivial addition.
* **Item rarity tiers with stat scaling.** `URiftFragment\_Display` already has a rarity tag. A new processor step in `URiftItemProcessor` could scale fragment values based on rarity at instance creation time.
* **Trading between players.** A trading session object holding two inventories in escrow until both players confirm. The inventory component's interface already supports the necessary move operations.
* **Mutable cosmetic customization UI.** A separate module (`RiftVaultCustomization`) could expose Mutable parameters directly to the player for cosmetic customization.
* **Split RiftVaultEquipmentMutable back out.** If a future game needs equipment without Mutable, the Mutable-specific classes are cleanly contained in `RiftVaultEquipment/Public/Components/` and `Actors/` and can be extracted into their own module at that time.

\---

## 16\. Glossary

|Term|Definition|
|-|-|
|**ASC**|Ability System Component. Unreal's `UAbilitySystemComponent`. Manages GAS abilities, effects, and attributes for an actor.|
|**COI**|Customizable Object Instance. Mutable's `UCustomizableObjectInstance`. The runtime unique mesh produced by Mutable for one character or item.|
|**DataAsset**|A `UDataAsset` subclass. A content browser asset that holds designer-authored data. No logic, just data.|
|**Fragment**|A `URiftItemFragment` subclass. Adds a specific capability or data group to an item definition by composition. Lives in RiftVaultInventory.|
|**Item State**|A `FRiftFragmentState` subclass. Stores per-instance runtime data for a fragment that needs it. Previously called "Fragment Memory." Named `FRift<Capability>State`.|
|**GAS**|Gameplay Ability System. Unreal's framework for abilities, effects, attributes, and gameplay tags.|
|**Item Definition**|`URiftItemDefinition`. A DataAsset describing an item type. Lives in RiftVaultInventory. Never instantiated at runtime directly.|
|**Item Instance**|`URiftItemInstance`. A runtime UObject representing one item currently in an inventory. Holds a reference to its definition and its item states.|
|**Mover**|Epic's experimental movement plugin for UE5, replacing `UCharacterMovementComponent`. Works at the APawn level.|
|**Mutable**|Epic's procedural mesh plugin. Generates unique skeletal meshes at runtime from a designer-authored `UCustomizableObject`.|
|**Net Proxy**|`URiftInventoryNetProxy`. An actor component that handles replication of inventory state, keeping that concern separate from inventory logic.|
|**PlayerState**|`APlayerState`. A replicated actor that persists across pawn respawns. Where `URiftInventoryComponent` lives.|
|**Processing Queue**|The serialized operation queue in `URiftInventoryComponent`. In-session mutations are synchronous. Initial persistence load is async.|
|**ViewModel**|A `UMVVMViewModelBase` subclass. Exposes inventory data to UMG widgets via field notifications. Owned by `URiftInventoryUISubsystem`.|
|**Widget Controller**|A `UObject` subclass that mediates between widget user intent and system operations. Keeps widgets free of business logic.|

\---

## 17\. Architecture Review — Lessons from Implementation Attempt 1

> This section documents issues discovered during the first build attempt. All actionable decisions have been moved to Section 12 — Closed Architecture Decisions. The UHT/compiler rules below remain here as a permanent reference — they apply to all future implementation work.

### 17.1 Module Boundary Problem — Fragments vs Items

**What went wrong:**  
`URiftItemFragment` was placed in `RiftVaultCore` because fragments are referenced by `URiftItemDefinition`. However fragment methods need to take `URiftItemInstance\*` as parameters (which lives in `RiftVaultInventory`). This created a circular dependency.

**The hack used (do not repeat):**  
Changed all UFUNCTION params from `URiftItemInstance\*` to `UObject\*` and cast inside implementations. This broke Blueprint type safety and caused LNK2005 duplicate symbol errors.

**Resolution (Section 12, Decision 1):**  
Fragments and `URiftItemDefinition` both move to `RiftVaultInventory`. Core holds no fragment or definition types.

\---

### 17.2 UHT Cross-Module Type Rules

**Rule:** A UFUNCTION in module A cannot take a parameter of a type defined in module B unless module A depends on module B in its Build.cs. The module dependency graph must be fully designed before writing any UFUNCTION signatures. Any UFUNCTION that takes a custom type as a parameter locks in a module dependency.

\---

### 17.3 BlueprintNativeEvent Wrapper Methods

**Rule:** For `UFUNCTION(BlueprintNativeEvent)`, UHT auto-generates the non-`\_Implementation` wrapper inside the module's generated `.cpp`. If the `.cpp` also defines that wrapper, the linker sees two definitions → LNK2005.

**Rule:** Only ever define `FunctionName\_Implementation` in your `.cpp`. Never define the wrapper.

\---

### 17.4 Tag Access Pattern

**Rule:** Tags declared with `UE\_DEFINE\_GAMEPLAY\_TAG` / `UE\_DECLARE\_GAMEPLAY\_TAG\_EXTERN` are global variables. Access them directly as `Tag\_Rift\_X`. Do NOT create a singleton struct `FRiftVaultTags` with a `Get()` method and member variables — that pattern causes `C2653` errors with native tags.

\---

### 17.5 DeveloperSettings Module Dependency

`UDeveloperSettings` requires `"DeveloperSettings"` in the module's Build.cs `PrivateDependencyModuleNames`. Missing this causes LNK2019 on all `UDeveloperSettings` vtable symbols.

\---

### 17.6 FindFragmentByClass Template Syntax

`Definition->FindFragmentByClass<UMyFragment>()` causes MSVC C2275/C2059 parse errors when `UMyFragment` is not fully defined at the call site (only forward declared). Use the explicit StaticClass form instead:

```cpp
Cast<UMyFragment>(Definition->FindFragmentByClass(UMyFragment::StaticClass()))
```

\---

*End of RiftVault Technical Design Document v1.0.2*

