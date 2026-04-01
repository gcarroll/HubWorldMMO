#pragma once

#include "CoreMinimal.h"
#include "ERiftMutableParameterType.generated.h"

/**
 * Identifies the value type stored in a FRiftMutableParameter entry.
 *
 * Mutable (UCustomizableObject) parameters are typed — you must call a
 * different API on UCustomizableObjectInstance depending on whether the
 * parameter is an integer option, a float, or a color. This enum tells
 * URiftMutableEquipmentComponent and URiftMutableWeaponComponent which
 * API to invoke when applying equipment visuals at equip/unequip time.
 *
 * Matches the three scalar parameter categories exposed by the
 * Mutable plugin. Texture parameters are not currently supported by
 * RiftVault (they are set separately via material instances).
 */
UENUM(BlueprintType)
enum class ERiftMutableParameterType : uint8
{
    /**
     * Integer option parameter.
     * Maps to UCustomizableObjectInstance::SetIntParameterSelectedOption.
     * The corresponding FRiftMutableParameter::IntOptionName holds the
     * option name string as defined in the CustomizableObject asset.
     */
    Int32   UMETA(DisplayName = "Integer"),

    /**
     * Float scalar parameter.
     * Maps to UCustomizableObjectInstance::SetFloatParameterSelectedOption.
     * The corresponding FRiftMutableParameter::FloatValue holds the value.
     */
    Float   UMETA(DisplayName = "Float"),

    /**
     * Linear color parameter.
     * Maps to UCustomizableObjectInstance::SetColorParameterSelectedOption.
     * The corresponding FRiftMutableParameter::ColorValue holds the color.
     */
    Color   UMETA(DisplayName = "Color"),
};
