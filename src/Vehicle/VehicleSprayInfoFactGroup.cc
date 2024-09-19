#include <VehicleSprayInfoFactGroup.h>
#include <Vehicle.h>

const char* VehicleSprayInfoFactGroup::_pumpRPMFirstChannelFactName =           "pumpRPMFirstChannel";
const char* VehicleSprayInfoFactGroup::_pumpRPMSecondChannelFactName =          "pumpRPMSecondChannel";
const char* VehicleSprayInfoFactGroup::_centrifugalRPMFirstChannelFactName =    "centrifugalRPMFirstChannel";
const char* VehicleSprayInfoFactGroup::_centrifugalRPMSecondChannelFactName =   "centrifugalRPMSecondChannel";
const char* VehicleSprayInfoFactGroup::_flowspeedFirstChannelFactName =         "flowspeedFirstChannel";
const char* VehicleSprayInfoFactGroup::_flowspeedSecondChannelFactName =        "flowspeedSecondChannel";
const char* VehicleSprayInfoFactGroup::_sVolFirstChannelFactName =              "sVolFirstChannel";
const char* VehicleSprayInfoFactGroup::_sVolSecondChannelFactName =             "sVolSecondChannel";
const char* VehicleSprayInfoFactGroup::_fuelTankPctFactName =                   "fuelTankPct";
const char* VehicleSprayInfoFactGroup::_weightADCValueFactName =                "weightADCvalue";
const char* VehicleSprayInfoFactGroup::_armFirstFactName =                      "armFirst";
const char* VehicleSprayInfoFactGroup::_armSecondFactName =                     "armSecond";
const char* VehicleSprayInfoFactGroup::_armThirdFactName =                      "armThird";
const char* VehicleSprayInfoFactGroup::_armFourthFactName =                     "armFourth";

//Constructor
VehicleSprayInfoFactGroup::VehicleSprayInfoFactGroup(QObject *parent)
    : FactGroup(1000, ":/json/Vehicle/SprayInfoFact.json", parent)
    , _pumpRPMFirstChannelFact          (0, _pumpRPMFirstChannelFactName,           FactMetaData::valueTypeUint16)
    , _pumpRPMSecondChannelFact         (0, _pumpRPMSecondChannelFactName,          FactMetaData::valueTypeUint16)
    , _centrifugalRPMFirstChannelFact   (0, _centrifugalRPMFirstChannelFactName,    FactMetaData::valueTypeUint16)
    , _centrifugalRPMSecondChannelFact  (0, _centrifugalRPMSecondChannelFactName,   FactMetaData::valueTypeUint16)
    , _flowspeedFirstChannelFact        (0, _flowspeedFirstChannelFactName,         FactMetaData::valueTypeFloat)
    , _flowspeedSecondChannelFact       (0, _flowspeedSecondChannelFactName,        FactMetaData::valueTypeFloat)
    , _sVolFirstChannelFact             (0, _sVolFirstChannelFactName,              FactMetaData::valueTypeFloat)
    , _sVolSecondChannelFact            (0, _sVolSecondChannelFactName,             FactMetaData::valueTypeFloat)
    , _fuelTankPctFact                  (0, _fuelTankPctFactName,                   FactMetaData::valueTypeInt8)
    , _weightADCValueFact               (0, _weightADCValueFactName,                FactMetaData::valueTypeUint16)
    , _armFirstFact                     (0, _armFirstFactName,                      FactMetaData::valueTypeInt8)
    , _armSecondFact                    (0, _armSecondFactName,                     FactMetaData::valueTypeInt8)
    , _armThirdFact                     (0, _armThirdFactName,                      FactMetaData::valueTypeInt8)
    , _armFourthFact                    (0, _armFourthFactName,                     FactMetaData::valueTypeInt8)
{
    //Init fact with a single value and name
    _addFact(&_pumpRPMFirstChannelFact,         _pumpRPMFirstChannelFactName);
    _addFact(&_pumpRPMSecondChannelFact,        _pumpRPMSecondChannelFactName);
    _addFact(&_centrifugalRPMFirstChannelFact,  _centrifugalRPMFirstChannelFactName);
    _addFact(&_centrifugalRPMSecondChannelFact, _centrifugalRPMSecondChannelFactName);
    _addFact(&_flowspeedFirstChannelFact,       _flowspeedFirstChannelFactName);
    _addFact(&_flowspeedSecondChannelFact,      _flowspeedSecondChannelFactName);
    _addFact(&_sVolFirstChannelFact,            _sVolFirstChannelFactName);
    _addFact(&_sVolSecondChannelFact,           _sVolSecondChannelFactName);
    _addFact(&_fuelTankPctFact,                 _fuelTankPctFactName);
    _addFact(&_weightADCValueFact,              _weightADCValueFactName);
    _addFact(&_armFirstFact,                    _armFirstFactName);
    _addFact(&_armSecondFact,                   _armSecondFactName);
    _addFact(&_armThirdFact,                    _armThirdFactName);
    _addFact(&_armFourthFact,                   _armFourthFactName);

    //Initialize with value not available "--.--"
    _pumpRPMFirstChannelFact.setRawValue(qQNaN());
    _pumpRPMSecondChannelFact.setRawValue(qQNaN());
    _centrifugalRPMFirstChannelFact.setRawValue(qQNaN());
    _centrifugalRPMSecondChannelFact.setRawValue(qQNaN());
    _flowspeedFirstChannelFact.setRawValue(qQNaN());
    _flowspeedSecondChannelFact.setRawValue(qQNaN());
    _sVolFirstChannelFact.setRawValue(qQNaN());
    _sVolSecondChannelFact.setRawValue(qQNaN());
    _fuelTankPctFact.setRawValue(qQNaN());
    _weightADCValueFact.setRawValue(qQNaN());
    _armFirstFact.setRawValue(qQNaN());
    _armSecondFact.setRawValue(qQNaN());
    _armThirdFact.setRawValue(qQNaN());
    _armFourthFact.setRawValue(qQNaN());
}

void VehicleSprayInfoFactGroup::handleMessage(Vehicle* /*vehicle*/, mavlink_message_t& message)
{
    //Spray Info is streaming through ESC(9-12) telemetry message
    switch (message.msgid) {
    case MAVLINK_MSG_ID_ESC_TELEMETRY_9_TO_12:
        _handleSprayInfo(message);
        break;
    default:
        break;
    }
}

void VehicleSprayInfoFactGroup::_handleSprayInfo(mavlink_message_t &msg)
{
    //Decode mavlink telemetry message
    mavlink_esc_telemetry_9_to_12_t vcu;
    mavlink_msg_esc_telemetry_9_to_12_decode(&msg, &vcu);

    //Set data to Fact System
    pumpRPMFirstChannel()->setRawValue(vcu.rpm[0]);
    pumpRPMSecondChannel()->setRawValue(vcu.rpm[1]);
    centrifugalRPMFirstChannel()->setRawValue(vcu.rpm[2]);
    centrifugalRPMSecondChannel()->setRawValue(vcu.rpm[3]);
    flowspeedFirstChannel()->setRawValue(vcu.current[0]);
    flowspeedSecondChannel()->setRawValue(vcu.current[1]);
    sVolFirstChannel()->setRawValue(vcu.totalcurrent[0]);
    sVolSecondChannel()->setRawValue(vcu.totalcurrent[0]);
    weightADCValue()->setRawValue(vcu.voltage[0]);
    fuelTankPct()->setRawValue(vcu.voltage[1]);
    armFirst()->setRawValue(vcu.temperature[0]);
    armSecond()->setRawValue(vcu.temperature[1]);
    armThird()->setRawValue(vcu.temperature[2]);
    armFourth()->setRawValue(vcu.temperature[3]);
}
