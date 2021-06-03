/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/
#include "QGCApplication.h"
#include "FlightHubManager.h"
#include "QJsonObject"
#include "QJsonDocument"
//#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(FlightHubManagerLog, "FlightHubManagerLog")

FlightHubManager::FlightHubManager(Vehicle* vehicle)
    :_vehicle(vehicle)
{
    qCDebug(FlightHubManagerLog) << "Instatiating FlightHubManager";

    //connect(_vehicle, &Vehicle::mavlinkMessageReceived, this, &FlightHubManager::_mavlinkMessageReceived);
    //connect(qgcApp()->toolbox()->multiVehicleManager(), &MultiVehicleManager::parameterReadyVehicleAvailableChanged, this, &FlightHubManager::_vehicleReady);
}

FlightHubManager::~FlightHubManager()
{
    if (_flightHubMQtt) {
        mqttclientThread.quit();
        //Note that we need a relatively high timeout to be sure the GPS thread finished.
        if (!mqttclientThread.wait(2000)) {
            qWarning() << "Failed to wait for GPS thread exit. Consider increasing the timeout";
        }
        delete(_flightHubMQtt);
        _flightHubMQtt = nullptr;
    }
}

//-----------------------------------------------------------------------------
void
FlightHubManager::_vehicleReady(bool ready)
{
    qCDebug(FlightHubManagerLog) << "_vehicleReady(" << ready << ")";
    if(ready) {
        if(qgcApp()->toolbox()->multiVehicleManager()->activeVehicle() == _vehicle) {
            _flightHubMQtt = new FlightHubMqtt(this);
            _flightHubMQtt->initParams("13.250.118.139", 1883, "QQYYJ4Qa47uz5XtnlFav", "QQYYJ4Qa47uz5XtnlFav");
            connect(this, &FlightHubManager::publishMsg, _flightHubMQtt, &FlightHubMqtt::publishMsg);

            _flightHubMQtt->moveToThread(&mqttclientThread);
            connect(&mqttclientThread, &QThread::started, _flightHubMQtt, &FlightHubMqtt::init);

            mqttclientThread.start();
        }
    }
}

////-----------------------------------------------------------------------------
void
FlightHubManager::_mavlinkMessageReceived(const mavlink_message_t& message)
{
    //-- Only pay attention to camera components, as identified by their compId
    if(message.sysid == _vehicle->id()) {
        QString telemetry;

        switch (message.msgid) {
        case MAVLINK_MSG_ID_ALTITUDE:
            telemetry = _handleAltitude(message);
            break;
        case MAVLINK_MSG_ID_GPS_RAW_INT:
            telemetry = QString(_handleGpsRawInt(message));
            break;
        case MAVLINK_MSG_ID_STORAGE_INFORMATION:
            //_handleStorageInfo(message);
            break;
        case MAVLINK_MSG_ID_HEARTBEAT:
            //_handleHeartbeat(message);
            break;
        case MAVLINK_MSG_ID_CAMERA_INFORMATION:
            //_handleCameraInfo(message);
            break;
        case MAVLINK_MSG_ID_CAMERA_SETTINGS:
            //_handleCameraSettings(message);
            break;
        case MAVLINK_MSG_ID_PARAM_EXT_ACK:
            //_handleParamAck(message);
            break;
        case MAVLINK_MSG_ID_PARAM_EXT_VALUE:
            //_handleParamValue(message);
            break;
        case MAVLINK_MSG_ID_VIDEO_STREAM_INFORMATION:
            //_handleVideoStreamInfo(message);
            break;
        case MAVLINK_MSG_ID_VIDEO_STREAM_STATUS:
            //_handleVideoStreamStatus(message);
            break;
        case MAVLINK_MSG_ID_BATTERY_STATUS:
            telemetry = QString(_handleBatteryStatus(message));
            break;
        }
        //telemetry = "{\"longitude\":106.6137614, \"latitude\":10.6137614}";
        if (!telemetry.isEmpty()) {
            qWarning() << telemetry;
            emit publishMsg(telemetry.toUtf8());
        }
    }
}

QString FlightHubManager::_handleAltitude(const mavlink_message_t& message)
{
    mavlink_altitude_t altitude;
    mavlink_msg_altitude_decode(&message, &altitude);

    QJsonObject jsonObj;
    jsonObj.insert("altitude_amsl", altitude.altitude_amsl);

    return QString(QJsonDocument(jsonObj).toJson(QJsonDocument::Compact));
    // Data from ALTITUDE message takes precedence over gps messages
    //_altitudeRelativeFact.setRawValue(altitude.altitude_relative);
//    _altitudeAMSLFact.setRawValue(altitude.altitude_amsl);
}

QString FlightHubManager::_handleGpsRawInt(const mavlink_message_t& message)
{
    mavlink_gps_raw_int_t gpsRawInt;
    mavlink_msg_gps_raw_int_decode(&message, &gpsRawInt);
    QGeoCoordinate newPosition(gpsRawInt.lat  / (double)1E7, gpsRawInt.lon / (double)1E7, gpsRawInt.alt  / 1000.0);

    QJsonObject jsonObj;
    jsonObj.insert("longitude", newPosition.longitude());
    jsonObj.insert("latitude", newPosition.latitude());
    return QString(QJsonDocument(jsonObj).toJson(QJsonDocument::Compact));

    //_gpsRawIntMessageAvailable = true;

//    if (gpsRawInt.fix_type >= GPS_FIX_TYPE_3D_FIX) {
//        //if (!_globalPositionIntMessageAvailable) {
//            QGeoCoordinate newPosition(gpsRawInt.lat  / (double)1E7, gpsRawInt.lon / (double)1E7, gpsRawInt.alt  / 1000.0);
//            if (newPosition != _coordinate) {
//                _coordinate = newPosition;
//                emit coordinateChanged(_coordinate);
//            }
//            if (!_altitudeMessageAvailable) {
//                _altitudeAMSLFact.setRawValue(gpsRawInt.alt / 1000.0);
//            }
//        //}
//    }
}

QString FlightHubManager::_handleBatteryStatus(const mavlink_message_t& message)
{
    mavlink_battery_status_t batteryStatus;
    mavlink_msg_battery_status_decode(&message, &batteryStatus);

    QJsonObject jsonObj;
    if (batteryStatus.id == 0) {
        jsonObj.insert("battery_pct", batteryStatus.battery_remaining);
    }
    return QString(QJsonDocument(jsonObj).toJson(QJsonDocument::Compact));
}
