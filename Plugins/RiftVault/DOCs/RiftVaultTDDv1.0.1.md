# RiftVault — Technical Design Document

**Version:** 0.1.0  
**Status:** Draft  
**Author:** RiftVault Development  
**Last Updated:** 2026-03-12  
**Engine:** Unreal Engine 5 (Mover-compatible)  
**Plugin Target:** Reusable across multiple game projects

\---

## Table of Contents

1. [Overview](#1-overview)
2. [Goals and Non-Goals](#2-goals-and-non-goals)
3. [Architecture Overview](#3-architecture-overview)
4. [Module Breakdown](#4-module-breakdown)

   * 4.1 [RiftVaultCore](#41-riftvaultcore)
   * 4.2 [RiftVaultInventory](#42-riftvaultinventory)
   * 4.3 [RiftVaultEquipment](#43-riftvaultequipment)
   * 4.4 [RiftVaultEquipmentMutable](#44-riftvaultequipmentmutable)
   * 4.5 [RiftVaultLoot](#45-riftvaultloot)
   * 4.6 [RiftVaultCrafting](#46-riftvaultcrafting)
   * 4.7 [RiftVaultEconomy](#47-riftvaulteconomy)
   * 4.8 [RiftVaultDurability](#48-riftvaultdurability)
   * 4.9 [RiftVaultUI](#49-riftvaultui)
   * 4.10 [RiftVaultEditor](#410-riftvaulteditor)
   * 4.11 [RiftVaultTests](#411-riftvaulttests)
5. [Core Systems Deep Dive](#5-core-systems-deep-dive)

   * 5.1 [Item Definition and Fragment System](#51-item-definition-and-fragment-system)
   * 5.2 [Item Instance and Fragment Memory](#52-item-instance-and-fragment-memory)
   * 5.3 [Container System](#53-container-system)
   * 5.4 [Inventory Component and Processing Queue](#54-inventory-component-and-processing-queue)
   * 5.5 [Replication Strategy](#55-replication-strategy)
   * 5.6 [Equipment and Mutable Integration](#56-equipment-and-mutable-integration)
   * 5.7 [GAS Integration](#57-gas-integration)
   * 5.8 [Persistence Strategy](#58-persistence-strategy)
   * 5.9 [MVVM UI Architecture](#59-mvvm-ui-architecture)
6. [Data Flow Diagrams](#6-data-flow-diagrams)

   * 6.1 [Adding an Item](#61-adding-an-item)
   * 6.2 [Equipping an Item](#62-equipping-an-item)
   * 6.3 [Crafting an Item](#63-crafting-an-item)
   * 6.4 [Player Respawn Flow](#64-player-respawn-flow)
7. [Naming Conventions](#7-naming-conventions)
8. [Dependency Graph](#8-dependency-graph)
9. [Fragment Reference](#9-fragment-reference)
10. [Known Constraints and Decisions](#10-known-constraints-and-decisions)
11. [Build Order](#11-build-order)
12. [Testing Strategy](#12-testing-strategy)
13. [Future Considerations](#13-future-considerations)
14. [Glossary](#14-glossary)

\---

## 1\. Overview

RiftVault is a modular, data-driven inventory and equipment plugin for Unreal Engine 5. It is designed to be game-agnostic — meaning it ships no assumptions about genre, camera perspective, or control scheme — and can be dropped into any UE5 C++ project as a plugin.

The first game it will power is a **sci-fi multiplayer title** using **Epic's Mover plugin** for character movement. This context has directly influenced several architectural decisions, most notably:

* All equipment and attachment logic operates at the **APawn level**, not ACharacter, because Mover replaces UCharacterMovementComponent and ACharacter carries too many assumptions about movement that conflict with Mover.
* **Mutable** (Unreal's procedural mesh plugin) is the **sole visual system** for both character equipment and weapons. There are no legacy skeletal mesh swap systems.
* The **URiftInventoryComponent lives on APlayerState**, not APawn, so inventory survives pawn death and respawn cleanly.
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

RiftVault is structured as a **layered plugin** with 11 modules organized into 4 tiers. Each tier can only depend on tiers below it, never above. This prevents circular dependencies and ensures any module can be included independently.

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
│  RiftVaultEquipmentMutable · RiftVaultLoot          │
├─────────────────────────────────────────────────────┤
│  Tier 1 — Foundation                                │
│  RiftVaultCore                                      │
└─────────────────────────────────────────────────────┘
```

### Key Architectural Patterns

**Fragment Pattern**  
Items are defined by composing `URiftItemFragment` subclasses onto a `URiftItemDefinition` DataAsset. A sword might have `URiftFragment\_Equippable`, `URiftFragment\_Display`, `URiftFragment\_Value`, and `URiftFragment\_Condition`. A consumable potion might have `URiftFragment\_Stack` and `URiftFragment\_Display`. No subclassing of items is required or encouraged — new item behaviours are new fragments.

**Memory Pattern**  
Fragments define the *type-level* data (e.g. "this item type has a max durability of 100"). Fragment Memories store the *instance-level* runtime data (e.g. "this specific sword instance currently has 73 durability"). Memories are structs stored on `URiftItemInstance` and serialized for persistence.

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
**Purpose:** The foundation layer. Contains all data types, base classes, interfaces, enums, and tags. Has zero knowledge of managers, components, or gameplay systems beyond GAS tags and attributes.

This module is intentionally kept small and pure. The rule is: if a class in RiftVaultCore needs to reference a class that is not also in RiftVaultCore or a UE5 engine module, it does not belong here.

**Key classes and their roles:**

|Class|Type|Role|
|-|-|-|
|`URiftItemDefinition`|UDataAsset|The "blueprint" for an item type. Authored by designers. Contains a list of fragments that describe the item's capabilities.|
|`URiftContainerDefinition`|UDataAsset|Defines the rules for a container — its max capacity, what tags items must have to be accepted, etc.|
|`URiftItemFragment`|UObject (abstract)|Base class for all fragments. Subclassed to add new item capabilities without modifying URiftItemDefinition.|
|`FRiftFragmentMemory`|Struct (abstract)|Base struct for per-instance fragment data. Subclassed by each fragment that needs runtime state.|
|`FRiftVaultTags`|Struct|Central declaration point for all GameplayTags used by RiftVault. Prevents magic strings scattered through code.|
|`IRiftInventoryInterface`|UInterface|Interface that any actor or component can implement to advertise that it has an inventory. Used by loot handlers and pickup actors without needing to know the concrete class.|
|`IRiftEquipmentInterface`|UInterface|Interface for actors/components that manage equipment state.|
|`EItemLifecycle`|Enum|The states an item instance moves through: Initializing → Active → PendingRemoval → Removed.|
|`EContainerType`|Enum|Categorizes containers: PlayerInventory, Equipment, Stash, Vendor, Loot, etc.|

**Fragment classes (defined in Core because they are pure data):**

|Class|Role|
|-|-|
|`URiftFragment\_Stack`|Stack size, maximum stack size, current quantity.|
|`URiftFragment\_Equippable`|Equipment slot tag, equipment states, list of GAS abilities granted when equipped.|
|`URiftFragment\_Condition`|Maximum condition value, current condition value (in memory), degradation rules.|
|`URiftFragment\_Display`|Display name, short description, icon texture, rarity tag.|
|`URiftFragment\_Value`|Base buy price, base sell price, currency GameplayTag.|

\---

### 4.2 RiftVaultInventory

**Type:** Runtime  
**Depends on:** RiftVaultCore  
**Purpose:** The heart of the system. Manages the lifecycle of all item instances and containers. Handles the processing queue, replication, and the interface to the persistence layer.

`URiftInventoryComponent` lives on `APlayerState`. This is a deliberate decision — it means inventory survives pawn death and respawn. When a new pawn spawns, it queries the PlayerState's inventory component to re-apply equipment state. The equipment component on the Pawn is ephemeral; the inventory component on PlayerState is persistent for the duration of a session.

**Key classes:**

|Class|Type|Role|
|-|-|-|
|`URiftInventoryComponent`|UActorComponent|The main manager. Owns all URiftItemInstance and URiftContainer objects. Processes all add/remove/move operations. Lives on APlayerState.|
|`URiftInventorySubsystem`|UWorldSubsystem|World-level subsystem for operations that span multiple inventories (e.g. loot distribution, vendor transactions). Also serves as a registry so any system can find an inventory by player.|
|`URiftItemInstance`|UObject|A runtime object representing one item in the world. Holds its definition reference and a map of fragment memories. Replicated.|
|`URiftContainer`|UObject|A runtime container holding an ordered array of item instances. Enforces capacity and acceptance rules from its URiftContainerDefinition.|
|`URiftItemProcessor`|UObject|Processes items during initialization — runs fragments in a defined order to apply initial state (e.g. rolling random starting condition, applying level-based stat scaling).|
|`URiftInventoryNetProxy`|UActorComponent|Handles the replication concern cleanly. The inventory component itself focuses on logic; the net proxy focuses on what data to replicate and how. Lives alongside URiftInventoryComponent on PlayerState.|
|`URiftSaveGame`|USaveGame (stub)|Stub that is replaced by the game-specific persistence implementation. Defines the serialization format for item instances and containers.|
|`IRiftPersistenceInterface`|UInterface|The boundary between RiftVault and the game's backend. RiftVault calls SaveInventory / LoadInventory on this interface. The game implements it against EOS, a custom REST API, or local disk.|

**The Processing Queue**  
Item operations (add, remove, move, split stack) do not execute immediately. They are enqueued and processed in order. This prevents race conditions when multiple operations fire in the same frame (e.g. looting a container while an ability consumes an item). The queue also provides a natural place to reject operations that would violate rules (full container, insufficient stack, etc.) and broadcast a result back to the requester.

\---

### 4.3 RiftVaultEquipment

**Type:** Runtime  
**Depends on:** RiftVaultCore, RiftVaultInventory  
**Purpose:** Manages the logical state of what is equipped on a Pawn. Does not own or manage meshes — that is entirely `RiftVaultEquipmentMutable`'s responsibility. This module is purely about *state*: what is in each equipment slot, what GAS abilities are currently granted by equipped items, and what happens when items are equipped or unequipped.

**Critical design note — Pawn not Character:**  
Every class in this module is written against `APawn` only. There is no call to `Cast<ACharacter>()`, no assumption of `GetMesh()` returning a valid `USkeletalMeshComponent`. This ensures compatibility with Mover-based pawns which do not inherit from ACharacter.

**Key classes:**

|Class|Type|Role|
|-|-|-|
|`URiftEquipmentComponent`|UActorComponent|Lives on APawn. Tracks what URiftItemInstance is in each equipment slot (identified by GameplayTag). Applies and removes GAS abilities when slots change. Notifies URiftMutableEquipmentComponent of slot changes.|
|`FRiftEquipmentState`|Struct|Represents the current state of a single equipment slot: which item is equipped, which ability handles are currently active, and what the visual state is (Active, Holstered, Stowed).|
|`URiftAbility\_Equip`|UGameplayAbility|GAS ability that handles the equip action — validates the item, moves it from inventory into an equipment slot, triggers the Mutable update.|
|`URiftAbility\_Unequip`|UGameplayAbility|GAS ability for unequipping — returns item to inventory, removes granted abilities, triggers the Mutable update.|

**Respawn handling:**  
`URiftEquipmentComponent::OnPawnReady()` is called by the game when a new pawn is fully initialized and ready to receive equipment. It queries the PlayerState's `URiftInventoryComponent` for any items that were previously in equipment slots and re-applies them. This is the bridge between the ephemeral pawn and the persistent inventory.

\---

### 4.4 RiftVaultEquipmentMutable

**Type:** Runtime (LoadingPhase: None — only loaded when Mutable plugin is available)  
**Depends on:** RiftVaultCore, RiftVaultInventory, RiftVaultEquipment  
**Purpose:** The sole visual authority for all character and weapon appearance. Listens to `URiftEquipmentComponent` for slot changes and drives `UCustomizableObjectInstance` updates accordingly.

Mutable works by taking a `UCustomizableObject` (a designer-authored asset defining all the mesh variations) and producing a `UCustomizableObjectInstance` (the runtime unique mesh for one character). When equipment changes, we update parameters on the instance and Mutable regenerates the mesh asynchronously.

**Key classes:**

|Class|Type|Role|
|-|-|-|
|`URiftMutableEquipmentComponent`|UActorComponent|Lives on APawn alongside URiftEquipmentComponent. Holds a reference to the character's UCustomizableObjectInstance. Responds to equipment slot change events and translates them into Mutable parameter updates.|
|`URiftMutableWeaponComponent`|UActorComponent|Lives on ARiftWeaponActor. Holds the weapon's own UCustomizableObjectInstance. Updates weapon appearance (attachments, wear, cosmetics) when item condition or cosmetic parameters change.|
|`ARiftWeaponActor`|AActor|A minimal actor spawned to hold a weapon's Mutable instance. Has a UCustomizableSkeletalComponent and is attached to the pawn's hand socket. Nothing else — all logic lives in URiftMutableWeaponComponent.|
|`FRiftMutableParameter`|Struct|A name/value pair passed to a Mutable instance. Abstracts the difference between int, float, colour, and mesh parameters so the equipment component does not need to know Mutable's API directly.|
|`IRiftMutableEquipmentInterface`|UInterface|Allows equipment actors and the main character component to be treated uniformly when broadcasting parameter updates.|

**Weapon attachment:**  
`ARiftWeaponActor` is spawned by `URiftEquipmentComponent` when a weapon slot is filled. It is attached to a socket on the character's Mutable mesh. The socket name is defined in `URiftFragment\_Equippable` on the weapon's item definition. When the slot is cleared, the weapon actor is destroyed.

\---

### 4.5 RiftVaultLoot

**Type:** Runtime  
**Depends on:** RiftVaultCore, RiftVaultInventory  
**Purpose:** Manages loot table evaluation, item selection, and pickup actors in the world. Fully data-driven via DataTables.

**Key classes:**

|Class|Type|Role|
|-|-|-|
|`ARiftPickup`|AActor|A world actor representing items on the ground. Holds one or more item definitions (or instances for dropped items). Triggers pickup when a player interacts with it.|
|`URiftPickupComponent`|UActorComponent|Reusable component that adds pickup capability to any actor. Handles the server-side logic of finding a valid inventory and delivering items.|
|`URiftLootSelector`|UObject (abstract)|Base class for loot selection strategies. Subclassed to implement different selection algorithms.|
|`URiftLootSelector\_DataTable`|URiftLootSelector|Selects items from a UDataTable of FRiftLootEntry rows. Supports weighted random selection.|
|`URiftLootSelector\_InventoryItems`|URiftLootSelector|Selects items from an existing inventory (e.g. looting a dead enemy's equipped items).|
|`URiftLootHandler`|UObject (abstract)|Base class for loot delivery strategies. Subclassed to implement different delivery methods.|
|`URiftLootHandler\_AddToInventory`|URiftLootHandler|Delivers selected items by adding them to the player's URiftInventoryComponent.|
|`FRiftLootEntry`|Struct (FTableRowBase)|A single row in a loot DataTable: item definition reference, min/max quantity, weight, conditions.|

\---

### 4.6 RiftVaultCrafting

**Type:** Runtime  
**Depends on:** RiftVaultCore, RiftVaultInventory  
**Purpose:** Recipe-based item crafting. Recipes are DataAssets. The crafting ability checks prerequisites, consumes ingredients via URiftInventoryComponent, and delivers results.

**Key classes:**

|Class|Type|Role|
|-|-|-|
|`URiftRecipeDefinition`|UDataAsset|Defines a crafting recipe: list of FRiftCraftIngredient inputs, list of FRiftCraftResult outputs, required station tag, prerequisites.|
|`URiftCraftingComponent`|UActorComponent|Lives on APawn or a crafting station actor. Manages the crafting queue and communicates with URiftInventoryComponent to consume/produce items.|
|`URiftAbility\_Craft`|UGameplayAbility|GAS ability that executes a craft. Validates prerequisites, checks inventory for ingredients, consumes them, and spawns results. Can be async for multi-step crafting.|
|`FRiftCraftIngredient`|Struct|One input requirement: item definition reference, required quantity, whether the item is consumed or just required.|
|`FRiftCraftResult`|Struct|One output: item definition reference, quantity, optional fragment overrides (e.g. crafting produces a specific quality tier).|

\---

### 4.7 RiftVaultEconomy

**Type:** Runtime  
**Depends on:** RiftVaultCore, RiftVaultInventory  
**Purpose:** Vendor interactions and currency management. Currency is a GAS attribute. Transactions are GAS abilities.

**Key classes:**

|Class|Type|Role|
|-|-|-|
|`URiftVendorComponent`|UActorComponent|Lives on an NPC actor. Exposes a list of FRiftVendorOffer structs representing items for sale. Can also accept items for sale from the player.|
|`URiftOfferEvaluator`|UObject|Calculates the final buy and sell prices for an offer. Takes reputation, bulk discounts, item condition, and other modifiers into account. Subclassable for game-specific pricing logic.|
|`URiftAbility\_Buy`|UGameplayAbility|GAS ability for purchasing. Verifies the player has sufficient Wealth attribute, deducts it, and adds the item to the player's inventory via URiftInventoryComponent.|
|`URiftAbility\_Sell`|UGameplayAbility|GAS ability for selling. Removes the item from the player's inventory and adds the sell value to the Wealth attribute.|
|`URiftAttributeSet\_Wealth`|UAttributeSet|GAS attribute set containing the Wealth attribute (the currency). Lives on the player's AbilitySystemComponent.|
|`FRiftVendorOffer`|Struct|One item in a vendor's stock: item definition, quantity available (-1 for infinite), base price, price curve reference.|

\---

### 4.8 RiftVaultDurability

**Type:** Runtime  
**Depends on:** RiftVaultCore, RiftVaultInventory, RiftVaultEquipment  
**Purpose:** Drives the wear and repair of items through GAS effects. The `URiftFragment\_Condition` data lives in RiftVaultCore, but the GAS effects that modify it live here to avoid making Core depend on the full GAS gameplay module.

**Key classes:**

|Class|Type|Role|
|-|-|-|
|`URiftEffect\_Wear`|UGameplayEffect|Applied to the player's ASC when equipped items take damage (on hit, on use, etc.). Modifies the condition memory of relevant item instances.|
|`URiftEffect\_Repair`|UGameplayEffect|Applied when an item is repaired. Restores condition memory up to the item's maximum.|
|`URiftAbility\_Repair`|UGameplayAbility|GAS ability for the repair action. Can require materials (consumed via URiftInventoryComponent), time, or a repair station tag.|
|`FRiftConditionPayload`|Struct|Event payload broadcast when an item's condition changes — consumed by URiftEquipmentComponent to update equipment state (e.g. disabling a broken item's abilities) and by URiftMutableEquipmentComponent to update visual wear state on the Mutable mesh.|

**Broken item flow:**  
When an item's condition reaches zero, `URiftEquipmentComponent` receives a `FRiftConditionPayload` with `bIsBroken = true`. It immediately removes all abilities granted by that item and sets the slot's visual state to reflect the broken condition. The item cannot be re-equipped until repaired. This interaction crosses module boundaries cleanly via the payload event — RiftVaultDurability does not need to reference URiftEquipmentComponent directly.

\---

### 4.9 RiftVaultUI

**Type:** Runtime  
**Depends on:** RiftVaultCore, RiftVaultInventory, RiftVaultEquipment, RiftVaultEconomy  
**Purpose:** The full MVVM UI stack. Provides C++ ViewModels, Widget base classes, Widget Controllers, and drag-and-drop infrastructure. Does not ship any Blueprint widget assets — those are game-specific.

**MVVM in Unreal Engine 5**  
Unreal 5.1+ ships a `ModelViewViewModel` plugin that provides `UMVVMViewModelBase`. A ViewModel is a UObject that exposes properties with field notifications — UMG widgets bind to these properties and automatically update when they change. The widget never directly queries the inventory component; it binds to the ViewModel, and the ViewModel observes the component.

This was the area where the previous `ModularInventory` implementation ran into difficulty. The common failure mode is ViewModels that hold strong references to UObjects that can be garbage collected, or ViewModels that are updated too eagerly (every tick) rather than event-driven. RiftVaultUI solves this by making all ViewModel updates event-driven — triggered by delegate broadcasts from URiftInventoryComponent — and by using `TWeakObjectPtr` for all cross-object references inside ViewModels.

**Key classes:**

|Class|Type|Role|
|-|-|-|
|`URiftWidget`|UUserWidget|Base class for all RiftVault UMG widgets. Provides helpers for finding the ViewModel subsystem and requesting controllers.|
|`URiftContainerWidget`|URiftWidget|Displays one container's worth of item slots. Binds to URiftViewModel\_Container.|
|`URiftItemWidget`|URiftWidget|Displays one item slot. Binds to URiftViewModel\_Item. Handles click and drag initiation.|
|`URiftDragOperation`|UDragDropOperation|Carries item move data during a UMG drag-and-drop operation. Holds the source container tag, source slot index, and item instance reference.|
|`URiftViewModel`|UMVVMViewModelBase|Base class for all RiftVault ViewModels. Provides common helpers for field notification and weak reference management.|
|`URiftViewModel\_Item`|URiftViewModel|Exposes a single item's display data: name, icon, stack count, condition, rarity. Updated when the underlying URiftItemInstance changes.|
|`URiftViewModel\_Container`|URiftViewModel|Exposes a container's slot array as a list of URiftViewModel\_Item. Updated when the container's contents change.|
|`URiftViewModel\_Equipment`|URiftViewModel|Exposes the current equipment state by slot — what's equipped, what condition it's in, what abilities are active.|
|`URiftViewModel\_Vendor`|URiftViewModel|Exposes a vendor's offer list and the player's current wealth for the vendor UI.|
|`URiftWidgetController`|UObject|Mediates between widget intent (user clicked "equip") and system operations (call URiftEquipmentComponent). Keeps widgets free of business logic.|
|`URiftInventoryUISubsystem`|UGameInstanceSubsystem|Manages ViewModel lifetime. ViewModels are owned here, not by widgets, so they survive widget teardown and are shared across multiple widgets showing the same data.|

\---

### 4.10 RiftVaultEditor

**Type:** Editor  
**Depends on:** RiftVaultCore, RiftVaultInventory, RiftVaultEquipment  
**Purpose:** Improves the authoring experience for designers. Provides right-click-to-create for item and recipe definitions, and custom Details panel customizations for complex properties.

**Key classes:**

|Class|Type|Role|
|-|-|-|
|`URiftFactory\_ItemDefinition`|UFactory|Enables right-click → New → RiftVault → Item Definition in the Content Browser.|
|`URiftFactory\_RecipeDefinition`|UFactory|Same for crafting recipe DataAssets.|
|`URiftAssetAction\_ItemDefinition`|FAssetTypeActions\_Base|Registers Item Definitions with the Asset Manager and gives them a friendly category name and thumbnail color in the Content Browser.|
|`URiftConditionCustomization`|IPropertyTypeCustomization|Makes the condition range (min/max on URiftFragment\_Condition) display as a min-max slider rather than two separate float fields in the Details panel.|
|`URiftEquipmentStateCustomization`|IPropertyTypeCustomization|Makes FRiftEquipmentState render with a visual slot diagram in the Details panel rather than raw struct fields.|

\---

### 4.11 RiftVaultTests

**Type:** DeveloperTool  
**Depends on:** All runtime modules  
**Purpose:** Automated testing for the systems where silent failure is most dangerous. Uses Unreal's `DEFINE\_SPEC` automation framework.

Tests are **not** written for everything. The guiding principle is: write a test where a bug could exist for days before being noticed. Visual bugs (wrong icon, wrong widget layout) do not qualify. Logic bugs in stacking math, save/load serialization, crafting ingredient consumption, and condition persistence absolutely do.

See [Section 12: Testing Strategy](#12-testing-strategy) for the full breakdown.

**Key infrastructure:**

|Class|Role|
|-|-|
|`FRiftWorldFixture`|Spins up a minimal UWorld with a GameInstance. Handles teardown. The foundation all other fixtures build on.|
|`FRiftInventoryFixture`|Builds on FRiftWorldFixture. Adds a test APawn with a PlayerState, an AbilitySystemComponent, and a fully initialized URiftInventoryComponent.|
|`FRiftItemFixture`|Helpers for creating test URiftItemDefinition assets in-memory without needing on-disk DataAssets.|
|`ARiftTestPawn`|A minimal APawn subclass with an ASC, used as the test subject.|

\---

## 5\. Core Systems Deep Dive

### 5.1 Item Definition and Fragment System

A `URiftItemDefinition` is a `UDataAsset` subclass — a designer-authored asset that defines what an item *type* is. It is never instantiated at runtime in the traditional sense; instead, a `URiftItemInstance` is created that holds a reference to its definition.

The definition holds a `TArray<TObjectPtr<URiftItemFragment>>` — an array of fragment objects. Each fragment is a `UObject` subclass that adds a specific capability or data group to the item. The philosophy is **composition over inheritance**: rather than having `USwordItem : UWeaponItem : URiftItemDefinition`, you have a `URiftItemDefinition` with `URiftFragment\_Equippable` and `URiftFragment\_Condition` attached.

**Why fragments and not subclassing?**  
Subclassing item types leads to deep inheritance hierarchies and the "diamond problem" — what if you want an item that is both a weapon *and* a crafting ingredient? With fragments, you simply add both `URiftFragment\_Equippable` and let the crafting system query for items that have ingredients, regardless of their other fragments. Any item can be anything by combining the right fragments.

**Finding a fragment:**  
Any code that needs to know about an item's stack size calls:

```
const URiftFragment\_Stack\* StackFragment = ItemDefinition->FindFragment<URiftFragment\_Stack>();
if (StackFragment)
{
    // Item supports stacking
}
```

This pattern is used everywhere. If the fragment isn't found, the feature simply doesn't apply to that item. A sword without a `URiftFragment\_Stack` cannot be stacked — no special case handling required.

\---

### 5.2 Item Instance and Fragment Memory

A `URiftItemInstance` is a `UObject` that represents one item currently in an inventory. It holds:

1. A `TObjectPtr<URiftItemDefinition>` — a reference to its definition (the type).
2. A `TArray<FInstancedStruct>` of fragment memories — the per-instance runtime data.

**Fragment Memory** is the separation between type-level data and instance-level data. The definition says "this sword type has a max condition of 100." The instance's `FRiftFragment\_ConditionMemory` says "this specific sword is currently at 73 condition."

Not all fragments need a memory. `URiftFragment\_Display` has no memory — the name and icon are the same for every instance of that item type. Only fragments that track per-instance state (condition, stack count, dynamic attributes) need a memory struct.

Reading instance memory:

```
FRiftFragment\_ConditionMemory\* Memory = ItemInstance->FindMemory<FRiftFragment\_ConditionMemory>();
if (Memory)
{
    float CurrentCondition = Memory->CurrentCondition;
}
```

Writing instance memory (always server-side):

```
FRiftFragment\_ConditionMemory\* Memory = ItemInstance->FindOrAddMemory<FRiftFragment\_ConditionMemory>();
Memory->CurrentCondition = NewValue;
// Notify replication system that this instance's data has changed
ItemInstance->MarkMemoryDirty();
```

\---

### 5.3 Container System

A `URiftContainer` holds an ordered array of `URiftItemInstance` pointers up to the capacity defined by its `URiftContainerDefinition`. Containers are owned by `URiftInventoryComponent` and identified by a `FGameplayTag` slot (e.g. `Inventory.Container.Backpack`, `Inventory.Container.Hotbar`).

**Container acceptance rules:**  
Before adding an item to a container, the container checks acceptance criteria from its definition:

* Does the item have a required tag? (e.g. only weapons in the weapon container)
* Is there space?
* If the item is stackable, is there an existing partial stack to merge into first?

These checks are run by `URiftItemProcessor` during the enqueue phase, before the operation hits the processing queue. A rejected operation returns a result enum immediately without entering the queue.

**Item layout:**  
All items are 1x1. There is no spatial placement logic. A container with capacity 20 holds up to 20 item instances in a flat array. The UI maps this array directly to a grid of slots — slot 0 is top-left, slot N-1 is bottom-right. Moving an item from slot 3 to slot 7 is an array swap operation, nothing more.

\---

### 5.4 Inventory Component and Processing Queue

`URiftInventoryComponent` lives on `APlayerState`. It is the single source of truth for all items a player owns.

**The processing queue** exists to serialize inventory operations. Because multiple systems can request inventory mutations in the same frame (an ability consuming an item at the same tick a loot handler is trying to add one), all operations are enqueued and processed sequentially. Each operation in the queue is:

1. **Validated** — can this operation succeed given the current state?
2. **Executed** — the state change is applied.
3. **Broadcast** — delegates fire to notify ViewModels, equipment component, persistence layer, etc.

The queue processes one operation per tick by default, configurable per project. This introduces a maximum one-frame delay on inventory operations — acceptable for all systems in this plugin.

\---

### 5.5 Replication Strategy

RiftVault uses a **server-authoritative, client-display** model.

* All inventory mutations originate on the server (via Server RPCs from GAS abilities).
* `URiftInventoryNetProxy` replicates the inventory state to owning clients.
* Other clients (e.g. for seeing another player's equipped items) receive equipment state via `URiftEquipmentComponent` replication, not full inventory state.
* Item instances replicate their definition reference and memory data to the owning client only.

**Why a separate NetProxy component?**  
Separating replication concerns from inventory logic keeps `URiftInventoryComponent` focused on logic and makes it easier to adjust replication strategy without touching the core system. The proxy can be configured per-project to replicate more or less data depending on bandwidth requirements.

\---

### 5.6 Equipment and Mutable Integration

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
    → Sets initial Mutable parameters from item definition and instance memory
    → Mutable generates the weapon mesh
```

\---

### 5.7 GAS Integration

RiftVault assumes a fully configured Ability System Component exists on the player. It does not create or manage the ASC — that is the game's responsibility. RiftVault interacts with the ASC in the following ways:

* **Equipping grants abilities.** `URiftFragment\_Equippable` holds a list of `TSubclassOf<UGameplayAbility>`. When an item is equipped, these are granted. When unequipped, the grants are removed using stored `FGameplayAbilitySpecHandle` values.
* **GAS effects drive system state.** Wear, repair, wealth changes are all `UGameplayEffect` applications.
* **Currency is a GAS attribute.** `URiftAttributeSet\_Wealth` is expected to be on the player's ASC. Buy and sell abilities modify this attribute via effects.
* **Tags gate operations.** `FRiftVaultTags` declares tags like `RiftVault.Status.Inventory.Busy` (set while the queue is processing) and `RiftVault.Item.Broken` (set on items at zero condition). These can be used in GAS tag requirements to block abilities appropriately.

\---

### 5.8 Persistence Strategy

RiftVault does not ship a persistence implementation. It ships `IRiftPersistenceInterface`:

```cpp
UINTERFACE()
class URiftPersistenceInterface : public UInterface
{
    GENERATED\_BODY()
};

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

The game project implements this interface against whatever backend it uses. `URiftInventoryComponent` calls `SaveInventory` when the inventory changes (debounced) and `LoadInventory` when the player's PlayerState is initialized.

`FRiftInventorySaveData` is a serializable struct containing all item instances, their definitions, and their memory data. It is backend-agnostic — JSON serializable, binary serializable, or however the game's backend expects it.

\---

### 5.9 MVVM UI Architecture

The previous `ModularInventory` implementation had difficulty in the MVVM layer. The core issue with MVVM in Unreal is that it is easy to create ViewModels that either hold stale references or update too aggressively. RiftVaultUI solves this with three rules:

**Rule 1 — ViewModels are owned by the subsystem, not by widgets.**  
`URiftInventoryUISubsystem` (a UGameInstanceSubsystem) owns all ViewModels. Widgets request a ViewModel from the subsystem rather than creating their own. This means a ViewModel survives widget teardown and is shared between multiple widgets displaying the same data.

**Rule 2 — All ViewModel updates are event-driven.**  
ViewModels do not poll. They bind to delegates on `URiftInventoryComponent` during initialization and update their fields only when relevant events fire. `UE\_MVVM\_SET\_PROPERTY\_VALUE` (Unreal's field notification macro) is used for every property update so UMG bindings receive change notifications correctly.

**Rule 3 — Cross-object references inside ViewModels use TWeakObjectPtr.**  
A ViewModel holding a `TObjectPtr<URiftItemInstance>` will keep that item alive even after it is removed from inventory, potentially causing stale display data. `TWeakObjectPtr<URiftItemInstance>` allows the ViewModel to check `IsValid()` before accessing the object and display an empty state if the item is gone.

\---

## 6\. Data Flow Diagrams

### 6.1 Adding an Item

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
    ├── Run each fragment's InitializeMemory()
    └── Return initialized instance
    │
    ▼
URiftContainer::TryAcceptItem()
    ├── Check capacity
    ├── Check acceptance tags
    └── Check for existing partial stack (if stackable)
    │
    ▼
Operation queued → Executed next tick
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

### 6.2 Equipping an Item

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
    └── Update FRiftEquipmentState
    │
    ▼
FRiftEquipmentSlotChanged event broadcast
    ├── URiftMutableEquipmentComponent
    │       ├── Build FRiftMutableParameter list from fragment + instance memory
    │       ├── Apply parameters to UCustomizableObjectInstance
    │       └── Mutable async regeneration begins
    └── URiftViewModel\_Equipment → field notification → widget updates
```

### 6.3 Crafting an Item

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

### 6.4 Player Respawn Flow

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

## 7\. Naming Conventions

### Class Prefixes

|Prefix|Meaning|Examples|
|-|-|-|
|`A`|AActor subclass|`ARiftPickup`, `ARiftWeaponActor`|
|`U`|UObject subclass|`URiftItemDefinition`, `URiftInventoryComponent`|
|`F`|Struct or plain C++ class|`FRiftLootEntry`, `FRiftEquipmentState`|
|`E`|Enum|`EItemLifecycle`, `EContainerType`|
|`I`|UInterface|`IRiftInventoryInterface`|
|`T`|Template class|`TRiftWeakArray<>` (if needed)|

### Module Naming

All modules are prefixed `RiftVault`. Classes within modules use `Rift` as the class prefix (not `RiftVault`) to keep class names readable.

### Fragment Naming

All fragment classes follow the pattern `URiftFragment\_<Capability>` where Capability is a noun describing what the fragment adds:

* `URiftFragment\_Stack` — adds stacking
* `URiftFragment\_Equippable` — adds equippability
* `URiftFragment\_Condition` — adds wear/condition tracking
* `URiftFragment\_Display` — adds display metadata
* `URiftFragment\_Value` — adds economic value

### Memory Naming

Fragment memory structs follow `FRift<FragmentCapability>Memory`:

* `FRiftStackMemory`
* `FRiftConditionMemory`

### Ability Naming

GAS abilities follow `URiftAbility\_<Verb>`:

* `URiftAbility\_Equip`
* `URiftAbility\_Craft`
* `URiftAbility\_Buy`
* `URiftAbility\_Repair`

### GAS Effect Naming

`URiftEffect\_<Noun>`:

* `URiftEffect\_Wear`
* `URiftEffect\_Repair`

### ViewModel Naming

`URiftViewModel\_<Subject>`:

* `URiftViewModel\_Item`
* `URiftViewModel\_Container`
* `URiftViewModel\_Equipment`
* `URiftViewModel\_Vendor`

\---

## 8\. Dependency Graph

```
RiftVaultTests
    └── RiftVaultUI
    └── RiftVaultEditor
    └── RiftVaultDurability
    └── RiftVaultEconomy
    └── RiftVaultCrafting
    └── RiftVaultLoot
    └── RiftVaultEquipmentMutable
    └── RiftVaultEquipment
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

RiftVaultEquipmentMutable
    └── RiftVaultEquipment
    └── RiftVaultInventory
    └── RiftVaultCore

RiftVaultEquipment
    └── RiftVaultInventory
    └── RiftVaultCore

RiftVaultInventory
    └── RiftVaultCore

RiftVaultCore
    └── (UE5 engine modules only)
```

\---

## 9\. Fragment Reference

Complete reference of all fragments shipping in the initial version.

### URiftFragment\_Stack

**Purpose:** Enables an item to stack — multiple instances of the same item type sharing a single inventory slot up to a maximum quantity.

|Property|Type|Description|
|-|-|-|
|`MaxStackSize`|int32|The maximum number of items that can share one slot. 1 = not stackable.|
|`InitialQuantity`|int32|The quantity when a new instance is created (defaults to 1).|

**Memory:** `FRiftStackMemory`

|Property|Type|Description|
|-|-|-|
|`CurrentQuantity`|int32|How many of this item are in this stack right now.|

\---

### URiftFragment\_Equippable

**Purpose:** Marks an item as equippable and defines what happens when it is equipped.

|Property|Type|Description|
|-|-|-|
|`EquipmentSlotTag`|FGameplayTag|The slot this item occupies when equipped (e.g. `RiftVault.Slot.Weapon.Primary`).|
|`GrantedAbilities`|TArray<TSubclassOf<UGameplayAbility>>|Abilities granted to the owner's ASC when this item is equipped.|
|`ActiveMutableParameters`|TArray<FRiftMutableParameter>|Mutable parameters to apply when the item is in the Active state.|
|`HolsteredMutableParameters`|TArray<FRiftMutableParameter>|Mutable parameters for the Holstered state.|
|`WeaponSocketName`|FName|Socket name on the character mesh to attach ARiftWeaponActor to (weapon items only). Empty for body equipment.|

**Memory:** None. Equipment state is tracked by `URiftEquipmentComponent`, not the item instance.

\---

### URiftFragment\_Condition

**Purpose:** Gives an item a durability/wear value that degrades over use and can be repaired.

|Property|Type|Description|
|-|-|-|
|`MaxCondition`|float|The maximum condition value (e.g. 100.0).|
|`DegradationTags`|TArray<FGameplayTag>|Gameplay event tags that trigger condition loss on this item (e.g. `GameEvent.Hit.Melee`).|
|`ConditionPerEvent`|float|How much condition is lost per degradation event.|
|`BrokenMutableParameters`|TArray<FRiftMutableParameter>|Mutable parameters applied when the item reaches zero condition (visual wear state).|

**Memory:** `FRiftConditionMemory`

|Property|Type|Description|
|-|-|-|
|`CurrentCondition`|float|The item's current condition. 0 = broken.|
|`bIsBroken`|bool|Convenience flag, true when CurrentCondition <= 0.|

\---

### URiftFragment\_Display

**Purpose:** Provides the display metadata for an item — name, icon, description, rarity.

|Property|Type|Description|
|-|-|-|
|`DisplayName`|FText|The player-facing item name. FText supports localization.|
|`ShortDescription`|FText|One-line description shown in tooltips.|
|`FullDescription`|FText|Longer lore/stat description for the item detail panel.|
|`Icon`|TSoftObjectPtr<UTexture2D>|Soft reference to the item icon texture. Soft to avoid loading all icons at startup.|
|`RarityTag`|FGameplayTag|Rarity tag (e.g. `RiftVault.Rarity.Rare`). Used by UI for color coding.|

**Memory:** None. Display data is the same for all instances of an item type.

\---

### URiftFragment\_Value

**Purpose:** Defines the economic value of an item for vendor interactions.

|Property|Type|Description|
|-|-|-|
|`BaseBuyPrice`|float|The base price to buy this item from a vendor.|
|`BaseSellPrice`|float|The base price a vendor will pay for this item.|
|`CurrencyTag`|FGameplayTag|Which currency attribute this item trades in (e.g. `RiftVault.Currency.Credits`).|
|`bCanBeSold`|bool|Whether this item can be sold to vendors at all.|

**Memory:** None. Prices are modified at runtime by `URiftOfferEvaluator`, not stored on the instance.

\---

## 10\. Known Constraints and Decisions

This section documents decisions that were explicitly made and why, so future maintainers understand the reasoning rather than treating constraints as accidents.

|Decision|Rationale|
|-|-|
|**No spatial inventory (all items 1x1)**|Spatial placement adds significant complexity (occupancy masks, rotation, grid queries) for minimal gameplay benefit in most games. Can be added as an optional module later without changing the core system.|
|**Mutable-only for equipment visuals**|Eliminates the entire legacy mesh swap system and all its edge cases. Mutable handles LODs, cloth, physics, and cosmetic customization better than manual mesh attachment.|
|**APawn not ACharacter throughout**|Mover replaces UCharacterMovementComponent and breaks many ACharacter assumptions. Building against APawn from the start costs nothing and ensures Mover compatibility.|
|**URiftInventoryComponent on PlayerState**|Inventory must survive pawn death. PlayerState is the correct place for persistent per-player data in a dedicated server setup.|
|**No client-side prediction for inventory**|Inventory data is authoritative and valuable. Rollbacks on misprediction would be jarring and confusing. The one-frame processing delay is imperceptible to players.|
|**Persistence behind an interface**|Every game has a different backend. Hardcoding a save game implementation in the plugin would make it useless for any project with online services.|
|**Currency as GAS attribute**|GAS already provides replication, clamping, and effect-based modification for attributes. Implementing a parallel currency system would duplicate all of that.|
|**Processing queue instead of immediate operations**|Prevents race conditions from simultaneous inventory mutations and provides a clean validation/rejection path.|
|**ViewModels owned by subsystem not by widgets**|Widget teardown should not destroy shared ViewModel state. The subsystem as owner ensures ViewModel lifetime matches the game session, not the UI session.|
|**Selective tests only**|Full test coverage for a plugin of this scale is not achievable for a solo project. Targeting only high-risk silent-failure scenarios maximizes the value per test written.|

\---

## 11\. Build Order

Modules must be built and validated in this order. Each module's tests should pass before moving to the next.

|Step|Module|Key deliverable|
|-|-|-|
|1|`RiftVaultCore`|URiftItemDefinition, URiftItemFragment, all 5 initial fragments, FRiftVaultTags, all interfaces and enums|
|2|`RiftVaultInventory`|URiftInventoryComponent, URiftItemInstance, URiftContainer, processing queue, URiftInventoryNetProxy|
|3|`RiftVaultEquipment`|URiftEquipmentComponent, FRiftEquipmentState, respawn flow|
|4|`RiftVaultEquipmentMutable`|URiftMutableEquipmentComponent, ARiftWeaponActor, URiftMutableWeaponComponent|
|5|`RiftVaultLoot`|ARiftPickup, URiftPickupComponent, URiftLootSelector\_DataTable, URiftLootHandler\_AddToInventory|
|6|`RiftVaultDurability`|URiftEffect\_Wear, URiftEffect\_Repair, URiftAbility\_Repair, broken item flow|
|7|`RiftVaultCrafting`|URiftRecipeDefinition, URiftCraftingComponent, URiftAbility\_Craft|
|8|`RiftVaultEconomy`|URiftVendorComponent, URiftAbility\_Buy, URiftAbility\_Sell, URiftAttributeSet\_Wealth|
|9|`RiftVaultUI`|URiftViewModel base, item and container ViewModels, URiftInventoryUISubsystem|
|10|`RiftVaultEditor`|Factories and customizations|
|11|`RiftVaultTests`|Written alongside steps 2-8, consolidated and run here|

\---

## 12\. Testing Strategy

Tests live in `RiftVaultTests` and use Unreal's `DEFINE\_SPEC` automation framework. Each spec file corresponds to one system and tests a focused set of scenarios.

### High Priority — Silent failure possible

**Stacking (RiftVaultInventory)**

* Adding items to a partial stack merges correctly
* Adding items that exceed stack max creates overflow into a new stack
* Adding items to a full container with a full stack of the same item correctly rejects
* Splitting a stack produces two valid stacks summing to the original quantity
* Moving a stack between containers preserves quantity

**Serialization (RiftVaultInventory)**

* A full inventory round-trips through `FRiftInventorySaveData` without data loss
* Fragment memories survive serialization (condition, stack count)
* An empty inventory serializes and deserializes without errors
* An inventory with items of every fragment type serializes correctly

**Crafting (RiftVaultCrafting)**

* Exact ingredients present → craft succeeds, ingredients consumed, result added
* Missing one ingredient → craft fails, no ingredients consumed
* Ingredient present but insufficient quantity → craft fails, no ingredients consumed
* Craft with `bConsumed = false` ingredient → ingredient not consumed
* Craft result correctly spawns item with expected fragment memories

**Condition (RiftVaultDurability)**

* Wear effect reduces condition memory value
* Condition reaching zero sets `bIsBroken = true` on memory
* Broken item triggers ability removal on URiftEquipmentComponent
* Repair effect restores condition, clears broken flag
* Condition value persists through serialization

### Medium Priority — Detectable but investigation required

**Economy (RiftVaultEconomy)**

* Buy deducts correct Wealth attribute amount
* Buy with insufficient Wealth is blocked by ability tag requirements
* Sell adds correct Wealth attribute amount
* URiftOfferEvaluator price calculation is correct for base case

**Loot (RiftVaultLoot)**

* DataTable loot selector produces a valid item definition
* Empty loot table returns no results without crashing
* Pickup delivers items to inventory correctly via URiftLootHandler\_AddToInventory

### Low Priority — Verifiable by inspection

**Equipment (RiftVaultEquipment)**

* Equipping an item grants the abilities from URiftFragment\_Equippable
* Unequipping removes those ability grants
* Equipping a broken item is blocked

\---

## 13\. Future Considerations

These are features that are deliberately out of scope for the initial version but that the architecture supports without modification.

* **Spatial inventory as an optional module.** The container system is built around a flat array. A `URiftSpatialContainer` subclass could override slot management to add occupancy mask logic without touching the base system.
* **Item modifiers / enchantments.** A new fragment `URiftFragment\_Modifiers` could hold a list of modifier objects that apply stat changes. The GAS attribute system would handle the application.
* **Crafting stations.** `URiftCraftingComponent` already accepts a required station tag on recipes. A crafting station actor that grants a tag to nearby players' ASCs is a trivial addition.
* **Item rarity tiers with stat scaling.** `URiftFragment\_Display` already has a rarity tag. A new processor step in `URiftItemProcessor` could scale fragment values based on rarity at instance creation time.
* **Trading between players.** A trading session object that holds two inventories in escrow until both players confirm. The inventory component's interface already supports the necessary move operations.
* **Mutable cosmetic customization UI.** A separate module (`RiftVaultCustomization`) could expose Mutable parameters directly to the player for cosmetic customization (colour pickers, pattern selectors etc.).

\---

## 14\. Glossary

|Term|Definition|
|-|-|
|**ASC**|Ability System Component. Unreal's `UAbilitySystemComponent`. Manages GAS abilities, effects, and attributes for an actor.|
|**COI**|Customizable Object Instance. Mutable's `UCustomizableObjectInstance`. The runtime unique mesh produced by Mutable for one character or item.|
|**DataAsset**|A `UDataAsset` subclass. A content browser asset that holds designer-authored data. No logic, just data.|
|**Fragment**|A `URiftItemFragment` subclass. Adds a specific capability or data group to an item definition by composition.|
|**Fragment Memory**|A `FRiftFragmentMemory` subclass. Stores per-instance runtime data for a fragment that needs it.|
|**GAS**|Gameplay Ability System. Unreal's framework for abilities, effects, attributes, and gameplay tags.|
|**Item Definition**|`URiftItemDefinition`. A DataAsset describing an item type. Never instantiated at runtime directly.|
|**Item Instance**|`URiftItemInstance`. A runtime UObject representing one item currently in an inventory. Holds a reference to its definition and its fragment memories.|
|**Mover**|Epic's experimental movement plugin for UE5, replacing `UCharacterMovementComponent`. Works at the APawn level.|
|**Mutable**|Epic's procedural mesh plugin. Generates unique skeletal meshes at runtime from a designer-authored `UCustomizableObject`.|
|**Net Proxy**|`URiftInventoryNetProxy`. An actor component that handles replication of inventory state, keeping that concern separate from inventory logic.|
|**PlayerState**|`APlayerState`. A replicated actor that persists across pawn respawns. The correct place for inventory in a dedicated server setup.|
|**Processing Queue**|A FIFO queue in `URiftInventoryComponent` that serializes inventory mutations to prevent race conditions.|
|**ViewModel**|A `UMVVMViewModelBase` subclass. Exposes inventory data to UMG widgets via field notifications. Owned by `URiftInventoryUISubsystem`.|
|**Widget Controller**|A `UObject` subclass that mediates between widget user intent and system operations. Keeps widgets free of business logic.|

\---

*End of RiftVault Technical Design Document v0.1.0*

\---

## 15\. Architecture Review — Lessons from Implementation Attempt 1

> Added after implementation session 1-2. These are issues discovered during the first build attempt that must be resolved before starting implementation again.

### 15.1 Module Boundary Problem — Fragments vs Items

**What went wrong:**
`URiftItemFragment` was placed in `RiftVaultCore` because fragments are referenced by `URiftItemDefinition` (also in Core). However `URiftItemFragment` methods need to take `URiftItemInstance\*` as parameters (which lives in `RiftVaultInventory`). This created a circular dependency:

* Core cannot depend on Inventory
* But Fragment (in Core) needs to reference ItemInstance (in Inventory)

**The hack we used (do not repeat):**
Changed all UFUNCTION params from `URiftItemInstance\*` to `UObject\*` and cast inside implementations. This is ugly, breaks Blueprint type safety, and caused LNK2005 duplicate symbol errors from UHT auto-generating wrapper methods.

**The correct solution — open question for architecture session:**

Option A — `RiftVaultFragments` module above Inventory:

```
RiftVaultCore          ← tags, definitions, FRiftFragmentMemory
RiftVaultInventory     ← URiftItemInstance, URiftContainer, manager
RiftVaultFragments     ← URiftItemFragment base + all fragment subclasses
RiftVaultEquipment     ← equipment-specific fragments + equipment instances
```

Option B — Keep fragments in Inventory:

```
RiftVaultCore          ← tags, definitions, FRiftFragmentMemory
RiftVaultInventory     ← URiftItemFragment, URiftItemInstance, URiftContainer, manager
RiftVaultEquipment     ← equipment-specific fragments + equipment instances
```

Option B is simpler. Option A gives cleaner separation if the fragment count grows large.

**Decision needed:** Which option before any code is written.

### 15.2 UHT Cross-Module Type Rules

**Rule discovered:** A UFUNCTION in module A cannot take a parameter of a type defined in module B unless module A depends on module B in its Build.cs. UHT generates `MODULEB\_API` declarations in module A's generated .cpp files — if module A doesn't link module B, those symbols are undefined.

**Implication:** The module dependency graph must be fully designed before writing any UFUNCTION signatures. Any UFUNCTION that takes a custom type as a parameter locks in a module dependency.

### 15.3 BlueprintNativeEvent Wrapper Methods

**Rule discovered:** For `UFUNCTION(BlueprintNativeEvent)`, UHT auto-generates the non-`\_Implementation` wrapper (e.g. `ActivateItem`) inside `Module.X.cpp`. If the `.cpp` file also defines that wrapper, the linker sees two definitions → LNK2005.

**Rule:** Only ever define `FunctionName\_Implementation` in your `.cpp`. Never define the wrapper.

### 15.4 Tag Access Pattern

**Rule discovered:** Tags declared with `UE\_DEFINE\_GAMEPLAY\_TAG` / `UE\_DECLARE\_GAMEPLAY\_TAG\_EXTERN` are global variables. Access them directly as `Tag\_Rift\_X`. Do NOT create a singleton struct `FRiftVaultTags` with a `Get()` method and member variables — that pattern doesn't work with native tags and causes `C2653` errors.

### 15.5 DeveloperSettings Module Dependency

`UDeveloperSettings` requires `"DeveloperSettings"` in the module's Build.cs `PrivateDependencyModuleNames`. Missing this causes LNK2019 on all `UDeveloperSettings` vtable symbols.

### 15.6 FindFragmentByClass Template Syntax

`Definition->FindFragmentByClass<UMyFragment>()` causes MSVC C2275/C2059 parse errors when `UMyFragment` is not fully defined at the call site (only forward declared). Use the explicit StaticClass form instead:

```cpp
Cast<UMyFragment>(Definition->FindFragmentByClass(UMyFragment::StaticClass()))
```

### 15.7 Open Architecture Questions for Next Session

These must be answered before writing any code:

1. **Fragment module placement** — Option A (dedicated module) or Option B (in Inventory)?
2. **Should `URiftItemDefinition` reference `URiftItemFragment` directly, or via an interface?** The current approach has definitions owning fragment instances, but this ties Core to Fragment.
3. **Should fragment memories be strongly typed per fragment, or use a generic key-value store?** The current `TInstancedStruct` approach works but is complex to extend.
4. **Equipment on APawn vs ACharacter** — confirmed APawn. Document this explicitly so it isn't relitigated.
5. **Inventory on APlayerState** — confirmed. Document this explicitly.
6. **Should the processing queue be synchronous or async?** Sync is simpler but blocks the game thread during large inventory loads (e.g. loading a saved inventory with 100 items).
7. **Is `RiftVaultEquipmentMutable` a separate module or part of Equipment?** The Mutable integration is currently stubbed — decide before Equipment is written.

\---

*End of Architecture Review*

