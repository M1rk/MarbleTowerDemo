// Copyright Epic Games, Inc. All Rights Reserved.

#include "MarbleTowerSessionSubsystem.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Online/OnlineSessionNames.h"
#include "OnlineSessionSettings.h"
#include "OnlineSubsystem.h"

namespace MarbleTowerSessionNames
{
	static const FName GameSession = NAME_GameSession;
	static const FName BuildTagKey(TEXT("MTD_BUILD_TAG"));
	static const FString BuildTagValue(TEXT("MarbleTowerDemo_MVP"));
	static const FName TagLobby(TEXT("Map.Lobby"));
	static const FName TagFortress(TEXT("Map.Fortress"));
	static const FName TagExpedition(TEXT("Map.Expedition"));
	static const TCHAR* LobbyMapPath = TEXT("/Game/LobbyMap");
	static const TCHAR* FortressMapPath = TEXT("/Game/FortressLevel");
	static const TCHAR* ExpeditionMapPath = TEXT("/Game/ExpeditionLevel");
}

void UMarbleTowerSessionSubsystem::SetState(EMarbleTowerSessionState NewState, const FString& NewStatusText)
{
	SessionState = NewState;
	LastStatusText = NewStatusText;
}

void UMarbleTowerSessionSubsystem::HostSession(int32 NumPublicConnections)
{
	PendingPublicConnections = FMath::Max(2, NumPublicConnections);
	SetState(EMarbleTowerSessionState::Hosting, TEXT("Hosting..."));

	IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
	if (OnlineSubsystem == nullptr)
	{
		bIsHosting = false;
		SetState(EMarbleTowerSessionState::HostingFailed, TEXT("Host failed: no online subsystem"));
		OnHostSessionComplete.Broadcast(false);
		return;
	}

	const IOnlineSessionPtr SessionInterface = OnlineSubsystem->GetSessionInterface();
	if (!SessionInterface.IsValid())
	{
		bIsHosting = false;
		SetState(EMarbleTowerSessionState::HostingFailed, TEXT("Host failed: session interface invalid"));
		OnHostSessionComplete.Broadcast(false);
		return;
	}

	if (SessionInterface->GetNamedSession(MarbleTowerSessionNames::GameSession) != nullptr)
	{
		bCreateAfterDestroy = true;
		SessionInterface->AddOnDestroySessionCompleteDelegate_Handle(
			FOnDestroySessionCompleteDelegate::CreateUObject(this, &UMarbleTowerSessionSubsystem::HandleDestroySessionComplete));
		SessionInterface->DestroySession(MarbleTowerSessionNames::GameSession);
		return;
	}

	CreateSessionInternal();
}

void UMarbleTowerSessionSubsystem::FindSessions(int32 MaxResults)
{
	SetState(EMarbleTowerSessionState::Searching, TEXT("Searching sessions..."));

	IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
	if (OnlineSubsystem == nullptr)
	{
		LastFindResultsCount = 0;
		SetState(EMarbleTowerSessionState::SearchDone, TEXT("Search failed: no online subsystem"));
		OnFindSessionsComplete.Broadcast(0);
		return;
	}

	const IOnlineSessionPtr SessionInterface = OnlineSubsystem->GetSessionInterface();
	if (!SessionInterface.IsValid())
	{
		LastFindResultsCount = 0;
		SetState(EMarbleTowerSessionState::SearchDone, TEXT("Search failed: session interface invalid"));
		OnFindSessionsComplete.Broadcast(0);
		return;
	}

	SessionSearch = MakeShared<FOnlineSessionSearch>();
	SessionSearch->MaxSearchResults = FMath::Max(1, MaxResults);
	SessionSearch->bIsLanQuery = false;
	SessionSearch->QuerySettings.Set(MarbleTowerSessionNames::BuildTagKey, MarbleTowerSessionNames::BuildTagValue, EOnlineComparisonOp::Equals);

	SessionInterface->AddOnFindSessionsCompleteDelegate_Handle(
		FOnFindSessionsCompleteDelegate::CreateUObject(this, &UMarbleTowerSessionSubsystem::HandleFindSessionsComplete));

	const ULocalPlayer* LocalPlayer = GetWorld() ? GetWorld()->GetFirstLocalPlayerFromController() : nullptr;
	if (LocalPlayer == nullptr)
	{
		LastFindResultsCount = 0;
		SetState(EMarbleTowerSessionState::SearchDone, TEXT("Search failed: local player missing"));
		OnFindSessionsComplete.Broadcast(0);
		return;
	}

	const FUniqueNetIdRepl NetId = LocalPlayer->GetPreferredUniqueNetId();
	if (!NetId.IsValid())
	{
		LastFindResultsCount = 0;
		SetState(EMarbleTowerSessionState::SearchDone, TEXT("Search failed: invalid Steam login"));
		OnFindSessionsComplete.Broadcast(0);
		return;
	}

	SessionInterface->FindSessions(*NetId, SessionSearch.ToSharedRef());
}

void UMarbleTowerSessionSubsystem::JoinFirstFoundSession()
{
	JoinFoundSessionByIndex(0);
}

void UMarbleTowerSessionSubsystem::JoinFoundSessionByIndex(int32 SessionIndex)
{
	JoinFoundSessionInternal(SessionIndex);
}

bool UMarbleTowerSessionSubsystem::HasAnyFoundSessions() const
{
	return SessionSearch.IsValid() && SessionSearch->SearchResults.Num() > 0;
}

bool UMarbleTowerSessionSubsystem::TravelHostedSessionByTag(FGameplayTag DestinationTag)
{
	if (!DestinationTag.IsValid())
	{
		SetState(EMarbleTowerSessionState::HostingFailed, TEXT("Travel failed: invalid gameplay tag"));
		return false;
	}

	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		SetState(EMarbleTowerSessionState::HostingFailed, TEXT("Travel failed: world missing"));
		return false;
	}

	if (!World->IsNetMode(NM_ListenServer))
	{
		SetState(EMarbleTowerSessionState::HostingFailed, TEXT("Travel failed: not listen server"));
		return false;
	}

	const FName DestinationName = DestinationTag.GetTagName();
	const TCHAR* DestinationMapPath = nullptr;
	if (DestinationName == MarbleTowerSessionNames::TagLobby)
	{
		DestinationMapPath = MarbleTowerSessionNames::LobbyMapPath;
	}
	else if (DestinationName == MarbleTowerSessionNames::TagFortress)
	{
		DestinationMapPath = MarbleTowerSessionNames::FortressMapPath;
	}
	else if (DestinationName == MarbleTowerSessionNames::TagExpedition)
	{
		DestinationMapPath = MarbleTowerSessionNames::ExpeditionMapPath;
	}

	if (DestinationMapPath == nullptr)
	{
		SetState(EMarbleTowerSessionState::HostingFailed, FString::Printf(TEXT("Travel failed: unknown tag '%s'"), *DestinationTag.ToString()));
		return false;
	}

	const FString TravelUrl = FString::Printf(TEXT("%s?listen"), DestinationMapPath);
	const bool bTravelStarted = World->ServerTravel(TravelUrl, true);
	SetState(
		bTravelStarted ? EMarbleTowerSessionState::HostingReady : EMarbleTowerSessionState::HostingFailed,
		bTravelStarted ? FString::Printf(TEXT("Traveling by tag: %s"), *DestinationTag.ToString()) : TEXT("Travel failed: ServerTravel rejected"));
	return bTravelStarted;
}

bool UMarbleTowerSessionSubsystem::TravelHostedSessionToFortress()
{
	return TravelHostedSessionByTag(FGameplayTag::RequestGameplayTag(MarbleTowerSessionNames::TagFortress, true));
}

void UMarbleTowerSessionSubsystem::CreateSessionInternal()
{
	IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
	if (OnlineSubsystem == nullptr)
	{
		OnHostSessionComplete.Broadcast(false);
		return;
	}

	const IOnlineSessionPtr SessionInterface = OnlineSubsystem->GetSessionInterface();
	if (!SessionInterface.IsValid())
	{
		OnHostSessionComplete.Broadcast(false);
		return;
	}

	FOnlineSessionSettings SessionSettings;
	SessionSettings.bIsLANMatch = false;
	SessionSettings.NumPublicConnections = PendingPublicConnections;
	SessionSettings.bAllowJoinInProgress = true;
	SessionSettings.bAllowJoinViaPresence = true;
	SessionSettings.bShouldAdvertise = true;
	SessionSettings.bUsesPresence = true;
	SessionSettings.bUseLobbiesIfAvailable = true;
	SessionSettings.Set(MarbleTowerSessionNames::BuildTagKey, MarbleTowerSessionNames::BuildTagValue, EOnlineDataAdvertisementType::ViaOnlineService);

	SessionInterface->AddOnCreateSessionCompleteDelegate_Handle(
		FOnCreateSessionCompleteDelegate::CreateUObject(this, &UMarbleTowerSessionSubsystem::HandleCreateSessionComplete));

	const ULocalPlayer* LocalPlayer = GetWorld() ? GetWorld()->GetFirstLocalPlayerFromController() : nullptr;
	if (LocalPlayer == nullptr)
	{
		OnHostSessionComplete.Broadcast(false);
		return;
	}

	const FUniqueNetIdRepl NetId = LocalPlayer->GetPreferredUniqueNetId();
	if (!NetId.IsValid())
	{
		OnHostSessionComplete.Broadcast(false);
		return;
	}

	SessionInterface->CreateSession(*NetId, MarbleTowerSessionNames::GameSession, SessionSettings);
}

void UMarbleTowerSessionSubsystem::JoinFoundSessionInternal(int32 SessionIndex)
{
	SetState(EMarbleTowerSessionState::Joining, TEXT("Joining session..."));

	IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
	if (OnlineSubsystem == nullptr)
	{
		SetState(EMarbleTowerSessionState::JoinFailed, TEXT("Join failed: no online subsystem"));
		OnJoinSessionComplete.Broadcast(false);
		return;
	}

	const IOnlineSessionPtr SessionInterface = OnlineSubsystem->GetSessionInterface();
	if (!SessionInterface.IsValid() || !SessionSearch.IsValid())
	{
		SetState(EMarbleTowerSessionState::JoinFailed, TEXT("Join failed: no session results"));
		OnJoinSessionComplete.Broadcast(false);
		return;
	}

	if (!SessionSearch->SearchResults.IsValidIndex(SessionIndex))
	{
		SetState(EMarbleTowerSessionState::JoinFailed, TEXT("Join failed: invalid session index"));
		OnJoinSessionComplete.Broadcast(false);
		return;
	}

	const ULocalPlayer* LocalPlayer = GetWorld() ? GetWorld()->GetFirstLocalPlayerFromController() : nullptr;
	if (LocalPlayer == nullptr)
	{
		SetState(EMarbleTowerSessionState::JoinFailed, TEXT("Join failed: local player missing"));
		OnJoinSessionComplete.Broadcast(false);
		return;
	}

	const FUniqueNetIdRepl NetId = LocalPlayer->GetPreferredUniqueNetId();
	if (!NetId.IsValid())
	{
		SetState(EMarbleTowerSessionState::JoinFailed, TEXT("Join failed: invalid Steam login"));
		OnJoinSessionComplete.Broadcast(false);
		return;
	}

	SessionInterface->AddOnJoinSessionCompleteDelegate_Handle(
		FOnJoinSessionCompleteDelegate::CreateUObject(this, &UMarbleTowerSessionSubsystem::HandleJoinSessionComplete));

	SessionInterface->JoinSession(*NetId, MarbleTowerSessionNames::GameSession, SessionSearch->SearchResults[SessionIndex]);
}

void UMarbleTowerSessionSubsystem::HandleCreateSessionComplete(FName /*SessionName*/, bool bWasSuccessful)
{
	bIsHosting = bWasSuccessful;
	SetState(
		bWasSuccessful ? EMarbleTowerSessionState::HostingReady : EMarbleTowerSessionState::HostingFailed,
		bWasSuccessful ? TEXT("Hosting ready") : TEXT("Host failed"));
	OnHostSessionComplete.Broadcast(bWasSuccessful);
}

void UMarbleTowerSessionSubsystem::HandleFindSessionsComplete(bool bWasSuccessful)
{
	if (!bWasSuccessful || !SessionSearch.IsValid())
	{
		LastFindResultsCount = 0;
		SetState(EMarbleTowerSessionState::SearchDone, TEXT("Search failed"));
		OnFindSessionsComplete.Broadcast(0);
		return;
	}

	TArray<FOnlineSessionSearchResult> FilteredResults;
	FilteredResults.Reserve(SessionSearch->SearchResults.Num());

	for (const FOnlineSessionSearchResult& Result : SessionSearch->SearchResults)
	{
		FString ResultBuildTag;
		if (Result.Session.SessionSettings.Get(MarbleTowerSessionNames::BuildTagKey, ResultBuildTag) &&
			ResultBuildTag == MarbleTowerSessionNames::BuildTagValue)
		{
			FilteredResults.Add(Result);
		}
	}

	SessionSearch->SearchResults = MoveTemp(FilteredResults);
	LastFindResultsCount = SessionSearch->SearchResults.Num();
	SetState(EMarbleTowerSessionState::SearchDone, FString::Printf(TEXT("Found sessions: %d"), LastFindResultsCount));
	OnFindSessionsComplete.Broadcast(LastFindResultsCount);
}

void UMarbleTowerSessionSubsystem::HandleJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
	if (OnlineSubsystem == nullptr)
	{
		SetState(EMarbleTowerSessionState::JoinFailed, TEXT("Join failed: no online subsystem"));
		OnJoinSessionComplete.Broadcast(false);
		return;
	}

	const IOnlineSessionPtr SessionInterface = OnlineSubsystem->GetSessionInterface();
	if (!SessionInterface.IsValid())
	{
		SetState(EMarbleTowerSessionState::JoinFailed, TEXT("Join failed: session interface invalid"));
		OnJoinSessionComplete.Broadcast(false);
		return;
	}

	FString ConnectString;
	if (!SessionInterface->GetResolvedConnectString(SessionName, ConnectString))
	{
		SetState(EMarbleTowerSessionState::JoinFailed, TEXT("Join failed: no connect string"));
		OnJoinSessionComplete.Broadcast(false);
		return;
	}

	APlayerController* PlayerController = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	if (PlayerController == nullptr)
	{
		SetState(EMarbleTowerSessionState::JoinFailed, TEXT("Join failed: player controller missing"));
		OnJoinSessionComplete.Broadcast(false);
		return;
	}

	PlayerController->ClientTravel(ConnectString, TRAVEL_Absolute);
	const bool bJoinSuccessful = Result == EOnJoinSessionCompleteResult::Success;
	SetState(bJoinSuccessful ? EMarbleTowerSessionState::Joined : EMarbleTowerSessionState::JoinFailed, bJoinSuccessful ? TEXT("Joined session") : TEXT("Join failed"));
	OnJoinSessionComplete.Broadcast(bJoinSuccessful);
}

void UMarbleTowerSessionSubsystem::HandleDestroySessionComplete(FName /*SessionName*/, bool bWasSuccessful)
{
	if (bCreateAfterDestroy && bWasSuccessful)
	{
		bCreateAfterDestroy = false;
		CreateSessionInternal();
		return;
	}

	bCreateAfterDestroy = false;
	bIsHosting = false;
	SetState(EMarbleTowerSessionState::HostingFailed, TEXT("Host failed: destroy previous session failed"));
	OnHostSessionComplete.Broadcast(false);
}
