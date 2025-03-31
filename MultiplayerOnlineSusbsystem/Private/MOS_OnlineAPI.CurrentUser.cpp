// Copyright June Rhodes. MIT Licensed.

#include "MultiplayerOnlineSusbsystem/Public/MOS_GameInstanceSubsystem.h"

#include "OnlineSubsystemUtils.h"

FString UMOS_GameInstanceSubsystem::GetCurrentUserDisplayName() const
{
    // Get the online subsystem.
    auto OSS = Online::GetSubsystem(this->GetWorld());
    if (OSS == nullptr)
    {
        return TEXT("");
    }

    // Get the identity interface.
    auto Identity = OSS->GetIdentityInterface();
    if (!Identity.IsValid())
    {
        return TEXT("");
    }

    // Return the nickname of the local user.
    return Identity->GetPlayerNickname(this->LocalUserNum);
}

FUniqueNetIdRepl UMOS_GameInstanceSubsystem::GetCurrentUserId() const
{
    // Get the online subsystem.
    auto OSS = Online::GetSubsystem(this->GetWorld());
    if (OSS == nullptr)
    {
        return FUniqueNetIdRepl();
    }

    // Get the identity interface.
    auto Identity = OSS->GetIdentityInterface();
    if (!Identity.IsValid())
    {
        return FUniqueNetIdRepl();
    }

    // Get the user ID, and if it is valid, return the string representation of it.
    auto UserId = Identity->GetUniquePlayerId(this->LocalUserNum);
    if (UserId.IsValid())
    {
        return UserId;
    }
    else
    {
        return FUniqueNetIdRepl();
    }
}

FString UMOS_GameInstanceSubsystem::GetCurrentUserSecondaryId() const
{
    // Get the online subsystem.
    auto OSS = Online::GetSubsystem(this->GetWorld());
    if (OSS == nullptr)
    {
        return TEXT("");
    }

    // Get the identity interface.
    auto Identity = OSS->GetIdentityInterface();
    if (!Identity.IsValid())
    {
        return TEXT("");
    }

    // Get the user ID if the user is signed in.
    auto UserId = Identity->GetUniquePlayerId(this->LocalUserNum);
    if (!UserId.IsValid())
    {
        return TEXT("");
    }

    // Get the signed in user's account.
    auto UserAccount = Identity->GetUserAccount(*UserId);
    checkf(UserAccount.IsValid(), TEXT("Expected GetUserAccount to return a valid account."));

    // Return the value of the 'epic.accountId' attribute.
    FString Value;
    return UserAccount->GetAuthAttribute(TEXT("epic.accountId"), Value) ? Value : TEXT("");
}

bool UMOS_GameInstanceSubsystem::ShouldRenderUserSecondaryIdField() const
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

    // Get the user ID if the user is signed in.
    auto UserId = Identity->GetUniquePlayerId(this->LocalUserNum);
    if (!UserId.IsValid())
    {
        return false;
    }

    // Get the signed in user's account.
    auto UserAccount = Identity->GetUserAccount(*UserId);
    checkf(UserAccount.IsValid(), TEXT("Expected GetUserAccount to return a valid account."));

    // Return whether the 'epic.accountId' attribute exists.
    FString Value;
    return UserAccount->GetAuthAttribute(TEXT("epic.accountId"), Value);
}

FString UMOS_GameInstanceSubsystem::GetCurrentUserAuthAttribute(const FString &Key) const
{
    // Get the online subsystem.
    auto OSS = Online::GetSubsystem(this->GetWorld());
    if (OSS == nullptr)
    {
        return TEXT("");
    }

    // Get the identity interface.
    auto Identity = OSS->GetIdentityInterface();
    if (!Identity.IsValid())
    {
        return TEXT("");
    }

    // Get the user ID if the user is signed in.
    auto UserId = Identity->GetUniquePlayerId(this->LocalUserNum);
    if (!UserId.IsValid())
    {
        return TEXT("");
    }

    // Get the signed in user's account.
    auto UserAccount = Identity->GetUserAccount(*UserId);
    checkf(UserId.IsValid(), TEXT("Expected GetUserAccount to return a valid account."));

    // Return the requested attribute.
    FString Value;
    return UserAccount->GetAuthAttribute(Key, Value) ? Value : TEXT("");
}