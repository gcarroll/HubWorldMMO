#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Fragments/URiftItemFragment.h"
#include "URiftFragment_Display.generated.h"

class UTexture2D;

/**
 * Provides display data for an item — name, description, icon and quality.
 *
 * No per-instance state needed — all display data is shared across instances
 * of the same type because the definition (not the instance) describes what an
 * item looks like. There is never a reason for two potions to have different names.
 *
 * This fragment is the only source of truth for player-visible identity.
 * InternalName on URiftItemDefinition is for engineering/debug use only.
 *
 * Soft-references are used for the icon texture so that all item icons are not
 * loaded into memory at startup — they are loaded on demand via GetIcon().
 */
UCLASS(DisplayName = "Display Fragment")
class RIFTVAULTINVENTORY_API URiftFragment_Display : public URiftItemFragment
{
    GENERATED_BODY()

public:

    URiftFragment_Display();

    /**
     * Returns the player-facing display name for this item.
     * FText is used (not FString) to support localization.
     */
    UFUNCTION(BlueprintPure, Category = "RiftVault|Fragments|Display")
    FORCEINLINE FText GetDisplayName() const { return DisplayName; }

    /**
     * Returns the short description shown in tooltips and item inspection panels.
     * FText is used to support localization.
     */
    UFUNCTION(BlueprintPure, Category = "RiftVault|Fragments|Display")
    FORCEINLINE FText GetDescription() const { return Description; }

    /**
     * Synchronously loads and returns the item icon texture.
     * Uses TSoftObjectPtr::LoadSynchronous() — safe to call from the game thread
     * but will stall on first access if the asset is not already loaded.
     * Consider using the async load path for large icon atlases.
     *
     * @return  The loaded UTexture2D, or null if the soft ref is unset.
     */
    UFUNCTION(BlueprintPure, Category = "RiftVault|Fragments|Display")
    UTexture2D* GetIcon() const;

    /**
     * Returns the rarity tag used by the UI for color coding and sort ordering.
     * Expected to be a tag under Rift.Item.Rarity (e.g. Rift.Item.Rarity.Rare).
     */
    UFUNCTION(BlueprintPure, Category = "RiftVault|Fragments|Display")
    FORCEINLINE FGameplayTag GetRarityTag() const { return RarityTag; }

    /**
     * Returns the numeric quality value (0-100) for this item type.
     * This is the base definition value used for scaling stats and vendor pricing.
     * At runtime the effective quality may be modified via GAS attributes on the instance.
     */
    UFUNCTION(BlueprintPure, Category = "RiftVault|Fragments|Display")
    FORCEINLINE int32 GetQuality() const { return Quality; }

protected:

    /**
     * Player-facing name for this item. FText supports localization.
     * Defaults to "Item" so null-name items are still usable during development.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Display")
    FText DisplayName;

    /**
     * Short description shown in tooltips and item panels.
     * Defaults to "No description." to avoid empty UI slots during development.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Display")
    FText Description;

    /**
     * Soft reference to the item icon texture.
     * Soft (not hard) reference to avoid loading all icons at startup —
     * only loaded when GetIcon() is called or the UI explicitly requests it.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Display")
    TSoftObjectPtr<UTexture2D> Icon;

    /**
     * Rarity tag used by the UI for color coding (e.g. Rift.Item.Rarity.Rare).
     * The Categories meta restricts the picker in the editor to the Rift.Item.Rarity namespace.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Display", meta = (Categories = "Rift.Item.Rarity"))
    FGameplayTag RarityTag;

    /**
     * Numeric quality value for this item (0-100).
     * Used for scaling stats, crafting requirements, and vendor pricing.
     * Driven by the GAS attribute system at runtime — this is the base definition value.
     * Clamped to [0, 100] via UIMin/UIMax and ClampMin/ClampMax meta.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Display", meta = (UIMin = 0, ClampMin = 0, UIMax = 100, ClampMax = 100))
    int32 Quality = 0;
};
