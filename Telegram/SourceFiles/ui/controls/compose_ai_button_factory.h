/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#pragma once

class QMimeData;

namespace Main {
class Session;
} // namespace Main

namespace Ui {
class InputField;
} // namespace Ui

namespace Ui {

struct PreparedList;

[[nodiscard]] bool HasEnoughLinesForExpand(not_null<Ui::InputField*> field);

[[nodiscard]] PreparedList PrepareTextAsFile(const QString &text);

struct LargeTextPasteResult {
	bool exceeds = false;
	QString resultingText;
};

[[nodiscard]] LargeTextPasteResult CheckLargeTextPaste(
	not_null<Main::Session*> session,
	not_null<Ui::InputField*> field,
	not_null<const QMimeData*> data);

} // namespace Ui
