/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "PlanMasterController.h"
#include "QGCApplication.h"
#include "QGCCorePlugin.h"
#include "MultiVehicleManager.h"
#include "SettingsManager.h"
#include "AppSettings.h"
#include "JsonHelper.h"
#include "MissionManager.h"
#include "ParameterManager.h"
#include "MAVLinkProtocol.h"
#include "KMLPlanDomDocument.h"
#include "SurveyPlanCreator.h"
#include "StructureScanPlanCreator.h"
#include "CorridorScanPlanCreator.h"
#include "BlankPlanCreator.h"
#include "NTRIP.h"
#if defined(QGC_AIRMAP_ENABLED)
#include "AirspaceFlightPlanProvider.h"
#endif

#include <QDomDocument>
#include <QJsonDocument>
#include <QFileInfo>
#include <QtMath>

QGC_LOGGING_CATEGORY(PlanMasterControllerLog, "PlanMasterControllerLog")

const int   PlanMasterController::kPlanFileVersion =            1;
const char* PlanMasterController::kPlanFileType =               "Plan";
const char* PlanMasterController::kRecentFile =               "/recently.plan";

const char* PlanMasterController::kJsonMissionObjectKey =       "mission";
const char* PlanMasterController::kJsonGeoFenceObjectKey =      "geoFence";
const char* PlanMasterController::kJsonRallyPointsObjectKey =   "rallyPoints";
const char* PlanMasterController::kJsonRecentFileObjectKey =  "recentFile";
const char* PlanMasterController::kJsonSprayedAreaObjectKey =  "sprayedArea";

PlanMasterController::PlanMasterController(QObject* parent)
    : QObject               (parent)
    , _multiVehicleMgr      (qgcApp()->toolbox()->multiVehicleManager())
    , _controllerVehicle    (new Vehicle(Vehicle::MAV_AUTOPILOT_TRACK, Vehicle::MAV_TYPE_TRACK, qgcApp()->toolbox()->firmwarePluginManager(), this))
    , _managerVehicle       (_controllerVehicle)
    , _missionController    (this)
    , _geoFenceController   (this)
    , _rallyPointController (this)
{
    _commonInit();
}

#ifdef QT_DEBUG
PlanMasterController::PlanMasterController(MAV_AUTOPILOT firmwareType, MAV_TYPE vehicleType, QObject* parent)
    : QObject               (parent)
    , _multiVehicleMgr      (qgcApp()->toolbox()->multiVehicleManager())
    , _controllerVehicle    (new Vehicle(firmwareType, vehicleType, qgcApp()->toolbox()->firmwarePluginManager()))
    , _managerVehicle       (_controllerVehicle)
    , _missionController    (this)
    , _geoFenceController   (this)
    , _rallyPointController (this)
{
    _commonInit();
}
#endif

void PlanMasterController::_commonInit(void)
{
    connect(&_missionController,    &MissionController::dirtyChanged,               this, &PlanMasterController::dirtyChanged);
    connect(&_geoFenceController,   &GeoFenceController::dirtyChanged,              this, &PlanMasterController::dirtyChanged);
    connect(&_rallyPointController, &RallyPointController::dirtyChanged,            this, &PlanMasterController::dirtyChanged);

    connect(&_missionController,    &MissionController::containsItemsChanged,       this, &PlanMasterController::containsItemsChanged);
    connect(&_geoFenceController,   &GeoFenceController::containsItemsChanged,      this, &PlanMasterController::containsItemsChanged);
    connect(&_rallyPointController, &RallyPointController::containsItemsChanged,    this, &PlanMasterController::containsItemsChanged);

    connect(&_missionController,    &MissionController::syncInProgressChanged,      this, &PlanMasterController::syncInProgressChanged);
    connect(&_geoFenceController,   &GeoFenceController::syncInProgressChanged,     this, &PlanMasterController::syncInProgressChanged);
    connect(&_rallyPointController, &RallyPointController::syncInProgressChanged,   this, &PlanMasterController::syncInProgressChanged);


    // Offline vehicle can change firmware/vehicle type
    connect(_controllerVehicle,     &Vehicle::vehicleTypeChanged,                   this, &PlanMasterController::_updatePlanCreatorsList);
}


PlanMasterController::~PlanMasterController()
{

}

void PlanMasterController::start(void)
{
    _missionController.start    (_flyView);
    _geoFenceController.start   (_flyView);
    _rallyPointController.start (_flyView);

    _activeVehicleChanged(_multiVehicleMgr->activeVehicle());
    connect(_multiVehicleMgr, &MultiVehicleManager::activeVehicleChanged, this, &PlanMasterController::_activeVehicleChanged);

    _updatePlanCreatorsList();

#if defined(QGC_AIRMAP_ENABLED)
    //-- This assumes there is one single instance of PlanMasterController in edit mode.
    if(!_flyView) {
        // Wait for signal confirming AirMap client connection before starting flight planning
        //connect(qgcApp()->toolbox()->airspaceManager(), &AirspaceManager::connectStatusChanged, this, &PlanMasterController::_startFlightPlanning);
    }
#endif
}

void PlanMasterController::startStaticActiveVehicle(Vehicle* vehicle, bool deleteWhenSendCompleted)
{
    _flyView = true;
    _deleteWhenSendCompleted = deleteWhenSendCompleted;
    _missionController.start(_flyView);
    _geoFenceController.start(_flyView);
    _rallyPointController.start(_flyView);
    _activeVehicleChanged(vehicle);
}

void PlanMasterController::_activeVehicleChanged(Vehicle* activeVehicle)
{
    if (_managerVehicle == activeVehicle) {
        // We are already setup for this vehicle
        return;
    }

    qCDebug(PlanMasterControllerLog) << "_activeVehicleChanged" << activeVehicle;

    if (_managerVehicle) {
        // Disconnect old vehicle. Be careful of wildcarding disconnect too much since _managerVehicle may equal _controllerVehicle
        disconnect(_managerVehicle->missionManager(),       nullptr, this, nullptr);
        disconnect(_managerVehicle->geoFenceManager(),      nullptr, this, nullptr);
        disconnect(_managerVehicle->rallyPointManager(),    nullptr, this, nullptr);
    }

    bool newOffline = false;
    if (activeVehicle == nullptr) {
        // Since there is no longer an active vehicle we use the offline controller vehicle as the manager vehicle
        _managerVehicle = _controllerVehicle;
        newOffline = true;
    } else {
        newOffline = false;
        _managerVehicle = activeVehicle;

        // Update controllerVehicle to the currently connected vehicle
        AppSettings* appSettings = qgcApp()->toolbox()->settingsManager()->appSettings();
        appSettings->offlineEditingFirmwareClass()->setRawValue(QGCMAVLink::firmwareClass(_managerVehicle->firmwareType()));
        appSettings->offlineEditingVehicleClass()->setRawValue(QGCMAVLink::vehicleClass(_managerVehicle->vehicleType()));

        // We use these signals to sequence upload and download to the multiple controller/managers
        connect(_managerVehicle->missionManager(),      &MissionManager::newMissionItemsAvailable,  this, &PlanMasterController::_loadMissionComplete);
        connect(_managerVehicle->geoFenceManager(),     &GeoFenceManager::loadComplete,             this, &PlanMasterController::_loadGeoFenceComplete);
        connect(_managerVehicle->rallyPointManager(),   &RallyPointManager::loadComplete,           this, &PlanMasterController::_loadRallyPointsComplete);
        connect(_managerVehicle->missionManager(),      &MissionManager::sendComplete,              this, &PlanMasterController::_sendMissionComplete);
        connect(_managerVehicle->geoFenceManager(),     &GeoFenceManager::sendComplete,             this, &PlanMasterController::_sendGeoFenceComplete);
        connect(_managerVehicle->rallyPointManager(),   &RallyPointManager::sendComplete,           this, &PlanMasterController::_sendRallyPointsComplete);
        connect(_managerVehicle->missionManager(),      &MissionManager::autoSaved,              this, &PlanMasterController::_saveToCurrentOnResumeFolder);
        connect(_managerVehicle->missionManager(),      &MissionManager::deleteResumed,              this, &PlanMasterController::deleteFileInResume);

    }

    _offline = newOffline;
    emit offlineChanged(offline());
    emit managerVehicleChanged(_managerVehicle);

    if (_flyView) {
        // We are in the Fly View
        if (newOffline) {
            // No active vehicle, clear mission
            qCDebug(PlanMasterControllerLog) << "_activeVehicleChanged: Fly View - No active vehicle, clearing stale plan";
            removeAll();
        } else {
            // Fly view has changed to a new active vehicle, update to show correct mission
            qCDebug(PlanMasterControllerLog) << "_activeVehicleChanged: Fly View - New active vehicle, loading new plan from manager vehicle";
            _showPlanFromManagerVehicle();
        }
    } else {
        // We are in the Plan view.
        if (containsItems()) {
            // The plan view has a stale plan in it
            if (dirty()) {
                // Plan is dirty, the user must decide what to do in all cases
                qCDebug(PlanMasterControllerLog) << "_activeVehicleChanged: Plan View - Previous dirty plan exists, no new active vehicle, sending promptForPlanUsageOnVehicleChange signal";
                emit promptForPlanUsageOnVehicleChange();
            } else {
                // Plan is not dirty
                if (newOffline) {
                    // The active vehicle went away with no new active vehicle
                    qCDebug(PlanMasterControllerLog) << "_activeVehicleChanged: Plan View - Previous clean plan exists, no new active vehicle, clear stale plan";
                    removeAll();
                } else {
                    // We are transitioning from one active vehicle to another. Show the plan from the new vehicle.
                    qCDebug(PlanMasterControllerLog) << "_activeVehicleChanged: Plan View - Previous clean plan exists, new active vehicle, loading from new manager vehicle";
                    _showPlanFromManagerVehicle();
                }
            }
        } else {
            // There is no previous Plan in the view
            if (newOffline) {
                // Nothing special to do in this case
                qCDebug(PlanMasterControllerLog) << "_activeVehicleChanged: Plan View - No previous plan, no longer connected to vehicle, nothing to do";
            } else {
                // Just show the plan from the new vehicle
                qCDebug(PlanMasterControllerLog) << "_activeVehicleChanged: Plan View - No previous plan, new active vehicle, loading from new manager vehicle";
                _showPlanFromManagerVehicle();
            }
        }
    }

    // Vehicle changed so we need to signal everything
    emit containsItemsChanged(containsItems());
    emit syncInProgressChanged();
    emit dirtyChanged(dirty());

    _updatePlanCreatorsList();
}

void PlanMasterController::loadFromVehicle(void)
{
    WeakLinkInterfacePtr weakLink = _managerVehicle->vehicleLinkManager()->primaryLink();
    if (weakLink.expired()) {
        // Vehicle is shutting down
        return;
    } else {
        SharedLinkInterfacePtr sharedLink = weakLink.lock();
        if (sharedLink->linkConfiguration()->isHighLatency()) {
            qgcApp()->showAppMessage(tr("Download not supported on high latency links."));
            return;
        }
    }

    if (offline()) {
        qCWarning(PlanMasterControllerLog) << "PlanMasterController::loadFromVehicle called while offline";
    } else if (_flyView) {
        qCWarning(PlanMasterControllerLog) << "PlanMasterController::loadFromVehicle called from Fly view";
    } else if (syncInProgress()) {
        qCWarning(PlanMasterControllerLog) << "PlanMasterController::loadFromVehicle called while syncInProgress";
    } else {
        _loadGeoFence = true;
        qCDebug(PlanMasterControllerLog) << "PlanMasterController::loadFromVehicle calling _missionController.loadFromVehicle";
        _missionController.loadFromVehicle();
        setDirty(false);
        setIsSourcePlan(false);
    }
}


void PlanMasterController::_loadMissionComplete(void)
{
    if (!_flyView && _loadGeoFence) {
        _loadGeoFence = false;
        _loadRallyPoints = true;
        if (_geoFenceController.supported()) {
            qCDebug(PlanMasterControllerLog) << "PlanMasterController::_loadMissionComplete calling _geoFenceController.loadFromVehicle";
            _geoFenceController.loadFromVehicle();
        } else {
            qCDebug(PlanMasterControllerLog) << "PlanMasterController::_loadMissionComplete GeoFence not supported skipping";
            _geoFenceController.removeAll();
            _loadGeoFenceComplete();
        }
        setDirty(false);
    }
}

void PlanMasterController::_loadGeoFenceComplete(void)
{
    if (!_flyView && _loadRallyPoints) {
        _loadRallyPoints = false;
        if (_rallyPointController.supported()) {
            qCDebug(PlanMasterControllerLog) << "PlanMasterController::_loadGeoFenceComplete calling _rallyPointController.loadFromVehicle";
            _rallyPointController.loadFromVehicle();
        } else {
            qCDebug(PlanMasterControllerLog) << "PlanMasterController::_loadMissionComplete Rally Points not supported skipping";
            _rallyPointController.removeAll();
            _loadRallyPointsComplete();
        }
        setDirty(false);
    }
}

void PlanMasterController::_loadRallyPointsComplete(void)
{
    qCDebug(PlanMasterControllerLog) << "PlanMasterController::_loadRallyPointsComplete";
}

void PlanMasterController::_sendMissionComplete(void)
{
    if (_sendGeoFence) {
        _sendGeoFence = false;
        _sendRallyPoints = true;
        if (_geoFenceController.supported()) {
            qCDebug(PlanMasterControllerLog) << "PlanMasterController::sendToVehicle start GeoFence sendToVehicle";
            _geoFenceController.sendToVehicle();
        } else {
            qCDebug(PlanMasterControllerLog) << "PlanMasterController::sendToVehicle GeoFence not supported skipping";
            _sendGeoFenceComplete();
        }
        setDirty(false);
    }
}

void PlanMasterController::_sendGeoFenceComplete(void)
{
    if (_sendRallyPoints) {
        _sendRallyPoints = false;
        if (_rallyPointController.supported()) {
            qCDebug(PlanMasterControllerLog) << "PlanMasterController::sendToVehicle start rally sendToVehicle";
            _rallyPointController.sendToVehicle();
        } else {
            qCDebug(PlanMasterControllerLog) << "PlanMasterController::sendToVehicle Rally Points not support skipping";
            _sendRallyPointsComplete();
        }
    }
}

void PlanMasterController::_sendRallyPointsComplete(void)
{
    qCDebug(PlanMasterControllerLog) << "PlanMasterController::sendToVehicle Rally Point send complete";
    if (_deleteWhenSendCompleted) {
        this->deleteLater();
    }
}

#if defined(QGC_AIRMAP_ENABLED)
void PlanMasterController::_startFlightPlanning(void) {
    if (qgcApp()->toolbox()->airspaceManager()->connected()) {
        qCDebug(PlanMasterControllerLog) << "PlanMasterController::_startFlightPlanning client connected, start flight planning";
        qgcApp()->toolbox()->airspaceManager()->flightPlan()->startFlightPlanning(this);
    }
}
#endif

void PlanMasterController::sendToVehicle(void)
{
    WeakLinkInterfacePtr weakLink = _managerVehicle->vehicleLinkManager()->primaryLink();
    if (weakLink.expired()) {
        // Vehicle is shutting down
        return;
    } else {
        SharedLinkInterfacePtr sharedLink = weakLink.lock();
        if (sharedLink->linkConfiguration()->isHighLatency()) {
            qgcApp()->showAppMessage(tr("Upload not supported on high latency links."));
            return;
        }
    }

    if (offline()) {
        qCWarning(PlanMasterControllerLog) << "PlanMasterController::sendToVehicle called while offline";
    } else if (syncInProgress()) {
        qCWarning(PlanMasterControllerLog) << "PlanMasterController::sendToVehicle called while syncInProgress";
    } else {
        qCDebug(PlanMasterControllerLog) << "PlanMasterController::sendToVehicle start mission sendToVehicle";
        _sendGeoFence = true;
        _missionController.sendToVehicle();
        setDirty(false);
    }
}

void PlanMasterController::loadFromFile(const QString& filename)
{
    QString errorString;
    QString errorMessage = tr("Error loading Plan file (%1). %2").arg(filename).arg("%1");

    if (filename.isEmpty()) {
        return;
    }

    removeAll();

    QFileInfo fileInfo(filename);
    QFile file(filename);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        errorString = file.errorString() + QStringLiteral(" ") + filename;
        qgcApp()->showAppMessage(errorMessage.arg(errorString));
        return;
    }

    bool success = false;
    if (fileInfo.suffix() == AppSettings::missionFileExtension) {
        if (!_missionController.loadJsonFile(file, errorString)) {
            qgcApp()->showAppMessage(errorMessage.arg(errorString));
        } else {
            success = true;
        }
    } else if (fileInfo.suffix() == AppSettings::waypointsFileExtension || fileInfo.suffix() == QStringLiteral("txt")) {
        if (!_missionController.loadTextFile(file, errorString)) {
            qgcApp()->showAppMessage(errorMessage.arg(errorString));
        } else {
            success = true;
        }
    } else {
        QJsonDocument   jsonDoc;
        QByteArray      bytes = file.readAll();

        if (!JsonHelper::isJsonFile(bytes, jsonDoc, errorString)) {
            qgcApp()->showAppMessage(errorMessage.arg(errorString));
            return;
        }

        QJsonObject json = jsonDoc.object();
        //-- Allow plugins to pre process the load
        qgcApp()->toolbox()->corePlugin()->preLoadFromJson(this, json);

        int version;
        if (!JsonHelper::validateExternalQGCJsonFile(json, kPlanFileType, kPlanFileVersion, kPlanFileVersion, version, errorString)) {
            qgcApp()->showAppMessage(errorMessage.arg(errorString));
            return;
        }

        QList<JsonHelper::KeyValidateInfo> rgKeyInfo = {
            { kJsonMissionObjectKey,        QJsonValue::Object, true },
            { kJsonGeoFenceObjectKey,       QJsonValue::Object, true },
            { kJsonRallyPointsObjectKey,    QJsonValue::Object, true },
        };
        if (!JsonHelper::validateKeys(json, rgKeyInfo, errorString)) {
            qgcApp()->showAppMessage(errorMessage.arg(errorString));
            return;
        }

        if (!_missionController.load(json[kJsonMissionObjectKey].toObject(), errorString) ||
                !_geoFenceController.load(json[kJsonGeoFenceObjectKey].toObject(), errorString) ||
                !_rallyPointController.load(json[kJsonRallyPointsObjectKey].toObject(), errorString)) {
            qgcApp()->showAppMessage(errorMessage.arg(errorString));
        } else {
            //-- Allow plugins to post process the load
            qgcApp()->toolbox()->corePlugin()->postLoadFromJson(this, json);
            success = true;
        }
    }

    if(success){
        _currentPlanFile = QString::asprintf("%s/%s.%s", fileInfo.path().toLocal8Bit().data(), fileInfo.completeBaseName().toLocal8Bit().data(), AppSettings::planFileExtension);
        setIsSourcePlan(true);
        _resumePlanFile = QString(_currentPlanFile);
        _saveRecentFile();
        QTimer::singleShot(3000, this, &PlanMasterController::_missionChanged);

    } else {
        _currentPlanFile.clear();
    }
    emit currentPlanFileChanged();

    if (!offline()) {
        setDirty(true);
    }
}

void PlanMasterController::loadFromFileInResume(const QString& filename)
{

    QString recentlyFile;
    QString errorString;
    QString errorMessage = tr("Error loading Plan file (%1). %2").arg(filename).arg("%1");

    QFileInfo fileInfo(filename);
    QFile file(filename);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        errorString = file.errorString() + QStringLiteral(" ") + filename;
        qgcApp()->showAppMessage(errorMessage.arg(errorString));
        return;
    }

    bool success = false;
    if (fileInfo.suffix() == AppSettings::missionFileExtension) {
        if (!_missionController.loadJsonFile(file, errorString)) {
            qgcApp()->showAppMessage(errorMessage.arg(errorString));
        } else {
            success = true;
        }
    } else if (fileInfo.suffix() == AppSettings::waypointsFileExtension || fileInfo.suffix() == QStringLiteral("txt")) {
        if (!_missionController.loadTextFile(file, errorString)) {
            qgcApp()->showAppMessage(errorMessage.arg(errorString));
        } else {
            success = true;
        }
    } else {
        QJsonDocument   jsonDoc;
        QByteArray      bytes = file.readAll();

        if (!JsonHelper::isJsonFile(bytes, jsonDoc, errorString)) {
            qgcApp()->showAppMessage(errorMessage.arg(errorString));
            return;
        }

        QJsonObject json = jsonDoc.object();
        //-- Allow plugins to pre process the load
        qgcApp()->toolbox()->corePlugin()->preLoadFromJson(this, json);
        if (json.contains(kJsonRecentFileObjectKey)) {
            recentlyFile = json[kJsonRecentFileObjectKey].toString();
            qCWarning(PlanMasterControllerLog()) << "recentlyFile: " << recentlyFile;
        }

        qgcApp()->toolbox()->corePlugin()->postLoadFromJson(this, json);
        success = true;
    }

    if(success){
        _currentPlanFile = recentlyFile;
        setIsSourcePlan(true);
        _resumePlanFile = QString(_currentPlanFile);
        loadResumeFile();
    } else {
        _currentPlanFile.clear();
    }
    emit currentPlanFileChanged();

    if (!offline()) {
        setDirty(true);
    }
}


void PlanMasterController::_missionChanged()
{
    _isMissionChange = false;

}

void PlanMasterController::loadFromRecentFile(void)
{
    QString filename = qgcApp()->toolbox()->settingsManager()->appSettings()->missionSavePath() + kRecentFile;
    QString recentlyFile;
    QString errorString;
    QString errorMessage = tr("Error loading Plan file (%1). %2").arg(filename).arg("%1");

    QFileInfo fileInfo(filename);
    QFile file(filename);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        errorString = file.errorString() + QStringLiteral(" ") + filename;
        qgcApp()->showAppMessage(errorMessage.arg(errorString));
        return;
    }

    bool success = false;
    if (fileInfo.suffix() == AppSettings::missionFileExtension) {
        if (!_missionController.loadJsonFile(file, errorString)) {
            qgcApp()->showAppMessage(errorMessage.arg(errorString));
        } else {
            success = true;
        }
    } else if (fileInfo.suffix() == AppSettings::waypointsFileExtension || fileInfo.suffix() == QStringLiteral("txt")) {
        if (!_missionController.loadTextFile(file, errorString)) {
            qgcApp()->showAppMessage(errorMessage.arg(errorString));
        } else {
            success = true;
        }
    } else {
        QJsonDocument   jsonDoc;
        QByteArray      bytes = file.readAll();

        if (!JsonHelper::isJsonFile(bytes, jsonDoc, errorString)) {
            qgcApp()->showAppMessage(errorMessage.arg(errorString));
            return;
        }

        QJsonObject json = jsonDoc.object();
        //-- Allow plugins to pre process the load
        qgcApp()->toolbox()->corePlugin()->preLoadFromJson(this, json);
        if (json.contains(kJsonRecentFileObjectKey)) {
            recentlyFile = json[kJsonRecentFileObjectKey].toString();
            qCWarning(PlanMasterControllerLog()) << "recentlyFile: " << recentlyFile;
        }

        qgcApp()->toolbox()->corePlugin()->postLoadFromJson(this, json);
        success = true;
    }

    if(success){
        _currentPlanFile = recentlyFile;
        setIsSourcePlan(true);
        _resumePlanFile = QString(_currentPlanFile);
        loadResumeFile();
    } else {
        _currentPlanFile.clear();
    }
    emit currentPlanFileChanged();

    if (!offline()) {
        setDirty(true);
    }
}

void PlanMasterController::deleteFileInResume(void)
{
    qCWarning(PlanMasterControllerLog()) << "deleteFileInResume: " << _currentPlanFile;

    if (!_currentPlanFile.isEmpty()) {
        QFileInfo fileInfo(_currentPlanFile);

        QString curentAutosavedPlanFile =  qgcApp()->toolbox()->settingsManager()->appSettings()->resumeSavePath() + "/" + fileInfo.fileName();
        QFile::remove(curentAutosavedPlanFile);
    }
}

void PlanMasterController::loadPolygonFromRecentFile(void)
{
    QString filename = qgcApp()->toolbox()->settingsManager()->appSettings()->missionSavePath() + kRecentFile;
    QString recentlyFile;
    QString errorString;
    QString errorMessage = tr("Error loading Plan file (%1). %2").arg(filename).arg("%1");

    QFileInfo fileInfo(filename);
    QFile file(filename);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        errorString = file.errorString() + QStringLiteral(" ") + filename;
        qgcApp()->showAppMessage(errorMessage.arg(errorString));
        return;
    }

    bool success = false;
    if (fileInfo.suffix() == AppSettings::missionFileExtension) {
        if (!_missionController.loadJsonFile(file, errorString)) {
            qgcApp()->showAppMessage(errorMessage.arg(errorString));
        } else {
            success = true;
        }
    } else if (fileInfo.suffix() == AppSettings::waypointsFileExtension || fileInfo.suffix() == QStringLiteral("txt")) {
        if (!_missionController.loadTextFile(file, errorString)) {
            qgcApp()->showAppMessage(errorMessage.arg(errorString));
        } else {
            success = true;
        }
    } else {
        QJsonDocument   jsonDoc;
        QByteArray      bytes = file.readAll();

        if (!JsonHelper::isJsonFile(bytes, jsonDoc, errorString)) {
            qgcApp()->showAppMessage(errorMessage.arg(errorString));
            return;
        }

        QJsonObject json = jsonDoc.object();
        //        //-- Allow plugins to pre process the load
        qgcApp()->toolbox()->corePlugin()->preLoadFromJson(this, json);
        if (!_surveyAreaPolygon.loadFromJson(json, true /* required */, errorString)) {
            _surveyAreaPolygon.clear();
        }
        qWarning(PlanMasterControllerLog()) << " _surveyAreaPolygon: " << _surveyAreaPolygon.path() ;

        if (json.contains(kJsonSprayedAreaObjectKey)) {
            _sprayedArea = json[kJsonSprayedAreaObjectKey].toDouble();
            qCWarning(PlanMasterControllerLog()) << "_sprayedArea: " << _sprayedArea;
        }

        qgcApp()->toolbox()->corePlugin()->postLoadFromJson(this, json);

        success = true;
    }

    if(success){
        //_currentPlanFile = recentlyFile;
        //setIsSourcePlan(true);
        //_resumePlanFile = QString(_currentPlanFile);
        //loadResumeFile();
    } else {
        //_currentPlanFile.clear();
    }
}

void PlanMasterController::setTracingPolygon(QGCMapPolygon tracingPolygon) {
    if (_tracingAreaPolygon.path().size() <= 0) {
        _tracingAreaPolygon.setTracingPath(tracingPolygon.path());
        emit _missionController.splitSegmentChanged();
    }
}

void PlanMasterController::setTracingPolygonFromFile(QGCMapPolygon tracingPolygon) {
    //if (_tracingAreaPolygon.path().size() <= 0) {
        _tracingAreaPolygon.setTracingPath(tracingPolygon.path());
        emit _missionController.splitSegmentChanged();
    //}
}

void PlanMasterController::clearTracingPolygon() {
    _tracingAreaPolygon.clear();
    emit _missionController.splitSegmentChanged();
}

QJsonDocument PlanMasterController::saveToJson()
{
    QJsonObject planJson;
    qgcApp()->toolbox()->corePlugin()->preSaveToJson(this, planJson);
    QJsonObject missionJson;
    QJsonObject fenceJson;
    QJsonObject rallyJson;
    JsonHelper::saveQGCJsonFileHeader(planJson, kPlanFileType, kPlanFileVersion);
    //-- Allow plugin to preemptly add its own keys to mission
    qgcApp()->toolbox()->corePlugin()->preSaveToMissionJson(this, missionJson);
    _missionController.save(missionJson);
    //-- Allow plugin to add its own keys to mission
    qgcApp()->toolbox()->corePlugin()->postSaveToMissionJson(this, missionJson);
    _geoFenceController.save(fenceJson);
    _rallyPointController.save(rallyJson);
    planJson[kJsonMissionObjectKey] = missionJson;
    planJson[kJsonGeoFenceObjectKey] = fenceJson;
    planJson[kJsonRallyPointsObjectKey] = rallyJson;
    qgcApp()->toolbox()->corePlugin()->postSaveToJson(this, planJson);
    return QJsonDocument(planJson);
}

void  PlanMasterController::savePolygon(QVariantList path)
{
    _surveyAreaPolygon.setPath(path);
    _saveRecentFile();
}

QJsonDocument PlanMasterController::saveRecentFileToJson(const QString& filename)
{
    QJsonObject planJson;
    qgcApp()->toolbox()->corePlugin()->preSaveToJson(this, planJson);
    planJson[kJsonRecentFileObjectKey] = filename;
    _surveyAreaPolygon.saveToJson(planJson);
    planJson[kJsonSprayedAreaObjectKey] = _sprayedArea;

    qgcApp()->toolbox()->corePlugin()->postSaveToJson(this, planJson);
    return QJsonDocument(planJson);
}

void
PlanMasterController::saveToCurrent()
{
    if(!_currentPlanFile.isEmpty()) {
        _saveToCurrentOnResumeFolder2();
        saveToFile(_currentPlanFile);
    }
}

void
PlanMasterController::_saveToCurrentOnResumeFolder()
{
    if(!_currentPlanFile.isEmpty()) {
        saveToFile(_currentPlanFile);

        QFileInfo fileInfo(_currentPlanFile);
        QString curentAutosavedPlanFile =  qgcApp()->toolbox()->settingsManager()->appSettings()->resumeSavePath() + "/" + fileInfo.fileName();

        qCWarning(MissionControllerLog) << "after: " << curentAutosavedPlanFile;
        QFile file(curentAutosavedPlanFile);

        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            qgcApp()->showAppMessage(tr("Plan save error %1 : %2").arg(curentAutosavedPlanFile).arg(file.errorString()));
            curentAutosavedPlanFile.clear();
            emit currentPlanFileChanged();
        } else {
            QJsonDocument saveDoc = saveRecentFileToJson(_currentPlanFile);
            file.write(saveDoc.toJson());
        }

        // Only clear dirty bit if we are offline
        if (offline()) {
            setDirty(false);
        }
    }
}

void
PlanMasterController::_saveToCurrentOnResumeFolder2()
{
    if(!_currentPlanFile.isEmpty()) {
        QFileInfo fileInfo(_currentPlanFile);
        QString curentAutosavedPlanFile =  qgcApp()->toolbox()->settingsManager()->appSettings()->resumeSavePath() + "/" + fileInfo.fileName();

        qCWarning(MissionControllerLog) << "after: " << curentAutosavedPlanFile;
        QFile file(curentAutosavedPlanFile);
        if (!file.exists())
            return;

        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            qgcApp()->showAppMessage(tr("Plan save error %1 : %2").arg(curentAutosavedPlanFile).arg(file.errorString()));
            curentAutosavedPlanFile.clear();
            emit currentPlanFileChanged();
        } else {
            QFileInfo fileInfo = QFileInfo(curentAutosavedPlanFile);
            QString planFilename = QString("%1/%2(%3ha).%4")
                    .arg(fileInfo.absoluteDir().absolutePath())
                    .arg(fileInfo.baseName().split("(").at(0))
                    .arg(QString::number(area() / 10000, 'f', 2))
                    .arg(fileExtension());
            qCWarning(PlanMasterControllerLog) << "fileInfo.absoluteDir(): " << fileInfo.absoluteDir().absolutePath();
            qCWarning(PlanMasterControllerLog) << "planFilename: " << planFilename;
            fileInfo.absoluteDir().rename(fileInfo.absoluteFilePath(), planFilename);


            QJsonDocument saveDoc = saveRecentFileToJson(planFilename);
            file.write(saveDoc.toJson());
        }

        // Only clear dirty bit if we are offline
        if (offline()) {
            setDirty(false);
        }
    }
}

void
PlanMasterController::saveToCurrentInBackground()
{
    if(!_currentPlanFile.isEmpty()) {
        _saveToCurrentOnResumeFolder2();
        saveToFile(_currentPlanFile);
    }
}

void
PlanMasterController::_saveRecentFile()
{
    if(!_currentPlanFile.isEmpty()) {
        QString curentAutosavedPlanFile =  qgcApp()->toolbox()->settingsManager()->appSettings()->missionSavePath() + kRecentFile;

        qCWarning(MissionControllerLog) << "after: " << curentAutosavedPlanFile;
        QFile file(curentAutosavedPlanFile);

        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            qgcApp()->showAppMessage(tr("Plan save error %1 : %2").arg(curentAutosavedPlanFile).arg(file.errorString()));
            curentAutosavedPlanFile.clear();
            emit currentPlanFileChanged();
        } else {
            QJsonDocument saveDoc = saveRecentFileToJson(_currentPlanFile);
            file.write(saveDoc.toJson());
            emit currentPlanFileChanged();
        }

        // Only clear dirty bit if we are offline
        if (offline()) {
            setDirty(false);
        }
    }
}

void PlanMasterController::saveToFile(const QString& filename)
{
    if (filename.isEmpty()) {
        return;
    }

    QString planFilename = filename;
    if (!QFileInfo(filename).fileName().contains("(") && !QFileInfo(filename).fileName().contains("ha)")) {
        planFilename += QString("(%1ha)").arg(QString::number(area() / 10000, 'f', 2));
    }

    if (!QFileInfo(filename).fileName().contains(".")) {
        planFilename += QString(".%1").arg(fileExtension());
    }

    QFile file(planFilename);

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qgcApp()->showAppMessage(tr("Plan save error %1 : %2").arg(filename).arg(file.errorString()));
        _currentPlanFile.clear();
        emit currentPlanFileChanged();
    } else {
        QJsonDocument saveDoc = saveToJson();
        file.write(saveDoc.toJson());

        QFileInfo fileInfo = QFileInfo(planFilename);
        planFilename = QString("%1/%2(%3ha).%4")
                .arg(fileInfo.absoluteDir().absolutePath())
                .arg(fileInfo.baseName().split("(").at(0))
                .arg(QString::number(area() / 10000, 'f', 2))
                .arg(fileExtension());
        qCWarning(PlanMasterControllerLog) << "fileInfo.absoluteDir(): " << fileInfo.absoluteDir().absolutePath();
        qCWarning(PlanMasterControllerLog) << "planFilename: " << planFilename;
        fileInfo.absoluteDir().rename(fileInfo.absoluteFilePath(), planFilename);

        if(_currentPlanFile != planFilename) {
            _currentPlanFile = planFilename;
            emit currentPlanFileChanged();
        }
        _saveRecentFile();
        _resumePlanFile = QString(_currentPlanFile);
        QTimer::singleShot(3000, this, &PlanMasterController::_missionChanged);

    }

    // Only clear dirty bit if we are offline
    if (offline()) {
        setDirty(false);
    }
}

void PlanMasterController::saveToFlightHub(const QString& filename, QGeoCoordinate coordinate){
    if (filename.isEmpty()) {
        return;
    }

    QString planFilename = filename;

    if (!QFileInfo(filename).fileName().contains(".")) {
        planFilename += QString(".%1").arg(fileExtension());
    }

    QJsonDocument json = saveToJson();
    auto uploadArea =  area();
    qCWarning(PlanMasterControllerLog) << "upload area" << uploadArea;
    qgcApp()->toolbox()->flightHubManager()->uploadPlanFile(json, coordinate, uploadArea, planFilename);
}

void PlanMasterController::saveToKml(const QString& filename)
{
    if (filename.isEmpty()) {
        return;
    }

    QString kmlFilename = filename;
    if (!QFileInfo(filename).fileName().contains(".")) {
        kmlFilename += QString(".%1").arg(kmlFileExtension());
    }

    QFile file(kmlFilename);

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qgcApp()->showAppMessage(tr("KML save error %1 : %2").arg(filename).arg(file.errorString()));
    } else {
        KMLPlanDomDocument planKML;
        _missionController.addMissionToKML(planKML);
        QTextStream stream(&file);
        stream << planKML.toString();
        file.close();
    }
}

void PlanMasterController::removeAll(void)
{
    _missionController.removeAll();
    _geoFenceController.removeAll();
    _rallyPointController.removeAll();
    _currentPlanFile.clear();
    emit currentPlanFileChanged();

    if (_offline) {
        _missionController.setDirty(false);
        _geoFenceController.setDirty(false);
        _rallyPointController.setDirty(false);

    }
}

void PlanMasterController::clearResumeFile(void)
{
    _resumePlanFile.clear();
}

void PlanMasterController::_uploadToVehicle()
{
    if (!offline()) {
        _missionController.clearTrajectoryPoints();
        setParam();
        sendToVehicle();
    }
}

void PlanMasterController::loadResumeFile(void)
{
    if (!_resumePlanFile.isEmpty()) {
        loadFromFile(_resumePlanFile);
        QTimer::singleShot(3000, this, &PlanMasterController::_uploadToVehicle);
    }
}

void PlanMasterController::removeAllFromVehicle(void)
{
    if (!offline()) {
        _missionController.removeAllFromVehicle();
        if (_geoFenceController.supported()) {
            _geoFenceController.removeAllFromVehicle();
        }
        if (_rallyPointController.supported()) {
            _rallyPointController.removeAllFromVehicle();
        }
        setDirty(false);
    } else {
        qWarning() << "PlanMasterController::removeAllFromVehicle called while offline";
    }
}

bool PlanMasterController::containsItems(void) const
{
    return _missionController.containsItems() || _geoFenceController.containsItems() || _rallyPointController.containsItems();
}

bool PlanMasterController::dirty(void) const
{
    return _missionController.dirty() || _geoFenceController.dirty() || _rallyPointController.dirty();
}

void PlanMasterController::setDirty(bool dirty)
{
    _missionController.setDirty(dirty);
    _geoFenceController.setDirty(dirty);
    _rallyPointController.setDirty(dirty);
}

QString PlanMasterController::fileExtension(void) const
{
    return AppSettings::planFileExtension;
}

QString PlanMasterController::kmlFileExtension(void) const
{
    return AppSettings::kmlFileExtension;
}

QStringList PlanMasterController::loadNameFilters(void) const
{
    QStringList filters;

    filters << tr("Supported types (*.%1 *.%2 *.%3 *.%4)").arg(AppSettings::planFileExtension).arg(AppSettings::missionFileExtension).arg(AppSettings::waypointsFileExtension).arg("txt") <<
               tr("All Files (*)");
    return filters;
}


QStringList PlanMasterController::saveNameFilters(void) const
{
    QStringList filters;

    filters << tr("Plan Files (*.%1)").arg(fileExtension()) << tr("All Files (*)");
    return filters;
}

void PlanMasterController::sendPlanToVehicle(Vehicle* vehicle, const QString& filename)
{
    // Use a transient PlanMasterController to accomplish this
    PlanMasterController* controller = new PlanMasterController();
    controller->startStaticActiveVehicle(vehicle, true /* deleteWhenSendCompleted */);
    controller->loadFromFile(filename);
    controller->sendToVehicle();
}

void PlanMasterController::_showPlanFromManagerVehicle(void)
{
    if (!_managerVehicle->initialPlanRequestComplete() && !syncInProgress()) {
        // Something went wrong with initial load. All controllers are idle, so just force it off
        _managerVehicle->forceInitialPlanRequestComplete();
    }

    // The crazy if structure is to handle the load propagating by itself through the system
    if (!_missionController.showPlanFromManagerVehicle()) {
        if (!_geoFenceController.showPlanFromManagerVehicle()) {
            _rallyPointController.showPlanFromManagerVehicle();
        }
    }
}

bool PlanMasterController::syncInProgress(void) const
{
    return _missionController.syncInProgress() ||
            _geoFenceController.syncInProgress() ||
            _rallyPointController.syncInProgress();
}

bool PlanMasterController::isEmpty(void) const
{
    return _missionController.isEmpty() &&
            _geoFenceController.isEmpty() &&
            _rallyPointController.isEmpty();
}

void PlanMasterController::_updatePlanCreatorsList(void)
{
    if (!_flyView) {
        if (!_planCreators) {
            _planCreators = new QmlObjectListModel(this);
            _planCreators->append(new BlankPlanCreator(this, this));
            _planCreators->append(new SurveyPlanCreator(this, this));
            _planCreators->append(new CorridorScanPlanCreator(this, this));
            emit planCreatorsChanged(_planCreators);
        }

        if (_managerVehicle->fixedWing()) {
            if (_planCreators->count() == 4) {
                _planCreators->removeAt(_planCreators->count() - 1);
            }
        } else {
            if (_planCreators->count() != 4) {
                _planCreators->append(new StructureScanPlanCreator(this, this));
            }
        }
    }
}

void PlanMasterController::showPlanFromManagerVehicle(void)
{
    if (offline()) {
        // There is no new vehicle so clear any previous plan
        qCDebug(PlanMasterControllerLog) << "showPlanFromManagerVehicle: Plan View - No new vehicle, clear any previous plan";
        removeAll();
    } else {
        // We have a new active vehicle, show the plan from that
        qCDebug(PlanMasterControllerLog) << "showPlanFromManagerVehicle: Plan View - New vehicle available, show plan from new manager vehicle";
        _showPlanFromManagerVehicle();
    }
}

void PlanMasterController::setParam()
{
    AgriSettings *agriSettings = qgcApp()->toolbox()->settingsManager()->agriSettings();
    bool isCentrifugal = (agriSettings->sprayOption()->rawValue() == agriSettings->sprayCentrifugalType());

    qCDebug(PlanMasterControllerLog) << "isCentrifugal: " << isCentrifugal;
    if (!isCentrifugal) { // pressure
        float velocity = this->_surveyComplexItem->velocity()->property("value").toFloat();
        float x = this->_surveyComplexItem->sprayFlowRate()->property("value").toFloat();
        float y = 0;
        if (x >= 5.15) {
            y = 72.0;
        } else if (x < 2.9) {
            y = 25.0;
        } else {
            // y =  -1.525x4 + 22.794x3 - 118.86x2 +  271.63x - 202.45
           //y = (-1.525 * qPow(x, 4.0)) + (22.794 * qPow(x, 3.0)) + (- 118.86 * qPow(x, 2.0)) + (271.63 * x)  +  (- 202.45);
           y = ( -7.1683 * qPow(x, 5.0)) + (147.42 * qPow(x, 4.0)) + ( - 1197.9 * qPow(x, 3.0)) + ( 4811.3 * qPow(x, 2.0)) + (- 9545.5 * x)  +  ( 7505.6);
        }

        y = y / velocity;
        this->_multiVehicleMgr->activeVehicle()->parameterManager()->getParameter(MAV_COMP_ID_AUTOPILOT1, "SPRAY_PUMP_RATE")->setProperty("value", y);
        this->_multiVehicleMgr->activeVehicle()->parameterManager()->getParameter(MAV_COMP_ID_AUTOPILOT1, "WPNAV_SPEED")->setProperty("value", (100 * velocity));
        this->_multiVehicleMgr->activeVehicle()->parameterManager()->getParameter(MAV_COMP_ID_AUTOPILOT1, "WP_YAW_BEHAVIOR")->setProperty("value", 0);
    } else { // centrifugal
        float velocity = this->_surveyComplexItem->velocity()->property("value").toFloat();
        float x = this->_surveyComplexItem->sprayFlowRate()->property("value").toFloat();
        float y = 0;
        if (x >= 6.74) {
            y = 53;
        } else if (x < 2.38) {
            y = 13;
        } else {
            if (x < 4.62)
                y = (4.4068 * qPow(x, 5.0)) + (- 76.658 * qPow(x, 4.0)) + (527.93 * qPow(x, 3.0)) + (- 1799 * qPow(x, 2.0))  +  (3039.1 * x) + (- 2022.9);
            else
                y = (-8.2058 * qPow(x, 6.0)) + (274.96 * qPow(x, 5.0)) + (- 3821.5 * qPow(x, 4.0)) + (28196 * qPow(x, 3.0)) + (- 116470 * qPow(x, 2.0))  +  (255383 * x) + (- 232201);
        }

        y = y / velocity;
        this->_multiVehicleMgr->activeVehicle()->parameterManager()->getParameter(MAV_COMP_ID_AUTOPILOT1, "SPRAY_PUMP_RATE")->setProperty("value", y);
        this->_multiVehicleMgr->activeVehicle()->parameterManager()->getParameter(MAV_COMP_ID_AUTOPILOT1, "WPNAV_SPEED")->setProperty("value", (100 * velocity));
        this->_multiVehicleMgr->activeVehicle()->parameterManager()->getParameter(MAV_COMP_ID_AUTOPILOT1, "WP_YAW_BEHAVIOR")->setProperty("value", 0);
    }
};

// <toanpt>: centrifugal RPM
//void PlanMasterController::setParam()
//{
//    float velocity = this->_surveyComplexItem->velocity()->property("value").toFloat();
//    float x = this->_surveyComplexItem->sprayFlowRate()->property("value").toFloat();
//    float y = 0;
//    if (x >= 6.74) {
//        y = 53;
//    } else if (x < 2.38) {
//        y = 13;
//    } else {
//        if (x < 4.62)
//            y = (4.4068 * qPow(x, 5.0)) + (- 76.658 * qPow(x, 4.0)) + (527.93 * qPow(x, 3.0)) + (- 1799 * qPow(x, 2.0))  +  (3039.1 * x) + (- 2022.9);
//        else
//            y = (-8.2058 * qPow(x, 6.0)) + (274.96 * qPow(x, 5.0)) + (- 3821.5 * qPow(x, 4.0)) + (28196 * qPow(x, 3.0)) + (- 116470 * qPow(x, 2.0))  +  (255383 * x) + (- 232201);
//    }

//    y = y / velocity;
//    this->_multiVehicleMgr->activeVehicle()->parameterManager()->getParameter(MAV_COMP_ID_AUTOPILOT1, "SPRAY_PUMP_RATE")->setProperty("value", y);
//    this->_multiVehicleMgr->activeVehicle()->parameterManager()->getParameter(MAV_COMP_ID_AUTOPILOT1, "WPNAV_SPEED")->setProperty("value", (100 * velocity));
//    this->_multiVehicleMgr->activeVehicle()->parameterManager()->getParameter(MAV_COMP_ID_AUTOPILOT1, "WP_YAW_BEHAVIOR")->setProperty("value", 0);
//};


double PlanMasterController::area()
{
    double surveyArea = _surveyComplexItem ?
            _surveyComplexItem->realCoveredArea() :
            0.0;
        double result = surveyArea;
        for (int i=0; i<_geoFenceController.polygons()->count(); i++) {
            result -= _geoFenceController.polygons()->value<QGCFencePolygon*>(i)->area();
        }
        return result;

}
