# RiftVault Plugin — Setup Guide

**Version:** 1.2
**Engine:** Unreal Engine 5.7+
**Author:** RiftVault Development
**Last Updated:** 2026-03-21

### Changelog

| Version | Date | Changes |
|---|---|---|
| 1.0 | 2026-03-14 | Initial setup guide |
| 1.1 | 2026-03-17 | Added Section 9 (Equipment Setup) and Section 10 (World Pickups) |
| 1.2 | 2026-03-21 | Added Section 11 (Stack Fragment). Updated Section 8 Troubleshooting with replication and pickup gotchas. Clarified WaitForInitialized usage. Noted Server RPC requirement for dedicated server drag-drop. Updated pickup section with OnRep_Items explanation. |

---

## Table of Contents

1. [Prerequisites](#1-prerequisites)
2. [Installing the Plugin](#2-installing-the-plugin)
3. [Project Configuration](#3-project-configuration)
4. [Setting Up the Inventory](#4-setting-up-the-inventory)
5. [Creating Your First Item](#5-creating-your-first-item)
6. [Setting Up the UI](#6-setting-up-the-ui)
7. [Testing the Setup](#7-testing-the-setup)
8. [Troubleshooting](#8-troubleshooting)
9. [Setting Up Equipment](#9-setting-up-equipment)
10. [Setting Up World Pickups](#10-setting-up-world-pickups)
11. [Adding Stack Support to Items](#11-adding-stack-support-to-items)

---

## 1. Prerequisites

Before you begin, ensure your project meets the following requirements:

- Unreal Engine 5.7 or newer
- A C++ project (not Blueprint-only)
- The **GameplayAbilities** plugin enabled (required for GAS tags and equip abilities)
- The **ModelViewViewModel** plugin enabled (required for UI)
- The **Mutable** (CustomizableObject) plugin enabled if you are using equipment visuals

To enable plugins, go to **Edit → Plugins** and search for each by name.

---

## 2. Installing the Plugin

1. Copy the `RiftVault` folder into your project's `Plugins/` directory. If the `Plugins` folder does not exist, create it at the root of your project.

Your project structure should look like this:

```
MyProject/
├── Content/
├── Plugins/
│   └── RiftVault/
│       ├── Source/
│       └── RiftVault.uplugin
├── Source/
└── MyProject.uproject
```

2. Right-click your `.uproject` file and select **Generate Visual Studio project files**.
3. Open the solution in Visual Studio or Rider and build the project.
4. Open the project in Unreal Editor. If prompted to rebuild modules, click **Yes**.
5. Go to **Edit → Plugins**, search for **RiftVault**, and ensure it is enabled.

---

## 3. Project Configuration

### 3.1 Asset Manager

RiftVault uses the Asset Manager to scan for item and container definitions. You must register the asset types.

1. Go to **Edit → Project Settings → Asset Manager**.
2. Under **Primary Asset Types to Scan**, click the **+** button and add the following two entries:

**Entry 1 — Item Definitions:**
- Primary Asset Type: `RiftItemDefinition`
- Asset Base Class: `URiftItemDefinition`
- Directories: `/Game`

**Entry 2 — Container Definitions:**
- Primary Asset Type: `RiftContainerDefinition`
- Asset Base Class: `URiftContainerDefinition`
- Directories: `/Game`

3. Save the project settings.

### 3.2 Gameplay Tags

RiftVault uses Gameplay Tags to identify containers and item traits. The required tags ship with the plugin and are registered automatically. No manual tag setup is required.

---

## 4. Setting Up the Inventory

### 4.1 Create a Player State Blueprint

RiftVault's inventory lives on `APlayerState` so it survives pawn death and respawn. The inventory is replicated to the owning client only — other players cannot see your inventory.

1. In the Content Browser, right-click → **Blueprint Class**.
2. Search for and select **PlayerState** as the parent class.
3. Name it `BP_MyPlayerState`.
4. Open the Blueprint and click **Add Component**.
5. Search for and add **Rift Inventory Component**.

### 4.2 Create a Container Definition

A Container Definition defines the rules for a container — its tag, capacity, and which items it accepts.

1. In the Content Browser, right-click → **Miscellaneous → Data Asset**.
2. Select `URiftContainerDefinition` as the class.
3. Name it `DA_Backpack`.
4. Open it and configure:
   - **Display Name:** `Backpack`
   - **Container Tag:** `Rift.Container.Backpack`
   - **Container Type:** `Player Inventory`
   - **Capacity:** `20` (or your desired size)
   - **Item Compatibility Query:** Add a tag query that accepts items with `Rift.Item.Trait`

### 4.3 Assign the Container to the Inventory Component

1. Open `BP_MyPlayerState`.
2. Select the **Rift Inventory Component** in the Components panel.
3. In the Details panel, find **Default Containers**.
4. Click **+** and assign `DA_Backpack`.

### 4.4 Register the Player State in the Game Mode

1. Open your **Game Mode Blueprint** (or create one).
2. In the Details panel, set **Player State Class** to `BP_MyPlayerState`.
3. Set the Game Mode as the default in **World Settings** or **Project Settings → Maps & Modes**.

---

## 5. Creating Your First Item

### 5.1 Create an Item Definition

1. In the Content Browser, right-click → **Miscellaneous → Data Asset**.
2. Select `URiftItemDefinition` as the class.
3. Name it `DA_Item_Sword` (or any item name).
4. Open it and configure:
   - **Gameplay Tags:** Add `Rift.Item.Trait` — this tag allows the item to be accepted by the backpack container.
5. Under **Fragments**, click **+** and add a **Rift Fragment Display** fragment:
   - **Display Name:** `Iron Sword`
   - **Description:** `A basic iron sword.`
   - **Icon:** Assign a texture (optional)

### 5.2 Adding Items at Runtime

To add an item to the inventory at runtime via Blueprint:

1. Get a reference to the **Player State**.
2. Get the **Rift Inventory Component** from the Player State.
3. Call **Add Item** and pass in your `DA_Item_Sword` definition.

Example Blueprint flow:

```
Get Player State → Get Component by Class (URiftInventoryComponent) → Add Item (Definition: DA_Item_Sword)
```

> **Important:** `Add Item` is `BlueprintAuthorityOnly` — it must be called on the server. Calling it from a client Blueprint will do nothing. Use a Server RPC or call it from the Game Mode / Player State.

---

## 6. Setting Up the UI

### 6.1 Create WBP_ItemSlot

This widget represents a single item in the grid.

1. Right-click in Content Browser → **User Interface → Widget Blueprint**.
2. Name it `WBP_ItemSlot`.
3. Open it and go to **Class Settings → Parent Class** — set it to `URiftItemSlotWidget`.
4. In the Designer, build the hierarchy:
   - **Size Box** (root) — set Width and Height Override to `64`
     - **Overlay**
       - **Image** — name it exactly `ItemIcon`
       - **Text Block** — name it exactly `ItemDescription`
5. Go to **View Models** tab at the bottom:
   - Click **+** to add a ViewModel.
   - Set **ViewModel Name** to `ItemDisplayViewModel`
   - Set **Notify Field Value Class** to `Item Display ViewModel`
   - Set **Creation Type** to `Create Instance`
6. In **View Settings**, enable **Create View Without Bindings**.
7. In the **Event Graph**, implement the `OnItemSet` event:
   - Call **Get View Model** on `self` with name `ItemDisplayViewModel`
   - Cast to `URiftViewModel_ItemDisplay`
   - Call **Get Icon** → set the result on the `ItemIcon` image widget
   - Call **Get Description** → set the result on the `ItemDescription` text widget

> **Widget names matter:** `ItemIcon` and `ItemDescription` are `BindWidgetOptional` — the names must match exactly (case-sensitive). If names differ, the C++ base class cannot find the widgets and automatic show/hide on item set/clear will not work.

### 6.2 Create WBP_InventoryGrid

This widget displays a grid of item slots.

1. Right-click → **User Interface → Widget Blueprint**.
2. Name it `WBP_InventoryGrid`.
3. Open it and go to **Class Settings → Parent Class** — set it to `URiftInventoryGridWidget`.
4. In the Designer, build the hierarchy:
   - **Uniform Grid Panel** — name it exactly `ItemGrid`
5. Select the root widget (`WBP_InventoryGrid`) and in the Details panel under **RiftVault | Grid**:
   - Set **Item Slot Widget Class** to `WBP_ItemSlot`
   - Set **Container Definition** to `DA_Backpack`
   - Set **Columns Per Row** to your desired column count (e.g. `4`)

### 6.3 Add the Grid to the Viewport

In your **Player Controller Blueprint**, on `BeginPlay`:

1. Call **Create Widget** → select `WBP_InventoryGrid`.
2. Call **Add to Viewport** on the result.

### 6.4 Waiting for Inventory Initialization

The inventory component initializes asynchronously on the server and replicates `bIsInitialized` to the client. If you create the grid widget before the inventory is ready, no items will populate.

Use `K2_WaitForInitialized` (Blueprint name: **Wait For Initialized**) instead of binding to `OnInventoryInitialized`:

```
[Player Controller BeginPlay]
  → Get Player State → Get Rift Inventory Component
  → Wait For Initialized
      → On Complete (bSuccess) → Create Widget → Add to Viewport
```

`WaitForInitialized` calls the delegate immediately if the inventory is already initialized, or queues it to fire when initialization completes. It is safe to call before or after `BeginPlay`.

---

## 7. Testing the Setup

1. Play in Editor (PIE).
2. Open the **Output Log** (`Window → Output Log`).
3. Look for the following log lines to confirm everything initialized correctly:

```
URiftInventoryComponent::InitializeInventory — creating 1 default containers.
URiftInventoryComponent — Created container with tag: Rift.Container.Backpack, capacity: 20
URiftInventoryComponent::BroadcastInitialized — bSuccess: true
URiftInventoryGridWidget::BuildSlots — ItemGrid valid: true, ItemSlotWidgetClass valid: true, InventoryComponent valid: true
```

4. If you have a Blueprint calling `AddItem` on BeginPlay, you should also see:

```
URiftInventoryComponent::AddItem — Successfully added item <ItemName> to container Rift.Container.Backpack at slot 0.
URiftInventoryGridWidget::OnItemAdded — Item: <ItemInstance>, Container: Rift.Container.Backpack
```

---

## 8. Troubleshooting

### "ItemSlotWidgetClass valid: false"
The `Item Slot Widget Class` is not set on `WBP_InventoryGrid`. Open the widget, select the root, and set it in the Details panel under **RiftVault | Grid**.

### "ItemGrid valid: false"
The `Uniform Grid Panel` inside `WBP_InventoryGrid` is not named `ItemGrid`. The name must match exactly — it is case-sensitive.

### "Items in container: 0" on startup
Items are being added before the widget subscribes to `OnItemAdded`. Use the `K2_WaitForInitialized` pattern (see Section 6.4) to defer widget creation until the inventory is ready.

### No items appear at runtime after AddItem (inventory or pickup)
Check in order:
1. Is `AddItem` being called server-side? It is `BlueprintAuthorityOnly` — client calls do nothing.
2. Does the item definition have `Rift.Item.Trait` in its Gameplay Tags? Without this tag the container's compatibility query will reject the item.
3. Are pickup items appearing on the server but not the client? This is the `OnRep_Items` issue — see below.

### Pickup items appear on the server (Output Log shows them added) but not in the client UI

Items added by server-side operations (like world pickups) broadcast `OnItemAdded` on the server only. The client widget never receives it unless `ReplicatedUsing = OnRep_Items` is set on `URiftContainer.Items`.

As of v1.0.5, `URiftContainer.Items` uses `ReplicatedUsing = OnRep_Items`. When the server's Items array replicates to the client, `OnRep_Items` diffs the new array against `PreviousItems` and broadcasts `OnItemAdded` / `OnItemRemoved` for each change. This drives the client UI.

If items still do not appear after walking into a pickup:
1. Confirm the plugin build includes the `OnRep_Items` implementation (check `URiftContainer.cpp` for the function).
2. Confirm `URiftInventoryComponent::ReplicateSubobjects` is replicating the `URiftContainer` subobject to the client (check that `RepFlags->bNetOwner` is true for the owning connection).
3. Confirm the widget is binding to `OnItemAdded` **after** calling `WaitForInitialized` — binding before the inventory is ready may miss the initial broadcast.

### Equip ability does nothing / item stays in backpack

Check:
1. `URiftAbility_Equip` and `URiftAbility_Unequip` are granted to the ASC before activation (or `bAutoGrantEquipAbilities` is true on `URiftEquipmentComponent`).
2. `SupportedSlots` on `URiftEquipmentComponent` contains the target slot tag.
3. A `URiftContainerDefinition` with `ContainerTag = <SlotTag>` exists and is in `DefaultContainers`.
4. The item's `URiftFragment_Equippable` has `EquipmentSlotTag` set to match the target slot.

### Drag-drop equip works in Standalone but not in Dedicated Server (multiplayer)

Drag-drop cross-container moves require the `Server_MoveItemToContainerAtSlot` RPC to be called. Check `URiftItemSlotWidget::OnItemDropped_Implementation` — both cross-container drop paths should call:

```cpp
InventoryComponent->MoveItemToContainerAtSlot(DraggedItem, Container.Get(), SlotIndex);  // local prediction
InventoryComponent->Server_MoveItemToContainerAtSlot(DraggedItem, Container.Get(), SlotIndex);  // authority
```

If only the local call is present, the drop works client-side visually but the server never processes it, so equipment abilities are never granted and Mutable visuals never update on other clients.

### Mutable visual not updating on first item swap

This is the deferred update issue. If `URiftMutableEquipmentComponent` calls `UpdateSkeletalMeshAsync` directly in both `ApplyMutableParameters` and `ResetMutableParameters`, a swap (unequip then equip in the same tick) produces two competing calls — Mutable ignores the second, so the reset wins and the new item's appearance never applies.

As of v1.0.5, both methods call `ScheduleMutableUpdate()` which coalesces multiple calls in the same tick into one `UpdateSkeletalMeshAsync`. Verify the fix is present in `URiftMutableEquipmentComponent.cpp`.

### Mutable parameters not applying after equip

1. Confirm `URiftMutableEquipmentComponent` is on the same pawn as `URiftEquipmentComponent`.
2. Confirm the body mesh `UCustomizableSkeletalComponent` has `"Rift.Component.BodyMesh"` in its **Component Tags** array (not in Gameplay Tags — in ComponentTags in the Details panel).
3. The `UCustomizableObjectInstance` on the body mesh must have a valid `UCustomizableObject` assigned before equipping.
4. Confirm the parameter names in `ActiveMutableParameters` exactly match the parameter names in your `UCustomizableObject` asset.

### "No component tagged Rift.Component.BodyMesh found" log warning

The tag string must be set in the `UCustomizableSkeletalComponent`'s built-in **Component Tags** array (under **Actor → Component Tags** in the Details panel), not in the actor's Gameplay Tags. The value must be exactly: `Rift.Component.BodyMesh`

### "Failed to FillRuntimeData for Primary Asset Type RiftItemDefinition"
The Asset Manager is not configured. Follow section **3.1 Asset Manager** above.

### Grid shows wrong number of columns
Set **Columns Per Row** on `WBP_InventoryGrid` in the Details panel under **RiftVault | Grid**.

### ViewModel not updating visuals
Ensure **Create View Without Bindings** is enabled in View Settings on `WBP_ItemSlot`, and that `OnItemSet` is implemented in the Blueprint event graph.

---

## 9. Setting Up Equipment

This section walks through wiring the equipment system on a pawn so items can be equipped, visual changes appear via Mutable, and GAS abilities are granted and revoked automatically.

### 9.1 Create Per-Slot Container Definitions

Each equipment slot needs its own `URiftContainerDefinition` data asset with capacity 1. The container tag **must equal the slot tag** — this is how `URiftEquipmentComponent` locates the right container at runtime.

For each slot you want to support, create one data asset:

1. Right-click in Content Browser → **Miscellaneous → Data Asset**.
2. Select `URiftContainerDefinition`.
3. Configure:

| Field | Example value (Head slot) |
|---|---|
| Display Name | `Head Slot` |
| Container Tag | `Rift.Slot.Armor.Head` |
| Capacity | `1` |
| Item Compatibility Query | `ANY(Rift.Slot.Armor.Head)` |

Repeat for every slot you want (Chest, Legs, Weapon.Primary, Weapon.Secondary, etc.).

4. Open `BP_MyPlayerState`, select **Rift Inventory Component**, and add each new slot container to **Default Containers** alongside `DA_Backpack`.

> **Why the slot tag equals the container tag:** `URiftEquipmentComponent::EquipItem` calls `GetContainerByTag(SlotTag)` directly. No separate mapping is needed.

### 9.2 Set Up the Pawn Blueprint

1. Open your Pawn Blueprint.
2. Add the following components:
   - **Rift Equipment Component** (`URiftEquipmentComponent`)
   - **Rift Mutable Equipment Component** (`URiftMutableEquipmentComponent`)
3. Select **Rift Equipment Component** in the Components panel, then in the Details panel:
   - Under **Equipment → Supported Slots**, click **+** and add each slot tag you created container definitions for (e.g. `Rift.Slot.Armor.Head`, `Rift.Slot.Weapon.Primary`).
   - Optionally set **Weapon Actor Class** if you have a custom `ARiftWeaponActor` subclass.
   - Leave **Auto Grant Equip Abilities** checked (default `true`) to have the equip/unequip abilities granted automatically.
4. Select your `UCustomizableSkeletalComponent` (the body mesh), then in the Details panel under **Component Tags**, click **+** and add: `Rift.Component.BodyMesh`

This tag is how `URiftMutableEquipmentComponent` finds the body mesh at runtime without a hard reference.

### 9.3 Grant Equip and Unequip Abilities

If **Auto Grant Equip Abilities** is checked on `URiftEquipmentComponent`, the equip and unequip abilities are granted automatically once the linked inventory is ready. You do not need to grant them manually.

If you have **Auto Grant Equip Abilities** unchecked (manual control), grant them in your PlayerState or pawn's `BeginPlay`:

In C++:
```cpp
UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
ASC->GiveAbility(FGameplayAbilitySpec(URiftAbility_Equip::StaticClass(), 1));
ASC->GiveAbility(FGameplayAbilitySpec(URiftAbility_Unequip::StaticClass(), 1));
```

In Blueprint, use **Give Ability** on the ASC node with the class set to `Rift Ability Equip` and `Rift Ability Unequip`.

### 9.4 Create an Equippable Item Definition

1. Create a `URiftItemDefinition` data asset (e.g. `DA_Item_IronHelmet`).
2. Under **Fragments**, add a **Rift Fragment Equippable** fragment.
3. Configure the fragment:

| Field | Description |
|---|---|
| Equipment Slot Tag | `Rift.Slot.Armor.Head` — must match a supported slot on the pawn and a container in the inventory |
| Weapon Socket Name | Leave empty for armor/helmets. Set to a socket name for weapons. |
| Active Mutable Parameters | Parameters to apply to the body mesh (or weapon mesh) when equipped. |
| Holstered Mutable Parameters | Parameters to apply when holstered (optional). |

4. Optionally add a **Rift Fragment Display** fragment for name and icon.

> **Item tags are automatic:** You do not need to manually add the slot tag to the item definition. `URiftFragment_Equippable` automatically emits `EquipmentSlotTag` into the item's gameplay tag container at runtime, so the per-slot container's compatibility query (`ANY(Rift.Slot.Armor.Head)`) accepts it automatically.

### 9.5 Equip an Item via Blueprint

To equip an item that is already in the player's inventory:

1. Get a reference to the **Player Pawn**.
2. Use **Send Gameplay Event To Actor** (from `UAbilitySystemBlueprintLibrary`):
   - **Actor:** the Player Pawn
   - **Event Tag:** `Rift.Ability.Equip`
   - **Payload → Optional Object:** the `URiftItemInstance` to equip
   - **Payload → Target Tags:** add the target slot tag (e.g. `Rift.Slot.Armor.Head`)

The ability activates server-side, validates the item, moves it to the slot container, grants abilities, and fires `OnItemEquipped` — which `URiftMutableEquipmentComponent` listens to and applies Mutable parameters.

Alternatively, you can drag the item directly from the backpack grid into an equipment slot widget. The drag-drop handler in `URiftItemSlotWidget` automatically sends the `Server_MoveItemToContainerAtSlot` RPC, which triggers `OnInventoryItemMoved` on `URiftEquipmentComponent`, which calls `FinishEquip` — the full equip setup runs without needing to send the gameplay event manually.

### 9.6 Mutable Parameters Setup

For equipment that changes appearance (armor, helmets, body paint):

1. On the `URiftFragment_Equippable` fragment, add entries to **Active Mutable Parameters**:
   - **Parameter Name:** The exact name of the Mutable parameter as defined in your `UCustomizableObject` asset.
   - **Parameter Type:** `Integer`, `Float`, or `Color`.
   - **Int Option Name / Float Value / Color Value:** The value to set when the item is equipped.

2. `URiftMutableEquipmentComponent` applies these to the pawn's body `UCustomizableObjectInstance` and calls `UpdateSkeletalMeshAsync()` automatically (deferred to the next tick to coalesce swap operations).

> **Mutable API note:** The exact method names (`SetIntParameterSelectedOption`, `SetFloatParameterSelectedOption`, `SetColorParameterSelectedOption`) have varied across Mutable versions. If you encounter compile errors, check `URiftMutableEquipmentComponent.cpp` and `URiftMutableWeaponComponent.cpp` — both contain `// NOTE:` comments marking the Mutable API calls to verify.

### 9.7 Unequip an Item via Blueprint

To unequip the item in a given slot:

1. Get a reference to the **Player Pawn**.
2. Use **Send Gameplay Event To Actor**:
   - **Actor:** the Player Pawn
   - **Event Tag:** `Rift.Ability.Unequip`
   - **Payload → Target Tags:** add the slot tag to unequip (e.g. `Rift.Slot.Armor.Head`)

The ability activates server-side, revokes GAS abilities, destroys weapon actors, returns the item to the backpack, and fires `OnItemUnequipped`.

---

## 10. Setting Up World Pickups

`ARiftPickup` is a ready-made actor you can place in the level or spawn dynamically to grant items to players who walk into it.

### 10.1 Place a Pickup in the Level

1. In the **Place Actors** panel or Content Browser, find `ARiftPickup` (or a Blueprint subclass of it).
2. Drag it into the level.
3. Select it and in the Details panel, find the **Pickup** component (`URiftPickupComponent`):
   - Set **Item Definition** to the `URiftItemDefinition` asset to grant.
   - Set **Quantity** to the amount to give.
   - Leave **Destroy On Collect** checked to remove the actor after pickup.

### 10.2 How Pickup Collection Reaches the Client UI

Understanding this flow prevents confusion about why items sometimes appear on the server log but not in the client inventory widget:

```
Player walks into pickup [Server]
→ URiftPickupComponent::TryCollect → URiftInventoryComponent::AddItem
→ Items array updated on server
→ OnItemAdded fires on server (server-only delegates)
→ Items array replicates to owning client (COND_OwnerOnly)
→ URiftContainer::OnRep_Items fires on client
→ Diffs new Items against PreviousItems
→ OnItemAdded.Broadcast(NewItem, Container) fires on CLIENT
→ Widget receives OnItemAdded and populates the slot
```

The key point: **client widgets must bind to `OnItemAdded` on the inventory component** (not the server), and they will receive it via `OnRep_Items` when replication arrives. Use `K2_WaitForInitialized` to ensure the widget is set up and bound before any items arrive.

### 10.3 Overlap Detection

`URiftPickupComponent` automatically binds to the first `UPrimitiveComponent` on the actor that has **Generate Overlap Events** enabled. For `ARiftPickup`, this is the built-in `OverlapSphere` (64 cm radius by default).

To change the radius:
1. Select the actor in the level.
2. In the Details panel, select the **Overlap Sphere** component.
3. Adjust **Sphere Radius** under **Shape**.

### 10.4 Spawn a Pickup Dynamically

To spawn a pickup from Blueprint (e.g. from a loot drop):

1. Call **Spawn Actor from Class** → `ARiftPickup` (or subclass).
2. After spawning, get the **Pickup** component and call **Set Item Definition** and **Set Quantity** if needed, or configure them in the Blueprint CDO.

### 10.5 Custom Pickup Actors

To add pickup behaviour to an existing actor (e.g. a chest):

1. Add `URiftPickupComponent` to the actor as a component.
2. Ensure at least one `UPrimitiveComponent` on the actor has **Generate Overlap Events = true**.
3. Set **Item Definition** and **Quantity** on the component.

### 10.6 Troubleshooting Pickups

**Overlap fires but item not added**
Check that `URiftInventoryComponent` exists on the player's `APlayerState`. `URiftPickupComponent` walks `Collector → GetPlayerState() → FindComponentByClass<URiftInventoryComponent>()`.

**Server log shows item added but client UI never updates**
This is the `OnRep_Items` mechanism — see Section 10.2. Verify:
1. The widget is binding to `OnItemAdded` after `WaitForInitialized` completes.
2. The plugin build includes `OnRep_Items` in `URiftContainer.cpp`.
3. `ReplicateSubobjects` in `URiftInventoryComponent.cpp` is correctly replicating the container subobject.

**Item added but wrong quantity**
The item has `URiftFragment_Stack` — stacking logic applies. Overflow (beyond `MaxStackSize`) creates new instances. If the backpack is full, `AddItem` will silently drop the excess.

**Pickup doesn't disappear after collect**
Ensure **Destroy On Collect** is checked on the `URiftPickupComponent`.

---

## 11. Adding Stack Support to Items

### 11.1 What Stacking Does

By default, each item occupies exactly one inventory slot regardless of quantity. Adding `URiftFragment_Stack` to an item definition allows multiple units to share one slot up to `MaxStackSize`.

When you call `AddItem(Definition, 10)` with `MaxStackSize = 5`, the inventory will:
1. Find or create a partial stack and fill it to 5.
2. Create a second instance with the remaining 5 units.

### 11.2 Add the Stack Fragment

1. Open your `URiftItemDefinition` data asset.
2. Under **Fragments**, click **+** and add **Stack Fragment**.
3. Configure:
   - **Max Stack Size:** The maximum units per slot (e.g. `20` for arrows, `5` for health potions).
   - **Show Stack Count:** Check to display the quantity overlay in the item slot widget. Uncheck for items like currency where quantity is shown elsewhere.

### 11.3 Merging Stacks via Drag-Drop

When the player drags a stackable item onto another item of the same type, `URiftItemSlotWidget::OnItemDropped_Implementation` automatically detects the merge opportunity (via `URiftFragment_Stack::CanMergeWith`) and transfers quantity instead of swapping.

If the target stack is full, no transfer occurs and both items remain where they are.

### 11.4 Splitting Stacks

To split a stack in C++ or Blueprint:

```cpp
// C++
URiftInventoryComponent* InvComp = ...;
FGuid NewStackId = InvComp->SplitStack(Item, AmountToSplit);
```

In Blueprint, call **Split Stack** on the Rift Inventory Component with the item and the number of units to split off. The original item retains the remainder. The new item is placed in the first available slot.

### 11.5 Design Note: RemoveQuantity Floors at 1

`URiftFragment_Stack::RemoveQuantity` will never reduce `CurrentQuantity` below 1. This is intentional — the stack always has at least 1 unit until `RemoveItem` is called explicitly. The inventory component handles this: when `ActuallyTransferred >= SourceQuantity` during a drag-merge, it calls `RemoveItem` to destroy the now-empty source stack.

If you are calling `RemoveQuantity` directly in C++ or Blueprint, you must also call `RemoveItem` when the quantity should reach zero.

---

*RiftVault Plugin — Setup Guide v1.2*
