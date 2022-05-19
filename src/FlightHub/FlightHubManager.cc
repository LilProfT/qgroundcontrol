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
#include "VehicleBatteryFactGroup.h"
#include "TrajectoryPoints.h"

QGC_LOGGING_CATEGORY(FlightHubManagerLog, "FlightHubManagerLog")

FlightHubManager::FlightHubManager(QGCApplication *app, QGCToolbox *toolbox)
    : QGCTool(app, toolbox), _uploadOfflineManager(new QNetworkAccessManager(this))
{
}

void FlightHubManager::setToolbox(QGCToolbox *toolbox)
{
    QGCTool::setToolbox(toolbox);
    qCWarning(FlightHubManagerLog) << "Instatiating FlightHubManager";

    connect(qgcApp()->toolbox()->multiVehicleManager(), &MultiVehicleManager::parameterReadyVehicleAvailableChanged, this, &FlightHubManager::_onVehicleReady);

    connect(_uploadOfflineManager, &QNetworkAccessManager::finished, this, &FlightHubManager::_uploadOfflineFinished);
    startUploadOfflineStatTimer(0);
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
void FlightHubManager::_onVehicleReady(bool isReady)
{
    if (isReady)
    {

        if (qgcApp()->toolbox()->multiVehicleManager()->activeVehicle() != _vehicle)
        {
            _vehicle = qgcApp()->toolbox()->multiVehicleManager()->activeVehicle();
            startConnectFlightHubTimer(0);
            _oldAreaValue = _vehicle->areaSprayed()->rawValue().toDouble();
        }
    }
}
void FlightHubManager::_onVehicleMissionCompleted()
{
    auto sprayedIndexes = _vehicle->trajectoryPoints()->sprayedIndexes();
    auto selectedItems =  _vehicle->trajectoryPoints()->list();

    qCWarning(FlightHubManagerLog) << "mission completed -----------------------" << _oldAreaValue;
    qCWarning(FlightHubManagerLog) << "mission completed -----------------------" << sprayedIndexes;



    auto flightTime = _vehicle->flightTime()->rawValue().toDouble();
    flightTime = flightTime / 3600;

    QJsonObject obj;
    obj["flightDuration"] = flightTime;
    obj["flights"] = 1;
    obj["taskArea"] = _vehicle->areaSprayed()->rawValue().toDouble() - _oldAreaValue;
    QJsonArray flywayPoints;

    foreach(QVariant item , selectedItems){
        QGeoCoordinate coor = qvariant_cast<QGeoCoordinate>(item);
        QJsonObject value;
        value["longitude"] = coor.longitude();
        value["latitude"] = coor.latitude();
        flywayPoints.append(value);
    }
    QJsonArray sprayedIndexesJsonArray;
    foreach(auto index , sprayedIndexes){
        sprayedIndexesJsonArray.append(index);
    }
    obj["sprayedIndexes"] = sprayedIndexesJsonArray;
    obj["flywayPoints"] = flywayPoints;
    obj["taskLocation"] = qgcApp()->toolbox()->settingsManager()->flightHubSettings()->flightHubLocation()->rawValueString();
    obj["fieldName"] = qgcApp()->toolbox()->settingsManager()->flightHubSettings()->flightHubLocation()->rawValueString();

    qCWarning(FlightHubManagerLog) << "publish stat";
    emit publishStat(obj);

     _oldAreaValue =  _vehicle->areaSprayed()->rawValue().toDouble();
     _vehicle->trajectoryPoints()->clearSprayedPointsData();
}

void FlightHubManager::_onClientReady(bool isReady)
{
    if (isReady)
    {
        _clientReady = true;
        qgcApp()->showAppMessage("Connected", "Flighthub");
        qCWarning(FlightHubManagerLog) << "Client ready";
        connect(_vehicle, &Vehicle::coordinateChanged, this, &FlightHubManager::_onVehicleCoordinatedChanged);
        connect(_vehicle, &Vehicle::missionCompleted, this, &FlightHubManager::_onVehicleMissionCompleted);
        connect(this, &FlightHubManager::publishTelemetry, _flightHubHttpClient, &FlightHubHttpClient::publishTelemetry);
        connect(this, &FlightHubManager::publishStat, _flightHubHttpClient, &FlightHubHttpClient::publishStat);
        startTimer(5000);
    }
    else
    {
        startConnectFlightHubTimer(10000);
    }
}

void FlightHubManager::startTimer(int interval)
{
    QTimer::singleShot(interval, this, &FlightHubManager::timerSlot);
}

void FlightHubManager::startUploadOfflineStatTimer(int interval)
{
    QTimer::singleShot(interval, this, &FlightHubManager::uploadOfflineStatTimerSlot);
}

void FlightHubManager::startConnectFlightHubTimer(int interval)
{
    QTimer::singleShot(interval, this, &FlightHubManager::connectFlightHubTimerSlot);
}

void FlightHubManager::connectFlightHubTimerSlot()
{
    qCWarning(FlightHubManagerLog) << "init client";
    _clientReady = false;
    FlightHubSettings *flightHubSettings = qgcApp()->toolbox()->settingsManager()->flightHubSettings();
    delete _flightHubHttpClient;
    _flightHubHttpClient = nullptr;
    _flightHubHttpClient = new FlightHubHttpClient(nullptr);
    _flightHubHttpClient->setParams(flightHubSettings->flightHubServerHostAddress()->rawValueString(),
                                    flightHubSettings->flightHubDeviceToken()->rawValueString(),
                                    flightHubSettings->authServerHostAddress()->rawValueString(),
                                    flightHubSettings->flightHubUserName()->rawValueString(),
                                    flightHubSettings->flightHubPasswd()->rawValueString(),
                                    flightHubSettings->deviceCode()->rawValueString());
    _flightHubHttpClient->moveToThread(&_clientThread);
    connect(&_clientThread, &QThread::started, _flightHubHttpClient, &FlightHubHttpClient::init);

    connect(_flightHubHttpClient, &FlightHubHttpClient::parameterReadyClientAvailableChanged, this, &FlightHubManager::_onClientReady);

    // connect(this, &FlightHubManager::publishMsg, _flightHubMQtt, &FlightHubMqtt::publishMsg);

    _clientThread.start();
}
void FlightHubManager::uploadOfflineStatTimerSlot()
{
    auto folderPath = qgcApp()->toolbox()->settingsManager()->appSettings()->resumeSavePath();
    QDir dir(folderPath);
    dir.cdUp();
    auto filePath = dir.filePath("offline_stats.json");
    QFile readFile(filePath);
    QString text = "[]";
    if (readFile.exists())
    {
        if (readFile.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            text = readFile.readAll();
        }
    }

    QJsonDocument readDoc = QJsonDocument::fromJson(text.toUtf8());
    QJsonArray array = readDoc.array();
    if (array.isEmpty())
    {

        startUploadOfflineStatTimer(10000);
        return;
    }
    qWarning() << "upload offline";
    QJsonObject uploadObj;
    uploadObj["data"] = array;

    QJsonDocument uploadDoc;
    uploadDoc.setObject(uploadObj);
    QNetworkRequest request;

    QString domain = qgcApp()->toolbox()->settingsManager()->flightHubSettings()->flightHubServerHostAddress()->rawValueString();
    auto url = domain + "/devices/uploadofflinestats";
    request.setUrl(QUrl(url));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QByteArray data = uploadDoc.toJson();
    _uploadOfflineManager->post(request, data);

    startUploadOfflineStatTimer(10000);
}

void FlightHubManager::_uploadOfflineFinished(QNetworkReply *reply)
{
    auto response = reply->readAll();
    qCWarning(FlightHubManagerLog) << "reply offline stat" << response;
    if (!reply->error())
    {

        auto folderPath = qgcApp()->toolbox()->settingsManager()->appSettings()->resumeSavePath();
        QDir dir(folderPath);
        dir.cdUp();
        auto filePath = dir.filePath("offline_stats.json");
        QFile writeFile(filePath);

        if (writeFile.open(QIODevice::WriteOnly))
        {
            QJsonArray array;
            QJsonDocument writeDoc;
            writeDoc.setArray(array);
            QTextStream stream(&writeFile);

            qCWarning(FlightHubHttpClientLog) << "Writing file" << writeDoc.toJson();
            stream << writeDoc.toJson();
        }
    }
}

void FlightHubManager::timerSlot()
{

    if (!_positionArray.isEmpty())
    {
        QJsonObject dataObj;
        dataObj.insert("data", _positionArray);
        dataObj.insert("batteryLogs", _batteryArray);

        emit publishTelemetry(dataObj);
        _positionArray = QJsonArray();
        _batteryArray = QJsonArray();
    }
    else
    {

    }

    startTimer(5000);
}

////-----------------------------------------------------------------------------
void FlightHubManager::_onVehicleCoordinatedChanged(const QGeoCoordinate &coordinate)
{
    //-- Only pay attention to camera components, as identified by their compId
    //        qCWarning(FlightHubManagerLog) << "Mavlink received-" << _vehicle->coordinate().longitude()
    //                                       << _vehicle->coordinate().latitude()
    //                                       << _vehicle->heading()->rawValue().toDouble()
    //                                       << _vehicle->vehicleUIDStr();
    QJsonObject newObj = QJsonObject();
    newObj.insert("longitude", _vehicle->coordinate().longitude());
    newObj.insert("latitude", _vehicle->coordinate().latitude());
    newObj.insert("direction", _vehicle->heading()->rawValue().toDouble());

    QJsonObject additionalInformationObj;
    additionalInformationObj.insert("airspeed", _vehicle->airSpeed()->rawValue().toDouble());
    additionalInformationObj.insert("groundspeed", _vehicle->groundSpeed()->rawValue().toDouble());
    additionalInformationObj.insert("climbRate", _vehicle->climbRate()->rawValue().toDouble());

    newObj.insert("additionalInformation", additionalInformationObj);
    _positionArray.append(newObj);
    QmlObjectListModel *batteries = _vehicle->batteries();
    for (int i = 0; i < batteries->count(); i++)
    {
        VehicleBatteryFactGroup *group = batteries->value<VehicleBatteryFactGroup *>(i);
        QString serialNumber = group->serialNumber()->rawValueString();
        if (serialNumber != "0")
        {
            QJsonObject batteryObj;
            batteryObj["actualID"] = serialNumber;
            batteryObj["cycleCount"] = group->cycleCount()->rawValueString();
            batteryObj["percentRemaining"] = group->percentRemaining()->rawValueString();
            // batteryObj["temperature"] = group->temperature()->rawValueString();
            // batteryObj["temperatureUnit"] = group->cycleCount()->rawValueString();
            batteryObj["cellMinimumVoltage"] = group->cellVoltageMin()->rawValueString();
            batteryObj["cellMinimumVoltageUnit"] = "V";
            batteryObj["cellMaximumVoltage"] = group->cellVoltageMax()->rawValueString();
            batteryObj["cellMaximumVoltageUnit"] = "V";
            batteryObj["current"] = group->current()->rawValueString();
            batteryObj["currentUnit"] = "A";

            _batteryArray.append(batteryObj);

        }
        else
        {
            QJsonObject batteryObj;
            batteryObj["actualID"] = "TestBattery";
            batteryObj["cycleCount"] = group->cycleCount()->rawValueString();
            batteryObj["percentRemaining"] = group->percentRemaining()->rawValueString();
            // batteryObj["temperature"] = group->temperature()->rawValueString();
            // batteryObj["temperatureUnit"] = group->cycleCount()->rawValueString();
            batteryObj["cellMinimumVoltage"] = group->cellVoltageMin()->rawValueString();
            batteryObj["cellMinimumVoltageUnit"] = "V";
            batteryObj["cellMaximumVoltage"] = group->cellVoltageMax()->rawValueString();
            batteryObj["cellMaximumVoltageUnit"] = "V";
            batteryObj["current"] = group->current()->rawValueString();
            batteryObj["currentUnit"] = "A";

            _batteryArray.append(batteryObj);

        }
    }
}
