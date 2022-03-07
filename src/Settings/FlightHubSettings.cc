#include "FlightHubSettings.h"

#include <QQmlEngine>
#include <QtQml>

DECLARE_SETTINGGROUP(FlightHub, "FlightHub")
{
    qmlRegisterUncreatableType<FlightHubSettings>("QGroundControl.SettingsManager", 1, 0, "FlightHubSettings", "Reference only");
}

DECLARE_SETTINGSFACT(FlightHubSettings, flightHubServerConnectEnable)
DECLARE_SETTINGSFACT(FlightHubSettings, flightHubServerHostAddress)
DECLARE_SETTINGSFACT(FlightHubSettings, authServerHostAddress)
DECLARE_SETTINGSFACT(FlightHubSettings, flightHubServerPort)
DECLARE_SETTINGSFACT(FlightHubSettings, flightHubUserName)
DECLARE_SETTINGSFACT(FlightHubSettings, flightHubPasswd)
DECLARE_SETTINGSFACT(FlightHubSettings, flightHubDeviceToken)
DECLARE_SETTINGSFACT(FlightHubSettings, flightHubLocation)
DECLARE_SETTINGSFACT(FlightHubSettings, deviceCode)
