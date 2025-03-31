// Copyright June Rhodes. MIT Licensed.

#include "MultiplayerOnlineSusbsystem/Public/MOS_GameInstanceSubsystem.h"

#include "Interfaces/OnlineAchievementsInterface.h"
#include "OnlineSubsystemUtils.h"

void UMOS_GameInstanceSubsystem::ExecuteAchievementsQueryAchievements(
    UMOS_QueryAchievementsAsyncResult *Result)
{
    // Get the online subsystem.
    auto OSS = Online::GetSubsystem(this->GetWorld());
    if (OSS == nullptr)
    {
        Result->OnResult(
            false,
            TArray<FOSSAchievementsAchievementState>(),
            TEXT("Online subsystem is not available."));
        return;
    }

    // Get the identity interface and the currently signed in user.
    auto Identity = OSS->GetIdentityInterface();
    checkf(Identity.IsValid(), TEXT("Expected all online subsystems to implement the identity interface."));
    auto UserId = Identity->GetUniquePlayerId(this->LocalUserNum);
    checkf(UserId.IsValid(), TEXT("Expected this function to not be called unless the user is signed in."));

    // Get the achievements interface, if the online subsystem supports it.
    auto Achievements = OSS->GetAchievementsInterface();
    if (!Achievements.IsValid())
    {
        Result->OnResult(
            false,
            TArray<FOSSAchievementsAchievementState>(),
            TEXT("Online subsystem does not support achievements."));
        return;
    }

    // Query the achievement descriptions.
    Achievements->QueryAchievementDescriptions(
        *UserId,
        FOnQueryAchievementsCompleteDelegate::CreateWeakLambda(
            this,
            [this,
             Achievements,
             UserId,
             ResultWk = TSoftObjectPtr<UMOS_QueryAchievementsAsyncResult>(
                 Result)](const FUniqueNetId &, const bool bWasDescriptionSuccessful) {
                // Make sure the result callback is still valid.
                if (!ResultWk.IsValid())
                {
                    return;
                }

                // If we couldn't query achievement descriptions, return failure.
                if (!bWasDescriptionSuccessful)
                {
                    ResultWk->OnResult(
                        false,
                        TArray<FOSSAchievementsAchievementState>(),
                        TEXT("QueryAchievementDescriptions call failed."));
                    return;
                }

                // Now query the achievement state for this user.
                Achievements->QueryAchievements(
                    *UserId,
                    FOnQueryAchievementsCompleteDelegate::CreateWeakLambda(
                        this,
                        [Achievements, UserId, ResultWk](const FUniqueNetId &, const bool bWasSuccessful) {
                            // Make sure the result callback is still valid.
                            if (!ResultWk.IsValid())
                            {
                                return;
                            }

                            // If we couldn't query achievement state, return failure.
                            if (!bWasSuccessful)
                            {
                                ResultWk->OnResult(
                                    false,
                                    TArray<FOSSAchievementsAchievementState>(),
                                    TEXT("QueryAchievements call failed."));
                                return;
                            }

                            // Get the achievement state for the current user, now that it's been cached.
                            TArray<FOnlineAchievement> CurrentAchievements;
                            if (Achievements->GetCachedAchievements(*UserId, CurrentAchievements) ==
                                EOnlineCachedResult::NotFound)
                            {
                                ResultWk->OnResult(
                                    false,
                                    TArray<FOSSAchievementsAchievementState>(),
                                    TEXT("GetCachedAchievements call failed."));
                                return;
                            }

                            // Now we have cached both the achievement descriptions and the achievement states, we can
                            // put both of them together to return a list of achievements and the current user's
                            // progress.
                            TArray<FOSSAchievementsAchievementState> States;
                            for (const auto &CurrentAchievement : CurrentAchievements)
                            {
                                FOnlineAchievementDesc AchievementDescription;
                                if (Achievements->GetCachedAchievementDescription(
                                        CurrentAchievement.Id,
                                        AchievementDescription) == EOnlineCachedResult::Success)
                                {
                                    FOSSAchievementsAchievementState State;
                                    State.Id = CurrentAchievement.Id;
                                    State.DisplayName = AchievementDescription.Title;
                                    State.Progress = static_cast<float>(CurrentAchievement.Progress);
                                    State.bUnlocked = CurrentAchievement.Progress > 100.0 ||
                                                      FMath::IsNearlyEqual(CurrentAchievement.Progress, 100.0);
                                    States.Add(State);
                                }
                            }

                            // Return the results.
                            ResultWk->OnResult(true, States, TEXT(""));
                        }));
            }));
}

void UMOS_GameInstanceSubsystem::ExecuteAchievementsUnlockAchievement(const FString &Id, UMOS_AsyncResult *Result)
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

    // Get the achievements interface, if the online subsystem supports it.
    auto Achievements = OSS->GetAchievementsInterface();
    if (!Achievements.IsValid())
    {
        Result->OnResult(false, TEXT("Online subsystem does not support achievements."));
        return;
    }

    // Create the "achievement write" object, which is how we specify what achievements will be unlocked.
    auto WriteObject = MakeShared<FOnlineAchievementsWrite>();

    // Set achievement to 100% progress.
#if REDPOINT_EXAMPLE_UE_5_5_OR_LATER
    WriteObject->SetFloatStat(Id, 100.0f);
#else
    WriteObject->SetFloatStat(FName(*Id), 100.0f);
#endif

    // Run the asynchronous call to unlock the achievement.
    Achievements->WriteAchievements(
        *UserId,
        WriteObject,
        FOnAchievementsWrittenDelegate::CreateWeakLambda(
            this,
            [ResultWk = TSoftObjectPtr<UMOS_AsyncResult>(Result)](const FUniqueNetId &, bool bWasSuccessful) {
                // Make sure the result callback is still valid.
                if (!ResultWk.IsValid())
                {
                    return;
                }

                // Return the success status.
                ResultWk->OnResult(bWasSuccessful, bWasSuccessful ? TEXT("") : TEXT("WriteAchievements call failed."));
            }));
}