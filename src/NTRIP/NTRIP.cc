/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "NTRIP.h"
#include "QGCLoggingCategory.h"
#include "QGCApplication.h"
#include "SettingsManager.h"
#include "NTRIPSettings.h"
#include "PositionManager.h"
#include <QDebug>

NTRIP::NTRIP(QGCApplication* app, QGCToolbox* toolbox)
    : QGCTool(app, toolbox)
{
}

NTRIP::~NTRIP()
{
    disconnectGPS();
}

void NTRIP::setToolbox(QGCToolbox* toolbox)
{
    QGCTool::setToolbox(toolbox);
}

void NTRIP::connectGPSNTRIP()
{
//    NTRIPSettings* ntripSettings = qgcApp()->toolbox()->settingsManager()->ntripSettings();
//    QGeoCoordinate gcsPosition = _toolbox->qgcPositionManager()->gcsPosition();
    //qCWarning(RTKGPSLog) << "connectGPSNTRIP";
    //qCWarning(RTKGPSLog) << gcsPosition.longitude();
    _positionChanged++;
    //qCWarning(RTKGPSLog) << "_positionChanged: " << _positionChanged ;

//    if (ntripSettings->ntripServerConnectEnable()->rawValue().toBool() && (gcsPosition.longitude() == gcsPosition.longitude()) && _positionChanged >= 10 ){
//        if (!_rtkProvider) {
//            qCWarning(RTKGPSLog) << "_rtkProvider" ;
//            _requestRTKStop = false;
//            _rtkProvider = new RTKProvider(ntripSettings->ntripServerHostAddress()->rawValue().toString(),
//                                           ntripSettings->ntripServerPort()->rawValue().toInt(),
//                                           ntripSettings->ntripServerUserName()->rawValue().toString(),
//                                           ntripSettings->ntripServerPasswd()->rawValue().toString(),
//                                           ntripSettings->ntripServerMntpnt()->rawValue().toString(),
//                                           _requestRTKStop);
//            _rtkProvider->start();
//            _rtkProvider->vhcPosition = _vhcPosition;
//            _rtkProvider->syncInProgress = false;

//            _rtcmMavlink = new RTCMMavlink(*_toolbox);

//            connect(_rtkProvider, &RTKProvider::RTCMDataUpdate, _rtcmMavlink, &RTCMMavlink::RTCMDataUpdate);

//            //test: connect to position update
//            connect(_rtkProvider, &RTKProvider::positionUpdate,         this, &NTRIP::GPSPositionUpdate);
//            connect(_rtkProvider, &RTKProvider::satelliteInfoUpdate,    this, &NTRIP::GPSSatelliteUpdate);
//            connect(_rtkProvider, &RTKProvider::finished,               this, &NTRIP::onDisconnect);
//            connect(_rtkProvider, &RTKProvider::surveyInStatus,         this, &NTRIP::surveyInStatus);
//        }
//    }
}

void NTRIP::_coordinateChanged(QGeoCoordinate coordinate)
{
    _vhcPosition = coordinate;
    if (_rtkProvider != nullptr)
        _rtkProvider->vhcPosition = _vhcPosition;
}

void NTRIP::syncInProgressChanged(bool syncInProgress)
{
    if (_rtkProvider != nullptr) {
        qCWarning(RTKGPSLog) << "syncInProgress: " << syncInProgress;

        _rtkProvider->syncInProgress = syncInProgress;
    }
}

void NTRIP::sendComplete(bool error) {
    if (_rtkProvider != nullptr) {
        qCWarning(RTKGPSLog) << "sendComplete error: " << error;

        _rtkProvider->syncInProgress = false;
    }
}


void NTRIP::_tcpError(const QString errorMsg)
{
    qgcApp()->showAppMessage(tr("NTRIP Server Error: %1").arg(errorMsg));
}

void NTRIP::disconnectGPS(void)
{
    if (_rtcmMavlink) {
        delete(_rtcmMavlink);
    }

    if (_rtkProvider) {
        _requestRTKStop = true;
        //Note that we need a relatively high timeout to be sure the GPS thread finished.
        if (!_rtkProvider->wait(2000)) {
            qWarning() << "Failed to wait for GPS thread exit. Consider increasing the timeout";
        }
        delete(_rtkProvider);
    }

    _rtcmMavlink = nullptr;
    _rtkProvider = nullptr;
}

void NTRIP::GPSPositionUpdate(GPSPositionMessage msg)
{
    qCDebug(RTKGPSLog) << QString("GPS: got position update: alt=%1, long=%2, lat=%3").arg(msg.position_data.alt).arg(msg.position_data.lon).arg(msg.position_data.lat);
}

void NTRIP::GPSSatelliteUpdate(GPSSatelliteMessage msg)
{
    qCDebug(RTKGPSLog) << QString("GPS: got satellite info update, %1 satellites").arg((int)msg.satellite_data.count);
    emit satelliteUpdate(msg.satellite_data.count);
}
