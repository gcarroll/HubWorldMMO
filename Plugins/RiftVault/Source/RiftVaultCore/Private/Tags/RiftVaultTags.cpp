#include "Tags/RiftVaultTags.h"

// ------------------------------------------------------------------
// Container Tags
// ------------------------------------------------------------------

UE_DEFINE_GAMEPLAY_TAG(Tag_Rift_Container, "Rift.Container");
UE_DEFINE_GAMEPLAY_TAG(Tag_Rift_Container_Backpack, "Rift.Container.Backpack");
UE_DEFINE_GAMEPLAY_TAG(Tag_Rift_Container_Equipment, "Rift.Container.Equipment");
UE_DEFINE_GAMEPLAY_TAG(Tag_Rift_Container_Vendor, "Rift.Container.Vendor");
UE_DEFINE_GAMEPLAY_TAG(Tag_Rift_Container_Loot, "Rift.Container.Loot");
UE_DEFINE_GAMEPLAY_TAG(Tag_Rift_Container_Ground, "Rift.Container.Ground");

// ------------------------------------------------------------------
// Item Trait Tags
// ------------------------------------------------------------------

UE_DEFINE_GAMEPLAY_TAG(Tag_Rift_Item_Trait, "Rift.Item.Trait");
UE_DEFINE_GAMEPLAY_TAG(Tag_Rift_Item_Trait_Stackable, "Rift.Item.Trait.Stackable");
UE_DEFINE_GAMEPLAY_TAG(Tag_Rift_Item_Trait_Equippable, "Rift.Item.Trait.Equippable");
UE_DEFINE_GAMEPLAY_TAG(Tag_Rift_Item_Trait_HasCondition, "Rift.Item.Trait.HasCondition");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(Tag_Rift_Item_Trait_Broken, "Rift.Item.Trait.Broken", "Set by the durability system when an item's condition reaches zero. Blocks equipping.");
UE_DEFINE_GAMEPLAY_TAG(Tag_Rift_Item_Trait_Droppable, "Rift.Item.Trait.Droppable");
UE_DEFINE_GAMEPLAY_TAG(Tag_Rift_Item_Trait_Deletable, "Rift.Item.Trait.Deletable");
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

// ------------------------------------------------------------------
// Component Identity Tags
// ------------------------------------------------------------------

UE_DEFINE_GAMEPLAY_TAG_COMMENT(Tag_Rift_Component_BodyMesh, "Rift.Component.BodyMesh", "Identifies the UCustomizableSkeletalComponent that represents the character body mesh. Add this tag string to the component's ComponentTags array in the pawn Blueprint.");

// ------------------------------------------------------------------
// Weapon State Tags
// ------------------------------------------------------------------

UE_DEFINE_GAMEPLAY_TAG_COMMENT(Tag_Rift_Weapon_State,        "Rift.Weapon.State",        "Root tag for weapon stance states. Pushed to the pawn's ASC when a weapon is equipped.");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(Tag_Rift_Weapon_State_Rifle,  "Rift.Weapon.State.Rifle",  "Active while a rifle is equipped.");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(Tag_Rift_Weapon_State_Pistol, "Rift.Weapon.State.Pistol", "Active while a pistol is equipped.");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(Tag_Rift_Weapon_State_Sword,  "Rift.Weapon.State.Sword",  "Active while a sword is equipped.");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(Tag_Rift_Weapon_State_Shield, "Rift.Weapon.State.Shield", "Active while a shield is equipped.");

// ------------------------------------------------------------------
// Ability Activation Tags
// ------------------------------------------------------------------

UE_DEFINE_GAMEPLAY_TAG_COMMENT(Tag_Rift_Ability_Equip,   "Rift.Ability.Equip",   "Send to activate URiftAbility_Equip. OptionalObject = URiftItemInstance, TargetTags contains the slot tag.");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(Tag_Rift_Ability_Unequip, "Rift.Ability.Unequip", "Send to activate URiftAbility_Unequip. TargetTags contains the slot tag.");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(Tag_Rift_Ability_Drop,   "Rift.Ability.Drop",   "Send to activate URiftAbility_Drop. OptionalObject = URiftItemInstance to drop.");
