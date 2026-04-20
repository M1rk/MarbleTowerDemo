# AI Conversation Log

## 2026-04-12
- Обсудили, что история чата может не подгружаться между сессиями и автоматической "памяти" нет.
- Зафиксировали рабочий подход: хранить контекст в файле `AI_conv.md` и читать его в начале новых задач.
- Договорились вести не дословный лог, а краткий дневник решений и статуса.
- Рекомендованный формат записи: что решили, что сделали, на чём остановились, следующий шаг.
- Принято правило читать последние 1-3 дня лога вместе с `CONTEXT.md` на старте задачи.
- Следующий шаг: поддерживать журнал в актуальном состоянии после заметных задач.

## 2026-04-13
- Проверили причину нестарта проекта после перехода с UE 5.0 на UE 5.7.
- По `Saved/Logs/MarbleTowerDemo.log` выявлено: не найден валидный MSVC toolchain для сборки C++ модуля `MarbleTowerDemo`; UE пытается пересобрать модуль и падает.
- Зафиксировали, что в `MarbleTowerDemo.uproject` уже стоит `EngineAssociation: 5.7`, проблема не в ассоциации движка.
- На чём остановились: нужно установить компоненты VS 2022 (MSVC v143) и пересобрать/перегенерировать проектные файлы.
- Следующий шаг: после установки toolchain запустить через `.uproject` или собрать `Development Editor Win64` из Rider/VS.
- Доппроверка: установлен VS 2022 Community `17.1.3` и MSVC `14.31.31103`; для UE 5.7 этого недостаточно (нужен минимум MSVC 14.38+, предпочтительно 14.44).
- Следующий шаг уточнён: обновить VS 2022 до актуального релиза 17.x и доустановить компонент `MSVC v143` новой версии, затем заново сгенерировать project files.
- Разобрали ошибку сборки `MSB3073 code 6`: в логе UBT причина — конфликт shared build environment (`bStrictConformanceMode` и `UndefinedIdentifierWarningLevel`) при старых `Target` настройках.
- Исправили `Source/MarbleTowerDemo.Target.cs` и `Source/MarbleTowerDemoEditor.Target.cs`: переключили `DefaultBuildSettings` на `V6` и `IncludeOrderVersion` на `Unreal5_7`; следующий шаг — пересобрать `MarbleTowerDemoEditor Win64 Development`.

## 2026-04-19
- По запросу "читай контекст" повторно прочитаны `CONTEXT.md` и последние записи `AI_conv.md` для синхронизации перед следующей задачей.
- На старте новой задачи прочитан `CONTEXT.md` и журнал `AI_conv.md` за последние доступные дни для восстановления рабочего контекста.
- Фокус подтверждён: MVP кооператива без persistent save, без изменений ключевого цикла и названий уровней.
- Текущий статус: контекст синхронизирован, готов продолжать следующую техническую задачу.
- После запуска проекта проверен свежий `Saved/Logs/MarbleTowerDemo.log` (время обновления 2026-04-19 15:57).
- Подтверждено: в текущем логе нет ошибок `MSVC/toolchain/MSB3073/UBT` и нет признаков падения при старте редактора.
- `FortressLevel` открыт и прогнан через PIE: `Map check complete: 0 Error(s), 0 Warning(s)` и `PIE: Play in editor total start time`.
- Замечены некритичные шумовые сообщения: `LogAutomationTest: Error: Condition failed` и `IOSRuntimeSettings MinimumiOSVersion = IOS_14` (не блокируют запуск Win64 Editor).
- Следующий шаг: отдельно открыть/прогнать `ExpeditionLevel` и затем перейти к проверке кооп-сценария host/client для MVP цикла.
- Проверен вопрос по предупреждению о зависимости `OpenTelemetry`: в проектных файлах `Q:\MarbleTowerDemo` прямых ссылок нет.
- Источник найден в подключённом проекте движка UE: `Q:\UE_5.7\Engine\Source\Programs\Shared\EpicGames.Perforce.Managed\EpicGames.Perforce.Managed.csproj` (`PackageReference Include="OpenTelemetry" Version="1.8.1"`).
- Вывод: предупреждение относится к C# tooling-проекту движка в `MarbleTowerDemo.sln`, а не к игровому C++ модулю MVP.
- Для задачи минимального Steam-коннекта проверены текущие настройки проекта: `OnlineSubsystemSteam` уже включён в `.uproject` и `DefaultEngine.ini` (AppID `480`, SteamNetDriver), но отсутствует логика сессий в `Source` (нет `Create/Find/Join Session`).
- Следующий шаг: добавить минимальный слой host/join (Blueprint или C++ через GameInstanceSubsystem), обновить module dependencies и прогнать packaged-тест между двумя Steam-аккаунтами.
- Выполнен шаг 1: в `Source/MarbleTowerDemo/MarbleTowerDemo.Build.cs` добавлены зависимости `OnlineSubsystem` и `OnlineSubsystemUtils` для реализации host/find/join сессий.
- Следующий рабочий шаг: пересобрать C++ модуль (или перезапустить редактор с rebuild), затем добавлять сессионную логику (пункт 2).
- Реализован минимальный сессионный слой C++: добавлен `UMarbleTowerSessionSubsystem` (`HostSession`, `FindSessions`, `JoinFirstFoundSession`, `JoinFoundSessionByIndex`) с Blueprint-эвентами завершения для UI в лобби.
- Для хоста после успешного `CreateSession` выполняется открытие `LobbyMap` в режиме `listen`; для клиента после `JoinSession` выполняется `ClientTravel` по resolved connect string.
- В `MarbleTowerDemo.Build.cs` добавлена динамическая загрузка `OnlineSubsystemSteam`.
- Обновлены стартовые карты проекта в `DefaultEngine.ini`: `EditorStartupMap` и `GameDefaultMap` переключены на `LobbyMap`.
- Следующий шаг: собрать проект, в `LobbyMap` создать UMG-кнопки Host/Find/Join и привязать к функциям `GameInstanceSubsystem`.
- Прогон сборки `MarbleTowerDemoEditor Win64 Development`: сначала ошибка компиляции по `SEARCH_PRESENCE`, после удаления этого фильтра в `FindSessions` сборка завершилась успешно (`Result: Succeeded`).
- По запросу упрощён host-пайплайн: в `UMarbleTowerSessionSubsystem::HandleCreateSessionComplete` убран автоматический `OpenLevel(..., "listen")`; теперь callback `OnHostSessionComplete` только возвращает `bSuccess`, а travel делается вручную из Blueprint/UI.
- Из subsystem также убраны связанные с авто-travel части (`SETTING_MAPNAME` и `Kismet/GameplayStatics` include), чтобы поведение было полностью под контролем UI-логики.
- Попытка сразу перепроверить сборкой упёрлась в активный Live Coding (`Unable to build while Live Coding is active`) — нужна закрытая сессия редактора или принудительная перекомпиляция в UE.
- Добавлен persistent UI-state в `UMarbleTowerSessionSubsystem`: enum `EMarbleTowerSessionState`, поля `SessionState`, `LastStatusText`, `LastFindResultsCount`, `bIsHosting` (BlueprintReadOnly), чтобы `WBP_Lobby` мог восстанавливать состояние в `Construct`.
- Во всех ветках `Host/Find/Join` добавлены обновления state+status (включая причины ошибок), а в `HandleFindSessionsComplete` сохраняется количество найденных сессий.
- Сборка после изменений проверена: `MarbleTowerDemoEditor Win64 Development` -> `Result: Succeeded`.
- Добавлен фильтр Steam-сессий по собственному тегу проекта: при `CreateSession` выставляется `MTD_BUILD_TAG=MarbleTowerDemo_MVP`, а в `FindSessions` включён query по этому тегу и дополнительная пост-фильтрация результатов в `HandleFindSessionsComplete`.
- Ожидаемое поведение: клиент видит и джойнится только в сессии этого прототипа, без "чужих" лобби AppID `480`.
- Перепроверка сборкой после правки не выполнена из-за активного Live Coding (`Unable to build while Live Coding is active`).

## 2026-04-20
- Для перехода из lobby в игровой уровень добавлен Blueprint-callable метод `TravelHostedSessionToFortress()` в `UMarbleTowerSessionSubsystem`.
- Метод проверяет, что вызов идёт на listen-server, и выполняет `ServerTravel("/Game/FortressLevel?listen")`, что переводит хоста и подключившихся клиентов на `FortressLevel`.
- Обновляется `LastStatusText`/`SessionState` для UI при успехе и при отказе travel.
- Попытка полной сборки снова остановлена активным Live Coding после UHT; для линковки нужен закрытый editor или отключение Live Coding.
- Добавлен универсальный переход `TravelHostedSessionByTag(FGameplayTag)` в `UMarbleTowerSessionSubsystem`: теги `Map.Lobby`, `Map.Fortress`, `Map.Expedition` маппятся на соответствующие уровни и запускают `ServerTravel(...?listen)`.
- Для обратной совместимости `TravelHostedSessionToFortress()` оставлен и переведён на обёртку над новым tag-based методом.
- В `MarbleTowerDemo.Build.cs` добавлен модуль `GameplayTags` для поддержки параметра `FGameplayTag` в Blueprint API.
- Проверка сборки снова упёрлась в активный Live Coding (`Unable to build while Live Coding is active`).
- Добавлен конфиг тегов `Config/DefaultGameplayTags.ini` с регистрацией `Map.Lobby`, `Map.Fortress`, `Map.Expedition` для выбора в Blueprint и валидной резолюции через GameplayTagsManager.
- В `TravelHostedSessionByTag` добавлена проверка `DestinationTag.IsValid()`, а fallback-метод `TravelHostedSessionToFortress()` переведён на `RequestGameplayTag(..., true)`.
- Повторная проверка сборки снова блокируется Live Coding (`Unable to build while Live Coding is active`).
