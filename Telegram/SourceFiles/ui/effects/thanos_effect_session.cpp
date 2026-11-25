/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#include "ui/effects/thanos_effect_session.h"

#include "ui/effects/thanos_effect.h"
#include "data/data_folder.h"
#include "data/data_session.h"
#include "main/main_session.h"

namespace Ui {

void ScheduleThanosEffectWarmUp(
		not_null<Main::Session*> session,
		rpl::lifetime &lifetime) {
	(void)session;
	(void)lifetime;
}

} // namespace Ui
