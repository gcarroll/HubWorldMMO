#pragma once

#include "CoreMinimal.h"
#include "ViewModels/URiftViewModel.h"
#include "URiftViewModel_Fragment.generated.h"

class URiftItemInstance;

/**
 * Base class for all fragment-driven ViewModels.
 *
 * Subclass this (in Blueprint or C++) to create a ViewModel for a specific
 * URiftItemFragment. The slot widget automatically discovers and wires any
 * ViewModel whose name matches the ViewModelName set on the fragment.
 *
 * HOW IT WORKS
 * ------------
 * 1. Add a URiftItemFragment subclass to your URiftItemDefinition.
 * 2. Set ViewModelName on that fragment to a chosen FName (e.g. "ConditionVM").
 * 3. In your slot widget Blueprint, add a ViewModel of this class to the MVVM
 *    settings with the same name ("ConditionVM").
 * 4. The slot widget calls SetItemInstance automatically when an item is set
 *    and ClearItemInstance when the slot is emptied.
 *
 * BLUEPRINT USAGE
 * ---------------
 * Override SetItemInstance in your Blueprint ViewModel to read the fragment
 * data from the item and update FieldNotify properties. For example:
 *
 *   Event SetItemInstance(Item)
 *     → Find Fragment by Class (URiftFragment_Condition)
 *     → Read CurrentCondition, update ConditionPercent property
 *     → Broadcast field value changed
 *
 * If the item has no matching fragment, SetItemInstance is still called —
 * guard with an IsValid check on the fragment and reset to defaults if absent.
 */
UCLASS(Abstract, Blueprintable, DisplayName = "Rift Fragment ViewModel")
class RIFTVAULTUI_API URiftViewModel_Fragment : public URiftViewModel
{
    GENERATED_BODY()

public:

    /**
     * Called by the slot widget when an item is placed into the slot.
     * Override in Blueprint or C++ to read fragment data from the item
     * and push it to FieldNotify properties for UI binding.
     *
     * @param NewItemInstance  The item now occupying the slot. Never null when called from SetItem.
     */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, BlueprintCosmetic, Category = "RiftVault|UI|ViewModels")
    void SetItemInstance(URiftItemInstance* NewItemInstance);
    virtual void SetItemInstance_Implementation(URiftItemInstance* NewItemInstance) {}

    /**
     * Called by the slot widget when the slot is emptied.
     * Override in Blueprint or C++ to reset all FieldNotify properties to
     * their default/empty state so the UI shows nothing.
     */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, BlueprintCosmetic, Category = "RiftVault|UI|ViewModels")
    void ClearItemInstance();
    virtual void ClearItemInstance_Implementation() {}
};
