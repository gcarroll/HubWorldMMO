#include "DragDrop/URiftDragDropOperation.h"

URiftDragDropOperation* URiftDragDropOperation::CreateRiftDragDropOperation(
    URiftItemInstance* InItemInstance,
    URiftContainer* InSourceContainer,
    int32 InSourceSlotIndex,
    TSubclassOf<URiftDragDropOperation> OperationClass)
{
    const UClass* SafeClass = IsValid(OperationClass) ? OperationClass.Get() : StaticClass();
    URiftDragDropOperation* Operation = NewObject<URiftDragDropOperation>(GetTransientPackage(), SafeClass);
    Operation->ItemInstance = InItemInstance;
    Operation->SourceContainer = InSourceContainer;
    Operation->SourceSlotIndex = InSourceSlotIndex;
    return Operation;
}
