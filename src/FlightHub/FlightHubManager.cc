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
            startTimer(5000);
        }
    }
}

void FlightHubManager::startTimer(int interval)
{
    QTimer::singleShot(interval, this, &FlightHubManager::timerSlot);
}

void FlightHubManager::timerSlot()
{
    qCWarning(FlightHubManagerLog) << "timerSlot";
    if (!jsonObj.isEmpty()) {
        QString telemetry;
        telemetry = QString(QJsonDocument(jsonObj).toJson(QJsonDocument::Compact));
        if (!telemetry.isEmpty()) {
            jsonObj = QJsonObject();
            qWarning() << telemetry;
            emit publishMsg(telemetry.toUtf8());
        }
    }

    startTimer(5000);
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
            //telemetry = _handleAltitude(message);
            break;
        case MAVLINK_MSG_ID_GPS_RAW_INT:
            //telemetry = QString(_handleGpsRawInt(message));
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
        case MAVLINK_MSG_ID_HIGH_LATENCY:
            telemetry = _handleHighLatency(message);
            break;
        case MAVLINK_MSG_ID_HIGH_LATENCY2:
            telemetry = _handleHighLatency2(message);
            break;
        }
//        //telemetry = "{\"longitude\":106.6137614, \"latitude\":10.6137614}";
//        if (!telemetry.isEmpty()) {
//            qWarning() << telemetry;
//            emit publishMsg(telemetry.toUtf8());
//        }
    }
}

QString FlightHubManager::_handleAltitude(const mavlink_message_t& message)
{
    mavlink_altitude_t altitude;
    mavlink_msg_altitude_decode(&message, &altitude);

    jsonObj.insert("altitude_amsl", altitude.altitude_amsl);

    return "";
}

QString FlightHubManager::_handleGpsRawInt(const mavlink_message_t& message)
{
    mavlink_gps_raw_int_t gpsRawInt;
    mavlink_msg_gps_raw_int_decode(&message, &gpsRawInt);
    QGeoCoordinate newPosition(gpsRawInt.lat  / (double)1E7, gpsRawInt.lon / (double)1E7, gpsRawInt.alt  / 1000.0);

    jsonObj.insert("longitude", newPosition.longitude());
    jsonObj.insert("latitude", newPosition.latitude());

    return "";
}

QString FlightHubManager::_handleBatteryStatus(const mavlink_message_t& message)
{
    mavlink_battery_status_t batteryStatus;
    mavlink_msg_battery_status_decode(&message, &batteryStatus);

    if (batteryStatus.id == 0) {
        jsonObj.insert("battery_pct", batteryStatus.battery_remaining);
    }

    return "";
}

QString FlightHubManager::_handleHighLatency(const mavlink_message_t& message)
{
    mavlink_high_latency_t highLatency;
    mavlink_msg_high_latency_decode(&message, &highLatency);

    struct {
        const double latitude;
        const double longitude;
        const double altitude;
    } coordinate {
        highLatency.latitude  / (double)1E7,
                highLatency.longitude  / (double)1E7,
                static_cast<double>(highLatency.altitude_amsl)
    };

    QGeoCoordinate newPosition(coordinate.latitude, coordinate.longitude, coordinate.altitude);

    jsonObj.insert("longitude", newPosition.longitude());
    jsonObj.insert("latitude", newPosition.latitude());
    jsonObj.insert("altitude", newPosition.latitude());
    jsonObj.insert("airspeed", (double)highLatency.airspeed / 5.0);
    jsonObj.insert("groundspeed", (double)highLatency.groundspeed / 5.0);
    jsonObj.insert("climb_rate", (double)highLatency.climb_rate / 10.0);
    jsonObj.insert("heading", (double)highLatency.heading * 2.0);
    return "";
}

QString FlightHubManager::_handleHighLatency2(const mavlink_message_t& message)
{
    mavlink_high_latency2_t highLatency2;
    mavlink_msg_high_latency2_decode(&message, &highLatency2);

    jsonObj.insert("longitude", highLatency2.latitude  / (double)1E7);
    jsonObj.insert("latitude", highLatency2.longitude / (double)1E7);
    jsonObj.insert("altitude", highLatency2.altitude);
    jsonObj.insert("airspeed", (double)highLatency2.airspeed / 5.0);
    jsonObj.insert("groundspeed", (double)highLatency2.groundspeed / 5.0);
    jsonObj.insert("climb_rate", (double)highLatency2.climb_rate / 10.0);
    jsonObj.insert("heading", (double)highLatency2.heading * 2.0);
    return "";
}
