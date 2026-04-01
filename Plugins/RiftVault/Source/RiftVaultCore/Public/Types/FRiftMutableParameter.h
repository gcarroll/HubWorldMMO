#pragma once

#include "CoreMinimal.h"
#include "Types/ERiftMutableParameterType.h"
#include "FRiftMutableParameter.generated.h"

/**
 * Describes a single Mutable (UCustomizableObject) parameter to apply when
 * an equippable item is equipped or holstered.
 *
 * USAGE
 * -----
 * These entries are authored in arrays on URiftFragment_Equippable:
 *   - ActiveMutableParameters    — applied when the item enters an equipment slot
 *   - HolsteredMutableParameters — applied when the item leaves an equipment slot
 *       (typically restores default / empty values)
 *
 * URiftMutableEquipmentComponent reads these arrays and calls the correct
 * UCustomizableObjectInstance API on the pawn's body mesh Mutable instance.
 * URiftMutableWeaponComponent reads the same data to configure weapon actor
 * appearance (e.g. material tint, damage channel float).
 *
 * VALUE FIELD SELECTION
 * ---------------------
 * Only one value field is used at runtime; which one depends on ParameterType:
 *   Int32 → IntOptionName  (the named option string in the CO asset)
 *   Float → FloatValue
 *   Color → ColorValue
 * The other two fields are hidden in the editor via EditConditionHides and are
 * ignored entirely at runtime.
 */
USTRUCT(BlueprintType)
struct RIFTVAULTCORE_API FRiftMutableParameter
{
    GENERATED_BODY()

    /**
     * The name of the Mutable parameter as declared in the CustomizableObject asset.
     * Must match exactly — parameter names are case-sensitive in Mutable.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mutable")
    FName ParameterName;

    /**
     * The type of value this parameter holds.
     * Controls which UCustomizableObjectInstance setter is called, and which
     * value field below is visible in the Details panel via EditConditionHides.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mutable")
    ERiftMutableParameterType ParameterType = ERiftMutableParameterType::Int32;

    /**
     * Option name string for integer parameters.
     *
     * Only relevant when ParameterType == Int32.
     * Passed verbatim to UCustomizableObjectInstance::SetIntParameterSelectedOption.
     * The string must match one of the option names defined in the CustomizableObject.
     * Hidden in the editor when ParameterType is not Int32.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mutable",
        meta = (EditCondition = "ParameterType == ERiftMutableParameterType::Int32", EditConditionHides))
    FString IntOptionName;

    /**
     * Float scalar value.
     *
     * Only relevant when ParameterType == Float.
     * Passed to UCustomizableObjectInstance::SetFloatParameterSelectedOption.
     * Hidden in the editor when ParameterType is not Float.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mutable",
        meta = (EditCondition = "ParameterType == ERiftMutableParameterType::Float", EditConditionHides))
    float FloatValue = 0.f;

    /**
     * Linear color value.
     *
     * Only relevant when ParameterType == Color.
     * Passed to UCustomizableObjectInstance::SetColorParameterSelectedOption.
     * Defaults to opaque white so a newly-added entry has a visible starting point.
     * Hidden in the editor when ParameterType is not Color.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mutable",
        meta = (EditCondition = "ParameterType == ERiftMutableParameterType::Color", EditConditionHides))
    FLinearColor ColorValue = FLinearColor::White;
};
