#include "Data/URiftAbilitySet.h"

#include "AbilitySystemComponent.h"
#include "AttributeSet.h"
#include "GameplayEffect.h"

// --------------------------------------------------------------------
// FRiftAbilitySetGrantedHandles
// --------------------------------------------------------------------

void FRiftAbilitySetGrantedHandles::RemoveFromASC(UAbilitySystemComponent* ASC)
{
    if (!ASC)
    {
        return;
    }

    // Revoke all granted abilities. ClearAbility cancels the ability if it is
    // currently active before removing its spec from the ASC.
    for (const FGameplayAbilitySpecHandle& Handle : AbilityHandles)
    {
        if (Handle.IsValid())
        {
            ASC->ClearAbility(Handle);
        }
    }
    AbilityHandles.Reset();

    // Remove all active gameplay effects that were applied by this set.
    for (const FActiveGameplayEffectHandle& Handle : EffectHandles)
    {
        if (Handle.IsValid())
        {
            ASC->RemoveActiveGameplayEffect(Handle);
        }
    }
    EffectHandles.Reset();

    // Remove all attribute set instances added by this grant.
    for (const TWeakObjectPtr<UAttributeSet>& WeakSet : AttributeSetInstances)
    {
        if (UAttributeSet* Set = WeakSet.Get())
        {
            ASC->RemoveSpawnedAttribute(Set);
        }
    }
    AttributeSetInstances.Reset();
}

bool FRiftAbilitySetGrantedHandles::HasAnyGrants() const
{
    return AbilityHandles.Num() > 0
        || EffectHandles.Num() > 0
        || AttributeSetInstances.Num() > 0;
}

// --------------------------------------------------------------------
// URiftAbilitySet
// --------------------------------------------------------------------

void URiftAbilitySet::GrantToASC(UAbilitySystemComponent* ASC, UObject* SourceObject,
    FRiftAbilitySetGrantedHandles& OutHandles) const
{
    if (!ASC)
    {
        return;
    }

    // Grant abilities. Level 1 is the default; designers can change this per-item
    // in a future version by exposing level on the ability set entry.
    for (const TSubclassOf<UGameplayAbility>& AbilityClass : GrantedAbilities)
    {
        if (!AbilityClass)
        {
            continue;
        }

        FGameplayAbilitySpec Spec(AbilityClass, 1);
        // SourceObject allows ability code to retrieve the equipped item via
        // GetCurrentSourceObject() — useful for reading item stats at runtime.
        Spec.SourceObject = SourceObject;

        OutHandles.AbilityHandles.Add(ASC->GiveAbility(Spec));
    }

    // Apply gameplay effects. We use the ASC owner as the instigator and pass level 1.
    for (const TSubclassOf<UGameplayEffect>& EffectClass : GrantedEffects)
    {
        if (!EffectClass)
        {
            continue;
        }

        // Build a context handle so the effect knows its instigator.
        FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
        Context.AddSourceObject(SourceObject);

        // Create a spec at level 1. The effect's own duration/period policy applies.
        const FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(EffectClass, 1.f, Context);
        if (SpecHandle.IsValid())
        {
            OutHandles.EffectHandles.Add(
                ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get()));
        }
    }

    // Instantiate and register attribute sets.
    for (const TSubclassOf<UAttributeSet>& SetClass : GrantedAttributeSets)
    {
        if (!SetClass)
        {
            continue;
        }

        // Outer must be the ASC's owner actor so GC roots the object correctly.
        UAttributeSet* NewSet = NewObject<UAttributeSet>(ASC->GetOwner(), SetClass);
        ASC->AddSpawnedAttribute(NewSet);
        OutHandles.AttributeSetInstances.Add(NewSet);
    }
}
