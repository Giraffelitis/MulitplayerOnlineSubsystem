// Copyright June Rhodes. MIT Licensed.

#include "MultiplayerOnlineSubsystem/Public/MOS_GameInstanceSubsystem.h"

#include "OnlineSubsystemUtils.h"
#include "DebugPlus/Public/DP_EnhancedLogging.h"

bool UMOS_GameInstanceSubsystem::GetAuthIsLoggedIn() const
{
    // Get the online subsystem.
    auto OSS = Online::GetSubsystem(this->GetWorld());
    if (OSS == nullptr)
    {
        return false;
    }

    // Get the identity interface.
    auto Identity = OSS->GetIdentityInterface();
    if (!Identity.IsValid())
    {
        return false;
    }

    // Try to get the user ID to see if there is someone signed in.
    auto UserId = Identity->GetUniquePlayerId(this->LocalUserNum);
    return UserId.IsValid();
}

void UMOS_GameInstanceSubsystem::ExecuteAuthAutoLogin(UMOS_AsyncResult *Result)
{
    // Get the online subsystem.
    auto OSS = Online::GetSubsystem(this->GetWorld());
    if (OSS == nullptr)
    {
        Result->OnResult(false, TEXT("Online subsystem is not available."));
        return;
    }

    // Get the identity interface.
    auto Identity = OSS->GetIdentityInterface();
    if (!Identity.IsValid())
    {
        Result->OnResult(false, TEXT("Online subsystem does not support identity."));
        return;
    }

    bIsAttemptingLogin = true;
    
    // Register an event so we can receive the login outcome.
    auto CallbackHandle = MakeShared<FDelegateHandle>();
    *CallbackHandle = Identity->AddOnLoginCompleteDelegate_Handle(
        this->LocalUserNum,
        FOnLoginCompleteDelegate::CreateWeakLambda(
            this,
            [this, Identity, CallbackHandle, ResultWk = TSoftObjectPtr<UMOS_AsyncResult>(Result)](
                int32 CallbackLocalUserNum,
                bool bCallbackWasSuccessful,
                const FUniqueNetId &,
                const FString &CallbackError) {
                // Check if this callback is for us.
                if (CallbackLocalUserNum != this->LocalUserNum)
                {
                    // This callback isn't for our call.
                    bIsAttemptingLogin = false;
                    return;
                }

                // Make sure the result callback is still valid.
                if (!ResultWk.IsValid())
                {
                    bIsAttemptingLogin = false;
                    return;
                }

                // Return whether the login was successful.
                ResultWk->OnResult(bCallbackWasSuccessful, CallbackError);

                // Unregister this callback since we've handled the call we care about.
                Identity->ClearOnLoginCompleteDelegate_Handle(this->LocalUserNum, *CallbackHandle);
            }));

    // Attempt to sign into the online subsystem.
    if (!Identity->Login(this->LocalUserNum, FOnlineAccountCredentials()))
    {
        // The call failed to start; unregister callback handler.
        Identity->ClearOnLoginCompleteDelegate_Handle(this->LocalUserNum, *CallbackHandle);
        Result->OnResult(false, TEXT("Login call failed to start."));
    }
}

void UMOS_GameInstanceSubsystem::OnLoginCompleted(int InUserNum, bool bWasSuccessful, const FUniqueNetId& UserId, const FString& Error)
{
    bIsAttemptingLogin = false;
    DP_LOG(MOSGameInstanceSubsystem, Warning, "User Login: %hs %s", bWasSuccessful ? "Success" : "Failed", *UserId.ToString());

    if(!bWasSuccessful)
    {
        DP_LOG(MOSGameInstanceSubsystem, Warning, "Error: %s", *Error);
    }

    // Get the online subsystem.
    auto OSS = Online::GetSubsystem(this->GetWorld());
    if (OSS == nullptr)
    {
        DP_LOG(MOSGameInstanceSubsystem, Warning,"Online subsystem is not available.");
        return;
    }

    // Get the identity interface.
    auto Identity = OSS->GetIdentityInterface();
    if (!Identity.IsValid())
    {
        DP_LOG(MOSGameInstanceSubsystem, Warning,"Online subsystem does not support identity.");
        return;
    }
    
    //Register Friend Invite Listener
    if(const IOnlineFriendsPtr FriendsPtr = OSS->GetFriendsInterface())
    {
//        FDelegateHandle Handle = FriendsPtr->AddOnInviteReceivedDelegate_Handle(FOnInviteReceivedDelegate::CreateUObject(this, &ThisClass::OnInviteReceived));
        // You can use `Handle` later to call `Friends->ClearOnInviteReceivedDelegate_Handle(Handle)`
        // if you want to unregister the event handler.
    }
}

void UMOS_GameInstanceSubsystem::ExecuteAuthLogout(UMOS_AsyncResult *Result)
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

    // Register an event so we can receive the logout outcome.
    auto CallbackHandle = MakeShared<FDelegateHandle>();
    *CallbackHandle = Identity->AddOnLogoutCompleteDelegate_Handle(
        this->LocalUserNum,
        FOnLogoutCompleteDelegate::CreateWeakLambda(
            this,
            [this, Identity, CallbackHandle, ResultWk = TSoftObjectPtr<UMOS_AsyncResult>(Result)](
                int32 CallbackLocalUserNum,
                bool bCallbackWasSuccessful) {
                // Check if this callback is for us.
                if (CallbackLocalUserNum != this->LocalUserNum)
                {
                    // This callback isn't for our call.
                    return;
                }

                // Make sure the result callback is still valid.
                if (!ResultWk.IsValid())
                {
                    return;
                }

                // Return whether the logout was successful.
                ResultWk->OnResult(
                    bCallbackWasSuccessful,
                    bCallbackWasSuccessful ? TEXT("") : TEXT("The Logout callback failed."));

                // Unregister this callback since we've handled the call we care about.
                Identity->ClearOnLogoutCompleteDelegate_Handle(this->LocalUserNum, *CallbackHandle);
            }));

    // Attempt to sign out of the online subsystem.
    if (!Identity->Logout(this->LocalUserNum))
    {
        // The call failed to start; unregister callback handler.
        Identity->ClearOnLogoutCompleteDelegate_Handle(this->LocalUserNum, *CallbackHandle);
        Result->OnResult(false, TEXT("Logout call failed to start."));
    }
}

void UMOS_GameInstanceSubsystem::OnLogoutCompleted(int InUserNum, bool bWasSuccessful)
{
    DP_LOG(MOSGameInstanceSubsystem, Log, "Logout Successful: %hs", bWasSuccessful ? "Success" : "Failed");
}

bool UMOS_GameInstanceSubsystem::GetAuthCanLinkCrossPlatformAccount() const
{
    // Get the online subsystem.
    auto OSS = Online::GetSubsystem(this->GetWorld());
    if (OSS == nullptr)
    {
        return false;
    }

    // Get the identity interface and the currently signed in user.
    auto Identity = OSS->GetIdentityInterface();
    checkf(Identity.IsValid(), TEXT("Expected all online subsystems to implement the identity interface."));
    auto UserId = Identity->GetUniquePlayerId(this->LocalUserNum);
    if (!UserId.IsValid())
    {
        return false;
    }

    // Get the signed in user's account.
    auto UserAccount = Identity->GetUserAccount(*UserId);
    checkf(UserId.IsValid(), TEXT("Expected GetUserAccount to return a valid account."));

    // Get the value of the 'crossPlatform.canLink' authentication attribute.
    FString Value;
    if (!UserAccount->GetAuthAttribute(TEXT("crossPlatform.canLink"), Value))
    {
        return false;
    }

    // If this attribute is true, then the user can link a cross-platform account.
    return Value == TEXT("true");
}
