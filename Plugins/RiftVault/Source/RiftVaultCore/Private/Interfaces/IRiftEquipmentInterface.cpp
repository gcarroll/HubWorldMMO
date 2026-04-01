#include "Interfaces/IRiftEquipmentInterface.h"

/**
 * Default implementation of GetEquipmentComponent.
 *
 * Returns nullptr to indicate "this pawn does not have an equipment component."
 * Concrete pawn classes that own a URiftEquipmentComponent override this to
 * return a pointer to their component subobject.
 *
 * UHT requires that BlueprintNativeEvent functions have a C++ _Implementation
 * body even when the default behavior is trivial — hence this file.
 */
UActorComponent* IRiftEquipmentInterface::GetEquipmentComponent_Implementation() const
{
    // Base implementation: no equipment component on this pawn.
    // Subclasses override this to return their URiftEquipmentComponent*.
    return nullptr;
}
