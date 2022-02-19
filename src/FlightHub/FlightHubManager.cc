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
#include "FlightHubSettings.h"
#include "SettingsManager.h"

QGC_LOGGING_CATEGORY(FlightHubManagerLog, "FlightHubManagerLog")

FlightHubManager::FlightHubManager(QGCApplication *app, QGCToolbox *toolbox)
    : QGCTool(app, toolbox)
{
}

void FlightHubManager::setToolbox(QGCToolbox *toolbox)
{
    QGCTool::setToolbox(toolbox);
    qCWarning(FlightHubManagerLog) << "Instatiating FlightHubManager";

    connect(qgcApp()->toolbox()->multiVehicleManager(), &MultiVehicleManager::parameterReadyVehicleAvailableChanged, this, &FlightHubManager::_vehicleReady);
}

FlightHubManager::~FlightHubManager()
{
    if (_flightHubHttpClient)
    {
        _clientThread.quit();
        // Note that we need a relatively high timeout to be sure the GPS thread finished.
        if (!_clientThread.wait(2000))
        {
            qWarning() << "Failed to wait for GPS thread exit. Consider increasing the timeout";
        }
        delete (_flightHubHttpClient);
        _flightHubHttpClient = nullptr;
    }
}

//-----------------------------------------------------------------------------
void FlightHubManager::_vehicleReady(bool isReady)
{
    qCWarning(FlightHubManagerLog) << "_vehicleReady(" << isReady << ")";
    if (isReady)
    {
        if (qgcApp()->toolbox()->multiVehicleManager()->activeVehicle() != _vehicle)
        {
            FlightHubSettings *flightHubSettings = qgcApp()->toolbox()->settingsManager()->flightHubSettings();
            _flightHubHttpClient = new FlightHubHttpClient(nullptr);
            _flightHubHttpClient->setParams(flightHubSettings->flightHubServerHostAddress()->rawValueString(), flightHubSettings->flightHubDeviceToken()->rawValueString());
            _flightHubHttpClient->moveToThread(&_clientThread);
            connect(&_clientThread, &QThread::started, _flightHubHttpClient, &FlightHubHttpClient::init);

            connect(_flightHubHttpClient, &FlightHubHttpClient::parameterReadyClientAvailableChanged, this, &FlightHubManager::_clientReady);

            // connect(this, &FlightHubManager::publishMsg, _flightHubMQtt, &FlightHubMqtt::publishMsg);

            _clientThread.start();
            startTimer(5000);
        }
    }
}

void FlightHubManager::_clientReady(bool isReady)
{
    if (isReady)
    {
        _vehicle = qgcApp()->toolbox()->multiVehicleManager()->activeVehicle();
        connect(_vehicle, &Vehicle::coordinateChanged, this, &FlightHubManager::_vehicleCoordinatedChanged);
        connect(this,&FlightHubManager::publishTelemetry, _flightHubHttpClient , &FlightHubHttpClient::publishTelemetry);
    }
}

void FlightHubManager::startTimer(int interval)
{
    QTimer::singleShot(interval, this, &FlightHubManager::timerSlot);
}

void FlightHubManager::timerSlot()
{



    if (!_positionArray.isEmpty())
    {
        QJsonObject dataObj;
        dataObj.insert("data",_positionArray);
        QJsonDocument doc;
        doc.setObject(dataObj);


        qCWarning(FlightHubManagerLog) << "count" << _positionArray.count();
        emit publishTelemetry(doc);
        _positionArray = QJsonArray();

    }
    else
    {
        qCWarning(FlightHubManagerLog) << "timerSlot"
                                       << "empty";
    }

    startTimer(5000);
}

////-----------------------------------------------------------------------------
void FlightHubManager::_vehicleCoordinatedChanged(const QGeoCoordinate &coordinate)
{
    //-- Only pay attention to camera components, as identified by their compId
//    qCWarning(FlightHubManagerLog) << "Mavlink received-" << _vehicle->coordinate().longitude()
//                                   << _vehicle->coordinate().latitude()
//                                   << _vehicle->heading()->rawValue().toDouble()
//                                   << _vehicle->vehicleUIDStr();
    QJsonObject newObj = QJsonObject();
    newObj.insert("longitude", _vehicle->coordinate().longitude());
    newObj.insert("latitude", _vehicle->coordinate().latitude());
    newObj.insert("direction",_vehicle->heading()->rawValue().toDouble());
    newObj.insert("airspeed", _vehicle->airSpeed()->rawValue().toDouble());
    newObj.insert("groundspeed",  _vehicle->groundSpeed()->rawValue().toDouble());
    newObj.insert("climbRate", _vehicle->climbRate()->rawValue().toDouble());
    _positionArray.append(newObj);
    //        //telemetry = "{\"longitude\":106.6137614, \"latitude\":10.6137614}";
    //        if (!telemetry.isEmpty()) {
    //            qWarning() << telemetry;
    //            emit publishMsg(telemetry.toUtf8());
    //        }
}

QString FlightHubManager::_handleAltitude(const mavlink_message_t &message)
{
    mavlink_altitude_t altitude;
    mavlink_msg_altitude_decode(&message, &altitude);
    QJsonObject jsonObj = QJsonObject();

    jsonObj.insert("altitude_amsl", altitude.altitude_amsl);

    return "";
}

QString FlightHubManager::_handleGpsRawInt(const mavlink_message_t &message)
{
    mavlink_gps_raw_int_t gpsRawInt;
    mavlink_msg_gps_raw_int_decode(&message, &gpsRawInt);
    QGeoCoordinate newPosition(gpsRawInt.lat / (double)1E7, gpsRawInt.lon / (double)1E7, gpsRawInt.alt / 1000.0);
    QJsonObject jsonObj = QJsonObject();

    jsonObj.insert("longitude", newPosition.longitude());
    jsonObj.insert("latitude", newPosition.latitude());

    return "";
}

QString FlightHubManager::_handleBatteryStatus(const mavlink_message_t &message)
{
    mavlink_battery_status_t batteryStatus;
    mavlink_msg_battery_status_decode(&message, &batteryStatus);
    QJsonObject jsonObj = QJsonObject();

    if (batteryStatus.id == 0)
    {
        jsonObj.insert("battery_pct", batteryStatus.battery_remaining);
    }

    return "";
}

void FlightHubManager::_handleHighLatency(const mavlink_message_t &message)
{
    QJsonObject newObj = QJsonObject();

    mavlink_high_latency_t highLatency;
    mavlink_msg_high_latency_decode(&message, &highLatency);

    struct
    {
        const double latitude;
        const double longitude;
        const double altitude;
    } coordinate{
        highLatency.latitude / (double)1E7,
                highLatency.longitude / (double)1E7,
                static_cast<double>(highLatency.altitude_amsl)};

    QGeoCoordinate newPosition(coordinate.latitude, coordinate.longitude, coordinate.altitude);

    newObj.insert("longitude", (double)newPosition.longitude());
    newObj.insert("latitude", (double)newPosition.latitude());
    newObj.insert("altitude", (double)newPosition.latitude());
    newObj.insert("airspeed", (double)highLatency.airspeed);
    newObj.insert("groundspeed", (double)highLatency.groundspeed);
    newObj.insert("climb_rate", (double)highLatency.climb_rate);
    newObj.insert("heading", (double)highLatency.heading);
    qCWarning(FlightHubManagerLog) << "latency"
                                   << "lng" << newPosition.longitude() << "heading" << (double)highLatency.heading;
    _positionArray.append(newObj);
}

void FlightHubManager::_handleHighLatency2(const mavlink_message_t &message)
{
    QJsonObject newObj = QJsonObject();
    mavlink_high_latency2_t highLatency2;
    mavlink_msg_high_latency2_decode(&message, &highLatency2);

    newObj.insert("longitude", (double)highLatency2.longitude);
    newObj.insert("latitude", (double)highLatency2.latitude);
    newObj.insert("altitude", (double)highLatency2.altitude);
    newObj.insert("airspeed", (double)highLatency2.airspeed);
    newObj.insert("groundspeed", (double)highLatency2.groundspeed);
    newObj.insert("climb_rate", (double)highLatency2.climb_rate);
    newObj.insert("heading", (double)highLatency2.heading);
    qCWarning(FlightHubManagerLog) << "latency2"
                                   << "lng" << (double)highLatency2.longitude << "heading" << (double)highLatency2.heading;

    _positionArray.append(newObj);
}
