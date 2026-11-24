/*
This file is part of KazeGram Desktop,
based on Telegram Desktop.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#pragma once

#include <QtGui/QColor>
#include <QtGui/QImage>
#include <optional>

namespace KazeGram {

[[nodiscard]] uint64 CurrentIconCacheKey();
[[nodiscard]] QColor CurrentIconPrimaryColor();
[[nodiscard]] QImage RenderKazeIcon(
	int size,
	bool withMargin,
	std::optional<QColor> colorOverride = std::nullopt);
void RefreshAllAppIcons();

} // namespace KazeGram
