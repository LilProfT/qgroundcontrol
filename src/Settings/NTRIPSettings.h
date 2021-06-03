#pragma once

#include "SettingsGroup.h"

class NTRIPSettings : public SettingsGroup
{
    Q_OBJECT    
    public:
        NTRIPSettings(QObject* parent = nullptr);
        DEFINE_SETTING_NAME_GROUP()

        DEFINE_SETTINGFACT(ntripServerConnectEnable)
        DEFINE_SETTINGFACT(ntripServerHostAddress)
        DEFINE_SETTINGFACT(ntripServerPort)
        DEFINE_SETTINGFACT(ntripServerUserName)
        DEFINE_SETTINGFACT(ntripServerPasswd)
        DEFINE_SETTINGFACT(ntripServerMntpnt)
};
