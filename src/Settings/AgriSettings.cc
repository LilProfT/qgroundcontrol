#include "AgriSettings.h"

#include <QQmlEngine>
#include <QtQml>

const char* AgriSettings::sprayPressure               = QT_TRANSLATE_NOOP("AgriSettings", "Pressure");
const char* AgriSettings::sprayCentrifugal              = QT_TRANSLATE_NOOP("AgriSettings", "Centrifugal");
const char* AgriSettings::sprayCentrifugalRPM           = QT_TRANSLATE_NOOP("AgriSettings", "Centrifugal RPM");

DECLARE_SETTINGGROUP(Agri, "Agri")
{
    qmlRegisterUncreatableType<AgriSettings>("QGroundControl.SettingsManager", 1, 0, "AgriSettings", "Reference only");

    // Setup enum values for videoSource settings into meta data
    QVariantList videoSourceList;

    videoSourceList.append(sprayPressure);
    videoSourceList.append(sprayCentrifugal);

//    for(const auto& value : removeForceVideoDecodeList) {
//        _nameToMetaDataMap[forceVideoDecoderName]->removeEnumInfo(value);
//    }

    // make translated strings
    QStringList videoSourceCookedList;
    for (const QVariant& videoSource: videoSourceList) {
        videoSourceCookedList.append( AgriSettings::tr(videoSource.toString().toStdString().c_str()) );
    }

    _nameToMetaDataMap[sprayOptionName]->setEnumInfo(videoSourceCookedList, videoSourceList);

    // Set default value for videoSource
    _setDefaults();
}

void AgriSettings::_setDefaults()
{
        _nameToMetaDataMap[sprayOptionName]->setRawDefaultValue(sprayPressure);

}

DECLARE_SETTINGSFACT_NO_FUNC(AgriSettings, sprayOption)
{
    if (!_sprayOptionFact) {
        _sprayOptionFact = _createSettingsFact(sprayOptionName);
        //-- Check for sources no longer available
//        if(!_sprayOptionFact->enumValues().contains(_sprayOptionFact->rawValue().toString())) {
//            if (_noVideo) {
//                _sprayOptionFact->setRawValue(sprayPressure);
//            } else {
//                _sprayOptionFact->setRawValue(sprayCentrifugal);
//            }
//        }
        connect(_sprayOptionFact, &Fact::valueChanged, this, &AgriSettings::_configChanged);
    }
    return _sprayOptionFact;
}


DECLARE_SETTINGSFACT_NO_FUNC(AgriSettings, sprayCentrifugalRPMSetting)
{
    if (!_sprayCentrifugalRPMSettingFact) {
        _sprayCentrifugalRPMSettingFact = _createSettingsFact(sprayCentrifugalRPMSettingName);
        connect(_sprayCentrifugalRPMSettingFact, &Fact::valueChanged, this, &AgriSettings::_configChanged);
    }
    return _sprayCentrifugalRPMSettingFact;
}


void AgriSettings::_configChanged(QVariant  value)
{
    emit centrifugalRPMConfiguredChanged(value.toDouble());
}
