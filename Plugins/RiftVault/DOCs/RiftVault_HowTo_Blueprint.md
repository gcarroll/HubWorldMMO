# RiftVault — Blueprint How-To Guide

**Version:** 1.0
**Engine:** Unreal Engine 5.7+
**Author:** RiftVault Development
**Last Updated:** 2026-03-21

This guide walks through the most common inventory and equipment tasks using Blueprint. It assumes the plugin is already installed and configured (see the Setup Guide). No C++ knowledge is required.

---

## Table of Contents

1. [Getting the Inventory Component](#1-getting-the-inventory-component)
2. [Adding Items to a Player's Inventory](#2-adding-items-to-a-players-inventory)
3. [Checking if a Player Has an Item](#3-checking-if-a-player-has-an-item)
4. [Removing an Item](#4-removing-an-item)
5. [Reading Item Display Data](#5-reading-item-display-data)
6. [Checking Stack Quantity](#6-checking-stack-quantity)
7. [Equipping an Item](#7-equipping-an-item)
8. [Unequipping an Item](#8-unequipping-an-item)
9. [Checking What is Equipped](#9-checking-what-is-equipped)
10. [Reacting to Inventory Events](#10-reacting-to-inventory-events)
11. [Building an Inventory Grid UI](#11-building-an-inventory-grid-ui)
12. [Building an Equipment Slot UI](#12-building-an-equipment-slot-ui)
13. [Waiting for Inventory Initialization](#13-waiting-for-inventory-initialization)
14. [Common Pitfalls](#14-common-pitfalls)

---

## 1. Getting the Inventory Component

The inventory lives on `APlayerState`, not the Pawn. Whenever you need to access the inventory, walk from the Player Controller to the Player State first.

**From Player Controller:**
```
Get Player Controller (index 0)
  → Get Player State
  → Get Component by Class (URiftInventoryComponent)
```

**From the Pawn:**
```
[Your Pawn reference]
  → Get Player State
  → Get Component by Class (URiftInventoryComponent)
```

**From BeginPlay on the Player State itself:**
```
[Self (Player State)]
  → Get Component by Class (URiftInventoryComponent)
```

> The inventory is `COND_OwnerOnly` — only the owning client and the server have it. If you try to access another player's inventory, it will be null.

---

## 2. Adding Items to a Player's Inventory

> **Server only.** Must be called from server-side logic (Game Mode, server RPC, or `HasAuthority()` check).

**Add one item:**
```
Rift Inventory Component
  → Add Item
      Definition: [Your URiftItemDefinition asset]
      Quantity:   1
```

**Add multiple of a stackable item:**
```
Rift Inventory Component
  → Add Item
      Definition: DA_Arrow
      Quantity:   50
```
If the item has `URiftFragment_Stack` with `MaxStackSize = 20`, this creates three instances: two stacks of 20 and one stack of 10.

**Add to a specific container (e.g. force into backpack only):**
```
Rift Inventory Component
  → Get Container by Tag (ContainerTag: Rift.Container.Backpack)
  → Add Item to Container
      Definition: DA_Item_Sword
      Target Container: [result of above]
      Quantity: 1
```

---

## 3. Checking if a Player Has an Item

**Does the player have any item of this type?**
```
Rift Inventory Component
  → Has Item (Definition: DA_Item_Sword)
  → [Branch on bool result]
```

**Get all instances of a specific item type:**
```
Rift Inventory Component
  → Get Items by Definition (Definition: DA_Arrow)
  → [Iterate array to count, inspect, or remove specific instances]
```

**Find which container holds an item:**
```
Rift Inventory Component
  → Get Container for Item (Item: [your URiftItemInstance])
  → [Use container reference as needed]
```

---

## 4. Removing an Item

> **Server only.**

**Remove a specific item instance:**
```
Rift Inventory Component
  → Remove Item (Item: [URiftItemInstance reference])
```

**Remove all items of a type (e.g. consume all arrows):**
```
Rift Inventory Component
  → Get Items by Definition (Definition: DA_Arrow)
  → For Each Loop
      → Remove Item (Item: [Array Element])
```

**Reduce stack quantity (consume some but not all):**
```
[URiftItemInstance reference]
  → Find Fragment by Class (Fragment Class: URiftFragment_Stack)
  → Cast to URiftFragment_Stack
  → Remove Quantity (Item: [instance], Amount: 5)
```
Note: `RemoveQuantity` floors at 1. If you want to consume the last unit, call `Remove Item` on the inventory component instead.

---

## 5. Reading Item Display Data

Item display data (name, icon, description) lives in `URiftFragment_Display` on the item's definition.

**Get display name:**
```
[URiftItemInstance reference]
  → Get Definition
  → Find Fragment by Class (Fragment Class: URiftFragment_Display)
  → Cast to URiftFragment_Display
  → Get Display Name
```

**Get icon:**
```
[URiftItemInstance reference]
  → Get Definition
  → Find Fragment by Class (Fragment Class: URiftFragment_Display)
  → Cast to URiftFragment_Display
  → Get Icon
  → [Set Brush from Texture on your Image widget]
```

**Get rarity tag (for color coding):**
```
[URiftItemInstance reference]
  → Get Definition
  → Find Fragment by Class (Fragment Class: URiftFragment_Display)
  → Cast to URiftFragment_Display
  → Get Rarity Tag
```

> If the item has no Display Fragment, `Find Fragment by Class` returns null — always check for null before casting.

---

## 6. Checking Stack Quantity

**Get current quantity of a specific item:**
```
[URiftItemInstance reference]
  → Find Fragment by Class (Fragment Class: URiftFragment_Stack)
  → Cast to URiftFragment_Stack
  → Get Current Quantity (Item: [instance])
```

**Get max stack size (type-level):**
```
[URiftItemInstance reference]
  → Get Definition
  → Find Fragment by Class (Fragment Class: URiftFragment_Stack)
  → Cast to URiftFragment_Stack
  → Get Max Stack Size
```

**Check if stack has room:**
```
[URiftItemInstance]
  → Find Fragment by Class (URiftFragment_Stack)
  → Cast to URiftFragment_Stack
  → Get Available Space (Item: [instance])   ← returns 0 if full
```

---

## 7. Equipping an Item

Equipping uses the Gameplay Ability System. Send a gameplay event to the pawn — the equip ability handles the rest on the server.

**Equip via Send Gameplay Event:**

1. Get a reference to the `URiftItemInstance` you want to equip (e.g. from the inventory grid widget's `OnItemSet` event or from `Get Items by Definition`).
2. Get a reference to the Player Pawn.

```
[From wherever you trigger the equip, e.g. a button click or drag-drop]

UAbilitySystemBlueprintLibrary → Send Gameplay Event To Actor
  Actor:        [Player Pawn reference]
  Event Tag:    Rift.Ability.Equip
  Payload:
    Optional Object:  [URiftItemInstance to equip]
    Target Tags:      [Slot tag, e.g. Rift.Slot.Armor.Head]
```

> The event is received server-side by `URiftAbility_Equip`. It reads the item from `Optional Object` and the slot from `Target Tags[0]`. The item must already be in the player's inventory.

**Quick Blueprint flow for equipping from a button press:**

```
[Button Clicked]
  → Get All Items From Inventory
  → Filter by Has Fragment of Class (URiftFragment_Equippable)
  → Get First Result
  → Get Definition → Find Fragment by Class (URiftFragment_Equippable) → Get Equipment Slot Tag
  → Send Gameplay Event To Actor
      Actor: Owning Pawn
      Event Tag: Rift.Ability.Equip
      Payload → Optional Object: item instance
      Payload → Target Tags: slot tag from above
```

---

## 8. Unequipping an Item

```
UAbilitySystemBlueprintLibrary → Send Gameplay Event To Actor
  Actor:        [Player Pawn reference]
  Event Tag:    Rift.Ability.Unequip
  Payload:
    Target Tags: [Slot tag to clear, e.g. Rift.Slot.Armor.Head]
```

No item reference is needed — the ability reads the slot tag and unequips whatever is there.

---

## 9. Checking What is Equipped

> These queries are safe on both server and clients — `URiftEquipmentComponent` replicates equipment state to all clients via FastArray.

**Get the item in a specific slot:**
```
[Player Pawn]
  → Get Component by Class (URiftEquipmentComponent)
  → Get Item in Slot (Slot Tag: Rift.Slot.Armor.Head)
  → [Check for null — null means slot is empty]
```

**Is a slot occupied?**
```
URiftEquipmentComponent
  → Is Slot Occupied (Slot Tag: Rift.Slot.Weapon.Primary)
  → [Branch]
```

**Get all equipped items:**
```
URiftEquipmentComponent
  → Get All Equipped Items
  → [Iterate array]
```

---

## 10. Reacting to Inventory Events

Bind to delegates on `URiftInventoryComponent` to react when items change. Always bind **after** calling `Wait For Initialized` to ensure the component is ready.

### OnItemAdded — item landed in a container

```
[After Wait For Initialized completes]
Rift Inventory Component
  → Bind Event to On Item Added
      → [Your custom event: Item (URiftItemInstance), Container (URiftContainer)]
      → Do something with the new item (update UI, play sound, etc.)
```

> On the owning client, this fires via `URiftContainer::OnRep_Items` when the server replicates the items array. This is what drives inventory UI updates after world pickups.

### OnItemRemoved — item was removed from the inventory

```
Rift Inventory Component
  → Bind Event to On Item Removed
      → Item, Container
      → Update UI / close tooltip / etc.
```

### OnItemMoved — item changed slot or container

```
Rift Inventory Component
  → Bind Event to On Item Moved
      → Item, From Container, To Container
      → [From Container == To Container means same-container slot swap]
```

### OnItemEquipped / OnItemUnequipped (Equipment Component)

```
[Player Pawn]
  → Get Component by Class (URiftEquipmentComponent)
  → Bind Event to On Item Equipped
      → Item (URiftItemInstance), Slot Tag (FGameplayTag)
      → Update equipment slot widget, play equip sound, etc.
```

These fire on both server and client via FastArray replication callbacks.

### OnInventoryInitialized — inventory is ready

See Section 13 for the correct pattern. Prefer `Wait For Initialized` over binding directly to this delegate.

---

## 11. Building an Inventory Grid UI

### Minimal setup

The fastest path to a working inventory grid is to subclass `URiftInventoryGridWidget` in Blueprint. The C++ base class handles all inventory listening, slot building, and item add/remove events automatically.

1. Create `WBP_InventoryGrid` with parent class `URiftInventoryGridWidget`.
2. Add a **Uniform Grid Panel** and name it `ItemGrid`.
3. Set **Item Slot Widget Class** to your slot widget Blueprint.
4. Set **Container Definition** to the container you want to display (e.g. `DA_Backpack`).

When the widget is added to the viewport, it:
- Calls `WaitForInitialized` on the player's inventory component.
- Builds slot widgets for every slot in the container.
- Binds to `OnItemAdded` and `OnItemRemoved` to keep the grid in sync.

### Slot widget setup

Create `WBP_ItemSlot` with parent class `URiftItemSlotWidget`. The minimum required content is:

- An `Image` widget named exactly `ItemIcon` (for the item icon).
- A `TextBlock` named exactly `ItemDescription` (for the item name).

In the Blueprint event graph:
- Implement `OnItemSet(NewItemInstance)` to update the icon and text.
- Implement `OnItemCleared()` to reset to an empty appearance.

The C++ base class automatically shows/hides these widgets when items are set and cleared. Your `OnItemSet` / `OnItemCleared` events are called after those visibility changes.

### Drag and drop

Drag-drop is handled entirely in C++ — no Blueprint setup is required for basic move/swap behavior. To customize:

- Set **Drag Visual Class** on `WBP_ItemSlot` to a different widget class that will be shown under the cursor during a drag.
- Implement `IRiftDragVisualInterface` on the drag visual widget and handle `OnDragItemSet` to display the correct icon.
- Override `OnDragEntered` and `OnDragLeft` in your slot widget Blueprint to show/hide a highlight border when a drag is hovering.
- Override `OnItemDropped` in Blueprint to add custom drop logic (e.g. confirm dialog before dropping a rare item).

---

## 12. Building an Equipment Slot UI

An equipment slot widget is just another `URiftItemSlotWidget` — the difference is it is linked to an equipment-slot container (capacity 1) rather than the backpack.

### Setup

1. Create `WBP_EquipmentSlot` with parent class `URiftItemSlotWidget`.
2. In the **Designer**, add the icon and name widgets (`ItemIcon`, `ItemDescription`).
3. In your HUD Blueprint or Equipment Panel widget, call `Init Slot` on each equipment slot widget, passing:
   - **Container:** `Get Container by Tag (Rift.Slot.Armor.Head)` from the inventory component.
   - **Slot Index:** `0` (equipment containers have capacity 1, so slot 0 is the only slot).

```
[Widget → Construct or when inventory is ready]
Rift Inventory Component
  → Get Container by Tag (Container Tag: Rift.Slot.Armor.Head)
  → WBP_EquipmentSlot → Init Slot
      In Container: [above container]
      In Slot Index: 0

Rift Equipment Component
  → Bind Event to On Item Equipped
      → If Slot Tag == Rift.Slot.Armor.Head:
          WBP_EquipmentSlot → Set Item (NewItemInstance, Container, SlotIndex: 0)

  → Bind Event to On Item Unequipped
      → If Slot Tag == Rift.Slot.Armor.Head:
          WBP_EquipmentSlot → Clear Item
```

### Drag-drop between backpack and equipment slot

Because both grids and equipment slots use `URiftItemSlotWidget`, drag-drop between them works automatically. Dropping a helmet onto an equipment slot widget calls `OnItemDropped_Implementation`, which calls `MoveItemToContainerAtSlot` locally and then sends `Server_MoveItemToContainerAtSlot` — the equipment component detects the move via `OnInventoryItemMoved` and runs the full equip setup.

---

## 13. Waiting for Inventory Initialization

The inventory component initializes asynchronously. The server loads data from the persistence backend, then sets `bIsInitialized = true`, which replicates to the owning client. Any code that reads inventory state must wait for this.

### The correct pattern

```
[Player Controller → Begin Play]
  → Get Player State → Get Rift Inventory Component
  → K2 Wait For Initialized (Delegate → On Initialized Complete)

[On Initialized Complete (bSuccess)]
  → [bSuccess] Branch → True path: create and add inventory widget to viewport
                         False path: log error, show "inventory unavailable" message
```

`K2_WaitForInitialized` (Blueprint name: **Wait For Initialized**) calls the delegate immediately if already initialized. This means it is safe to call at any time — even if the inventory was initialized before your widget was created.

### What NOT to do

```
// WRONG — binds to the delegate but may miss it if inventory was already initialized
Rift Inventory Component → On Inventory Initialized → Bind Event → ...
```

If the inventory initialized before the widget was created and bound, the delegate fires once and is gone. `WaitForInitialized` handles this race condition correctly.

---

## 14. Common Pitfalls

### AddItem does nothing
`AddItem` is server-only (`BlueprintAuthorityOnly`). Calling it from a client-side Blueprint silently does nothing. Put inventory mutations in server RPCs, Game Mode, or behind an `Is Server` / `Has Authority` check.

### Item is added but the UI never updates
The widget is probably not bound to `OnItemAdded` yet when items arrive. Use the `Wait For Initialized` pattern (Section 13) to ensure bindings are in place before inventory data is received.

### Get Player State returns null
`GetPlayerState()` from the pawn may return null briefly after possession. If you see this on clients, use a short timer delay or bind to `OnPawnSet` on the Player State.

### Items added before the widget is ready (on server startup)
Default items (from `DefaultItems` on the inventory component) are added during `InitializeInventory` on the server. The owning client will receive them via `OnRep_Items` replication after the widget is set up — as long as the widget is bound to `OnItemAdded` before the replication arrives. The `Wait For Initialized` pattern handles this.

### Fragment not found / null cast after Find Fragment by Class
The item's definition does not have that fragment. Always check for null after `Find Fragment by Class` before casting or calling methods on the result.

### Equip ability fires but item stays in backpack
The most common cause is `SupportedSlots` not containing the slot tag on `URiftEquipmentComponent`, or the slot container definition missing from `DefaultContainers` on the inventory component. Check both.

### Mutable parameters not updating visually on first swap
On the first equip after loading, the Mutable instance may still be generating. Subsequent equips (swapping one item for another) were affected by a double-`UpdateSkeletalMeshAsync` bug (see TDD 18.16). As of v1.0.5 this is fixed by the deferred flush timer in `URiftMutableEquipmentComponent`.

### GetContainerByTag returns null
Either no container with that tag was created (check `DefaultContainers` on the inventory component), or you are calling it before `WaitForInitialized` has completed.

### SendGameplayEvent does nothing on client
`URiftAbility_Equip` and `URiftAbility_Unequip` are `ServerOnly` abilities — they only activate on the server. `SendGameplayEventToActor` can be called from the client and will correctly route to the server, but the target actor must be the pawn (not the player state), and the ASC must be on the pawn.
