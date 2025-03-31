// Copyright June Rhodes. MIT Licensed.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"

#include "MOS_SaveGame.generated.h"

UCLASS()
class UMOS_SaveGame : public USaveGame
{
    GENERATED_BODY()

public:
    UPROPERTY()
    float StoredFloat;
};
