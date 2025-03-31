// Copyright June Rhodes. MIT Licensed.

#include "MultiplayerOnlineSubsystem/Public/MOS_GameInstanceSubsystem.h"

#include "Interfaces/OnlineLeaderboardInterface.h"
#include "OnlineSubsystemUtils.h"


void UMOS_GameInstanceSubsystem::ExecuteLeaderboardsQueryGlobalLeaderboards(
    UMOS_QueryLeaderboardsAsyncResult *Result)
{
    // Get the online subsystem.
    auto OSS = Online::GetSubsystem(this->GetWorld());
    if (OSS == nullptr)
    {
        Result->OnResult(
            false,
            TArray<FMOSLeaderboardsLeaderboardEntry>(),
            TEXT("Online subsystem is not available."));
        return;
    }

    // Get the identity interface and the currently signed in user.
    auto Identity = OSS->GetIdentityInterface();
    checkf(Identity.IsValid(), TEXT("Expected all online subsystems to implement the identity interface."));
    auto UserId = Identity->GetUniquePlayerId(this->LocalUserNum);
    checkf(UserId.IsValid(), TEXT("Expected this function to not be called unless the user is signed in."));

    // Get the leaderboards interface, if the online subsystem supports it.
    auto Leaderboards = OSS->GetLeaderboardsInterface();
    if (!Leaderboards.IsValid())
    {
        Result->OnResult(
            false,
            TArray<FMOSLeaderboardsLeaderboardEntry>(),
            TEXT("Online subsystem does not support friends."));
        return;
    }

    // Construct the read object, which will be populated with our results.
    auto ReadObject = MakeShared<FOnlineLeaderboardRead>();

    // Set the leaderboard name, which is the global leaderboard we'll query on EOS.
#if REDPOINT_EXAMPLE_UE_5_5_OR_LATER
    ReadObject->LeaderboardName = FString(TEXT("TestScore"));
#else
    ReadObject->LeaderboardName = FName(TEXT("TestScore"));
#endif

    // Register an event so we can receive the query outcome.
    auto CallbackHandle = MakeShared<FDelegateHandle>();
    *CallbackHandle =
        Leaderboards->AddOnLeaderboardReadCompleteDelegate_Handle(FOnLeaderboardReadCompleteDelegate::CreateWeakLambda(
            this,
            [Leaderboards,
             CallbackHandle,
             ResultWk = TSoftObjectPtr<UMOS_QueryLeaderboardsAsyncResult>(Result),
             ReadObject](bool bCallbackWasSuccessful) {
                // Check if this callback is for us.
                if (ReadObject->ReadState != EOnlineAsyncTaskState::Failed &&
                    ReadObject->ReadState != EOnlineAsyncTaskState::Done)
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
                if (ReadObject->ReadState == EOnlineAsyncTaskState::Failed)
                {
                    ResultWk->OnResult(
                        false,
                        TArray<FMOSLeaderboardsLeaderboardEntry>(),
                        TEXT("Leaderboard read failed."));
                    Leaderboards->ClearOnLeaderboardReadCompleteDelegate_Handle(*CallbackHandle);
                    return;
                }

                // Otherise, convert the results.
                TArray<FMOSLeaderboardsLeaderboardEntry> Results;
                for (const auto &Row : ReadObject->Rows)
                {
                    // @note: On EOS, when you perform a ranked search, the value is always in the "Score" column.
                    int Score;
                    Row.Columns["Score"].GetValue(Score);

                    FMOSLeaderboardsLeaderboardEntry Entry;
                    Entry.CurrentValue = Score;
                    Entry.PlayerName = FString::Printf(TEXT("%s - %s"), *Row.PlayerId->ToString(), *Row.NickName);
                    Entry.Rank = Row.Rank;
                    Results.Add(Entry);
                }

                // Return the results.
                ResultWk->OnResult(true, Results, TEXT(""));

                // Unregister this callback since we've handled the call we care about.
                Leaderboards->ClearOnLeaderboardReadCompleteDelegate_Handle(*CallbackHandle);
            }));

    // Query the global leaderboard.
    if (!Leaderboards->ReadLeaderboardsAroundRank(0, 100, ReadObject))
    {
        Result->OnResult(
            false,
            TArray<FMOSLeaderboardsLeaderboardEntry>(),
            TEXT("ReadLeaderboardsAroundRank call failed to start."));
        Leaderboards->ClearOnLeaderboardReadCompleteDelegate_Handle(*CallbackHandle);
    }
}

void UMOS_GameInstanceSubsystem::ExecuteLeaderboardsQueryFriendsLeaderboards(
    UMOS_QueryLeaderboardsAsyncResult *Result)
{
    // Get the online subsystem.
    auto OSS = Online::GetSubsystem(this->GetWorld());
    if (OSS == nullptr)
    {
        Result->OnResult(
            false,
            TArray<FMOSLeaderboardsLeaderboardEntry>(),
            TEXT("Online subsystem is not available."));
        return;
    }

    // Get the identity interface and the currently signed in user.
    auto Identity = OSS->GetIdentityInterface();
    checkf(Identity.IsValid(), TEXT("Expected all online subsystems to implement the identity interface."));
    auto UserId = Identity->GetUniquePlayerId(this->LocalUserNum);
    checkf(UserId.IsValid(), TEXT("Expected this function to not be called unless the user is signed in."));

    // Get the leaderboards interface, if the online subsystem supports it.
    auto Leaderboards = OSS->GetLeaderboardsInterface();
    if (!Leaderboards.IsValid())
    {
        Result->OnResult(
            false,
            TArray<FMOSLeaderboardsLeaderboardEntry>(),
            TEXT("Online subsystem does not support friends."));
        return;
    }

    // Construct the read object, which will be populated with our results.
    auto ReadObject = MakeShared<FOnlineLeaderboardRead>();

    // Add the stat name to query.
#if REDPOINT_EXAMPLE_UE_5_5_OR_LATER
    ReadObject->ColumnMetadata.Add(FColumnMetaData(FString(TEXT("TestScore")), EOnlineKeyValuePairDataType::Int32));
#else
    ReadObject->ColumnMetadata.Add(FColumnMetaData(FName(TEXT("TestScore")), EOnlineKeyValuePairDataType::Int32));
#endif

    // Register an event so we can receive the query outcome.
    auto CallbackHandle = MakeShared<FDelegateHandle>();
    *CallbackHandle =
        Leaderboards->AddOnLeaderboardReadCompleteDelegate_Handle(FOnLeaderboardReadCompleteDelegate::CreateWeakLambda(
            this,
            [Leaderboards,
             CallbackHandle,
             ResultWk = TSoftObjectPtr<UMOS_QueryLeaderboardsAsyncResult>(Result),
             ReadObject](bool bCallbackWasSuccessful) {
                // Check if this callback is for us.
                if (ReadObject->ReadState != EOnlineAsyncTaskState::Failed &&
                    ReadObject->ReadState != EOnlineAsyncTaskState::Done)
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
                if (ReadObject->ReadState == EOnlineAsyncTaskState::Failed)
                {
                    ResultWk->OnResult(
                        false,
                        TArray<FMOSLeaderboardsLeaderboardEntry>(),
                        TEXT("Leaderboard read failed."));
                    Leaderboards->ClearOnLeaderboardReadCompleteDelegate_Handle(*CallbackHandle);
                    return;
                }

                // Otherise, convert the results.
                TArray<FMOSLeaderboardsLeaderboardEntry> Results;
                for (const auto &Row : ReadObject->Rows)
                {
                    // @note: On EOS, when you perform a friend search, the value is in a column that matches the stat
                    // name.
                    int Score;
                    Row.Columns["TestScore"].GetValue(Score);

                    FMOSLeaderboardsLeaderboardEntry Entry;
                    Entry.CurrentValue = Score;
                    Entry.PlayerName = FString::Printf(TEXT("%s - %s"), *Row.PlayerId->ToString(), *Row.NickName);
                    Entry.Rank = Row.Rank;
                    Results.Add(Entry);
                }

                // Return the results.
                ResultWk->OnResult(true, Results, TEXT(""));

                // Unregister this callback since we've handled the call we care about.
                Leaderboards->ClearOnLeaderboardReadCompleteDelegate_Handle(*CallbackHandle);
            }));

    // Query the stats for our friends.
    if (!Leaderboards->ReadLeaderboardsForFriends(this->LocalUserNum, ReadObject))
    {
        Result->OnResult(
            true,
            TArray<FMOSLeaderboardsLeaderboardEntry>(),
            TEXT("ReadLeaderboardsForFriends call failed to start."));
        Leaderboards->ClearOnLeaderboardReadCompleteDelegate_Handle(*CallbackHandle);
    }
}