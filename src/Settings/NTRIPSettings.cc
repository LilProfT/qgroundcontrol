#include "NTRIPSettings.h"

#include <QQmlEngine>
#include <QtQml>

DECLARE_SETTINGGROUP(NTRIP, "NTRIP")
{
    qmlRegisterUncreatableType<NTRIPSettings>("QGroundControl.SettingsManager", 1, 0, "NTRIPSettings", "Reference only"); \
}

DECLARE_SETTINGSFACT(NTRIPSettings, ntripServerConnectEnable)
DECLARE_SETTINGSFACT(NTRIPSettings, ntripServerHostAddress)
DECLARE_SETTINGSFACT(NTRIPSettings, ntripServerPort)
DECLARE_SETTINGSFACT(NTRIPSettings, ntripServerUserName)
DECLARE_SETTINGSFACT(NTRIPSettings, ntripServerPasswd)
DECLARE_SETTINGSFACT(NTRIPSettings, ntripServerMntpnt)
