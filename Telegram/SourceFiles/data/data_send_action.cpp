/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#include "data/data_send_action.h"

#include "data/data_user.h"
#include "history/history.h"
#include "history/view/history_view_send_action.h"
#include "kazegram/kazegram_memory.h"

namespace Data {
namespace {

constexpr auto kInactiveCheckPeriod = crl::time(1000);

} // namespace

SendActionManager::SendActionManager()
: _animation([=](crl::time now) { return callback(now); })
, _inactiveTimer([=] { checkInactiveActions(); }) {
}

HistoryView::SendActionPainter *SendActionManager::lookupPainter(
		not_null<History*> history,
		MsgId rootId) {
	if (!rootId) {
		return history->sendActionPainter();
	}
	const auto i = _painters.find(history);
	if (i == end(_painters)) {
		return nullptr;
	}
	const auto j = i->second.find(rootId);
	if (j == end(i->second)) {
		return nullptr;
	}
	const auto result = j->second.lock();
	if (!result) {
		i->second.erase(j);
		if (i->second.empty()) {
			_painters.erase(i);
		}
		return nullptr;
	}
	crl::on_main([copy = result] {
	});
	return result.get();
}

void SendActionManager::registerFor(
		not_null<History*> history,
		MsgId rootId,
		not_null<UserData*> user,
		const MTPSendMessageAction &action,
		TimeId when) {
	if (history->peer->isSelf()) {
		return;
	}
	const auto sendAction = lookupPainter(history, rootId);
	if (!sendAction) {
		return;
	}
	if (sendAction->updateNeedsAnimating(user, action)) {
		user->madeAction(when);

		if (!_sendActions.contains(std::pair{ history, rootId })) {
			_sendActions.emplace(std::pair{ history, rootId }, crl::now());
			if (KazeGram::HasActiveForegroundWindow()) {
				_inactiveTimer.cancel();
				_animation.start();
			} else if (!_animation.animating() && !_inactiveTimer.isActive()) {
				_inactiveTimer.callEach(kInactiveCheckPeriod);
			}
		}
	}
}

auto SendActionManager::repliesPainter(
	not_null<History*> history,
	MsgId rootId)
-> std::shared_ptr<SendActionPainter> {
	auto &weak = _painters[history][rootId];
	if (auto strong = weak.lock()) {
		return strong;
	}
	auto result = std::make_shared<SendActionPainter>(history, rootId);
	weak = result;
	return result;
}

void SendActionManager::repliesPainterRemoved(
		not_null<History*> history,
		MsgId rootId) {
	const auto i = _painters.find(history);
	if (i == end(_painters)) {
		return;
	}
	const auto j = i->second.find(rootId);
	if (j == end(i->second) || j->second.lock()) {
		return;
	}
	i->second.erase(j);
	if (i->second.empty()) {
		_painters.erase(i);
	}
}

void SendActionManager::repliesPaintersClear(
		not_null<History*> history,
		not_null<UserData*> user) {
	auto &map = _painters[history];
	for (auto i = map.begin(); i != map.end();) {
		if (auto strong = i->second.lock()) {
			strong->clear(user);
			++i;
		} else {
			i = map.erase(i);
		}
	}
	if (map.empty()) {
		_painters.erase(history);
	}
}

void SendActionManager::updateSendActions(crl::time now) {
	for (auto i = begin(_sendActions); i != end(_sendActions);) {
		const auto sendAction = lookupPainter(
			i->first.first,
			i->first.second);
		if (sendAction && sendAction->updateNeedsAnimating(now)) {
			++i;
		} else {
			i = _sendActions.erase(i);
		}
	}
}

void SendActionManager::checkInactiveActions() {
	if (KazeGram::HasActiveForegroundWindow()) {
		updateActiveWindowState();
		return;
	}
	updateSendActions(crl::now());
	if (_sendActions.empty()) {
		_inactiveTimer.cancel();
	}
}

bool SendActionManager::callback(crl::time now) {
	updateSendActions(now);
	if (_sendActions.empty()) {
		_inactiveTimer.cancel();
		return false;
	}
	if (!KazeGram::HasActiveForegroundWindow()) {
		if (!_inactiveTimer.isActive()) {
			_inactiveTimer.callEach(kInactiveCheckPeriod);
		}
		return false;
	}
	return true;
}

auto SendActionManager::animationUpdated() const
-> rpl::producer<SendActionManager::AnimationUpdate> {
	return _animationUpdate.events();
}

void SendActionManager::updateAnimation(AnimationUpdate &&update) {
	_animationUpdate.fire(std::move(update));
}

auto SendActionManager::speakingAnimationUpdated() const
-> rpl::producer<not_null<History*>> {
	return _speakingAnimationUpdate.events();
}

void SendActionManager::updateSpeakingAnimation(not_null<History*> history) {
	_speakingAnimationUpdate.fire_copy(history);
}

void SendActionManager::updateActiveWindowState() {
	if (_sendActions.empty()) {
		_inactiveTimer.cancel();
		_animation.stop();
		return;
	}
	if (KazeGram::HasActiveForegroundWindow()) {
		_inactiveTimer.cancel();
		if (!_animation.animating()) {
			_animation.start();
		}
	} else {
		_animation.stop();
		if (!_inactiveTimer.isActive()) {
			_inactiveTimer.callEach(kInactiveCheckPeriod);
		}
	}
}

void SendActionManager::clear() {
	_sendActions.clear();
	_inactiveTimer.cancel();
	_animation.stop();
}

} // namespace Data
