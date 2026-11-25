/*
This file is part of KazeGram Desktop,
based on Telegram Desktop.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#pragma once

#include "base/basic_types.h"

class History;

namespace rpl {
class lifetime;
} // namespace rpl

namespace Main {
class Session;
} // namespace Main

namespace KazeGram {

void InitMemoryManager(rpl::lifetime &appLifetime);
void NotifyWindowVisibilityChanged();

[[nodiscard]] bool HasVisibleUnminimizedWindow();
[[nodiscard]] bool HasActiveForegroundWindow();

void RefreshBackgroundPowerSaving();
void OptimizeProcessMemory();

void TrackVisitedHistory(History *history);
void TrimBackgroundHistories(
	not_null<Main::Session*> session,
	std::size_t keepCount);

} // namespace KazeGram
