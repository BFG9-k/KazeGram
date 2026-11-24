/*
This file is part of KazeGram Desktop,
based on Telegram Desktop.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#include "settings/sections/settings_kazegram.h"

#include "settings/settings_common_session.h"

#include "kazegram/kazegram_icon.h"
#include "kazegram/kazegram_settings.h"
#include "core/application.h"
#include "core/core_settings.h"
#include "lang/lang_keys.h"
#include "ui/layers/generic_box.h"
#include "ui/painter.h"
#include "ui/vertical_list.h"
#include "ui/widgets/buttons.h"
#include "ui/widgets/checkbox.h"
#include "ui/widgets/color_editor.h"
#include "ui/widgets/tooltip.h"
#include "ui/wrap/vertical_layout.h"
#include "window/themes/window_theme.h"
#include "window/window_session_controller.h"
#include "styles/style_menu_icons.h"
#include "styles/style_settings.h"

namespace Settings {
namespace {

class KazeGramSection final : public Section<KazeGramSection> {
public:
	KazeGramSection(
		QWidget *parent,
		not_null<Window::SessionController*> controller);

	[[nodiscard]] rpl::producer<QString> title() override;

private:
	void setupContent();

};

KazeGramSection::KazeGramSection(
	QWidget *parent,
	not_null<Window::SessionController*> controller)
: Section(parent, controller) {
	setupContent();
}

rpl::producer<QString> KazeGramSection::title() {
	return ::KazeGram::Tr(u"Настройки KazeGram"_q, u"KazeGram Settings"_q);
}

void KazeGramSection::setupContent() {
	const auto content = Ui::CreateChild<Ui::VerticalLayout>(this);

	Ui::AddSkip(content);
	Ui::AddSubsectionTitle(
		content,
		::KazeGram::Tr(u"Иконка приложения"_q, u"Application Icon"_q));

	const auto preview = content->add(
		object_ptr<Ui::FixedHeightWidget>(content, 68));
	const auto taskbarPreview = Ui::CreateChild<Ui::RpWidget>(preview);
	taskbarPreview->resize(48, 48);
	const auto trayPreview = Ui::CreateChild<Ui::RpWidget>(preview);
	trayPreview->resize(28, 28);

	preview->widthValue(
	) | rpl::on_next([=](int width) {
		constexpr auto kGap = 16;
		const auto total = taskbarPreview->width() + kGap + trayPreview->width();
		const auto left = (width - total) / 2;
		taskbarPreview->move(
			left,
			(preview->height() - taskbarPreview->height()) / 2);
		trayPreview->move(
			left + taskbarPreview->width() + kGap,
			(preview->height() - trayPreview->height()) / 2);
	}, preview->lifetime());

	struct PreviewCache {
		QImage taskbar;
		QImage tray;
		int ratio = 0;
	};
	const auto cache = preview->lifetime().make_state<PreviewCache>();
	const auto rebuildCache = [=] {
		const auto ratio = style::DevicePixelRatio();
		cache->ratio = ratio;
		cache->taskbar = ::KazeGram::RenderKazeIcon(48 * ratio, true);
		cache->taskbar.setDevicePixelRatio(ratio);
		cache->tray = Core::App().settings().trayIconMonochrome()
			? ::KazeGram::RenderKazeIcon(
				16 * ratio,
				false,
				QColor(255, 255, 255))
			: ::KazeGram::RenderKazeIcon(16 * ratio, false);
		cache->tray.setDevicePixelRatio(ratio);
		taskbarPreview->update();
		trayPreview->update();
	};
	rebuildCache();

	rpl::merge(
		::KazeGram::AppSettings().iconVisualChanges(),
		Core::App().settings().trayIconMonochromeChanges() | rpl::to_empty
	) | rpl::on_next(rebuildCache, preview->lifetime());

	taskbarPreview->paintRequest(
	) | rpl::on_next([=] {
		if (style::DevicePixelRatio() != cache->ratio) {
			rebuildCache();
		}
		auto p = Painter(taskbarPreview);
		PainterHighQualityEnabler hq(p);
		if (!Window::Theme::IsNightMode()
			&& ::KazeGram::CurrentIconPrimaryColor().lightness() > 220) {
			p.setPen(Qt::NoPen);
			p.setBrush(QColor(20, 25, 35));
			p.drawRoundedRect(QRectF(0, 0, 48, 48), 10.0, 10.0);
		}
		p.drawImage(0, 0, cache->taskbar);
	}, taskbarPreview->lifetime());

	trayPreview->paintRequest(
	) | rpl::on_next([=] {
		if (style::DevicePixelRatio() != cache->ratio) {
			rebuildCache();
		}
		auto p = Painter(trayPreview);
		PainterHighQualityEnabler hq(p);
		p.setPen(QPen(QColor(255, 255, 255, 28), 1.0));
		p.setBrush(QColor(20, 25, 35));
		p.drawRoundedRect(QRectF(0.5, 0.5, 27.0, 27.0), 7.0, 7.0);
		p.drawImage(6, 6, cache->tray);
	}, trayPreview->lifetime());

	Ui::InstallTooltip(taskbarPreview, [] {
		return ::KazeGram::TrNow(
			u"Эта иконка отображается на панели задач"_q,
			u"This icon is displayed in the taskbar"_q);
	});
	Ui::InstallTooltip(trayPreview, [] {
		return ::KazeGram::TrNow(
			u"Эта иконка отображается в системном трее"_q,
			u"This icon is displayed in the system tray"_q);
	});

	Ui::AddSkip(content);

	const auto group = std::make_shared<Ui::RadiobuttonGroup>(
		int(::KazeGram::AppSettings().iconColorMode()));

	const struct RadioOption {
		::KazeGram::IconColorMode mode;
		QString ru;
		QString en;
	} options[] = {
		{
			::KazeGram::IconColorMode::Default,
			u"По умолчанию"_q,
			u"Default"_q,
		},
		{
			::KazeGram::IconColorMode::ThemeAccent,
			u"Под акцент активной темы"_q,
			u"Match Active Theme Accent"_q,
		},
		{
			::KazeGram::IconColorMode::Custom,
			u"Свой цвет"_q,
			u"Custom Color"_q,
		},
	};
	for (const auto &option : options) {
		content->add(
			object_ptr<Ui::Radiobutton>(
				content,
				group,
				int(option.mode),
				::KazeGram::TrNow(option.ru, option.en),
				st::settingsSendType),
			st::settingsSendTypePadding);
	}

	group->setChangedCallback([=](int value) {
		const auto mode = static_cast<::KazeGram::IconColorMode>(value);
		::KazeGram::AppSettings().setIconColorMode(mode);
		if (mode != ::KazeGram::IconColorMode::Default
			&& Core::App().settings().trayIconMonochrome()) {
			Core::App().settings().setTrayIconMonochrome(false);
			Core::App().saveSettingsDelayed();
		}
		::KazeGram::RefreshAllAppIcons();
	});

	Ui::AddSkip(content, st::settingsCheckboxesSkip);

	const auto chooseBtn = AddButtonWithLabel(
		content,
		::KazeGram::Tr(u"Выбрать свой цвет..."_q, u"Choose Custom Color..."_q),
		rpl::single(
			rpl::empty
		) | rpl::then(
			::KazeGram::AppSettings().iconCustomColorChanges() | rpl::to_empty
		) | rpl::map([] {
			return ::KazeGram::AppSettings().iconCustomColor().name(
				QColor::HexRgb).toUpper();
		}),
		st::settingsButton,
		{ &st::menuIconChangeColors });

	const auto weakSection = base::make_weak(this);
	chooseBtn->setClickedCallback([=] {
		controller()->show(Box([=](not_null<Ui::GenericBox*> box) {
			const auto editor = box->addRow(object_ptr<ColorEditor>(
				box,
				ColorEditor::Mode::HSL,
				::KazeGram::AppSettings().iconCustomColor()));

			const auto save = [=] {
				::KazeGram::AppSettings().setIconCustomColor(editor->color());
				::KazeGram::AppSettings().setIconColorMode(
					::KazeGram::IconColorMode::Custom);
				if (weakSection) {
					group->setValue(int(::KazeGram::IconColorMode::Custom));
				}
				if (Core::App().settings().trayIconMonochrome()) {
					Core::App().settings().setTrayIconMonochrome(false);
					Core::App().saveSettingsDelayed();
				}
				::KazeGram::RefreshAllAppIcons();
				box->closeBox();
			};
			editor->submitRequests(
			) | rpl::on_next(save, editor->lifetime());

			box->setFocusCallback([=] {
				editor->setInnerFocus();
			});
			box->addButton(tr::lng_settings_save(), save);
			box->addButton(tr::lng_cancel(), [=] { box->closeBox(); });
			box->setTitle(::KazeGram::Tr(
				u"Цвет иконки KazeGram"_q,
				u"KazeGram Icon Color"_q));
			box->setWidth(editor->width());
		}));
	});

	const auto colorTray = AddButtonWithIcon(
		content,
		::KazeGram::Tr(
			u"Цветная иконка в системном трее"_q,
			u"Colorize System Tray Icon"_q),
		st::settingsButtonNoIcon);
	colorTray->toggleOn(
		rpl::single(!Core::App().settings().trayIconMonochrome()) | rpl::then(
			Core::App().settings().trayIconMonochromeChanges(
			) | rpl::map([](bool mono) { return !mono; })));
	colorTray->toggledChanges(
	) | rpl::filter([](bool colored) {
		return colored == Core::App().settings().trayIconMonochrome();
	}) | rpl::on_next([](bool colored) {
		Core::App().settings().setTrayIconMonochrome(!colored);
		Core::App().saveSettingsDelayed();
		::KazeGram::RefreshAllAppIcons();
	}, colorTray->lifetime());

	Ui::AddSkip(content);

	Ui::ResizeFitChild(this, content);
}

} // namespace

Type KazeGramId() {
	return KazeGramSection::Id();
}

} // namespace Settings
