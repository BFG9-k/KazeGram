/*
This file is part of KazeGram Desktop,
based on Telegram Desktop.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#pragma once

#include <QtGui/QColor>

namespace KazeGram {

enum class IconColorMode {
	Default = 0,
	ThemeAccent = 1,
	Custom = 2,
};

class Settings final {
public:
	Settings();

	void ensureLoaded();
	void save();

	[[nodiscard]] IconColorMode iconColorMode();
	void setIconColorMode(IconColorMode mode);
	[[nodiscard]] rpl::producer<IconColorMode> iconColorModeChanges() const;

	[[nodiscard]] QColor iconCustomColor();
	void setIconCustomColor(QColor color);
	[[nodiscard]] rpl::producer<QColor> iconCustomColorChanges() const;

	[[nodiscard]] rpl::producer<> iconVisualChanges() const;

private:
	[[nodiscard]] QString filePath() const;

	bool _loaded = false;
	rpl::variable<IconColorMode> _iconColorMode = IconColorMode::Default;
	rpl::variable<QColor> _iconCustomColor = QColor(255, 255, 255);

};

[[nodiscard]] Settings &AppSettings();

[[nodiscard]] bool IsRussianLocale();
[[nodiscard]] QString TrNow(const QString &ru, const QString &en);
[[nodiscard]] rpl::producer<QString> Tr(QString ru, QString en);

} // namespace KazeGram
