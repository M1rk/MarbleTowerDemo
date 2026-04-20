// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "MarbleTowerSessionSubsystem.generated.h"

class FOnlineSessionSearch;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMarbleTowerSessionResult, bool, bSuccess);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMarbleTowerFindSessionsResult, int32, NumResults);

UENUM(BlueprintType)
enum class EMarbleTowerSessionState : uint8
{
	Idle UMETA(DisplayName = "Idle"),
	Hosting UMETA(DisplayName = "Hosting"),
	HostingReady UMETA(DisplayName = "HostingReady"),
	HostingFailed UMETA(DisplayName = "HostingFailed"),
	Searching UMETA(DisplayName = "Searching"),
	SearchDone UMETA(DisplayName = "SearchDone"),
	Joining UMETA(DisplayName = "Joining"),
	Joined UMETA(DisplayName = "Joined"),
	JoinFailed UMETA(DisplayName = "JoinFailed")
};

UCLASS()
class MARBLETOWERDEMO_API UMarbleTowerSessionSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable, Category = "Sessions")
	FMarbleTowerSessionResult OnHostSessionComplete;

	UPROPERTY(BlueprintAssignable, Category = "Sessions")
	FMarbleTowerFindSessionsResult OnFindSessionsComplete;

	UPROPERTY(BlueprintAssignable, Category = "Sessions")
	FMarbleTowerSessionResult OnJoinSessionComplete;

	UPROPERTY(BlueprintReadOnly, Category = "Sessions|State")
	EMarbleTowerSessionState SessionState = EMarbleTowerSessionState::Idle;

	UPROPERTY(BlueprintReadOnly, Category = "Sessions|State")
	FString LastStatusText = TEXT("Idle");

	UPROPERTY(BlueprintReadOnly, Category = "Sessions|State")
	int32 LastFindResultsCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Sessions|State")
	bool bIsHosting = false;

	UFUNCTION(BlueprintCallable, Category = "Sessions")
	void HostSession(int32 NumPublicConnections = 2);

	UFUNCTION(BlueprintCallable, Category = "Sessions")
	void FindSessions(int32 MaxResults = 20);

	UFUNCTION(BlueprintCallable, Category = "Sessions")
	void JoinFirstFoundSession();

	UFUNCTION(BlueprintCallable, Category = "Sessions")
	void JoinFoundSessionByIndex(int32 SessionIndex);

	UFUNCTION(BlueprintPure, Category = "Sessions")
	bool HasAnyFoundSessions() const;

	UFUNCTION(BlueprintCallable, Category = "Sessions")
	bool TravelHostedSessionByTag(FGameplayTag DestinationTag);

	UFUNCTION(BlueprintCallable, Category = "Sessions")
	bool TravelHostedSessionToFortress();

private:
	void SetState(EMarbleTowerSessionState NewState, const FString& NewStatusText);

	void CreateSessionInternal();
	void JoinFoundSessionInternal(int32 SessionIndex);

	void HandleCreateSessionComplete(FName SessionName, bool bWasSuccessful);
	void HandleFindSessionsComplete(bool bWasSuccessful);
	void HandleJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
	void HandleDestroySessionComplete(FName SessionName, bool bWasSuccessful);

	TSharedPtr<FOnlineSessionSearch> SessionSearch;

	int32 PendingPublicConnections = 2;
	bool bCreateAfterDestroy = false;
};
