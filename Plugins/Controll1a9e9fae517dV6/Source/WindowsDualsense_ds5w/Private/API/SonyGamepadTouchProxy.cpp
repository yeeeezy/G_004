// Copyright (c) 2025 Rafael Valoto/Publisher. All rights reserved.
// Created for: WindowsDualsense_ds5w - Plugin to support DualSense controller on Windows.
// Planned Release Year: 2025

#include "API/SonyGamepadTouchProxy.h"
#include "API/SonyGamepadProxyHelpers.h"
#include "GCore/Interfaces/Segregations/IGamepadBase.h"

using namespace SonyGamepadProxyHelpers;
void USonyGamepadTouchProxy::EnableTouch(int32 ControllerId, bool bEnableTouch)
{
	if (IGamepadBase* Gamepad = GetGamepad(ControllerId))
	{
		if (auto* Touch = Gamepad->GetIGamepadTouch())
		{
			Touch->EnableTouch(bEnableTouch);
		}
	}
}

void USonyGamepadTouchProxy::EnableGesture(int32 ControllerId, bool bEnableGesture)
{
	if (IGamepadBase* Gamepad = GetGamepad(ControllerId))
	{
		if (auto* Touch = Gamepad->GetIGamepadTouch())
		{
			Touch->EnableGesture(bEnableGesture);
		}
	}
}
