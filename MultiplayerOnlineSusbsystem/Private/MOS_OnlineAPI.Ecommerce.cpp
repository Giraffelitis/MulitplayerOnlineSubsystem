// Copyright June Rhodes. MIT Licensed.

#include "MultiplayerOnlineSusbsystem/Public/MOS_GameInstanceSubsystem.h"

#include "Interfaces/OnlineEntitlementsInterface.h"
#include "Interfaces/OnlinePurchaseInterface.h"
#include "Interfaces/OnlineStoreInterfaceV2.h"
#include "OnlineSubsystemUtils.h"


void UMOS_GameInstanceSubsystem::ExecuteEcommerceQueryOffers(UMOS_AsyncResult *Result)
{
    // Get the online subsystem.
    auto OSS = Online::GetSubsystem(this->GetWorld());
    if (OSS == nullptr)
    {
        Result->OnResult(false, TEXT("Online subsystem is not available."));
        return;
    }

    // Get the identity interface and the currently signed in user.
    auto Identity = OSS->GetIdentityInterface();
    checkf(Identity.IsValid(), TEXT("Expected all online subsystems to implement the identity interface."));
    auto UserId = Identity->GetUniquePlayerId(this->LocalUserNum);
    checkf(UserId.IsValid(), TEXT("Expected this function to not be called unless the user is signed in."));

    // Get the store interface, if the online subsystem supports it.
    auto StoreV2 = OSS->GetStoreV2Interface();
    if (!StoreV2.IsValid())
    {
        Result->OnResult(false, TEXT("Online subsystem does not support e-commerce store."));
        return;
    }

    // Query for all offers.
    StoreV2->QueryOffersByFilter(
        *UserId,
        FOnlineStoreFilter(),
        FOnQueryOnlineStoreOffersComplete::CreateWeakLambda(
            this,
            [ResultWk = TSoftObjectPtr<UMOS_AsyncResult>(
                 Result)](bool bWasSuccessful, const TArray<FUniqueOfferId> &, const FString &Error) {
                // Make sure the result callback is still valid.
                if (!ResultWk.IsValid())
                {
                    return;
                }

                // Return whether the query was successful or not.
                ResultWk->OnResult(bWasSuccessful, Error);
            }));
}

TArray<FOSSEcommerceOffer> UMOS_GameInstanceSubsystem::GetEcommerceCachedOffers() const
{
    // Get the online subsystem.
    auto OSS = Online::GetSubsystem(this->GetWorld());
    if (OSS == nullptr)
    {
        return TArray<FOSSEcommerceOffer>();
    }

    // Get the identity interface and the currently signed in user.
    auto Identity = OSS->GetIdentityInterface();
    checkf(Identity.IsValid(), TEXT("Expected all online subsystems to implement the identity interface."));
    auto UserId = Identity->GetUniquePlayerId(this->LocalUserNum);
    checkf(UserId.IsValid(), TEXT("Expected this function to not be called unless the user is signed in."));

    // Get the store interface, if the online subsystem supports it.
    auto StoreV2 = OSS->GetStoreV2Interface();
    if (!StoreV2.IsValid())
    {
        return TArray<FOSSEcommerceOffer>();
    }

    // Get all of the cached offers and return them.
    TArray<FOSSEcommerceOffer> Results;
    TArray<FOnlineStoreOfferRef> Offers;
    StoreV2->GetOffers(Offers);
    for (const auto &Offer : Offers)
    {
        FOSSEcommerceOffer Result;
        Result.Id = Offer->OfferId;
        Result.Title = Offer->Title;
        Result.CurrentPrice = Offer->GetDisplayPrice();
        Results.Add(Result);
    }
    return Results;
}

void UMOS_GameInstanceSubsystem::ExecuteEcommerceStartPurchase(
    UMOS_AsyncResult *Result,
    const TMap<FString, int32> &OfferIdsToQuantities)
{
    // Get the online subsystem.
    auto OSS = Online::GetSubsystem(this->GetWorld());
    if (OSS == nullptr)
    {
        Result->OnResult(false, TEXT("Online subsystem is not available."));
        return;
    }

    // Get the identity interface and the currently signed in user.
    auto Identity = OSS->GetIdentityInterface();
    checkf(Identity.IsValid(), TEXT("Expected all online subsystems to implement the identity interface."));
    auto UserId = Identity->GetUniquePlayerId(this->LocalUserNum);
    checkf(UserId.IsValid(), TEXT("Expected this function to not be called unless the user is signed in."));

    // Get the purchase interface, if the online subsystem supports it.
    auto Purchase = OSS->GetPurchaseInterface();
    if (!Purchase.IsValid())
    {
        Result->OnResult(false, TEXT("Online subsystem does not support e-commerce purchasing."));
        return;
    }

    // Create our checkout request.
    FPurchaseCheckoutRequest CheckoutRequest;
    for (const auto &KV : OfferIdsToQuantities)
    {
        CheckoutRequest.AddPurchaseOffer(TEXT(""), KV.Key, KV.Value);
    }

    // Start the purchase.
    Purchase->Checkout(
        *UserId,
        CheckoutRequest,
        FOnPurchaseReceiptlessCheckoutComplete::CreateWeakLambda(
            this,
            [ResultWk = TSoftObjectPtr<UMOS_AsyncResult>(Result)](const FOnlineError &Error) {
                // Make sure the result callback is still valid.
                if (!ResultWk.IsValid())
                {
                    return;
                }

                // Return whether the query was successful or not.
                ResultWk->OnResult(Error.bSucceeded, Error.ToLogString());
            }));
}

void UMOS_GameInstanceSubsystem::ExecuteEcommerceQueryEntitlements(UMOS_AsyncResult *Result)
{
    // Get the online subsystem.
    auto OSS = Online::GetSubsystem(this->GetWorld());
    if (OSS == nullptr)
    {
        Result->OnResult(false, TEXT("Online subsystem is not available."));
        return;
    }

    // Get the identity interface and the currently signed in user.
    auto Identity = OSS->GetIdentityInterface();
    checkf(Identity.IsValid(), TEXT("Expected all online subsystems to implement the identity interface."));
    auto UserId = Identity->GetUniquePlayerId(this->LocalUserNum);
    checkf(UserId.IsValid(), TEXT("Expected this function to not be called unless the user is signed in."));

    // Get the entitlements interface, if the online subsystem supports it.
    auto Entitlements = OSS->GetEntitlementsInterface();
    if (!Entitlements.IsValid())
    {
        Result->OnResult(false, TEXT("Online subsystem does not support e-commerce entitlements."));
        return;
    }

    // Register an event so we can receive the query outcome.
    auto CallbackHandle = MakeShared<FDelegateHandle>();
    *CallbackHandle = Entitlements->AddOnQueryEntitlementsCompleteDelegate_Handle(
        FOnQueryEntitlementsCompleteDelegate::CreateWeakLambda(
            this,
            [UserId, Entitlements, CallbackHandle, ResultWk = TSoftObjectPtr<UMOS_AsyncResult>(Result)](
                bool bCallbackWasSuccessful,
                const FUniqueNetId &CallbackUserId,
                const FString &,
                const FString &CallbackError) {
                // Check if this callback is for us.
                if (CallbackUserId != *UserId)
                {
                    // This callback isn't for our call.
                    return;
                }

                // Make sure the result callback is still valid.
                if (!ResultWk.IsValid())
                {
                    return;
                }

                // Return whether the query was successful.
                ResultWk->OnResult(bCallbackWasSuccessful, CallbackError);

                // Unregister this callback since we've handled the call we care about.
                Entitlements->ClearOnQueryEntitlementsCompleteDelegate_Handle(*CallbackHandle);
            }));

    // Attempt to query all of the entitlements.
    if (!Entitlements->QueryEntitlements(*UserId, TEXT(""), FPagedQuery()))
    {
        // The call failed to start; unregister callback handler.
        Entitlements->ClearOnQueryEntitlementsCompleteDelegate_Handle(*CallbackHandle);
        Result->OnResult(false, TEXT("QueryEntitlements call failed to start."));
    }
}

TArray<FOSSEcommerceEntitlement> UMOS_GameInstanceSubsystem::GetEcommerceCachedEntitlements() const
{
    // Get the online subsystem.
    auto OSS = Online::GetSubsystem(this->GetWorld());
    if (OSS == nullptr)
    {
        return TArray<FOSSEcommerceEntitlement>();
    }

    // Get the identity interface and the currently signed in user.
    auto Identity = OSS->GetIdentityInterface();
    checkf(Identity.IsValid(), TEXT("Expected all online subsystems to implement the identity interface."));
    auto UserId = Identity->GetUniquePlayerId(this->LocalUserNum);
    checkf(UserId.IsValid(), TEXT("Expected this function to not be called unless the user is signed in."));

    // Get the entitlements interface, if the online subsystem supports it.
    auto Entitlements = OSS->GetEntitlementsInterface();
    if (!Entitlements.IsValid())
    {
        return TArray<FOSSEcommerceEntitlement>();
    }

    // Get all of the cached entitlements and return them.
    TArray<FOSSEcommerceEntitlement> Results;
    TArray<TSharedRef<FOnlineEntitlement>> EntitlementsArray;
    Entitlements->GetAllEntitlements(*UserId, TEXT(""), EntitlementsArray);
    for (const auto &Entitlement : EntitlementsArray)
    {
        FOSSEcommerceEntitlement Result;
        Result.Id = Entitlement->Id;
        Result.Title = FText::FromString(Entitlement->Name);
        Result.Status = Entitlement->Status;
        Result.Quantity = Entitlement->RemainingCount;
        Results.Add(Result);
    }
    return Results;
}

void UMOS_GameInstanceSubsystem::ExecuteEcommerceQueryReceipts(UMOS_AsyncResult *Result)
{
    // Get the online subsystem.
    auto OSS = Online::GetSubsystem(this->GetWorld());
    if (OSS == nullptr)
    {
        Result->OnResult(false, TEXT("Online subsystem is not available."));
        return;
    }

    // Get the identity interface and the currently signed in user.
    auto Identity = OSS->GetIdentityInterface();
    checkf(Identity.IsValid(), TEXT("Expected all online subsystems to implement the identity interface."));
    auto UserId = Identity->GetUniquePlayerId(this->LocalUserNum);
    checkf(UserId.IsValid(), TEXT("Expected this function to not be called unless the user is signed in."));

    // Get the purchase interface, if the online subsystem supports it.
    auto Purchase = OSS->GetPurchaseInterface();
    if (!Purchase.IsValid())
    {
        Result->OnResult(false, TEXT("Online subsystem does not support e-commerce purchasing."));
        return;
    }

    // Query for all receipts.
    Purchase->QueryReceipts(
        *UserId,
        false,
        FOnQueryReceiptsComplete::CreateWeakLambda(
            this,
            [ResultWk = TSoftObjectPtr<UMOS_AsyncResult>(Result)](const FOnlineError &Error) {
                // Make sure the result callback is still valid.
                if (!ResultWk.IsValid())
                {
                    return;
                }

                // Return whether the query was successful or not.
                ResultWk->OnResult(Error.bSucceeded, Error.ToLogString());
            }));
}

TArray<FOSSEcommerceReceipt> UMOS_GameInstanceSubsystem::GetEcommerceCachedReceipts() const
{
    // Get the online subsystem.
    auto OSS = Online::GetSubsystem(this->GetWorld());
    if (OSS == nullptr)
    {
        return TArray<FOSSEcommerceReceipt>();
    }

    // Get the identity interface and the currently signed in user.
    auto Identity = OSS->GetIdentityInterface();
    checkf(Identity.IsValid(), TEXT("Expected all online subsystems to implement the identity interface."));
    auto UserId = Identity->GetUniquePlayerId(this->LocalUserNum);
    checkf(UserId.IsValid(), TEXT("Expected this function to not be called unless the user is signed in."));

    // Get the purchase interface, if the online subsystem supports it.
    auto Purchase = OSS->GetPurchaseInterface();
    if (!Purchase.IsValid())
    {
        return TArray<FOSSEcommerceReceipt>();
    }

    // Get all of the cached receipts and return them.
    TArray<FOSSEcommerceReceipt> Results;
    TArray<FPurchaseReceipt> Receipts;
    Purchase->GetReceipts(*UserId, Receipts);
    for (const auto &Receipt : Receipts)
    {
        FOSSEcommerceReceipt Result;
        Result.Id = Receipt.TransactionId;
        Result.Title = FText::FromString(Receipt.TransactionId);
        Results.Add(Result);
    }
    return Results;
}