#pragma once

#include "SettingsGroup.h"

class FlightHubSettings : public SettingsGroup
{
    Q_OBJECT    
    public:
        FlightHubSettings(QObject* parent = nullptr);
        DEFINE_SETTING_NAME_GROUP()

        DEFINE_SETTINGFACT(flightHubServerConnectEnable)
        DEFINE_SETTINGFACT(flightHubServerHostAddress)
        DEFINE_SETTINGFACT(authServerHostAddress)
        DEFINE_SETTINGFACT(flightHubServerPort)
        DEFINE_SETTINGFACT(flightHubUserName)
        DEFINE_SETTINGFACT(flightHubPasswd)
        DEFINE_SETTINGFACT(flightHubDeviceToken)
        DEFINE_SETTINGFACT(flightHubLocation)
        DEFINE_SETTINGFACT(deviceCode)
};
