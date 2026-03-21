#include "Tags/RiftVaultTags.h"

// ------------------------------------------------------------------
// Container Tags
// ------------------------------------------------------------------

UE_DEFINE_GAMEPLAY_TAG(Tag_Rift_Container, "Rift.Container");
UE_DEFINE_GAMEPLAY_TAG(Tag_Rift_Container_Backpack, "Rift.Container.Backpack");
UE_DEFINE_GAMEPLAY_TAG(Tag_Rift_Container_Equipment, "Rift.Container.Equipment");
UE_DEFINE_GAMEPLAY_TAG(Tag_Rift_Container_Vendor, "Rift.Container.Vendor");
UE_DEFINE_GAMEPLAY_TAG(Tag_Rift_Container_Loot, "Rift.Container.Loot");

// ------------------------------------------------------------------
// Item Trait Tags
// ------------------------------------------------------------------

UE_DEFINE_GAMEPLAY_TAG(Tag_Rift_Item_Trait, "Rift.Item.Trait");
UE_DEFINE_GAMEPLAY_TAG(Tag_Rift_Item_Trait_Stackable, "Rift.Item.Trait.Stackable");
UE_DEFINE_GAMEPLAY_TAG(Tag_Rift_Item_Trait_Equippable, "Rift.Item.Trait.Equippable");
UE_DEFINE_GAMEPLAY_TAG(Tag_Rift_Item_Trait_HasCondition, "Rift.Item.Trait.HasCondition");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(Tag_Rift_Item_Trait_Broken, "Rift.Item.Trait.Broken", "Set by the durability system when an item's condition reaches zero. Blocks equipping.");
UE_DEFINE_GAMEPLAY_TAG(Tag_Rift_Item_Trait_Droppable, "Rift.Item.Trait.Droppable");
UE_DEFINE_GAMEPLAY_TAG(Tag_Rift_Item_Trait_Sellable, "Rift.Item.Trait.Sellable");

// ------------------------------------------------------------------
// Item Rarity Tags
// ------------------------------------------------------------------

UE_DEFINE_GAMEPLAY_TAG(Tag_Rift_Item_Rarity, "Rift.Item.Rarity");
UE_DEFINE_GAMEPLAY_TAG(Tag_Rift_Item_Rarity_Common, "Rift.Item.Rarity.Common");
UE_DEFINE_GAMEPLAY_TAG(Tag_Rift_Item_Rarity_Uncommon, "Rift.Item.Rarity.Uncommon");
UE_DEFINE_GAMEPLAY_TAG(Tag_Rift_Item_Rarity_Rare, "Rift.Item.Rarity.Rare");
UE_DEFINE_GAMEPLAY_TAG(Tag_Rift_Item_Rarity_Epic, "Rift.Item.Rarity.Epic");
UE_DEFINE_GAMEPLAY_TAG(Tag_Rift_Item_Rarity_Legendary, "Rift.Item.Rarity.Legendary");

// ------------------------------------------------------------------
// Event Tags
// ------------------------------------------------------------------

UE_DEFINE_GAMEPLAY_TAG(Tag_Rift_Event, "Rift.Event");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(Tag_Rift_Event_Item_Added, "Rift.Event.Item.Added", "Broadcast when an item is added to an inventory.");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(Tag_Rift_Event_Item_Removed, "Rift.Event.Item.Removed", "Broadcast when an item is removed from an inventory.");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(Tag_Rift_Event_Item_StackChanged, "Rift.Event.Item.StackChanged", "Broadcast when an item's stack size changes.");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(Tag_Rift_Event_Item_ConditionChanged, "Rift.Event.Item.ConditionChanged", "Broadcast when an item's condition value changes.");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(Tag_Rift_Event_Item_Equipped, "Rift.Event.Item.Equipped", "Broadcast when an item is moved into an equipment slot.");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(Tag_Rift_Event_Item_Unequipped, "Rift.Event.Item.Unequipped", "Broadcast when an item is removed from an equipment slot.");

// ------------------------------------------------------------------
// Status Tags
// ------------------------------------------------------------------

UE_DEFINE_GAMEPLAY_TAG_COMMENT(Tag_Rift_Status_Inventory_Busy, "Rift.Status.Inventory.Busy", "Set on the player's ASC while the inventory processing queue is active. Use in GAS tag requirements to block abilities that mutate inventory.");

// ------------------------------------------------------------------
// Equipment Slot Tags
// ------------------------------------------------------------------

UE_DEFINE_GAMEPLAY_TAG(Tag_Rift_Slot, "Rift.Slot");
UE_DEFINE_GAMEPLAY_TAG(Tag_Rift_Slot_Weapon_Primary, "Rift.Slot.Weapon.Primary");
UE_DEFINE_GAMEPLAY_TAG(Tag_Rift_Slot_Weapon_Secondary, "Rift.Slot.Weapon.Secondary");
UE_DEFINE_GAMEPLAY_TAG(Tag_Rift_Slot_Armor_Head, "Rift.Slot.Armor.Head");
UE_DEFINE_GAMEPLAY_TAG(Tag_Rift_Slot_Armor_Chest, "Rift.Slot.Armor.Chest");
UE_DEFINE_GAMEPLAY_TAG(Tag_Rift_Slot_Armor_Legs, "Rift.Slot.Armor.Legs");

// ------------------------------------------------------------------
// Currency Tags
// ------------------------------------------------------------------

UE_DEFINE_GAMEPLAY_TAG(Tag_Rift_Currency, "Rift.Currency");
UE_DEFINE_GAMEPLAY_TAG(Tag_Rift_Currency_Credits, "Rift.Currency.Credits");
