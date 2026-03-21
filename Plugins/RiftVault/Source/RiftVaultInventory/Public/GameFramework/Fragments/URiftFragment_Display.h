#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Fragments/URiftItemFragment.h"
#include "URiftFragment_Display.generated.h"

class UTexture2D;

/**
 * Provides display data for an item — name, description, icon and quality.
 * No per-instance state needed — all display data is shared across instances of the same type.
 */
UCLASS(DisplayName = "Display Fragment")
class RIFTVAULTINVENTORY_API URiftFragment_Display : public URiftItemFragment
{
    GENERATED_BODY()

public:

    URiftFragment_Display();

    UFUNCTION(BlueprintPure, Category = "RiftVault|Fragments|Display")
    FORCEINLINE FText GetDisplayName() const { return DisplayName; }

    UFUNCTION(BlueprintPure, Category = "RiftVault|Fragments|Display")
    FORCEINLINE FText GetDescription() const { return Description; }

    UFUNCTION(BlueprintPure, Category = "RiftVault|Fragments|Display")
    UTexture2D* GetIcon() const;

    UFUNCTION(BlueprintPure, Category = "RiftVault|Fragments|Display")
    FORCEINLINE FGameplayTag GetRarityTag() const { return RarityTag; }

    UFUNCTION(BlueprintPure, Category = "RiftVault|Fragments|Display")
    FORCEINLINE int32 GetQuality() const { return Quality; }

protected:

    /** Player-facing name for this item. FText supports localization. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Display")
    FText DisplayName;

    /** Short description shown in tooltips and item panels. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Display")
    FText Description;

    /**
     * Soft reference to the item icon texture.
     * Soft to avoid loading all icons at startup.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Display")
    TSoftObjectPtr<UTexture2D> Icon;

    /** Rarity tag used by the UI for color coding (e.g. Rift.Item.Rarity.Rare). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Display", meta = (Categories = "Rift.Item.Rarity"))
    FGameplayTag RarityTag;

    /**
     * Numeric quality value for this item (0-100).
     * Used for scaling stats, crafting requirements, and vendor pricing.
     * Driven by the GAS attribute system at runtime — this is the base definition value.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Display", meta = (UIMin = 0, ClampMin = 0, UIMax = 100, ClampMax = 100))
    int32 Quality = 0;
};
