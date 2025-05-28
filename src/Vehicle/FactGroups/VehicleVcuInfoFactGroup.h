#pragma once

#include <FactGroup.h>
#include <QGCMAVLink.h>

class VehicleVcuInfoFactGroup : public FactGroup
{
    Q_OBJECT
public:
    //Constructor
    VehicleVcuInfoFactGroup(QObject *parent = nullptr);

    //Derive class properties
    Q_PROPERTY(Fact* coolantWaterInletTempChannel            READ coolantWaterInletTempChannel            CONSTANT)
    Q_PROPERTY(Fact* coolantWaterOutletTempChannel           READ coolantWaterOutletTempChannel           CONSTANT)
    Q_PROPERTY(Fact* engineCabinTempChannel     READ engineCabinTempChannel     CONSTANT)
    Q_PROPERTY(Fact* fuelCabinTempChannel    READ fuelCabinTempChannel    CONSTANT)
    Q_PROPERTY(Fact* exhaustPipeTempChannel          READ exhaustPipeTempChannel          CONSTANT)
    Q_PROPERTY(Fact* thermoErrorMask         READ thermoErrorMask         CONSTANT)
    Q_PROPERTY(Fact* throttlePercent               READ throttlePercent               CONSTANT)
    Q_PROPERTY(Fact* steeringAngle              READ steeringAngle              CONSTANT)
    Q_PROPERTY(Fact* fuelTankPct                    READ fuelTankPct                    CONSTANT)
    Q_PROPERTY(Fact* contactorState                 READ contactorState                 CONSTANT)
    Q_PROPERTY(Fact* cameraPanPosition                       READ cameraPanPosition                       CONSTANT)
    Q_PROPERTY(Fact* cameraTiltPosition                      READ cameraTiltPosition                      CONSTANT)
    Q_PROPERTY(Fact* cameraZoomPosition                       READ cameraZoomPosition                       CONSTANT)
    Q_PROPERTY(Fact* cameraDeltaYaw                      READ cameraDeltaYaw                      CONSTANT)

    Fact* coolantWaterInletTempChannel               () { return &_coolantWaterInletTempChannelFact; }
    Fact* coolantWaterOutletTempChannel              () { return &_coolantWaterOutletTempChannelFact; }
    Fact* engineCabinTempChannel        () { return &_engineCabinTempChannelFact; }
    Fact* fuelCabinTempChannel       () { return & _fuelCabinTempChannelFact; }
    Fact* exhaustPipeTempChannel             () { return &_exhaustPipeTempChannelFact; }
    Fact* thermoErrorMask            () { return &_thermoErrorMaskFact; }
    Fact* throttlePercent                  () { return &_throttlePercentFact; }
    Fact* steeringAngle                 () { return &_steeringAngleFact; }
    Fact* fuelTankPct                       () { return &_fuelTankPctFact; }
    Fact* contactorState                    () { return &_contactorStateFact; }
    Fact* cameraPanPosition                          () { return &_cameraPanPositionFact; }
    Fact* cameraTiltPosition                         () { return &_cameraTiltPositionFact; }
    Fact* cameraZoomPosition                          () { return &_cameraZoomPositionFact; }
    Fact* cameraDeltaYaw                         () { return &_cameraDeltaYawFact; }

    //Overrides from FactGroup. Call by vehicle factgroup class
    void handleMessage(Vehicle *vehicle, const mavlink_message_t &message) override;

    static const char* _coolantWaterInletTempChannelFactName;
    static const char* _coolantWaterOutletTempChannelFactName;
    static const char* _engineCabinTempChannelFactName;
    static const char* _fuelCabinTempChannelFactName;
    static const char* _exhaustPipeTempChannelFactName;
    static const char* _thermoErrorMaskFactName;
    static const char* _throttlePercentFactName;
    static const char* _steeringAngleFactName;
    static const char* _fuelTankPctFactName;
    static const char* _contactorStateFactName;
    static const char* _cameraPanPositionFactName;
    static const char* _cameraTiltPositionFactName;
    static const char* _cameraZoomPositionFactName;
    static const char* _cameraDeltaYawFactName;

protected:
    //Handle vcu telemetry data
    void _handleVcuInfo(const mavlink_message_t &msg);

    //Handle camera telemetry data
    void _handleCameraInfo(const mavlink_message_t &msg);

    //Fact variables define
    Fact _coolantWaterInletTempChannelFact;
    Fact _coolantWaterOutletTempChannelFact;
    Fact _engineCabinTempChannelFact;
    Fact _fuelCabinTempChannelFact;
    Fact _exhaustPipeTempChannelFact;
    Fact _thermoErrorMaskFact;
    Fact _throttlePercentFact;
    Fact _steeringAngleFact;
    Fact _fuelTankPctFact;
    Fact _contactorStateFact;
    Fact _cameraPanPositionFact;
    Fact _cameraTiltPositionFact;
    Fact _cameraZoomPositionFact;
    Fact _cameraDeltaYawFact;
};

