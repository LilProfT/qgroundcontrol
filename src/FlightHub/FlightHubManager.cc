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

QString FlightHubManager::_handleHighLatency(const mavlink_message_t& message)
{
    mavlink_high_latency_t highLatency;
    mavlink_msg_high_latency_decode(&message, &highLatency);

//    QString previousFlightMode;
//    if (_base_mode != 0 || _custom_mode != 0){
//        // Vehicle is initialized with _base_mode=0 and _custom_mode=0. Don't pass this to flightMode() since it will complain about
//        // bad modes while unit testing.
//        previousFlightMode = flightMode();
//    }
//    _base_mode = MAV_MODE_FLAG_CUSTOM_MODE_ENABLED;
//    _custom_mode = _firmwarePlugin->highLatencyCustomModeTo32Bits(highLatency.custom_mode);
//    if (previousFlightMode != flightMode()) {
//        emit flightModeChanged(flightMode());
//    }

//    // Assume armed since we don't know
//    if (_armed != true) {
//        _armed = true;
//        emit armedChanged(_armed);
//    }

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

    QJsonObject jsonObj;
    jsonObj.insert("longitude", newPosition.longitude());
    jsonObj.insert("latitude", newPosition.latitude());
    jsonObj.insert("altitude", newPosition.latitude());
    jsonObj.insert("airspeed", (double)highLatency.airspeed / 5.0);
    jsonObj.insert("groundspeed", (double)highLatency.groundspeed / 5.0);
    jsonObj.insert("climb_rate", (double)highLatency.climb_rate / 10.0);
    jsonObj.insert("heading", (double)highLatency.heading * 2.0);

    return QString(QJsonDocument(jsonObj).toJson(QJsonDocument::Compact));

//    emit coordinateChanged(_coordinate);

//    _airSpeedFact.setRawValue((double)highLatency.airspeed / 5.0);
//    _groundSpeedFact.setRawValue((double)highLatency.groundspeed / 5.0);
//    _climbRateFact.setRawValue((double)highLatency.climb_rate / 10.0);
//    _headingFact.setRawValue((double)highLatency.heading * 2.0);
//    _altitudeRelativeFact.setRawValue(qQNaN());
//    _altitudeAMSLFact.setRawValue(coordinate.altitude);
}

QString FlightHubManager::_handleHighLatency2(const mavlink_message_t& message)
{
    mavlink_high_latency2_t highLatency2;
    mavlink_msg_high_latency2_decode(&message, &highLatency2);

//    QString previousFlightMode;
//    if (_base_mode != 0 || _custom_mode != 0){
//        // Vehicle is initialized with _base_mode=0 and _custom_mode=0. Don't pass this to flightMode() since it will complain about
//        // bad modes while unit testing.
//        previousFlightMode = flightMode();
//    }
//    _base_mode = MAV_MODE_FLAG_CUSTOM_MODE_ENABLED;
//    _custom_mode = _firmwarePlugin->highLatencyCustomModeTo32Bits(highLatency2.custom_mode);
//    if (previousFlightMode != flightMode()) {
//        emit flightModeChanged(flightMode());
//    }

    // Assume armed since we don't know
//    if (_armed != true) {
//        _armed = true;
//        emit armedChanged(_armed);
//    }

    QJsonObject jsonObj;
    jsonObj.insert("longitude", highLatency2.latitude  / (double)1E7);
    jsonObj.insert("latitude", highLatency2.longitude / (double)1E7);
    jsonObj.insert("altitude", highLatency2.altitude);
    jsonObj.insert("airspeed", (double)highLatency2.airspeed / 5.0);
    jsonObj.insert("groundspeed", (double)highLatency2.groundspeed / 5.0);
    jsonObj.insert("climb_rate", (double)highLatency2.climb_rate / 10.0);
    jsonObj.insert("heading", (double)highLatency2.heading * 2.0);

    return QString(QJsonDocument(jsonObj).toJson(QJsonDocument::Compact));

//    _coordinate.setLatitude(highLatency2.latitude  / (double)1E7);
//    _coordinate.setLongitude(highLatency2.longitude / (double)1E7);
//    _coordinate.setAltitude(highLatency2.altitude);
//    emit coordinateChanged(_coordinate);

//    _airSpeedFact.setRawValue((double)highLatency2.airspeed / 5.0);
//    _groundSpeedFact.setRawValue((double)highLatency2.groundspeed / 5.0);
//    _climbRateFact.setRawValue((double)highLatency2.climb_rate / 10.0);
//    _headingFact.setRawValue((double)highLatency2.heading * 2.0);
//    _altitudeRelativeFact.setRawValue(qQNaN());
//    _altitudeAMSLFact.setRawValue(highLatency2.altitude);

//    struct failure2Sensor_s {
//        HL_FAILURE_FLAG         failureBit;
//        MAV_SYS_STATUS_SENSOR   sensorBit;
//    };

//    static const failure2Sensor_s rgFailure2Sensor[] = {
//        { HL_FAILURE_FLAG_GPS,                      MAV_SYS_STATUS_SENSOR_GPS },
//        { HL_FAILURE_FLAG_DIFFERENTIAL_PRESSURE,    MAV_SYS_STATUS_SENSOR_DIFFERENTIAL_PRESSURE },
//        { HL_FAILURE_FLAG_ABSOLUTE_PRESSURE,        MAV_SYS_STATUS_SENSOR_ABSOLUTE_PRESSURE },
//        { HL_FAILURE_FLAG_3D_ACCEL,                 MAV_SYS_STATUS_SENSOR_3D_ACCEL },
//        { HL_FAILURE_FLAG_3D_GYRO,                  MAV_SYS_STATUS_SENSOR_3D_GYRO },
//        { HL_FAILURE_FLAG_3D_MAG,                   MAV_SYS_STATUS_SENSOR_3D_MAG },
//    };

//    // Map from MAV_FAILURE bits to standard SYS_STATUS message handling
//    uint32_t newOnboardControlSensorsEnabled = 0;
//    for (size_t i=0; i<sizeof(rgFailure2Sensor)/sizeof(failure2Sensor_s); i++) {
//        const failure2Sensor_s* pFailure2Sensor = &rgFailure2Sensor[i];
//        if (highLatency2.failure_flags & pFailure2Sensor->failureBit) {
//            // Assume if reporting as unhealthy that is it present and enabled
//            newOnboardControlSensorsEnabled |= pFailure2Sensor->sensorBit;
//        }
//    }
//    if (newOnboardControlSensorsEnabled != _onboardControlSensorsEnabled) {
//        _onboardControlSensorsEnabled = newOnboardControlSensorsEnabled;
//        _onboardControlSensorsPresent = newOnboardControlSensorsEnabled;
//        _onboardControlSensorsUnhealthy = 0;
//    }
}
