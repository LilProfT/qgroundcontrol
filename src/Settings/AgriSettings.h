#pragma once

#include "SettingsGroup.h"


class AgriSettings : public SettingsGroup
{
    Q_OBJECT

public:
    AgriSettings(QObject* parent = nullptr);
    DEFINE_SETTING_NAME_GROUP()

    DEFINE_SETTINGFACT(sprayOption)
    DEFINE_SETTINGFACT(sprayCentrifugalRPMSetting)

    Q_PROPERTY(QString  sprayPressureType        READ sprayPressureType        CONSTANT)
    Q_PROPERTY(QString  sprayCentrifugalType       READ sprayCentrifugalType      CONSTANT)
    Q_PROPERTY(QString  centrifugalRPM       READ centrifugalRPM      CONSTANT)

    QString  sprayPressureType      () { return sprayPressure; }
    QString  sprayCentrifugalType      () { return sprayCentrifugal; }
    QString  centrifugalRPM         () { return sprayCentrifugalRPM; }

    static const char* sprayPressure;
    static const char* sprayCentrifugal;
    static const char* sprayCentrifugalRPM;

signals:
    void centrifugalRPMConfiguredChanged    (double percent);

private slots:
    void _configChanged             (QVariant value);

private:
    void _setDefaults               ();

private:
    bool _noVideo = false;

};
