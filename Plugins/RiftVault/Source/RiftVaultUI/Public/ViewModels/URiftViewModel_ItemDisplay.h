#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "ViewModels/URiftViewModel.h"
#include "GameFramework/Items/URiftItemInstance.h"
#include "URiftViewModel_ItemDisplay.generated.h"

class UTexture2D;

UCLASS(BlueprintType, Blueprintable, DisplayName = "Item Display ViewModel")
class RIFTVAULTUI_API URiftViewModel_ItemDisplay : public URiftViewModel
{
    GENERATED_BODY()

public:

    virtual void ShutdownViewModel_Implementation() override;

    UFUNCTION(BlueprintCallable, BlueprintCosmetic, Category = "RiftVault|UI|ViewModels|Item Display")
    void SetItemInstance(URiftItemInstance* NewItemInstance);

    UFUNCTION(BlueprintCallable, BlueprintCosmetic, Category = "RiftVault|UI|ViewModels|Item Display")
    void ClearItemInstance();

    UFUNCTION(BlueprintPure, BlueprintCosmetic, FieldNotify, Category = "RiftVault|UI|ViewModels|Item Display")
    bool HasItem() const;

    UFUNCTION(BlueprintPure, BlueprintCosmetic, FieldNotify, Category = "RiftVault|UI|ViewModels|Item Display")
    FText GetDisplayName() const;

    UFUNCTION(BlueprintPure, BlueprintCosmetic, FieldNotify, Category = "RiftVault|UI|ViewModels|Item Display")
    FText GetDescription() const;

    UFUNCTION(BlueprintPure, BlueprintCosmetic, FieldNotify, Category = "RiftVault|UI|ViewModels|Item Display")
    UTexture2D* GetIcon() const;

    UFUNCTION(BlueprintPure, BlueprintCosmetic, FieldNotify, Category = "RiftVault|UI|ViewModels|Item Display")
    FGameplayTag GetRarityTag() const;

protected:

    UPROPERTY(FieldNotify, BlueprintReadOnly)
    FText DisplayName;

    UPROPERTY(FieldNotify, BlueprintReadOnly)
    FText Description;

    UPROPERTY(FieldNotify, BlueprintReadOnly)
    TObjectPtr<UTexture2D> Icon;

    UPROPERTY(FieldNotify, BlueprintReadOnly)
    FGameplayTag RarityTag;

private:

    TWeakObjectPtr<URiftItemInstance> ItemInstance;

    void RefreshFromFragment();
    void ResetToDefaults();
};
