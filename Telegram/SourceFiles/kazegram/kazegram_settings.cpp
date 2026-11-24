/*
This file is part of KazeGram Desktop,
based on Telegram Desktop.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#include "kazegram/kazegram_settings.h"

#include "lang/lang_instance.h"
#include "settings.h"
#include "ui/style/style_core.h"

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QLocale>

namespace KazeGram {
namespace {

constexpr auto kDefaultCustomColor = QColor(255, 255, 255);

[[nodiscard]] IconColorMode ClampIconColorMode(int raw) {
	switch (raw) {
	case int(IconColorMode::ThemeAccent):
		return IconColorMode::ThemeAccent;
	case int(IconColorMode::Custom):
		return IconColorMode::Custom;
	default:
		return IconColorMode::Default;
	}
}

} // namespace

Settings::Settings() = default;

QString Settings::filePath() const {
	const auto dir = cWorkingDir();
	return dir.isEmpty()
		? QString()
		: (dir + u"tdata/kazegram_settings.json"_q);
}

void Settings::ensureLoaded() {
	if (_loaded) {
		return;
	}
	const auto path = filePath();
	if (path.isEmpty()) {
		return;
	}
	_loaded = true;

	auto file = QFile(path);
	if (!file.open(QIODevice::ReadOnly)) {
		return;
	}
	auto error = QJsonParseError();
	const auto doc = QJsonDocument::fromJson(file.readAll(), &error);
	if (error.error != QJsonParseError::NoError || !doc.isObject()) {
		return;
	}
	const auto obj = doc.object();
	if (obj.contains(u"icon_color_mode"_q)) {
		_iconColorMode = ClampIconColorMode(
			obj.value(u"icon_color_mode"_q).toInt(0));
	}
	if (obj.contains(u"icon_custom_color"_q)) {
		auto color = QColor::fromString(
			obj.value(u"icon_custom_color"_q).toString());
		if (color.isValid()) {
			color.setAlpha(255);
			_iconCustomColor = color;
		}
	}
}

void Settings::save() {
	const auto path = filePath();
	if (path.isEmpty()) {
		return;
	}
	QDir().mkpath(cWorkingDir() + u"tdata"_q);
	auto obj = QJsonObject();
	obj.insert(u"icon_color_mode"_q, int(_iconColorMode.current()));
	obj.insert(
		u"icon_custom_color"_q,
		_iconCustomColor.current().name(QColor::HexRgb));

	auto file = QFile(path);
	if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
		file.write(QJsonDocument(obj).toJson(QJsonDocument::Indented));
	}
}

IconColorMode Settings::iconColorMode() {
	ensureLoaded();
	return _iconColorMode.current();
}

void Settings::setIconColorMode(IconColorMode mode) {
	ensureLoaded();
	if (_iconColorMode.current() == mode) {
		return;
	}
	_iconColorMode = mode;
	save();
}

rpl::producer<IconColorMode> Settings::iconColorModeChanges() const {
	return _iconColorMode.changes();
}

QColor Settings::iconCustomColor() {
	ensureLoaded();
	return _iconCustomColor.current();
}

void Settings::setIconCustomColor(QColor color) {
	ensureLoaded();
	if (!color.isValid()) {
		color = kDefaultCustomColor;
	}
	color.setAlpha(255);
	if (_iconCustomColor.current() == color) {
		return;
	}
	_iconCustomColor = color;
	save();
}

rpl::producer<QColor> Settings::iconCustomColorChanges() const {
	return _iconCustomColor.changes();
}

rpl::producer<> Settings::iconVisualChanges() const {
	return rpl::merge(
		_iconColorMode.changes() | rpl::to_empty,
		_iconCustomColor.changes() | rpl::to_empty,
		style::PaletteChanged()
	);
}

Settings &AppSettings() {
	static Settings instance;
	return instance;
}

bool IsRussianLocale() {
	const auto &lang = Lang::GetInstance();
	const auto id = lang.id().toLower();
	const auto base = lang.baseId().toLower();
	if (!id.isEmpty()) {
		return id.startsWith(u"ru"_q)
			|| base.startsWith(u"ru"_q)
			|| id.startsWith(u"uk"_q)
			|| id.startsWith(u"be"_q)
			|| id.startsWith(u"kk"_q);
	}
	const auto sys = QLocale::system().language();
	return (sys == QLocale::Russian)
		|| (sys == QLocale::Ukrainian)
		|| (sys == QLocale::Belarusian)
		|| (sys == QLocale::Kazakh);
}

QString TrNow(const QString &ru, const QString &en) {
	return IsRussianLocale() ? ru : en;
}

rpl::producer<QString> Tr(QString ru, QString en) {
	return rpl::single(
		rpl::empty
	) | rpl::then(
		Lang::GetInstance().updated()
	) | rpl::map([=] {
		return TrNow(ru, en);
	});
}

} // namespace KazeGram
