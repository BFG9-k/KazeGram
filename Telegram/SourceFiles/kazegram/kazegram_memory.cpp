/*
This file is part of KazeGram Desktop,
based on Telegram Desktop.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#include "kazegram/kazegram_memory.h"

#include "base/battery_saving.h"
#include "base/flat_set.h"
#include "base/timer.h"
#include "base/weak_ptr.h"
#include "chat_helpers/tabbed_selector.h"
#include "core/application.h"
#include "core/core_settings.h"
#include "data/data_chat.h"
#include "data/data_peer.h"
#include "data/data_session.h"
#include "data/data_thread.h"
#include "dialogs/dialogs_key.h"
#include "history/history.h"
#include "main/main_session.h"
#include "mainwindow.h"
#include "mainwidget.h"
#include "ui/power_saving.h"
#include "window/main_window.h"
#include "window/window_controller.h"
#include "window/window_separate_id.h"
#include "window/window_session_controller.h"

#include <QtGui/QPixmapCache>

#ifdef Q_OS_WIN
#include <windows.h>
#endif // Q_OS_WIN

namespace KazeGram {
namespace {

constexpr auto kMaxActiveHistories = std::size_t(8);
constexpr auto kMaxBackgroundHistories = std::size_t(2);
constexpr auto kTrayCleanupDelay = crl::time(2000);
constexpr auto kMinimizedCleanupDelay = crl::time(15000);

struct HistoryLruEntry {
	base::weak_ptr<Main::Session> session;
	PeerId peerId = 0;
};

struct ManagerState {
	explicit ManagerState(Fn<void()> callback)
	: cleanupTimer(std::move(callback)) {
	}

	base::Timer cleanupTimer;
	std::vector<HistoryLruEntry> historyLru;
};

ManagerState *GlobalState = nullptr;

[[nodiscard]] base::flat_set<not_null<History*>> CollectOpenHistories() {
	auto result = base::flat_set<not_null<History*>>();
	if (!Core::IsAppLaunched() || Core::Quitting()) {
		return result;
	}
	const auto addWithMigrated = [&](History *history) {
		if (!history) {
			return;
		}
		result.emplace(history);
		if (const auto from = history->peer->migrateFrom()) {
			if (const auto mh = history->owner().historyLoaded(from)) {
				result.emplace(mh);
			}
		}
	};
	Core::App().enumerateWindows([&](not_null<Window::Controller*> w) {
		if (const auto thread = w->id().thread) {
			addWithMigrated(thread->owningHistory());
		}
		if (const auto sc = w->sessionController()) {
			addWithMigrated(sc->activeChatCurrent().owningHistory());
			if (const auto content = w->widget()->sessionContent()) {
				if (const auto peer = content->peer()) {
					addWithMigrated(sc->session().data().historyLoaded(peer));
				}
			}
		}
	});
	return result;
}

void PerformBackgroundCleanup() {
	if (!Core::IsAppLaunched()
		|| Core::Quitting()
		|| HasVisibleUnminimizedWindow()) {
		return;
	}
	auto sessions = base::flat_set<not_null<Main::Session*>>();
	Core::App().enumerateWindows([&](not_null<Window::Controller*> w) {
		if (const auto sc = w->sessionController()) {
			sc->tabbedSelector()->hideFinished();
			sessions.emplace(&sc->session());
		}
	});
	for (const auto &session : sessions) {
		session->data().unloadAllHeavyViewParts();
		session->data().clearReplyPreviews();
		TrimBackgroundHistories(session, kMaxBackgroundHistories);
	}
	QPixmapCache::clear();
	OptimizeProcessMemory();
}

} // namespace

void InitMemoryManager(rpl::lifetime &appLifetime) {
	GlobalState = appLifetime.make_state<ManagerState>([] {
		PerformBackgroundCleanup();
	});
	appLifetime.add([] {
		GlobalState = nullptr;
	});
}

bool HasVisibleUnminimizedWindow() {
	if (!Core::IsAppLaunched() || Core::Quitting()) {
		return true;
	}
	auto anyVisible = false;
	auto hasWindows = false;
	Core::App().enumerateWindows([&](not_null<Window::Controller*> w) {
		hasWindows = true;
		const auto widget = w->widget();
		if (widget->isVisible()
			&& !(widget->windowState() & Qt::WindowMinimized)) {
			anyVisible = true;
		}
	});
	return !hasWindows || anyVisible;
}

bool HasActiveForegroundWindow() {
	if (!Core::IsAppLaunched() || Core::Quitting()) {
		return true;
	}
	auto anyActive = false;
	Core::App().enumerateWindows([&](not_null<Window::Controller*> w) {
		const auto widget = w->widget();
		if (widget->isActiveWindow()
			&& widget->isVisible()
			&& !(widget->windowState() & Qt::WindowMinimized)) {
			anyActive = true;
		}
	});
	return anyActive;
}

void RefreshBackgroundPowerSaving() {
	if (!Core::IsAppLaunched() || Core::Quitting()) {
		return;
	}
	const auto batterySaving = Core::App().batterySaving().enabled();
	const auto ignore = Core::App().settings().ignoreBatterySaving();
	const auto forceByBattery = batterySaving.value_or(false) && !ignore;
	const auto forceByHidden = !HasVisibleUnminimizedWindow();
	PowerSaving::SetForceAll(forceByBattery || forceByHidden);
}

void NotifyWindowVisibilityChanged() {
	if (!Core::IsAppLaunched() || Core::Quitting()) {
		return;
	}
	RefreshBackgroundPowerSaving();
	if (!GlobalState) {
		return;
	}
	if (HasVisibleUnminimizedWindow()) {
		GlobalState->cleanupTimer.cancel();
		Core::App().enumerateWindows([](not_null<Window::Controller*> w) {
			const auto widget = w->widget();
			if (widget->isVisible()
				&& !(widget->windowState() & Qt::WindowMinimized)) {
				widget->update();
			}
		});
		return;
	}
	auto anyShownOnTaskbar = false;
	Core::App().enumerateWindows([&](not_null<Window::Controller*> w) {
		if (w->widget()->isVisible()) {
			anyShownOnTaskbar = true;
		}
	});
	const auto delay = anyShownOnTaskbar
		? kMinimizedCleanupDelay
		: kTrayCleanupDelay;
	GlobalState->cleanupTimer.callOnce(delay);
}

void OptimizeProcessMemory() {
#ifdef Q_OS_WIN
	if (const auto processHeap = GetProcessHeap()) {
		HeapCompact(processHeap, 0);
	}
	struct OptimizeInfo {
		DWORD version = 1;
		DWORD flags = 0;
	};
	using HeapOptimizeResourcesFn = BOOL(WINAPI*)(HANDLE, OptimizeInfo*);
	static const auto fn = []() -> HeapOptimizeResourcesFn {
		const auto kernel32 = GetModuleHandleW(L"kernel32.dll");
		return kernel32
			? reinterpret_cast<HeapOptimizeResourcesFn>(
				GetProcAddress(kernel32, "HeapOptimizeResources"))
			: nullptr;
	}();
	if (fn) {
		auto info = OptimizeInfo{};
		fn(nullptr, &info);
	}
#endif // Q_OS_WIN
}

void TrackVisitedHistory(History *history) {
	if (!GlobalState || !history || Core::Quitting()) {
		return;
	}
	const auto session = &history->session();
	const auto peerId = history->peer->id;
	auto &lru = GlobalState->historyLru;
	lru.erase(
		ranges::remove_if(lru, [&](const HistoryLruEntry &entry) {
			const auto strong = entry.session.get();
			return !strong || (strong == session && entry.peerId == peerId);
		}),
		end(lru));
	lru.insert(begin(lru), HistoryLruEntry{
		.session = base::make_weak(session),
		.peerId = peerId,
	});
	TrimBackgroundHistories(session, kMaxActiveHistories);
}

void TrimBackgroundHistories(
		not_null<Main::Session*> session,
		std::size_t keepCount) {
	if (!GlobalState || Core::Quitting()) {
		return;
	}
	const auto openHistories = CollectOpenHistories();
	auto &lru = GlobalState->historyLru;
	auto keptForSession = std::size_t(0);
	for (auto i = begin(lru); i != end(lru);) {
		const auto strong = i->session.get();
		if (!strong) {
			i = lru.erase(i);
			continue;
		}
		if (strong != session.get()) {
			++i;
			continue;
		}
		const auto history = session->data().historyLoaded(i->peerId);
		if (!history || history->isEmpty()) {
			i = lru.erase(i);
			continue;
		}
		if (openHistories.contains(history)
			|| !history->clientSideMessages().empty()) {
			++keptForSession;
			++i;
			continue;
		}
		if (keptForSession < keepCount) {
			++keptForSession;
			++i;
			continue;
		}
		if (const auto from = history->peer->migrateFrom()) {
			if (const auto mh = session->data().historyLoaded(from)) {
				if (!openHistories.contains(mh)
					&& mh->clientSideMessages().empty()) {
					mh->clear(History::ClearType::Unload);
				}
			}
		}
		history->clear(History::ClearType::Unload);
		i = lru.erase(i);
	}
}

} // namespace KazeGram
