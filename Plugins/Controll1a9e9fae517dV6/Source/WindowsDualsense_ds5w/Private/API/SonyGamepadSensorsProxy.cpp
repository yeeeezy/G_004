// Copyright (c) 2025 Rafael Valoto/Publisher. All rights reserved.
// Created for: WindowsDualsense_ds5w - Plugin to support DualSense controller on Windows.
// Planned Release Year: 2025

#include "API/SonyGamepadSensorsProxy.h"
#include "API/SonyGamepadProxyHelpers.h"

using namespace SonyGamepadProxyHelpers;

void USonyGamepadSensorsProxy::ResetGyroOrientation(int32 ControllerId)
{
	auto* Gamepad = GetGamepad(ControllerId);
	if (!Gamepad)
	{
		return;
	}

	if (auto* Sensors = Gamepad->GetIGamepadSensors())
	{
		Sensors->ResetGyroOrientation();
	}
}

void USonyGamepadSensorsProxy::EnableGyroscopeValues(int32 ControllerId, bool bEnableGyroscope)
{
	auto* Gamepad = GetGamepad(ControllerId);
	if (!Gamepad)
	{
		return;
	}

	if (auto* Sensors = Gamepad->GetIGamepadSensors())
	{
		Sensors->EnableMotionSensor(bEnableGyroscope);
	}
}
