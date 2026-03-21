#pragma once

#include "CoreMinimal.h"
#include "ViewModels/URiftViewModel.h"
#include "GameFramework/Items/URiftItemInstance.h"
#include "URiftViewModel_Inventory.generated.h"

class URiftInventoryComponent;
class URiftContainer;

UCLASS(BlueprintType, Blueprintable, DisplayName = "Inventory ViewModel")
class RIFTVAULTUI_API URiftViewModel_Inventory : public URiftViewModel
{
    GENERATED_BODY()

public:

    virtual void InitializeViewModel_Implementation(const UUserWidget* UserWidget) override;
    virtual void ShutdownViewModel_Implementation() override;

    UFUNCTION(BlueprintCallable, BlueprintCosmetic, Category = "RiftVault|UI|ViewModels|Inventory")
    void SetInventoryComponent(URiftInventoryComponent* NewInventoryComponent);

    UFUNCTION(BlueprintPure, BlueprintCosmetic, FieldNotify, Category = "RiftVault|UI|ViewModels|Inventory")
    TArray<URiftItemInstance*> GetItems() const;

    UFUNCTION(BlueprintPure, BlueprintCosmetic, FieldNotify, Category = "RiftVault|UI|ViewModels|Inventory")
    int32 GetItemCount() const;

    UFUNCTION()
    void OnItemAdded(URiftItemInstance* Item, URiftContainer* Container);

    UFUNCTION()
    void OnItemRemoved(URiftItemInstance* Item, URiftContainer* Container);

protected:

    UPROPERTY(FieldNotify, BlueprintReadOnly)
    TArray<TObjectPtr<URiftItemInstance>> Items;

private:

    TWeakObjectPtr<URiftInventoryComponent> InventoryComponent;

    void BindToInventoryComponent();
    void UnbindFromInventoryComponent();
    void RefreshItems();
};
