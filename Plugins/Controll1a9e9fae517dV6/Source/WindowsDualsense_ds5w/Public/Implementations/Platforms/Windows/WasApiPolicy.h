// Copyright (c) 2025 Rafael Valoto/Publisher. All rights reserved.
// Created for: WindowsDualsense_ds5w - Plugin to support DualSense controller on Windows.
// Planned Release Year: 2025
#pragma once
#include "Helpers/DualSenseLog.h"
#include <SetupAPI.h>
#include <audioclient.h>
#include <cstdint>
#include <cstring>
#include <devpkey.h>
#include <hidsdi.h>
#include <limits>
#include <mmdeviceapi.h>
#include <mutex>
#include <propsys.h>
#include <string>
#include <vector>

// clang-format off
#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"
#include <Windows.h>
#include <Functiondiscoverykeys_devpkey.h>
#include "Windows/HideWindowsPlatformTypes.h"
#endif
// clang-format on

#pragma comment(lib, "Propsys.lib")

struct WasApiPolicy
{
public:
	using Policy = WasApiPolicy;

	using DevicePathType = std::string;
	using AudioDeviceType = IAudioClient*;
	using AudioDeviceIdType = std::wstring;
	using ContextType = FDeviceContext;

	// Send-only path: Unreal already provides samples from submix; this policy only writes to WASAPI output.
	using AudioRingBufferType = std::vector<float>;
	using AudioFrameCountType = int;

	int NumChannels = 2;
	int SampleRate = 48000;
	bool bInitialized = false;
	bool bHasDeviceId = false;
	bool bRingBufferInitialized = false;
	bool bFoundDevice = false;
	bool bComInitialized = false;
	bool bAudioStarted = false;

	DevicePathType DevicePath;
	AudioDeviceIdType DeviceId;
	AudioRingBufferType RingBuffer;

#if PLATFORM_WINDOWS
	IMMDeviceEnumerator* DeviceEnumerator = nullptr;
	IMMDevice* DeviceEndpoint = nullptr;
	IAudioClient* AudioClient = nullptr;
	IAudioRenderClient* AudioRenderClient = nullptr;
	UINT32 WasapiBufferFrameCount = 0;
#endif

	std::mutex WriteMutex;

	WasApiPolicy() = default;
	~WasApiPolicy()
	{
		Close();
	}

	bool InitializeWithDeviceId(const AudioDeviceIdType& InDeviceId)
	{
		return InitializeWithDeviceId(&InDeviceId, SampleRate, NumChannels);
	}

	bool InitializeWithDeviceId(const AudioDeviceIdType* InDeviceId, int InSampleRate = 48000, int InNumChannels = 4)
	{
		Close();

		if (!InDeviceId || InDeviceId->empty())
		{
			return false;
		}

		DeviceId = *InDeviceId;
		SampleRate = InSampleRate;
		NumChannels = InNumChannels;
		bHasDeviceId = true;

		if (!InitializeWasapiClient())
		{
			Close();
			return false;
		}

		bRingBufferInitialized = true;
		bInitialized = true;
		return true;
	}

	void Close()
	{
		std::lock_guard<std::mutex> Lock(WriteMutex);

#if PLATFORM_WINDOWS
		if (AudioClient && bAudioStarted)
		{
			AudioClient->Stop();
			bAudioStarted = false;
		}

		if (AudioRenderClient)
		{
			AudioRenderClient->Release();
			AudioRenderClient = nullptr;
		}

		if (AudioClient)
		{
			AudioClient->Release();
			AudioClient = nullptr;
		}

		if (DeviceEndpoint)
		{
			DeviceEndpoint->Release();
			DeviceEndpoint = nullptr;
		}

		if (DeviceEnumerator)
		{
			DeviceEnumerator->Release();
			DeviceEnumerator = nullptr;
		}

		if (bComInitialized)
		{
			CoUninitialize();
			bComInitialized = false;
		}
#endif

		WasapiBufferFrameCount = 0;
		RingBuffer.clear();
		DeviceId.clear();
		DevicePath.clear();
		bInitialized = false;
		bHasDeviceId = false;
		bRingBufferInitialized = false;
		bFoundDevice = false;
		bAudioStarted = false;
	}

	[[nodiscard]] bool IsValid() const
	{
		return bInitialized && bRingBufferInitialized;
	}

	void RegisterAudioDevice(const DevicePathType& InDevicePath, const AudioDeviceIdType* InDeviceId = nullptr)
	{
		DevicePath = InDevicePath;
		if (InDeviceId)
		{
			InitializeWithDeviceId(InDeviceId, SampleRate, NumChannels);
		}
	}

	void UnregisterAudioDevice(const DevicePathType& InDevicePath)
	{
		if (DevicePath == InDevicePath)
		{
			Close();
		}
	}

	bool WriteHapticData(const std::vector<std::int16_t>& InterleavedData)
	{
		std::lock_guard<std::mutex> Lock(WriteMutex);

		if (!IsValid() || InterleavedData.empty() || NumChannels < 2)
		{
			return false;
		}

		const AudioFrameCountType framesToWrite = static_cast<AudioFrameCountType>(InterleavedData.size() / 2);

		if (framesToWrite == 0)
		{
			return true;
		}

		RingBuffer.resize(static_cast<size_t>(framesToWrite) * static_cast<size_t>(NumChannels));
		auto* pOutputBuffer = RingBuffer.data();
		constexpr float kNormalization = 1.0f / 32768.0f;
		for (AudioFrameCountType i = 0; i < framesToWrite; i++)
		{
			const float LeftFloat = static_cast<float>(InterleavedData[static_cast<size_t>(i) * 2]) * kNormalization;
			const float RightFloat = static_cast<float>(InterleavedData[(static_cast<size_t>(i) * 2) + 1]) * kNormalization;
			const AudioFrameCountType baseIndex = i * NumChannels;

			if (NumChannels >= 4)
			{
				pOutputBuffer[baseIndex + 0] = 0.0f;
				pOutputBuffer[baseIndex + 1] = 0.0f;
				pOutputBuffer[baseIndex + 2] = LeftFloat;
				pOutputBuffer[baseIndex + 3] = RightFloat;
			}
			else
			{
				pOutputBuffer[baseIndex + 0] = LeftFloat;
				pOutputBuffer[baseIndex + 1] = RightFloat;
			}
		}

		const bool bResult = WriteToWasapiEndpoint(RingBuffer.data(), framesToWrite);
		RingBuffer.clear();
		return bResult;
	}

	bool InitializeAudioContainer(const ContextType* Context)
	{
		if (!Context)
		{
			return false;
		}

		DevicePath = Context->Path;
		if (DevicePath.empty())
		{
			return false;
		}

		const std::string TargetContainerId = get_container_id(DevicePath);
		if (TargetContainerId.empty())
		{
			UE_LOG(LogDualSense, Warning, TEXT("InitializeAudioContainer: Failed to get HID container id."));
			return false;
		}

#if PLATFORM_WINDOWS
		HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
		const bool bTempComInitialized = SUCCEEDED(hr);
		if (FAILED(hr) && hr != RPC_E_CHANGED_MODE)
		{
			UE_LOG(LogDualSense, Error, TEXT("InitializeAudioContainer: CoInitializeEx failed (0x%08X)"), hr);
			return false;
		}

		IMMDeviceEnumerator* pEnumerator = nullptr;
		hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
		                      __uuidof(IMMDeviceEnumerator), reinterpret_cast<void**>(&pEnumerator));
		if (FAILED(hr) || !pEnumerator)
		{
			if (bTempComInitialized)
			{
				CoUninitialize();
			}
			UE_LOG(LogDualSense, Error, TEXT("InitializeAudioContainer: CoCreateInstance failed (0x%08X)"), hr);
			return false;
		}

		IMMDeviceCollection* pCollection = nullptr;
		hr = pEnumerator->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &pCollection);
		pEnumerator->Release();
		if (FAILED(hr) || !pCollection)
		{
			if (bTempComInitialized)
			{
				CoUninitialize();
			}
			UE_LOG(LogDualSense, Error, TEXT("InitializeAudioContainer: EnumAudioEndpoints failed (0x%08X)"), hr);
			return false;
		}

		UINT Count = 0;
		pCollection->GetCount(&Count);

		AudioDeviceIdType FoundEndpointId;
		for (UINT i = 0; i < Count; ++i)
		{
			IMMDevice* pDevice = nullptr;
			if (FAILED(pCollection->Item(i, &pDevice)) || !pDevice)
			{
				continue;
			}

			LPWSTR pwszId = nullptr;
			if (SUCCEEDED(pDevice->GetId(&pwszId)) && pwszId)
			{
				const std::string AudioContainerId = GetAudioContainerId(pwszId);
				if (AudioContainerId == TargetContainerId)
				{
					FoundEndpointId = pwszId;
					CoTaskMemFree(pwszId);
					pDevice->Release();
					break;
				}
				CoTaskMemFree(pwszId);
			}

			pDevice->Release();
		}

		pCollection->Release();

		if (bTempComInitialized)
		{
			CoUninitialize();
		}

		if (FoundEndpointId.empty())
		{
			UE_LOG(LogDualSense, Warning, TEXT("InitializeAudioContainer: No audio endpoint matched HID container id."));
			return false;
		}

		bFoundDevice = true;
		return InitializeWithDeviceId(&FoundEndpointId, SampleRate, NumChannels);
#else
		return false;
#endif
	}

	std::string get_container_id(const std::string& InDevicePath)
	{
		std::wstring WPath(InDevicePath.begin(), InDevicePath.end());
		GUID HidGuid;
		HidD_GetHidGuid(&HidGuid);

		HDEVINFO DeviceInfoSet = SetupDiGetClassDevsW(&HidGuid, nullptr, nullptr, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
		if (DeviceInfoSet == INVALID_HANDLE_VALUE)
		{
			return "";
		}

		SP_DEVICE_INTERFACE_DATA DeviceInterfaceData = {sizeof(SP_DEVICE_INTERFACE_DATA)};
		if (SetupDiOpenDeviceInterfaceW(DeviceInfoSet, WPath.c_str(), 0, &DeviceInterfaceData))
		{
			SP_DEVINFO_DATA DeviceInfoData = {sizeof(SP_DEVINFO_DATA)};
			// DetailData is needed to get the DevInfoData associated with the interface
			DWORD RequiredSize = 0;
			SetupDiGetDeviceInterfaceDetailW(DeviceInfoSet, &DeviceInterfaceData, nullptr, 0, &RequiredSize, nullptr);
			if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
			{
				std::vector<char> Buffer(RequiredSize);
				PSP_DEVICE_INTERFACE_DETAIL_DATA_W pDetail = (PSP_DEVICE_INTERFACE_DETAIL_DATA_W)Buffer.data();
				pDetail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W);
				if (SetupDiGetDeviceInterfaceDetailW(DeviceInfoSet, &DeviceInterfaceData, pDetail, RequiredSize, nullptr, &DeviceInfoData))
				{
					DEVPROPTYPE PropType;
					GUID ContainerId = {0};
					if (SetupDiGetDevicePropertyW(DeviceInfoSet, &DeviceInfoData, &DEVPKEY_Device_ContainerId, &PropType, (PBYTE)&ContainerId, sizeof(GUID), nullptr, 0))
					{
						wchar_t GuidString[40];
						StringFromGUID2(ContainerId, GuidString, 40);
						SetupDiDestroyDeviceInfoList(DeviceInfoSet);

						char GuidStr[40];
						WideCharToMultiByte(CP_ACP, 0, GuidString, -1, GuidStr, 40, nullptr, nullptr);
						return std::string(GuidStr);
					}
				}
			}
		}
		SetupDiDestroyDeviceInfoList(DeviceInfoSet);
		return "";
	}

	std::string GetAudioContainerId(const wchar_t* AudioDeviceId)
	{
		IMMDeviceEnumerator* pEnumerator = nullptr;
		IMMDevice* pDevice = nullptr;
		IPropertyStore* pProps = nullptr;
		std::string Result;

		HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&pEnumerator);
		if (SUCCEEDED(hr))
		{
			hr = pEnumerator->GetDevice(AudioDeviceId, &pDevice);
			if (SUCCEEDED(hr))
			{
				hr = pDevice->OpenPropertyStore(STGM_READ, &pProps);
				if (SUCCEEDED(hr))
				{
					PROPVARIANT var;
					PropVariantInit(&var);
					hr = pProps->GetValue(PKEY_Device_ContainerId, &var);
					if (SUCCEEDED(hr) && var.vt == VT_CLSID)
					{
						wchar_t GuidString[40];
						StringFromGUID2(*var.puuid, GuidString, 40);

						char GuidStr[40];
						WideCharToMultiByte(CP_ACP, 0, GuidString, -1, GuidStr, 40, nullptr, nullptr);
						Result = std::string(GuidStr);
					}
					PropVariantClear(&var);
					pProps->Release();
				}
				pDevice->Release();
			}
			pEnumerator->Release();
		}

		return Result;
	}

private:
	bool InitializeWasapiClient()
	{
#if PLATFORM_WINDOWS
		if (!bHasDeviceId || DeviceId.empty())
		{
			return false;
		}

		HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
		const bool bComCallSucceeded = SUCCEEDED(hr);
		if (FAILED(hr) && hr != RPC_E_CHANGED_MODE)
		{
			UE_LOG(LogDualSense, Error, TEXT("InitializeWasapiClient: CoInitializeEx failed (0x%08X)"), hr);
			return false;
		}
		if (bComCallSucceeded)
		{
			bComInitialized = true;
		}

		hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
		                      __uuidof(IMMDeviceEnumerator), reinterpret_cast<void**>(&DeviceEnumerator));
		if (FAILED(hr) || !DeviceEnumerator)
		{
			UE_LOG(LogDualSense, Error, TEXT("InitializeWasapiClient: CoCreateInstance failed (0x%08X)"), hr);
			return false;
		}

		hr = DeviceEnumerator->GetDevice(DeviceId.c_str(), &DeviceEndpoint);
		if (FAILED(hr) || !DeviceEndpoint)
		{
			UE_LOG(LogDualSense, Error, TEXT("InitializeWasapiClient: GetDevice failed (0x%08X)"), hr);
			return false;
		}

		hr = DeviceEndpoint->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, reinterpret_cast<void**>(&AudioClient));
		if (FAILED(hr) || !AudioClient)
		{
			UE_LOG(LogDualSense, Error, TEXT("InitializeWasapiClient: Activate(IAudioClient) failed (0x%08X)"), hr);
			return false;
		}

		WAVEFORMATEX* MixFormat = nullptr;
		hr = AudioClient->GetMixFormat(&MixFormat);
		if (FAILED(hr) || !MixFormat)
		{
			UE_LOG(LogDualSense, Error, TEXT("InitializeWasapiClient: GetMixFormat failed (0x%08X)"), hr);
			return false;
		}

		NumChannels = MixFormat->nChannels;
		SampleRate = MixFormat->nSamplesPerSec;

		hr = AudioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, 0, 0, 0, MixFormat, nullptr);
		CoTaskMemFree(MixFormat);
		if (FAILED(hr))
		{
			UE_LOG(LogDualSense, Error, TEXT("InitializeWasapiClient: IAudioClient::Initialize failed (0x%08X)"), hr);
			return false;
		}

		hr = AudioClient->GetService(__uuidof(IAudioRenderClient), reinterpret_cast<void**>(&AudioRenderClient));
		if (FAILED(hr) || !AudioRenderClient)
		{
			UE_LOG(LogDualSense, Error, TEXT("InitializeWasapiClient: GetService(IAudioRenderClient) failed (0x%08X)"), hr);
			return false;
		}

		hr = AudioClient->GetBufferSize(&WasapiBufferFrameCount);
		if (FAILED(hr) || WasapiBufferFrameCount == 0)
		{
			UE_LOG(LogDualSense, Error, TEXT("InitializeWasapiClient: GetBufferSize failed (0x%08X)"), hr);
			return false;
		}

		hr = AudioClient->Start();
		if (FAILED(hr))
		{
			UE_LOG(LogDualSense, Error, TEXT("InitializeWasapiClient: AudioClient->Start failed (0x%08X)"), hr);
			return false;
		}

		bAudioStarted = true;
		return true;
#else
		return false;
#endif
	}

	bool WriteToWasapiEndpoint(const float* InAudioBuffer, AudioFrameCountType InFrameCount)
	{
#if PLATFORM_WINDOWS
		// Write-only render path. No capture/readback is used by this policy.
		if (!InAudioBuffer || InFrameCount <= 0 || !AudioClient || !AudioRenderClient || !bAudioStarted || NumChannels <= 0)
		{
			return false;
		}

		UINT32 Padding = 0;
		HRESULT hr = AudioClient->GetCurrentPadding(&Padding);
		if (FAILED(hr) || Padding > WasapiBufferFrameCount)
		{
			return false;
		}

		const UINT32 AvailableFrames = WasapiBufferFrameCount - Padding;
		const UINT32 FramesToWrite = static_cast<UINT32>(
		    std::min<AudioFrameCountType>(static_cast<AudioFrameCountType>(AvailableFrames), InFrameCount));
		if (FramesToWrite == 0)
		{
			return true;
		}

		BYTE* pData = nullptr;
		hr = AudioRenderClient->GetBuffer(FramesToWrite, &pData);
		if (FAILED(hr) || !pData)
		{
			return false;
		}

		const size_t BytesToCopy = static_cast<size_t>(FramesToWrite) * static_cast<size_t>(NumChannels) * sizeof(float);
		std::memcpy(pData, InAudioBuffer, BytesToCopy);

		hr = AudioRenderClient->ReleaseBuffer(FramesToWrite, 0);
		return SUCCEEDED(hr);
#else
		return false;
#endif
	}
};
