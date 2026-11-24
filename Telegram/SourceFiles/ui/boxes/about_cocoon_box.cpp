/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#include "ui/boxes/about_cocoon_box.h"

#include "ui/layers/generic_box.h"
#include "ui/widgets/buttons.h"
#include "styles/style_credits.h"

namespace Ui {

void AddUniqueCloseButton(not_null<GenericBox*> box) {
	const auto close = Ui::CreateChild<IconButton>(
		box,
		st::uniqueCloseButton);
	close->show();
	close->raise();
	box->widthValue() | rpl::on_next([=](int width) {
		close->moveToRight(0, 0, width);
		close->raise();
	}, close->lifetime());
	close->setClickedCallback([=] {
		box->closeBox();
	});
}

} // namespace Ui
