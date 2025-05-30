/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "InitialConnectStateMachine.h"
#include "Vehicle.h"
#include "QGCCorePlugin.h"
#include "QGCOptions.h"
#include "FirmwarePlugin.h"
#include "ParameterManager.h"
#include "ComponentInformationManager.h"
#include "MissionManager.h"
#include "StandardModes.h"
#include "GeoFenceManager.h"
#include "RallyPointManager.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(InitialConnectStateMachineLog, "qgc.vehicle.initialconnectstatemachine")

InitialConnectStateMachine::InitialConnectStateMachine(Vehicle *vehicle, QObject *parent)
    : StateMachine(parent)
    , _vehicle(vehicle)
{
    static_assert(std::size(_rgStates) == std::size(_rgProgressWeights), "array size mismatch");

    _progressWeightTotal = 0;
    for (int i = 0; i < _cStates; ++i) {
        _progressWeightTotal += _rgProgressWeights[i];
    }

    // qCDebug(InitialConnectStateMachineLog) << Q_FUNC_INFO << this;
}

InitialConnectStateMachine::~InitialConnectStateMachine()
{
    // qCDebug(InitialConnectStateMachineLog) << Q_FUNC_INFO << this;
}

int InitialConnectStateMachine::stateCount(void) const
{
    return _cStates;
}

const InitialConnectStateMachine::StateFn* InitialConnectStateMachine::rgStates(void) const
{
    return &_rgStates[0];
}

void InitialConnectStateMachine::statesCompleted(void) const
{

}

void InitialConnectStateMachine::advance()
{
    StateMachine::advance();
    emit progressUpdate(_progress());
}

void InitialConnectStateMachine::gotProgressUpdate(double progressValue)
{
    emit progressUpdate(_progress(static_cast<float>(progressValue)));
}

float InitialConnectStateMachine::_progress(float subProgress)
{
    int progressWeight = 0;
    for (int i = 0; i < _stateIndex && i < _cStates; ++i) {
        progressWeight += _rgProgressWeights[i];
    }
    int currentWeight = _stateIndex < _cStates ? _rgProgressWeights[_stateIndex] : 1;
    return (progressWeight + currentWeight * subProgress) / static_cast<float>(_progressWeightTotal);
}

void InitialConnectStateMachine::_stateRequestAutopilotVersion(StateMachine* stateMachine)
{
    InitialConnectStateMachine* connectMachine  = static_cast<InitialConnectStateMachine*>(stateMachine);
    Vehicle*                    vehicle         = connectMachine->_vehicle;
    SharedLinkInterfacePtr      sharedLink      = vehicle->vehicleLinkManager()->primaryLink().lock();

    if (!sharedLink) {
        qCDebug(InitialConnectStateMachineLog) << "Skipping REQUEST_MESSAGE:AUTOPILOT_VERSION request due to no primary link";
        connectMachine->advance();
    } else {
        if (sharedLink->linkConfiguration()->isHighLatency() || sharedLink->isLogReplay()) {
            qCDebug(InitialConnectStateMachineLog) << "Skipping REQUEST_MESSAGE:AUTOPILOT_VERSION request due to link type";
            connectMachine->advance();
        } else {
            qCDebug(InitialConnectStateMachineLog) << "Sending REQUEST_MESSAGE:AUTOPILOT_VERSION";
            vehicle->requestMessage(_autopilotVersionRequestMessageHandler,
                                    connectMachine,
                                    MAV_COMP_ID_AUTOPILOT1,
                                    MAVLINK_MSG_ID_AUTOPILOT_VERSION);
        }
    }
}

void InitialConnectStateMachine::_autopilotVersionRequestMessageHandler(void* resultHandlerData, MAV_RESULT commandResult, Vehicle::RequestMessageResultHandlerFailureCode_t failureCode, const mavlink_message_t& message)
{
    InitialConnectStateMachine* connectMachine  = static_cast<InitialConnectStateMachine*>(resultHandlerData);
    Vehicle*                    vehicle         = connectMachine->_vehicle;

    switch (failureCode) {
    case Vehicle::RequestMessageNoFailure:
    {
        qCDebug(InitialConnectStateMachineLog) << "AUTOPILOT_VERSION received";

        mavlink_autopilot_version_t autopilotVersion;
        mavlink_msg_autopilot_version_decode(&message, &autopilotVersion);

        vehicle->_uid = (quint64)autopilotVersion.uid;
        vehicle->_firmwareBoardVendorId = autopilotVersion.vendor_id;
        vehicle->_firmwareBoardProductId = autopilotVersion.product_id;
        emit vehicle->vehicleUIDChanged();

        if (autopilotVersion.flight_sw_version != 0) {
            int majorVersion, minorVersion, patchVersion;
            FIRMWARE_VERSION_TYPE versionType;

            majorVersion = (autopilotVersion.flight_sw_version >> (8*3)) & 0xFF;
            minorVersion = (autopilotVersion.flight_sw_version >> (8*2)) & 0xFF;
            patchVersion = (autopilotVersion.flight_sw_version >> (8*1)) & 0xFF;
            versionType = (FIRMWARE_VERSION_TYPE)((autopilotVersion.flight_sw_version >> (8*0)) & 0xFF);
            vehicle->setFirmwareVersion(majorVersion, minorVersion, patchVersion, versionType);
        }

        if (vehicle->px4Firmware()) {
            // Lower 3 bytes is custom version
            int majorVersion, minorVersion, patchVersion;
            majorVersion = autopilotVersion.flight_custom_version[2];
            minorVersion = autopilotVersion.flight_custom_version[1];
            patchVersion = autopilotVersion.flight_custom_version[0];
            vehicle->setFirmwareCustomVersion(majorVersion, minorVersion, patchVersion);

            // PX4 Firmware stores the first 16 characters of the git hash as binary, with the individual bytes in reverse order
            vehicle->_gitHash = "";
            for (int i = 7; i >= 0; i--) {
                vehicle->_gitHash.append(QString("%1").arg(autopilotVersion.flight_custom_version[i], 2, 16, QChar('0')));
            }
        } else {
            // APM Firmware stores the first 8 characters of the git hash as an ASCII character string
            char nullStr[9];
            strncpy(nullStr, (char*)autopilotVersion.flight_custom_version, 8);
            nullStr[8] = 0;
            vehicle->_gitHash = nullStr;
        }
        if (QGCCorePlugin::instance()->options()->checkFirmwareVersion() && !vehicle->_checkLatestStableFWDone) {
            vehicle->_checkLatestStableFWDone = true;
            vehicle->_firmwarePlugin->checkIfIsLatestStable(vehicle);
        }
        emit vehicle->gitHashChanged(vehicle->_gitHash);

        vehicle->_setCapabilities(autopilotVersion.capabilities);
    }
        break;
    case Vehicle::RequestMessageFailureCommandError:
        qCDebug(InitialConnectStateMachineLog) << QStringLiteral("REQUEST_MESSAGE:AUTOPILOT_VERSION command error(%1)").arg(commandResult);
        break;
    case Vehicle::RequestMessageFailureCommandNotAcked:
        qCDebug(InitialConnectStateMachineLog) << "REQUEST_MESSAGE:AUTOPILOT_VERSION command never acked";
        break;
    case Vehicle::RequestMessageFailureMessageNotReceived:
        qCDebug(InitialConnectStateMachineLog) << "REQUEST_MESSAGE:AUTOPILOT_VERSION command acked but message never received";
        break;
    case Vehicle::RequestMessageFailureDuplicateCommand:
        qCDebug(InitialConnectStateMachineLog) << "REQUEST_MESSAGE:AUTOPILOT_VERSION Internal Error: Duplicate command";
        break;
    }

    if (failureCode != Vehicle::RequestMessageNoFailure) {
        qCDebug(InitialConnectStateMachineLog) << "REQUEST_MESSAGE:AUTOPILOT_VERSION failed. Setting no capabilities";
        uint64_t assumedCapabilities = 0;
        if (vehicle->_mavlinkProtocolRequestMaxProtoVersion >= 200) {
            // Link already running mavlink 2
            assumedCapabilities |= MAV_PROTOCOL_CAPABILITY_MAVLINK2;
        }
        if (vehicle->px4Firmware() || vehicle->apmFirmware()) {
            // We make some assumptions for known firmware
            assumedCapabilities |= MAV_PROTOCOL_CAPABILITY_MISSION_INT | MAV_PROTOCOL_CAPABILITY_COMMAND_INT | MAV_PROTOCOL_CAPABILITY_MISSION_FENCE | MAV_PROTOCOL_CAPABILITY_MISSION_RALLY;
        }
        vehicle->_setCapabilities(assumedCapabilities);
    }

    connectMachine->advance();
}

void InitialConnectStateMachine::_stateRequestProtocolVersion(StateMachine* stateMachine)
{
    InitialConnectStateMachine* connectMachine  = static_cast<InitialConnectStateMachine*>(stateMachine);
    Vehicle*                    vehicle         = connectMachine->_vehicle;
    SharedLinkInterfacePtr      sharedLink      = vehicle->vehicleLinkManager()->primaryLink().lock();

    if (!sharedLink) {
        qCDebug(InitialConnectStateMachineLog) << "Skipping REQUEST_MESSAGE:PROTOCOL_VERSION request due to no primary link";
        connectMachine->advance();
    } else {
        if (sharedLink->linkConfiguration()->isHighLatency() || sharedLink->isLogReplay()) {
            qCDebug(InitialConnectStateMachineLog) << "Skipping REQUEST_MESSAGE:PROTOCOL_VERSION request due to link type";
            connectMachine->advance();
        } else if (vehicle->apmFirmware()) {
            qCDebug(InitialConnectStateMachineLog) << "Skipping REQUEST_MESSAGE:PROTOCOL_VERSION request due to Ardupilot firmware";
            connectMachine->advance();
        } else {
            qCDebug(InitialConnectStateMachineLog) << "Sending REQUEST_MESSAGE:PROTOCOL_VERSION";
            vehicle->requestMessage(_protocolVersionRequestMessageHandler,
                                    connectMachine,
                                    MAV_COMP_ID_AUTOPILOT1,
                                    MAVLINK_MSG_ID_PROTOCOL_VERSION);
        }
    }
}

void InitialConnectStateMachine::_protocolVersionRequestMessageHandler(void* resultHandlerData, MAV_RESULT commandResult, Vehicle::RequestMessageResultHandlerFailureCode_t failureCode, const mavlink_message_t& message)
{
    InitialConnectStateMachine* connectMachine  = static_cast<InitialConnectStateMachine*>(resultHandlerData);
    Vehicle*                    vehicle         = connectMachine->_vehicle;

    switch (failureCode) {
    case Vehicle::RequestMessageNoFailure:
    {
        mavlink_protocol_version_t protoVersion;
        mavlink_msg_protocol_version_decode(&message, &protoVersion);

        qCDebug(InitialConnectStateMachineLog) << "PROTOCOL_VERSION received mav_version:" << protoVersion.max_version;
        vehicle->_mavlinkProtocolRequestMaxProtoVersion = protoVersion.max_version;
        vehicle->_mavlinkProtocolRequestComplete = true;
        vehicle->_setMaxProtoVersionFromBothSources();
    }
        break;
    case Vehicle::RequestMessageFailureCommandError:
        qCDebug(InitialConnectStateMachineLog) << QStringLiteral("REQUEST_MESSAGE PROTOCOL_VERSION command error(%1)").arg(commandResult);
        break;
    case Vehicle::RequestMessageFailureCommandNotAcked:
        qCDebug(InitialConnectStateMachineLog) << "REQUEST_MESSAGE PROTOCOL_VERSION command never acked";
        break;
    case Vehicle::RequestMessageFailureMessageNotReceived:
        qCDebug(InitialConnectStateMachineLog) << "REQUEST_MESSAGE PROTOCOL_VERSION command acked but message never received";
        break;
    case Vehicle::RequestMessageFailureDuplicateCommand:
        qCDebug(InitialConnectStateMachineLog) << "REQUEST_MESSAGE PROTOCOL_VERSION Internal Error: Duplicate command";
        break;
    }

    if (failureCode != Vehicle::RequestMessageNoFailure) {
        // Either the PROTOCOL_VERSION message didn't make it through the pipe from Vehicle->QGC because the pipe is mavlink 1.
        // Or the PROTOCOL_VERSION message was lost on a noisy connection. Either way the best we can do is fall back to mavlink 1.
        qCDebug(InitialConnectStateMachineLog) << QStringLiteral("Setting _maxProtoVersion to 100 due to timeout on receiving PROTOCOL_VERSION message.");
        vehicle->_mavlinkProtocolRequestMaxProtoVersion = 100;
        vehicle->_mavlinkProtocolRequestComplete = true;
        vehicle->_setMaxProtoVersionFromBothSources();
    }

    connectMachine->advance();
}
void InitialConnectStateMachine::_stateRequestCompInfo(StateMachine* stateMachine)
{
    InitialConnectStateMachine* connectMachine  = static_cast<InitialConnectStateMachine*>(stateMachine);
    Vehicle*                    vehicle         = connectMachine->_vehicle;

    qCDebug(InitialConnectStateMachineLog) << "_stateRequestCompInfo";
    connect(vehicle->_componentInformationManager, &ComponentInformationManager::progressUpdate, connectMachine,
            &InitialConnectStateMachine::gotProgressUpdate);
    vehicle->_componentInformationManager->requestAllComponentInformation(_stateRequestCompInfoComplete, connectMachine);
}

void InitialConnectStateMachine::_stateRequestStandardModes(StateMachine *stateMachine)
{
    InitialConnectStateMachine* connectMachine  = static_cast<InitialConnectStateMachine*>(stateMachine);
    Vehicle*                    vehicle         = connectMachine->_vehicle;

    qCDebug(InitialConnectStateMachineLog) << "_stateRequestStandardModes";
    connect(vehicle->_standardModes, &StandardModes::requestCompleted, connectMachine,
            &InitialConnectStateMachine::standardModesRequestCompleted);
    vehicle->_standardModes->request();
}

void InitialConnectStateMachine::standardModesRequestCompleted()
{
    disconnect(_vehicle->_standardModes, &StandardModes::requestCompleted, this,
               &InitialConnectStateMachine::standardModesRequestCompleted);
    advance();
}

void InitialConnectStateMachine::_stateRequestCompInfoComplete(void* requestAllCompleteFnData)
{
    InitialConnectStateMachine* connectMachine  = static_cast<InitialConnectStateMachine*>(requestAllCompleteFnData);
    disconnect(connectMachine->_vehicle->_componentInformationManager, &ComponentInformationManager::progressUpdate,
            connectMachine, &InitialConnectStateMachine::gotProgressUpdate);

    connectMachine->advance();
}

void InitialConnectStateMachine::_stateRequestParameters(StateMachine* stateMachine)
{
    InitialConnectStateMachine* connectMachine  = static_cast<InitialConnectStateMachine*>(stateMachine);
    Vehicle*                    vehicle         = connectMachine->_vehicle;

    qCDebug(InitialConnectStateMachineLog) << "_stateRequestParameters";
    connect(vehicle->_parameterManager, &ParameterManager::loadProgressChanged, connectMachine,
            &InitialConnectStateMachine::gotProgressUpdate);
    vehicle->_parameterManager->refreshAllParameters();
}

void InitialConnectStateMachine::_stateRequestMission(StateMachine* stateMachine)
{
    InitialConnectStateMachine* connectMachine  = static_cast<InitialConnectStateMachine*>(stateMachine);
    Vehicle*                    vehicle         = connectMachine->_vehicle;
    SharedLinkInterfacePtr      sharedLink      = vehicle->vehicleLinkManager()->primaryLink().lock();

    disconnect(vehicle->_parameterManager, &ParameterManager::loadProgressChanged, connectMachine,
            &InitialConnectStateMachine::gotProgressUpdate);

    if (!sharedLink) {
        qCDebug(InitialConnectStateMachineLog) << "_stateRequestMission: Skipping first mission load request due to no primary link";
        connectMachine->advance();
    } else {
        if (sharedLink->linkConfiguration()->isHighLatency() || sharedLink->isLogReplay()) {
            qCDebug(InitialConnectStateMachineLog) << "_stateRequestMission: Skipping first mission load request due to link type";
            vehicle->_firstMissionLoadComplete();
        } else {
            qCDebug(InitialConnectStateMachineLog) << "_stateRequestMission";
            vehicle->_missionManager->loadFromVehicle();
            (void) connect(vehicle->_missionManager, &MissionManager::progressPctChanged, connectMachine, &InitialConnectStateMachine::gotProgressUpdate);
        }
    }
}

void InitialConnectStateMachine::_stateRequestGeoFence(StateMachine* stateMachine)
{
    InitialConnectStateMachine* connectMachine  = static_cast<InitialConnectStateMachine*>(stateMachine);
    Vehicle*                    vehicle         = connectMachine->_vehicle;
    SharedLinkInterfacePtr      sharedLink      = vehicle->vehicleLinkManager()->primaryLink().lock();

    disconnect(vehicle->_missionManager, &MissionManager::progressPctChanged, connectMachine,
               &InitialConnectStateMachine::gotProgressUpdate);

    if (!sharedLink) {
        qCDebug(InitialConnectStateMachineLog) << "_stateRequestGeoFence: Skipping first geofence load request due to no primary link";
        connectMachine->advance();
    } else {
        if (sharedLink->linkConfiguration()->isHighLatency() || sharedLink->isLogReplay()) {
            qCDebug(InitialConnectStateMachineLog) << "_stateRequestGeoFence: Skipping first geofence load request due to link type";
            vehicle->_firstGeoFenceLoadComplete();
        } else {
            if (vehicle->_geoFenceManager->supported()) {
                qCDebug(InitialConnectStateMachineLog) << "_stateRequestGeoFence";
                vehicle->_geoFenceManager->loadFromVehicle();
                connect(vehicle->_geoFenceManager, &GeoFenceManager::progressPctChanged, connectMachine,
                        &InitialConnectStateMachine::gotProgressUpdate);
            } else {
                qCDebug(InitialConnectStateMachineLog) << "_stateRequestGeoFence: skipped due to no support";
                vehicle->_firstGeoFenceLoadComplete();
            }
        }
    }
}

void InitialConnectStateMachine::_stateRequestRallyPoints(StateMachine* stateMachine)
{
    InitialConnectStateMachine* connectMachine  = static_cast<InitialConnectStateMachine*>(stateMachine);
    Vehicle*                    vehicle         = connectMachine->_vehicle;
    SharedLinkInterfacePtr      sharedLink      = vehicle->vehicleLinkManager()->primaryLink().lock();

    disconnect(vehicle->_geoFenceManager, &GeoFenceManager::progressPctChanged, connectMachine,
               &InitialConnectStateMachine::gotProgressUpdate);

    if (!sharedLink) {
        qCDebug(InitialConnectStateMachineLog) << "_stateRequestRallyPoints: Skipping first rally point load request due to no primary link";
        connectMachine->advance();
    } else {
        if (sharedLink->linkConfiguration()->isHighLatency() || sharedLink->isLogReplay()) {
            qCDebug(InitialConnectStateMachineLog) << "_stateRequestRallyPoints: Skipping first rally point load request due to link type";
            vehicle->_firstRallyPointLoadComplete();
        } else {
            if (vehicle->_rallyPointManager->supported()) {
                vehicle->_rallyPointManager->loadFromVehicle();
                connect(vehicle->_rallyPointManager, &RallyPointManager::progressPctChanged, connectMachine,
                        &InitialConnectStateMachine::gotProgressUpdate);
            } else {
                qCDebug(InitialConnectStateMachineLog) << "_stateRequestRallyPoints: skipping due to no support";
                vehicle->_firstRallyPointLoadComplete();
            }
        }
    }
}

void InitialConnectStateMachine::_stateSignalInitialConnectComplete(StateMachine* stateMachine)
{
    InitialConnectStateMachine* connectMachine  = static_cast<InitialConnectStateMachine*>(stateMachine);
    Vehicle*                    vehicle         = connectMachine->_vehicle;

    disconnect(vehicle->_rallyPointManager, &RallyPointManager::progressPctChanged, connectMachine,
               &InitialConnectStateMachine::gotProgressUpdate);

    connectMachine->advance();
    qCDebug(InitialConnectStateMachineLog) << "Signalling initialConnectComplete";
    emit vehicle->initialConnectComplete();
}

