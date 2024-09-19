#pragma once

#include <QObject>
#include <QQmlContext>
#include <QQuickItem>

#include <Vehicle.h>
#include <FactPanelController.h>
#include <QGCLoggingCategory.h>

Q_DECLARE_LOGGING_CATEGORY(APMSprayCalibrationControllerLog)

class APMSprayCalibrationController : public FactPanelController
{
    Q_OBJECT
public:
    //Constructor
    APMSprayCalibrationController(void);
    ~APMSprayCalibrationController();

    Q_PROPERTY(QQuickItem* nextButton               MEMBER _nextButton)
    Q_PROPERTY(QQuickItem* cancelButton             MEMBER _cancelButton)
    Q_PROPERTY(QQuickItem* calibrateHelpText        MEMBER _calibrateHelpText)
    Q_PROPERTY(QQuickItem* startButton              MEMBER _startButton)
    Q_PROPERTY(QQuickItem* measuredTextField        MEMBER _measuredTextField)
    Q_PROPERTY(bool calibFirstStepInProgress        MEMBER _calibFirstStepInProgress    NOTIFY calibProcessStepInProgressChanged)
    Q_PROPERTY(bool calibSecondStepInProgress       MEMBER _calibSecondStepInProgress   NOTIFY calibProcessStepInProgressChanged)
    Q_PROPERTY(bool calibThirdStepInProgresss       MEMBER _calibThirdStepInProgress    NOTIFY calibProcessStepInProgressChanged)

    Q_PROPERTY(bool calibFirstStepDone              MEMBER _calibFirstStepDone          NOTIFY calibProcessStepDoneChanged)
    Q_PROPERTY(bool calibSecondStepDone             MEMBER _calibSecondStepDone         NOTIFY calibProcessStepDoneChanged)
    Q_PROPERTY(bool calibThirdStepDone              MEMBER _calibThirdStepDone          NOTIFY calibProcessStepDoneChanged)

    Q_PROPERTY(bool isValidMeasuredValue            MEMBER _isValidMeasuredValue        NOTIFY isValidMeasuredValueChanged)
    Q_PROPERTY(bool isTextFieldValueEdited          MEMBER _isTextFieldValueEdited      NOTIFY isTextFieldValueEditedChanged)

    Q_INVOKABLE void calibrateTank(void);
    Q_INVOKABLE void nextButtonClicked(void);
    Q_INVOKABLE void cancelCalibration(void);
    Q_INVOKABLE void saveMeasuredParam(float value);

    typedef enum {
        CalTypeTank
    } cal_type_t;
    Q_ENUM(cal_type_t)

signals:
    void calibProcessStepInProgressChanged(void);
    void calibProcessStepDoneChanged(void);
    void calibCompleteSign(cal_type_t type);
    void isValidMeasuredValueChanged(void);
    void isTextFieldValueEditedChanged(void);

private slots:
    void _mavlinkMessageReceived(LinkInterface *link, mavlink_message_t message);

protected:
private:
    enum CalibProgressCode : uint8_t {
        CalibProgressZero   = 0,
        CalibProgressMiddle = 1,
        CalibProgressFull   = 2,
        CalibProgressFinish = 3
    };

    void _startCalibration          (CalibProgressCode code);
    void _stopCalibration           (void);
    void _refreshParams             (void);
    void _handleCommandAck          (mavlink_message_t& message);
    void _restoreCalibrationParams  (void);

    QQuickItem* _nextButton;
    QQuickItem* _cancelButton;
    QQuickItem* _calibrateHelpText;
    QQuickItem* _startButton;
    QQuickItem* _measuredTextField;

    cal_type_t _type;
    CalibProgressCode _calibProgressState;

    bool _calibFirstStepInProgress;
    bool _calibSecondStepInProgress;
    bool _calibThirdStepInProgress;
    bool _calibFirstStepDone;
    bool _calibSecondStepDone;
    bool _calibThirdStepDone;
    bool _isValidMeasuredValue;
    bool _isTextFieldValueEdited;
    bool _restoreParamFlags;

    static const char* _weightZeroPointParamName;
    static const char* _weightMiddlePointParamName;
    static const char* _weightFullPointParamName;
    static const char* _weightMiddleLitersParamName;
    static const char* _sprayFactGroupName;

    float _preWeightZeroPointParamValue;
    float _preWeightMiddlePointParamValue;
    float _preWeightFullPointParamValue;
    float _preWeightMiddleLitersParamValue;

};
