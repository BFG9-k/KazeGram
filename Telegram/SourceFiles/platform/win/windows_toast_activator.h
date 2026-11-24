/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#pragma once

#include "base/platform/win/base_windows_rpcndr_h.h"
#include "windows_toastactivator_h.h"

#include "base/platform/win/wrl/wrl_implements_h.h"

// {8B4E9A12-3D7C-4F58-9E21-6C0D4B8A5F3E}
class DECLSPEC_UUID("8B4E9A12-3D7C-4F58-9E21-6C0D4B8A5F3E") ToastActivator
	: public ::Microsoft::WRL::RuntimeClass<
		::Microsoft::WRL::RuntimeClassFlags<::Microsoft::WRL::ClassicCom>,
		INotificationActivationCallback,
		::Microsoft::WRL::FtmBase> {
public:
	ToastActivator() = default;
	~ToastActivator() = default;

	HRESULT STDMETHODCALLTYPE Activate(
		_In_ LPCWSTR appUserModelId,
		_In_ LPCWSTR invokedArgs,
		_In_reads_(dataCount) const NOTIFICATION_USER_INPUT_DATA *data,
		ULONG dataCount) override;

	HRESULT STDMETHODCALLTYPE QueryInterface(
		REFIID riid,
		void **ppObj);
	ULONG STDMETHODCALLTYPE AddRef();
	ULONG STDMETHODCALLTYPE Release();

private:
	long _ref = 1;

};

struct ToastActivation {
	struct UserInput {
		QString key;
		QString value;
	};
	QString args;
	std::vector<UserInput> input;

	[[nodiscard]] static QString String(LPCWSTR value);
};
[[nodiscard]] rpl::producer<ToastActivation> ToastActivations();
