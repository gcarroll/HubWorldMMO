#pragma once

#include "CoreMinimal.h"
#include "NativeGameplayTags.h"

// ------------------------------------------------------------------
// Container Tags
// ------------------------------------------------------------------

/** Root tag for all RiftVault containers. */
RIFTVAULTCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Tag_Rift_Container);

/** Identifies the player's main backpack container. */
RIFTVAULTCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Tag_Rift_Container_Backpack);

/** Identifies the player's equipment container. Items here are considered equipped. */
RIFTVAULTCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Tag_Rift_Container_Equipment);

/** Identifies a vendor's stock container. */
RIFTVAULTCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Tag_Rift_Container_Vendor);

/** Identifies a loot container in the world. */
RIFTVAULTCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Tag_Rift_Container_Loot);

// ------------------------------------------------------------------
// Item Trait Tags
// ------------------------------------------------------------------

/** Root tag for all item traits. */
RIFTVAULTCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Tag_Rift_Item_Trait);

/** Item can be stacked. Requires URiftFragment_Stack. */
RIFTVAULTCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Tag_Rift_Item_Trait_Stackable);

/** Item can be equipped. Requires URiftFragment_Equippable. */
RIFTVAULTCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Tag_Rift_Item_Trait_Equippable);

/** Item has a condition value that degrades. Requires URiftFragment_Condition. */
RIFTVAULTCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Tag_Rift_Item_Trait_HasCondition);

/** Item is currently broken (condition reached zero). Set by the durability system. */
RIFTVAULTCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Tag_Rift_Item_Trait_Broken);

/** Item can be dropped into the world. */
RIFTVAULTCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Tag_Rift_Item_Trait_Droppable);

/** Item can be sold to a vendor. */
RIFTVAULTCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Tag_Rift_Item_Trait_Sellable);

// ------------------------------------------------------------------
// Item Rarity Tags
// ------------------------------------------------------------------

/** Root tag for all item rarity tiers. */
RIFTVAULTCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Tag_Rift_Item_Rarity);

RIFTVAULTCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Tag_Rift_Item_Rarity_Common);
RIFTVAULTCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Tag_Rift_Item_Rarity_Uncommon);
RIFTVAULTCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Tag_Rift_Item_Rarity_Rare);
RIFTVAULTCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Tag_Rift_Item_Rarity_Epic);
RIFTVAULTCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Tag_Rift_Item_Rarity_Legendary);

// ------------------------------------------------------------------
// Event Tags
// ------------------------------------------------------------------

/** Root tag for all RiftVault events. */
RIFTVAULTCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Tag_Rift_Event);

/** Broadcast when an item is added to an inventory. */
RIFTVAULTCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Tag_Rift_Event_Item_Added);

/** Broadcast when an item is removed from an inventory. */
RIFTVAULTCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Tag_Rift_Event_Item_Removed);

/** Broadcast when an item's stack size changes. */
RIFTVAULTCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Tag_Rift_Event_Item_StackChanged);

/** Broadcast when an item's condition changes. */
RIFTVAULTCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Tag_Rift_Event_Item_ConditionChanged);

/** Broadcast when an item is equipped into an equipment slot. */
RIFTVAULTCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Tag_Rift_Event_Item_Equipped);

/** Broadcast when an item is removed from an equipment slot. */
RIFTVAULTCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Tag_Rift_Event_Item_Unequipped);

// ------------------------------------------------------------------
// Status Tags
// ------------------------------------------------------------------

/** Set on the player's ASC while the inventory processing queue is active. Gates abilities that mutate inventory. */
RIFTVAULTCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Tag_Rift_Status_Inventory_Busy);

// ------------------------------------------------------------------
// Equipment Slot Tags
// ------------------------------------------------------------------

/** Root tag for all equipment slots. Game projects add their own slot tags beneath this. */
RIFTVAULTCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Tag_Rift_Slot);

/** Primary weapon slot. */
RIFTVAULTCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Tag_Rift_Slot_Weapon_Primary);

/** Secondary weapon slot. */
RIFTVAULTCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Tag_Rift_Slot_Weapon_Secondary);

/** Head armor slot. */
RIFTVAULTCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Tag_Rift_Slot_Armor_Head);

/** Chest armor slot. */
RIFTVAULTCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Tag_Rift_Slot_Armor_Chest);

/** Legs armor slot. */
RIFTVAULTCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Tag_Rift_Slot_Armor_Legs);

// ------------------------------------------------------------------
// Currency Tags
// ------------------------------------------------------------------

/** Root tag for all currency types. */
RIFTVAULTCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Tag_Rift_Currency);

/** Default credits currency used in economy transactions. */
RIFTVAULTCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Tag_Rift_Currency_Credits);
