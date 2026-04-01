# RiftVault Plugin — Setup Guide

**Version:** 1.0  
**Engine:** Unreal Engine 5.7+  
**Author:** RiftVault Development

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

---

## 1. Prerequisites

Before you begin, ensure your project meets the following requirements:

- Unreal Engine 5.7 or newer
- A C++ project (not Blueprint-only)
- The **GameplayAbilities** plugin enabled (required for GAS tags)
- The **ModelViewViewModel** plugin enabled (required for UI)

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

RiftVault's inventory lives on the `APlayerState` so it survives pawn death and respawn.

1. In the Content Browser, right-click → **Blueprint Class**.
2. Search for and select **PlayerState** as the parent class.
3. Name it `BP_MyPlayerState`.
4. Open the Blueprint and click **Add Component**.
5. Search for and add **Rift Inventory Component**.

### 4.2 Create a Container Definition

A Container Definition defines the rules for a container — its tag and capacity.

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
       - **Image** — name it exactly `Icon`
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
   - Call **Get Icon** → set the result on the `Icon` image widget
   - Call **Get Description** → set the result on the `ItemDescription` text widget

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
Items are being added before the widget subscribes to `OnItemAdded`. This is normal — the widget will catch all subsequent `AddItem` calls via the delegate. If you need items present on startup, add them in the Player State's `BeginPlay` after a short delay, or use the `WaitForInitialized` pattern.

### No items appear at runtime after AddItem
Check that the item definition has `Rift.Item.Trait` in its Gameplay Tags. Without this tag the container's compatibility query will reject the item.

### "Failed to FillRuntimeData for Primary Asset Type RiftItemDefinition"
The Asset Manager is not configured. Follow section **3.1 Asset Manager** above.

### Grid shows wrong number of columns
Set **Columns Per Row** on `WBP_InventoryGrid` in the Details panel under **RiftVault | Grid**.

### ViewModel not updating visuals
Ensure **Create View Without Bindings** is enabled in View Settings on `WBP_ItemSlot`, and that `OnItemSet` is implemented in the Blueprint event graph.

---

*RiftVault Plugin — User Guide v1.0*
