#include <VehicleVcuInfoFactGroup.h>
#include <Vehicle.h>

const char* VehicleVcuInfoFactGroup::_coolantWaterInletTempChannelFactName =        "coolantWaterInletTempChannel";
const char* VehicleVcuInfoFactGroup::_coolantWaterOutletTempChannelFactName =       "coolantWaterOutletTempChannel";
const char* VehicleVcuInfoFactGroup::_engineCabinTempChannelFactName =              "engineCabinTempChannel";
const char* VehicleVcuInfoFactGroup::_fuelCabinTempChannelFactName =                "fuelCabinTempChannel";
const char* VehicleVcuInfoFactGroup::_exhaustPipeTempChannelFactName =              "exhaustPipeTempChannel";
const char* VehicleVcuInfoFactGroup::_thermoErrorMaskFactName =                     "thermoErrorMask";
const char* VehicleVcuInfoFactGroup::_throttlePercentFactName =                     "throttlePercent";
const char* VehicleVcuInfoFactGroup::_steeringAngleFactName =                       "steeringAngle";
const char* VehicleVcuInfoFactGroup::_fuelTankPctFactName =                         "fuelTankPct";
const char* VehicleVcuInfoFactGroup::_contactorStateFactName =                      "contactorState";
const char* VehicleVcuInfoFactGroup::_cameraPanPositionFactName =                   "cameraPanPosition";
const char* VehicleVcuInfoFactGroup::_cameraTiltPositionFactName =                           "cameraTiltPosition";
const char* VehicleVcuInfoFactGroup::_cameraZoomPositionFactName =                            "cameraZoomPosition";
const char* VehicleVcuInfoFactGroup::_cameraDeltaYawFactName =                           "cameraDeltaYaw";
const char* VehicleVcuInfoFactGroup::_relayDetonatorStateFactName =                      "relayDetonatorState";
const char* VehicleVcuInfoFactGroup::_relayExplodeStateFactName =                      "relayExplodeState";

//Constructor
VehicleVcuInfoFactGroup::VehicleVcuInfoFactGroup(QObject *parent)
    : FactGroup(200, ":/json/Vehicle/VcuInfoFact.json", parent)
    , _coolantWaterInletTempChannelFact         (0, _coolantWaterInletTempChannelFactName,          FactMetaData::valueTypeUint16)
    , _coolantWaterOutletTempChannelFact        (0, _coolantWaterOutletTempChannelFactName,         FactMetaData::valueTypeUint16)
    , _engineCabinTempChannelFact               (0, _engineCabinTempChannelFactName,                FactMetaData::valueTypeUint16)
    , _fuelCabinTempChannelFact                 (0, _fuelCabinTempChannelFactName,                  FactMetaData::valueTypeUint16)
    , _exhaustPipeTempChannelFact               (0, _exhaustPipeTempChannelFactName,                FactMetaData::valueTypeFloat)
    , _thermoErrorMaskFact                      (0, _thermoErrorMaskFactName,                       FactMetaData::valueTypeFloat)
    , _throttlePercentFact                      (0, _throttlePercentFactName,                       FactMetaData::valueTypeFloat)
    , _steeringAngleFact                        (0, _steeringAngleFactName,                         FactMetaData::valueTypeFloat)
    , _fuelTankPctFact                          (0, _fuelTankPctFactName,                           FactMetaData::valueTypeInt8)
    , _contactorStateFact                       (0, _contactorStateFactName,                        FactMetaData::valueTypeUint16)
    , _cameraPanPositionFact                    (0, _cameraPanPositionFactName,                     FactMetaData::valueTypeInt8)
    , _cameraTiltPositionFact                   (0, _cameraTiltPositionFactName,                    FactMetaData::valueTypeInt8)
    , _cameraZoomPositionFact                   (0, _cameraZoomPositionFactName,                    FactMetaData::valueTypeInt8)
    , _cameraDeltaYawFact                       (0, _cameraDeltaYawFactName,                        FactMetaData::valueTypeInt8)
    , _relayDetonatorStateFact                  (0, _relayDetonatorStateFactName,                   FactMetaData::valueTypeUint16)
    , _relayExplodeStateFact                    (0, _relayExplodeStateFactName,                     FactMetaData::valueTypeUint16)
{
    //Init fact with a single value and name
    _addFact(&_coolantWaterInletTempChannelFact,         _coolantWaterInletTempChannelFactName);
    _addFact(&_coolantWaterOutletTempChannelFact,        _coolantWaterOutletTempChannelFactName);
    _addFact(&_engineCabinTempChannelFact,              _engineCabinTempChannelFactName);
    _addFact(&_fuelCabinTempChannelFact,                _fuelCabinTempChannelFactName);
    _addFact(&_exhaustPipeTempChannelFact,              _exhaustPipeTempChannelFactName);
    _addFact(&_thermoErrorMaskFact,                     _thermoErrorMaskFactName);
    _addFact(&_throttlePercentFact,                     _throttlePercentFactName);
    _addFact(&_steeringAngleFact,                       _steeringAngleFactName);
    _addFact(&_fuelTankPctFact,                         _fuelTankPctFactName);
    _addFact(&_contactorStateFact,                      _contactorStateFactName);
    _addFact(&_cameraPanPositionFact,                            _cameraPanPositionFactName);
    _addFact(&_cameraTiltPositionFact,                           _cameraTiltPositionFactName);
    _addFact(&_cameraZoomPositionFact,                            _cameraZoomPositionFactName);
    _addFact(&_cameraDeltaYawFact,                           _cameraDeltaYawFactName);
    _addFact(&_relayDetonatorStateFact,                      _relayDetonatorStateFactName);
    _addFact(&_relayExplodeStateFact,                      _relayExplodeStateFactName);

    //Initialize with value not available "--.--"
    _coolantWaterInletTempChannelFact.setRawValue(qQNaN());
    _coolantWaterOutletTempChannelFact.setRawValue(qQNaN());
    _engineCabinTempChannelFact.setRawValue(qQNaN());
    _fuelCabinTempChannelFact.setRawValue(qQNaN());
    _exhaustPipeTempChannelFact.setRawValue(qQNaN());
    _thermoErrorMaskFact.setRawValue(qQNaN());
    _throttlePercentFact.setRawValue(qQNaN());
    _steeringAngleFact.setRawValue(qQNaN());
    _fuelTankPctFact.setRawValue(qQNaN());
    _contactorStateFact.setRawValue(qQNaN());
    _cameraPanPositionFact.setRawValue(qQNaN());
    _cameraTiltPositionFact.setRawValue(qQNaN());
    _cameraZoomPositionFact.setRawValue(qQNaN());
    _cameraDeltaYawFact.setRawValue(qQNaN());
    _relayDetonatorStateFact.setRawValue(qQNaN());
    _relayExplodeStateFact.setRawValue(qQNaN());
}

void VehicleVcuInfoFactGroup::handleMessage(Vehicle *vehicle, const mavlink_message_t &message)
{
    //Spray Info is streaming through ESC(9-12) telemetry message
    Q_UNUSED(vehicle);

    switch (message.msgid) {
    case MAVLINK_MSG_ID_BATTERY_STATUS:
        _handleVcuInfo(message);
        break;
    case MAVLINK_MSG_ID_GIMBAL_DEVICE_ATTITUDE_STATUS:
        _handleCameraInfo(message);
        break;
    default:
        break;
    }
}

void VehicleVcuInfoFactGroup::_handleCameraInfo(const mavlink_message_t &msg)
{
    mavlink_gimbal_device_attitude_status_t cameraStatus;
    mavlink_msg_gimbal_device_attitude_status_decode(&msg, &cameraStatus);

    if(cameraStatus.gimbal_device_id == 1) {
        cameraPanPosition()->setRawValue(cameraStatus.angular_velocity_x);
        cameraTiltPosition()->setRawValue(cameraStatus.angular_velocity_y);
        cameraZoomPosition()->setRawValue(cameraStatus.angular_velocity_z);
        cameraDeltaYaw()->setRawValue(cameraStatus.delta_yaw);
    }

}

void VehicleVcuInfoFactGroup::_handleVcuInfo(const mavlink_message_t &msg)
{
    //Decode mavlink telemetry message
    mavlink_battery_status_t batteryStatus;
    mavlink_msg_battery_status_decode(&msg, &batteryStatus);
    if (batteryStatus.id != 2) {
        //VCU handle
        return;
    }
    //Set data to Fact System
    coolantWaterInletTempChannel()->setRawValue(batteryStatus.voltages[0]/10.0f);
    coolantWaterOutletTempChannel()->setRawValue(batteryStatus.voltages[1]/10.0f);
    engineCabinTempChannel()->setRawValue(batteryStatus.voltages[2]/10.0f);
    fuelCabinTempChannel()->setRawValue(batteryStatus.voltages[3]/10.0f);
    exhaustPipeTempChannel()->setRawValue(batteryStatus.voltages[4]/10.0f);
    thermoErrorMask()->setRawValue(batteryStatus.fault_bitmask);
    throttlePercent()->setRawValue(batteryStatus.temperature);
    steeringAngle()->setRawValue(batteryStatus.current_battery);
    // contactorState()->setRawValue(batteryStatus.charge_state);
    fuelTankPct()->setRawValue(batteryStatus.battery_remaining);
    _handleRelayState(batteryStatus.charge_state);
}

void VehicleVcuInfoFactGroup::_handleRelayState(uint8_t state) {
    contactorState()->setRawValue(((state>>0)& 0x01));
    relayDetonatorState()->setRawValue(((state>>1)& 0x01));
    relayExplodeState()->setRawValue(((state>>2)& 0x01));

}
