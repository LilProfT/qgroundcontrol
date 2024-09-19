#pragma once

#include <FactGroup.h>
#include <QGCMAVLink.h>

class VehicleSprayInfoFactGroup : public FactGroup
{
    Q_OBJECT
public:
    //Constructor
    VehicleSprayInfoFactGroup(QObject *parent = nullptr);

    //Derive class properties
    Q_PROPERTY(Fact* pumpRPMFirstChannel            READ pumpRPMFirstChannel            CONSTANT)
    Q_PROPERTY(Fact* pumpRPMSecondChannel           READ pumpRPMSecondChannel           CONSTANT)
    Q_PROPERTY(Fact* centrifugalRPMFirstChannel     READ centrifugalRPMFirstChannel     CONSTANT)
    Q_PROPERTY(Fact* centrifugalRPMSecondChannel    READ centrifugalRPMSecondChannel    CONSTANT)
    Q_PROPERTY(Fact* flowspeedFirstChannel          READ flowspeedFirstChannel          CONSTANT)
    Q_PROPERTY(Fact* flowspeedSecondChannel         READ flowspeedSecondChannel         CONSTANT)
    Q_PROPERTY(Fact* sVolFirstChannel               READ sVolFirstChannel               CONSTANT)
    Q_PROPERTY(Fact* sVolSecondChannel              READ sVolSecondChannel              CONSTANT)
    Q_PROPERTY(Fact* fuelTankPct                    READ fuelTankPct                    CONSTANT)
    Q_PROPERTY(Fact* weightADCValue                 READ weightADCValue                 CONSTANT)
    Q_PROPERTY(Fact* armFirst                       READ armFirst                       CONSTANT)
    Q_PROPERTY(Fact* armSecond                      READ armSecond                      CONSTANT)
    Q_PROPERTY(Fact* armThird                       READ armThird                       CONSTANT)
    Q_PROPERTY(Fact* armFourth                      READ armFourth                      CONSTANT)

    Fact* pumpRPMFirstChannel               () { return &_pumpRPMFirstChannelFact; }
    Fact* pumpRPMSecondChannel              () { return &_pumpRPMSecondChannelFact; }
    Fact* centrifugalRPMFirstChannel        () { return &_centrifugalRPMFirstChannelFact; }
    Fact* centrifugalRPMSecondChannel       () { return & _centrifugalRPMSecondChannelFact; }
    Fact* flowspeedFirstChannel             () { return &_flowspeedFirstChannelFact; }
    Fact* flowspeedSecondChannel            () { return &_flowspeedSecondChannelFact; }
    Fact* sVolFirstChannel                  () { return &_sVolFirstChannelFact; }
    Fact* sVolSecondChannel                 () { return &_sVolSecondChannelFact; }
    Fact* fuelTankPct                       () { return &_fuelTankPctFact; }
    Fact* weightADCValue                    () { return &_weightADCValueFact; }
    Fact* armFirst                          () { return &_armFirstFact; }
    Fact* armSecond                         () { return &_armSecondFact; }
    Fact* armThird                          () { return &_armThirdFact; }
    Fact* armFourth                         () { return &_armFourthFact; }

    //Overrides from FactGroup. Call by vehicle factgroup class
    virtual void handleMessage(Vehicle* vehicle, mavlink_message_t& message);

    static const char* _pumpRPMFirstChannelFactName;
    static const char* _pumpRPMSecondChannelFactName;
    static const char* _centrifugalRPMFirstChannelFactName;
    static const char* _centrifugalRPMSecondChannelFactName;
    static const char* _flowspeedFirstChannelFactName;
    static const char* _flowspeedSecondChannelFactName;
    static const char* _sVolFirstChannelFactName;
    static const char* _sVolSecondChannelFactName;
    static const char* _fuelTankPctFactName;
    static const char* _weightADCValueFactName;
    static const char* _armFirstFactName;
    static const char* _armSecondFactName;
    static const char* _armThirdFactName;
    static const char* _armFourthFactName;

protected:
    //Handle spray telemetry data
    void _handleSprayInfo(mavlink_message_t &msg);

    //Fact variables define
    Fact _pumpRPMFirstChannelFact;
    Fact _pumpRPMSecondChannelFact;
    Fact _centrifugalRPMFirstChannelFact;
    Fact _centrifugalRPMSecondChannelFact;
    Fact _flowspeedFirstChannelFact;
    Fact _flowspeedSecondChannelFact;
    Fact _sVolFirstChannelFact;
    Fact _sVolSecondChannelFact;
    Fact _fuelTankPctFact;
    Fact _weightADCValueFact;
    Fact _armFirstFact;
    Fact _armSecondFact;
    Fact _armThirdFact;
    Fact _armFourthFact;
};

