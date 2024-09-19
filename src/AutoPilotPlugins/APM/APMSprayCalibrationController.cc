#include <APMSprayCalibrationController.h>
#include <ParameterManager.h>
#include <QGCMAVLink.h>
#include <QGCApplication.h>

const char* APMSprayCalibrationController::_weightZeroPointParamName        = "VCU_ZERO_VAL";
const char* APMSprayCalibrationController::_weightMiddlePointParamName      = "VCU_MID_VAL";
const char* APMSprayCalibrationController::_weightFullPointParamName        = "VCU_MAX_VAL";
const char* APMSprayCalibrationController::_weightMiddleLitersParamName     = "VCU_MID_VOL";
const char* APMSprayCalibrationController::_sprayFactGroupName              = "vcu";

//Constructor
APMSprayCalibrationController::APMSprayCalibrationController(void)
    : _nextButton(nullptr)
    , _cancelButton(nullptr)
    , _calibProgressState(CalibProgressCode::CalibProgressZero)
    , _calibFirstStepInProgress(false)
    , _calibSecondStepInProgress(false)
    , _calibThirdStepInProgress(false)
    , _calibFirstStepDone(false)
    , _calibSecondStepDone(false)
    , _calibThirdStepDone(false)
    , _isValidMeasuredValue(true)
    , _isTextFieldValueEdited(false)
    , _restoreParamFlags(false)
{}

APMSprayCalibrationController::~APMSprayCalibrationController()
{
    _restoreCalibrationParams();
}

void APMSprayCalibrationController::calibrateTank(void)
{
    //Reset next button to default
    _nextButton->setProperty("text",tr("Next"));
    _vehicle->vehicleLinkManager()->setCommunicationLostEnabled(false);
    _cancelButton->setEnabled(false);
    _nextButton->setEnabled(false);
    _startButton->setEnabled(false);
    _measuredTextField->setEnabled(false);

    _type = CalTypeTank;
    connect(qgcApp()->toolbox()->mavlinkProtocol(),&MAVLinkProtocol::messageReceived,this,&APMSprayCalibrationController::_mavlinkMessageReceived);

    // Reset all progress indication
    _calibFirstStepInProgress = false;
    _calibSecondStepInProgress = false;
    _calibThirdStepInProgress = false;
    _calibFirstStepDone = false;
    _calibSecondStepDone = false;
    _calibThirdStepDone = false;
    _isTextFieldValueEdited = false;
    //Emit for changing in signals
    emit calibProcessStepDoneChanged();
    emit calibProcessStepInProgressChanged();
    emit isTextFieldValueEditedChanged();

    //Store previous param value
    _restoreParamFlags = true;
    _preWeightZeroPointParamValue = getParameterFact(FactSystem::defaultComponentId, _weightZeroPointParamName)->rawValue().toFloat();
    _preWeightMiddlePointParamValue = getParameterFact(FactSystem::defaultComponentId, _weightMiddlePointParamName)->rawValue().toFloat();
    _preWeightFullPointParamValue = getParameterFact(FactSystem::defaultComponentId, _weightFullPointParamName)->rawValue().toFloat();
    _preWeightMiddleLitersParamValue = getParameterFact(FactSystem::defaultComponentId, _weightMiddleLitersParamName)->rawValue().toFloat();

    //Use sendMavCommand since it not required a fast response
    _startCalibration(CalibProgressCode::CalibProgressZero);
}

void APMSprayCalibrationController::_startCalibration(APMSprayCalibrationController::CalibProgressCode code)
{
    bool updateSignal = false;
    //For testing
    FactGroup *vcu = _vehicle->getFactGroup(_sprayFactGroupName);

    uint16_t weightSensorValue = vcu->getFact("weightADCvalue")->rawValue().toUInt();
    switch (code) {
    case CalibProgressZero:
        updateSignal = true;

        //UI Config
        _calibrateHelpText->setProperty("text", tr("Make sure there is no water in Tank and press Next when ready"));
        _nextButton->setEnabled(true);
        _cancelButton->setEnabled(true);
        _calibFirstStepInProgress = true;

        //Set to next state
        _calibProgressState = CalibProgressMiddle;
        break;

    case CalibProgressMiddle:
        updateSignal = true;
        //Save the previous state value
        getParameterFact(FactSystem::defaultComponentId,_weightZeroPointParamName)->setRawValue(weightSensorValue);
        _measuredTextField->setEnabled(true);

        //UI Config
        _calibrateHelpText->setProperty("text", tr("Fill water tank at middle level, wait for steady, write measured value to 'Measured liters' then click Next"));
        _calibFirstStepDone = true;
        _calibFirstStepInProgress = false;
        _calibSecondStepInProgress = true;
        //Disable nextButton until the value on Text field correctly
        if(!_isTextFieldValueEdited) {
            _nextButton->setEnabled(false);
        }

        //Set to next state
        _calibProgressState = CalibProgressFull;

        break;

    case CalibProgressFull:
        updateSignal = true;

        //Save the previous state value
        getParameterFact(FactSystem::defaultComponentId,_weightMiddlePointParamName)->setRawValue(weightSensorValue);

        _calibrateHelpText->setProperty("text", tr("Fill water tank until full, wait for steady then click Next"));
        _calibSecondStepDone = true;
        _calibSecondStepInProgress = false;
        _calibThirdStepInProgress = true;
        _measuredTextField->setEnabled(false);

        //Set to next state
        _calibProgressState = CalibProgressFinish;
        break;

    case CalibProgressFinish:
        //Save the previous state value
        getParameterFact(FactSystem::defaultComponentId,_weightFullPointParamName)->setRawValue(weightSensorValue);
        _nextButton->setProperty("text",tr("Finish"));
        _calibrateHelpText->setProperty("text", tr("Click Finish to complete calibration"));

        updateSignal = true;
        _calibThirdStepDone = true;
        _calibThirdStepInProgress = false;

        //Reset back to the init state
        _calibProgressState = CalibProgressZero;
        break;
    default:
        break;
    }

    if (updateSignal) {
        emit calibProcessStepInProgressChanged();
        emit calibProcessStepDoneChanged();
    }
}

void APMSprayCalibrationController::_mavlinkMessageReceived(LinkInterface *link, mavlink_message_t message)
{
    Q_UNUSED(link);

    switch (message.msgid) {
    case MAVLINK_MSG_ID_COMMAND_ACK:
        _handleCommandAck(message);
        break;
    }
}

void APMSprayCalibrationController::_refreshParams(void)
{
    QStringList fastRefreshList;
    fastRefreshList << QStringLiteral("VCU_ZERO_VAL")
                    << QStringLiteral("VCU_MID_VAL")
                    << QStringLiteral("VCU_MAX_VAL")
                    << QStringLiteral("VCU_MID_VOL");

    foreach (const QString &paramID, fastRefreshList) {
        _vehicle->parameterManager()->refreshParameter(FactSystem::defaultComponentId, paramID);
    }

}
void APMSprayCalibrationController::_handleCommandAck(mavlink_message_t& message)
{
    mavlink_command_ack_t cmdAck;
    mavlink_msg_command_ack_decode(&message, &cmdAck);

    if (cmdAck.command == MAV_CMD_DO_SPRAYER) {
        switch (cmdAck.result) {
        case MAV_RESULT_ACCEPTED:
            _calibrateHelpText->setProperty("text", tr("Tank calibration is finished"));
            _stopCalibration();
            emit calibCompleteSign(_type);
            break;
        default:
            break;
        }
    }

}

void APMSprayCalibrationController::nextButtonClicked(void)
{
    if (_calibProgressState != CalibProgressZero) {
        _startCalibration(_calibProgressState);
        return;
    }

    bool sprayerEnable = false;
    _vehicle->triggerSprayCalibration(sprayerEnable, 2);

    WeakLinkInterfacePtr weakLink = _vehicle->vehicleLinkManager()->primaryLink();
    if (!weakLink.expired()) {
        mavlink_message_t       msg;
        SharedLinkInterfacePtr  sharedLink = weakLink.lock();

        mavlink_msg_command_ack_pack_chan(qgcApp()->toolbox()->mavlinkProtocol()->getSystemId(),
                                          qgcApp()->toolbox()->mavlinkProtocol()->getComponentId(),
                                          sharedLink->mavlinkChannel(),
                                          &msg,
                                          0,    // command
                                          1,    // result
                                          0,    // progress
                                          0,    // result_param2
                                          0,    // target_system
                                          0);   // target_component

        _vehicle->sendMessageOnLinkThreadSafe(sharedLink.get(), msg);
    }

}

//Function when finish editing the Textfield
void APMSprayCalibrationController::saveMeasuredParam(float value)
{
    qDebug()<< "Measured value: " << value;

    if (qIsNaN(value)) {
        return;
    }

    _isTextFieldValueEdited = true;
    emit isTextFieldValueEditedChanged();
    float fuelLiters = getParameterFact(FactSystem::defaultComponentId, "VCU_FUEL_TANK")->rawValue().toFloat();
    if (value <= 0.0f || value >= fuelLiters) {
        _isValidMeasuredValue = false;
        emit isValidMeasuredValueChanged();
        _nextButton->setEnabled(false);
    }
    else {
        if(parameterExists(FactSystem::defaultComponentId,_weightMiddleLitersParamName)) {
            _isValidMeasuredValue = true;
            emit isValidMeasuredValueChanged();
            _nextButton->setEnabled(true);
            getParameterFact(FactSystem::defaultComponentId, _weightMiddleLitersParamName)->setRawValue(value);
        }
    }
}

void APMSprayCalibrationController::cancelCalibration(void)
{
    _restoreCalibrationParams();
    _stopCalibration();
}

void APMSprayCalibrationController::_stopCalibration(void)
{
    disconnect(qgcApp()->toolbox()->mavlinkProtocol(),&MAVLinkProtocol::messageReceived,this,&APMSprayCalibrationController::_mavlinkMessageReceived);
    //Reset next button to default
    _nextButton->setProperty("text",tr("Next"));
    _nextButton->setEnabled(false);
    _cancelButton->setEnabled(false);
    _startButton->setEnabled(true);
    _refreshParams();
}

void APMSprayCalibrationController::_restoreCalibrationParams(void)
{
    if(_restoreParamFlags) {
        _restoreParamFlags = false;

        getParameterFact(FactSystem::defaultComponentId,_weightZeroPointParamName)->setRawValue(_preWeightZeroPointParamValue);
        getParameterFact(FactSystem::defaultComponentId,_weightMiddlePointParamName)->setRawValue(_preWeightMiddlePointParamValue);
        getParameterFact(FactSystem::defaultComponentId,_weightFullPointParamName)->setRawValue(_preWeightFullPointParamValue);
        getParameterFact(FactSystem::defaultComponentId, _weightMiddleLitersParamName)->setRawValue(_preWeightMiddleLitersParamValue);
    }
}
