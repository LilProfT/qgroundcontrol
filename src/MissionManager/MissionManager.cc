/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "MissionManager.h"
#include "Vehicle.h"
#include "FirmwarePlugin.h"
#include "MAVLinkProtocol.h"
#include "QGCApplication.h"
#include "MissionCommandTree.h"
#include "MissionCommandUIInfo.h"
#include "PlanMasterController.h"
#include <QGeoCoordinate>

QGC_LOGGING_CATEGORY(MissionManagerLog, "MissionManagerLog")

MissionManager::MissionManager(Vehicle* vehicle)
    : PlanManager               (vehicle, MAV_MISSION_TYPE_MISSION)
    , _cachedLastCurrentIndex   (-1)
    , _blockNextResume          (false)
{
    connect(_vehicle, &Vehicle::mavlinkMessageReceived, this, &MissionManager::_mavlinkMessageReceived);

    connect(this, &PlanManager::currentIndexChanged, this, &MissionManager::_maybeResetTrimResume);
}

MissionManager::~MissionManager()
{

}
void MissionManager::writeArduPilotGuidedMissionItem(const QGeoCoordinate& gotoCoord, bool altChangeOnly)
{
    if (inProgress()) {
        qCDebug(MissionManagerLog) << "writeArduPilotGuidedMissionItem called while transaction in progress";
        return;
    }

    _transactionInProgress = TransactionWrite;

    _connectToMavlink();

    WeakLinkInterfacePtr weakLink = _vehicle->vehicleLinkManager()->primaryLink();
    if (!weakLink.expired()) {
        SharedLinkInterfacePtr sharedLink = weakLink.lock();


        mavlink_message_t       messageOut;
        mavlink_mission_item_t  missionItem;

        memset(&missionItem, 0, sizeof(missionItem));
        missionItem.target_system =     _vehicle->id();
        missionItem.target_component =  _vehicle->defaultComponentId();
        missionItem.seq =               0;
        missionItem.command =           MAV_CMD_NAV_WAYPOINT;
        missionItem.param1 =            0;
        missionItem.param2 =            0;
        missionItem.param3 =            0;
        missionItem.param4 =            0;
        missionItem.x =                 gotoCoord.latitude();
        missionItem.y =                 gotoCoord.longitude();
        missionItem.z =                 gotoCoord.altitude();
        missionItem.frame =             MAV_FRAME_GLOBAL_RELATIVE_ALT;
        missionItem.current =           altChangeOnly ? 3 : 2;
        missionItem.autocontinue =      true;

        mavlink_msg_mission_item_encode_chan(qgcApp()->toolbox()->mavlinkProtocol()->getSystemId(),
                                             qgcApp()->toolbox()->mavlinkProtocol()->getComponentId(),
                                             sharedLink->mavlinkChannel(),
                                             &messageOut,
                                             &missionItem);

        _vehicle->sendMessageOnLinkThreadSafe(sharedLink.get(), messageOut);
    }
    _startAckTimeout(AckGuidedItem);
    emit inProgressChanged(true);
}

void MissionManager::generateResumeMission(int resumeIndex)
{
    if (_blockNextResume) {
        _blockNextResume = false;
        return;
    }
    auto survey = qgcApp()->toolbox()->planMasterControllerPlanView()->surveyComplexItem();

    bool addHomePosition = _vehicle->firmwarePlugin()->sendHomePositionToVehicle();
    QList<MissionItem*> resumeMission;
    for (int i=0; i<_missionItems.count(); i++) {
        MissionItem* oldItem = _missionItems[i];
        if ((addHomePosition && (i==0)) || (oldItem->command() == MAV_CMD_NAV_TAKEOFF)) {
            MissionItem* newItem = new MissionItem(*oldItem, this);
            newItem->setIsCurrentItem(false);
            resumeMission.append(newItem);
        }
    }

    survey->missionModel()->restore();
    survey->missionModel()->setResumeCoord(_vehicle->resumeCoordinate());
    survey->missionModel()->setUseResumeCoord();
    survey->missionModel()->setMissionItemParent(_missionItems[0]->parent());
    resumeMission << survey->missionModel()->generateItems();
    qDebug() << "trim resume in manager" << survey->missionModel()->trimResume();
    survey->trimResume()->setRawValue(survey->missionModel()->trimResume());

    MissionItem* lastRTL = _missionItems[_missionItems.count()-1];
    if (lastRTL->command() == MAV_CMD_NAV_RETURN_TO_LAUNCH) {
        MissionItem* newItem = new MissionItem(*lastRTL, this);
        newItem->setIsCurrentItem(false);
        resumeMission.append(newItem);
    }

    // Adjust sequence numbers and current item
    int seqNum = 0;
    for (int i=0; i<resumeMission.count(); i++) {
        resumeMission[i]->setSequenceNumber(seqNum++);
    }
    int setCurrentIndex = addHomePosition ? 1 : 0;
    resumeMission[setCurrentIndex]->setIsCurrentItem(true);

    // Send to vehicle
    _clearAndDeleteWriteMissionItems();
    for (int i=0; i<resumeMission.count(); i++) {
        _writeMissionItems.append(new MissionItem(*resumeMission[i], this));
    }
    _resumeMission = true;
    _writeMissionItemsWorker();
}

void MissionManager::autoSaveMission(void) {
    emit autoSaved();
}

void MissionManager::deleteResumeMission(void) {
    emit deleteResumed();
}

/// Called when a new mavlink message for out vehicle is received
void MissionManager::_mavlinkMessageReceived(const mavlink_message_t& message)
{
    switch (message.msgid) {
    case MAVLINK_MSG_ID_HIGH_LATENCY:
        _handleHighLatency(message);
        break;

    case MAVLINK_MSG_ID_HIGH_LATENCY2:
        _handleHighLatency2(message);
        break;

    case MAVLINK_MSG_ID_MISSION_CURRENT:
        _handleMissionCurrent(message);
        break;

    case MAVLINK_MSG_ID_HEARTBEAT:
        _handleHeartbeat(message);
        break;
    }
}

void MissionManager::_updateMissionIndex(int index)
{
    if (index != _currentMissionIndex) {
        qCDebug(MissionManagerLog) << "_updateMissionIndex currentIndex:" << index;
        _currentMissionIndex = index;
        emit currentIndexChanged(_currentMissionIndex);
    }

    if (_currentMissionIndex != _lastCurrentIndex && _cachedLastCurrentIndex != _currentMissionIndex) {
        // We have to be careful of an RTL sequence causing a change of index to the DO_LAND_START sequence. This also triggers
        // a flight mode change away from mission flight mode. So we only update _lastCurrentIndex when the flight mode is mission.
        // But we can run into problems where we may get the MISSION_CURRENT message for the RTL/DO_LAND_START sequenc change prior
        // to the HEARTBEAT message which contains the flight mode change which will cause things to work incorrectly. To fix this
        // We force the sequencing of HEARTBEAT following by MISSION_CURRENT by caching the possible _lastCurrentIndex update until
        // the next HEARTBEAT comes through.
        qCDebug(MissionManagerLog) << "_updateMissionIndex caching _lastCurrentIndex for possible update:" << _currentMissionIndex;
        _cachedLastCurrentIndex = _currentMissionIndex;
    }
}

void MissionManager::_handleHighLatency(const mavlink_message_t& message)
{
    mavlink_high_latency_t highLatency;
    mavlink_msg_high_latency_decode(&message, &highLatency);
    _updateMissionIndex(highLatency.wp_num);
}

void MissionManager::_handleHighLatency2(const mavlink_message_t& message)
{
    mavlink_high_latency2_t highLatency2;
    mavlink_msg_high_latency2_decode(&message, &highLatency2);
    _updateMissionIndex(highLatency2.wp_num);
}

void MissionManager::_handleMissionCurrent(const mavlink_message_t& message)
{
    mavlink_mission_current_t missionCurrent;
    mavlink_msg_mission_current_decode(&message, &missionCurrent);
    _updateMissionIndex(missionCurrent.seq);
}

void MissionManager::_handleHeartbeat(const mavlink_message_t& message)
{
    Q_UNUSED(message);

    if (_cachedLastCurrentIndex != -1 &&  _vehicle->flightMode() == _vehicle->missionFlightMode()) {
        qCDebug(MissionManagerLog) << "_handleHeartbeat updating lastCurrentIndex from cached value:" << _cachedLastCurrentIndex;
        _lastCurrentIndex = _cachedLastCurrentIndex;
        _cachedLastCurrentIndex = -1;
        emit lastCurrentIndexChanged(_lastCurrentIndex);
    }
}

void MissionManager::_maybeResetTrimResume(int currentIndex)
{
    if (_missionItems.count() == 0) return;
    if (_currentMissionIndex >= _missionItems.count()-1) {
        auto survey = qgcApp()->toolbox()->planMasterControllerPlanView()->surveyComplexItem();
        if (survey) {
            survey->trimResume()->setRawValue(0.0);
            survey->missionModel()->clearUseResumeCoord();
            _blockNextResume = true;
        }
    }
}
