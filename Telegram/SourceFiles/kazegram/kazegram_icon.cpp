/*
This file is part of KazeGram Desktop,
based on Telegram Desktop.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#include "kazegram/kazegram_icon.h"

#include "kazegram/kazegram_settings.h"
#include "core/application.h"
#include "tray.h"
#include "window/main_window.h"
#include "window/window_controller.h"
#include "styles/palette.h"

#include <QtGui/QPainter>
#include <QtGui/QPainterPath>

namespace KazeGram {

QColor CurrentIconPrimaryColor() {
	switch (AppSettings().iconColorMode()) {
	case IconColorMode::Default:
		return QColor(255, 255, 255);
	case IconColorMode::ThemeAccent: {
		auto c = st::windowBgActive->c;
		c.setAlpha(255);
		return c;
	}
	case IconColorMode::Custom: {
		auto c = AppSettings().iconCustomColor();
		c.setAlpha(255);
		return c.isValid() ? c : QColor(255, 255, 255);
	}
	}
	return QColor(255, 255, 255);
}

uint64 CurrentIconCacheKey() {
	const auto mode = uint64(AppSettings().iconColorMode());
	const auto rgba = uint64(CurrentIconPrimaryColor().rgba());
	return ((mode + 1) << 32) | rgba;
}

QImage RenderKazeIcon(
		int size,
		bool withMargin,
		std::optional<QColor> colorOverride) {
	if (size <= 0) {
		return QImage();
	}
	auto result = QImage(size, size, QImage::Format_ARGB32_Premultiplied);
	result.fill(Qt::transparent);

	auto p = QPainter(&result);
	p.setRenderHint(QPainter::Antialiasing, true);

	const qreal minCoord = withMargin ? 8.0 : 15.0;
	const qreal span = withMargin ? 84.0 : 70.0;
	const qreal scale = qreal(size) / span;
	p.scale(scale, scale);
	p.translate(-minCoord, -minCoord);

	auto color = colorOverride.value_or(CurrentIconPrimaryColor());
	color.setAlpha(255);
	const auto brush = QBrush(color);

	const qreal boost = (size <= 20)
		? 1.18
		: (size <= 32)
		? 1.08
		: 1.0;

	p.setBrush(Qt::NoBrush);

	QPainterPath canopy;
	canopy.moveTo(21.0, 74.0);
	canopy.cubicTo(25.0, 62.0, 26.0, 45.0, 26.0, 26.0);
	canopy.lineTo(64.0, 26.0);
	canopy.cubicTo(63.0, 47.0, 64.0, 65.0, 72.0, 73.0);
	canopy.cubicTo(75.0, 76.0, 78.0, 73.0, 79.0, 62.0);
	p.setPen(QPen(
		brush,
		6.8 * boost,
		Qt::SolidLine,
		Qt::RoundCap,
		Qt::RoundJoin));
	p.drawPath(canopy);

	QPainterPath arms;
	arms.moveTo(53.0, 38.0);
	arms.quadTo(42.0, 45.0, 29.0, 51.0);
	arms.moveTo(36.0, 48.0);
	arms.quadTo(46.0, 59.0, 55.0, 71.0);
	p.setPen(QPen(
		brush,
		6.4 * boost,
		Qt::SolidLine,
		Qt::RoundCap,
		Qt::RoundJoin));
	p.drawPath(arms);

	return result;
}

void RefreshAllAppIcons() {
	if (!Core::IsAppLaunched()) {
		return;
	}
	auto &app = Core::App();
	app.refreshApplicationIcon();
	app.enumerateWindows([](not_null<Window::Controller*> window) {
		window->widget()->updateWindowIcon();
	});
	app.tray().updateIconCounters();
}

} // namespace KazeGram
