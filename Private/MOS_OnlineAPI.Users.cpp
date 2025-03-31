// Copyright June Rhodes. MIT Licensed.

#include "MultiplayerOnlineSubsystem/Public/MOS_GameInstanceSubsystem.h"

#include "Interfaces/OnlineUserInterface.h"
#include "OnlineSubsystemUtils.h"


void UMOS_GameInstanceSubsystem::ExecuteUsersQueryUserInfo(const FString &UserIdStr, UMOS_TextAsyncResult *Result)
{
    // Get the online subsystem.
    auto OSS = Online::GetSubsystem(this->GetWorld());
    if (OSS == nullptr)
    {
        Result->OnResult(false, TEXT(""), TEXT("Online subsystem is not available."));
        return;
    }

    // Get the identity interface and the currently signed in user.
    auto Identity = OSS->GetIdentityInterface();
    checkf(Identity.IsValid(), TEXT("Expected all online subsystems to implement the identity interface."));
    auto UserId = Identity->GetUniquePlayerId(this->LocalUserNum);
    checkf(UserId.IsValid(), TEXT("Expected this function to not be called unless the user is signed in."));

    // Get the user interface, if the online subsystem supports it.
    auto UserInfo = OSS->GetUserInterface();
    if (!UserInfo.IsValid())
    {
        Result->OnResult(false, TEXT(""), TEXT("Online subsystem does not support user lookup."));
        return;
    }

    // Parse the target user ID.
    auto TargetUserId = Identity->CreateUniquePlayerId(UserIdStr);
    if (!TargetUserId.IsValid())
    {
        Result->OnResult(false, TEXT(""), TEXT("The target user ID is not valid."));
        return;
    }

    // Register an event so we can receive the query outcome.
    auto CallbackHandle = MakeShared<FDelegateHandle>();
    *CallbackHandle = UserInfo->AddOnQueryUserInfoCompleteDelegate_Handle(
        this->LocalUserNum,
        FOnQueryUserInfoCompleteDelegate::CreateWeakLambda(
            this,
            [this,
             UserInfo,
             CallbackHandle,
             TargetUserId,
             ResultWk = TSoftObjectPtr<UMOS_TextAsyncResult>(Result)](
                int32 CallbackLocalUserNum,
                bool bCallbackWasSuccessful,
                const TArray<FUniqueNetIdRef> &CallbackUserIds,
                const FString &CallbackError) {
                // Check if this callback is for us.
                if (!CallbackUserIds.Contains(TargetUserId.ToSharedRef()))
                {
                    // This callback isn't for our call.
                    return;
                }

                // Make sure the result callback is still valid.
                if (!ResultWk.IsValid())
                {
                    return;
                }

                // Return if the read failed.
                if (!bCallbackWasSuccessful)
                {
                    ResultWk->OnResult(false, TEXT(""), TEXT("User query failed."));
                    UserInfo->ClearOnQueryUserInfoCompleteDelegate_Handle(this->LocalUserNum, *CallbackHandle);
                    return;
                }

                // Otherwise, return the target user's info.
                auto TargetUser = UserInfo->GetUserInfo(this->LocalUserNum, *TargetUserId);
                if (!TargetUser.IsValid())
                {
                    ResultWk->OnResult(false, TEXT(""), TEXT("GetUserInfo call did not return a user."));
                    UserInfo->ClearOnQueryUserInfoCompleteDelegate_Handle(this->LocalUserNum, *CallbackHandle);
                    return;
                }
                ResultWk->OnResult(
                    true,
                    FString::Printf(
                        TEXT("%s = %s"),
                        *TargetUser->GetUserId()->ToString(),
                        *TargetUser->GetDisplayName()),
                    TEXT(""));

                // Unregister this callback since we've handled the call we care about.
                UserInfo->ClearOnQueryUserInfoCompleteDelegate_Handle(this->LocalUserNum, *CallbackHandle);
            }));

    // Query for the user.
    if (!UserInfo->QueryUserInfo(this->LocalUserNum, TArray<FUniqueNetIdRef>{TargetUserId.ToSharedRef()}))
    {
        Result->OnResult(false, TEXT(""), TEXT("QueryUserInfo call failed to start."));
        UserInfo->ClearOnQueryUserInfoCompleteDelegate_Handle(this->LocalUserNum, *CallbackHandle);
    }
}

void UMOS_GameInstanceSubsystem::ExecuteUsersQueryUserByDisplayName(
    const FString &DisplayName,
    UMOS_TextAsyncResult *Result)
{
    // Get the online subsystem.
    auto OSS = Online::GetSubsystem(this->GetWorld());
    if (OSS == nullptr)
    {
        Result->OnResult(false, TEXT(""), TEXT("Online subsystem is not available."));
        return;
    }

    // Get the identity interface and the currently signed in user.
    auto Identity = OSS->GetIdentityInterface();
    checkf(Identity.IsValid(), TEXT("Expected all online subsystems to implement the identity interface."));
    auto UserId = Identity->GetUniquePlayerId(this->LocalUserNum);
    checkf(UserId.IsValid(), TEXT("Expected this function to not be called unless the user is signed in."));

    // Get the user interface, if the online subsystem supports it.
    auto UserInfo = OSS->GetUserInterface();
    if (!UserInfo.IsValid())
    {
        Result->OnResult(false, TEXT(""), TEXT("Online subsystem does not support user lookup."));
        return;
    }

    // Query for the user by their display name.
    if (!UserInfo->QueryUserIdMapping(
            *UserId,
            DisplayName,
            IOnlineUser::FOnQueryUserMappingComplete::CreateWeakLambda(
                this,
                [ResultWk = TSoftObjectPtr<UMOS_TextAsyncResult>(Result)](
                    bool bWasSuccessful,
                    const FUniqueNetId &,
                    const FString &,
                    const FUniqueNetId &FoundUserId,
                    const FString &Error) {
                    // Make sure the result callback is still valid.
                    if (!ResultWk.IsValid())
                    {
                        return;
                    }

                    // Return the result.
                    ResultWk->OnResult(
                        bWasSuccessful,
                        bWasSuccessful ? FoundUserId.ToString() : TEXT(""),
                        bWasSuccessful ? TEXT("") : Error);
                })))
    {
        Result->OnResult(false, TEXT(""), TEXT("QueryUserIdMapping call failed to start."));
    }
}

void UMOS_GameInstanceSubsystem::ExecuteUsersQueryExternalIds(
    const FString &PlatformName,
    const TArray<FString> &ExternalIds,
    UMOS_TextAsyncResult *Result)
{
    // Get the online subsystem.
    auto OSS = Online::GetSubsystem(this->GetWorld());
    if (OSS == nullptr)
    {
        Result->OnResult(false, TEXT(""), TEXT("Online subsystem is not available."));
        return;
    }

    // Get the identity interface and the currently signed in user.
    auto Identity = OSS->GetIdentityInterface();
    checkf(Identity.IsValid(), TEXT("Expected all online subsystems to implement the identity interface."));
    auto UserId = Identity->GetUniquePlayerId(this->LocalUserNum);
    checkf(UserId.IsValid(), TEXT("Expected this function to not be called unless the user is signed in."));

    // Get the user interface, if the online subsystem supports it.
    auto UserInfo = OSS->GetUserInterface();
    if (!UserInfo.IsValid())
    {
        Result->OnResult(false, TEXT(""), TEXT("Online subsystem does not support user lookup."));
        return;
    }

    // Query for external user IDs.
    if (!UserInfo->QueryExternalIdMappings(
            *UserId,
            FExternalIdQueryOptions(PlatformName, false),
            ExternalIds,
            IOnlineUser::FOnQueryExternalIdMappingsComplete::CreateWeakLambda(
                this,
                [UserInfo, PlatformName, ExternalIds, ResultWk = TSoftObjectPtr<UMOS_TextAsyncResult>(Result)](
                    bool bWasSuccessful,
                    const FUniqueNetId &,
                    const FExternalIdQueryOptions &,
                    const TArray<FString> &,
                    const FString &Error) {
                    // Make sure the result callback is still valid.
                    if (!ResultWk.IsValid())
                    {
                        return;
                    }

                    // Generate lines from found user IDs.
                    TArray<FUniqueNetIdPtr> FoundUserIds;
                    UserInfo->GetExternalIdMappings(
                        FExternalIdQueryOptions(PlatformName, false),
                        ExternalIds,
                        FoundUserIds);
                    TArray<FString> Lines;
                    for (const auto &FoundUserId : FoundUserIds)
                    {
                        if (FoundUserId.IsValid())
                        {
                            Lines.Add(FoundUserId->ToString());
                        }
                    }

                    // Return the result.
                    ResultWk->OnResult(
                        bWasSuccessful,
                        bWasSuccessful ? FString::Join(Lines, TEXT("\n")) : TEXT(""),
                        bWasSuccessful ? TEXT("") : Error);
                })))
    {
        Result->OnResult(false, TEXT(""), TEXT("QueryExternalIdMappings call failed to start."));
    }
}