#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Styling/SlateBrush.h"
#include "ViewModels/URiftViewModel_Fragment.h"
#include "GameFramework/Items/URiftItemInstance.h"
#include "URiftViewModel_ItemDisplay.generated.h"

class UTexture2D;
class URiftInventoryComponent;

UCLASS(BlueprintType, Blueprintable, DisplayName = "Item Display ViewModel")
class RIFTVAULTUI_API URiftViewModel_ItemDisplay : public URiftViewModel_Fragment
{
    GENERATED_BODY()

public:

    virtual void ShutdownViewModel_Implementation() override;

    virtual void SetItemInstance_Implementation(URiftItemInstance* NewItemInstance) override;
    virtual void ClearItemInstance_Implementation() override;

    UFUNCTION(BlueprintPure, BlueprintCosmetic, FieldNotify, Category = "RiftVault|UI|ViewModels|Item Display")
    bool HasItem() const;

    UFUNCTION(BlueprintPure, BlueprintCosmetic, FieldNotify, Category = "RiftVault|UI|ViewModels|Item Display")
    FText GetDisplayName() const;

    UFUNCTION(BlueprintPure, BlueprintCosmetic, FieldNotify, Category = "RiftVault|UI|ViewModels|Item Display")
    FText GetDescription() const;

    UFUNCTION(BlueprintPure, BlueprintCosmetic, FieldNotify, Category = "RiftVault|UI|ViewModels|Item Display")
    UTexture2D* GetIcon() const;

    /**
     * Returns the icon as an FSlateBrush, suitable for binding directly to a UImage widget.
     * Prefer this over GetIcon() for MVVM bindings — UImage.Brush expects FSlateBrush,
     * not UTexture2D*, so GetIcon() bindings silently do nothing in Blueprint.
     */
    UFUNCTION(BlueprintPure, BlueprintCosmetic, FieldNotify, Category = "RiftVault|UI|ViewModels|Item Display")
    FSlateBrush GetIconBrush() const;

    UFUNCTION(BlueprintPure, BlueprintCosmetic, FieldNotify, Category = "RiftVault|UI|ViewModels|Item Display")
    FGameplayTag GetRarityTag() const;

    /**
     * Returns the current stack quantity for this item.
     * Returns 1 if the item has no Stack fragment (non-stackable items always count as 1).
     * Returns 0 if no item is set.
     */
    UFUNCTION(BlueprintPure, BlueprintCosmetic, FieldNotify, Category = "RiftVault|UI|ViewModels|Item Display")
    int32 GetCurrentQuantity() const;

    /**
     * Returns true if the stack count widget should be visible for this item.
     * False when: no item is set, item has no Stack fragment, or bShowStackCount is false on the fragment.
     * Bind this directly to the Visibility of your stack count text widget in Blueprint.
     */
    UFUNCTION(BlueprintPure, BlueprintCosmetic, FieldNotify, Category = "RiftVault|UI|ViewModels|Item Display")
    bool IsStackCountVisible() const;

protected:

    UPROPERTY(FieldNotify, BlueprintReadOnly)
    FText DisplayName;

    UPROPERTY(FieldNotify, BlueprintReadOnly)
    FText Description;

    UPROPERTY(FieldNotify, BlueprintReadOnly)
    TObjectPtr<UTexture2D> Icon;

    UPROPERTY(FieldNotify, BlueprintReadOnly)
    FGameplayTag RarityTag;

    /** Current stack quantity. 0 when no item is set. 1 for non-stackable items. */
    UPROPERTY(FieldNotify, BlueprintReadOnly)
    int32 CurrentQuantity = 0;

    /** Whether the stack count widget should be visible. */
    UPROPERTY(FieldNotify, BlueprintReadOnly)
    bool bIsStackCountVisible = false;

private:

    TWeakObjectPtr<URiftItemInstance> ItemInstance;
    TWeakObjectPtr<URiftInventoryComponent> BoundInventory;

    void RefreshFromFragment();
    void RefreshStackFields();
    void ResetToDefaults();

    void BindToInventoryEvents();
    void UnbindFromInventoryEvents();

    UFUNCTION()
    void OnInventoryItemEvent(URiftItemInstance* Item, FGameplayTag EventTag);
};
