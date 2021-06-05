#pragma once

#include <QObject>
#include <QVariant>
#include <QGeoCoordinate>
#include <QPointF>
#include <QDebug>

#include "MissionItem.h"
#include "QGCFencePolygon.h"

// Modelling the mission into synchronous steps
// synchronous mean: the previous step is always terminated before the next step start taking effect
// This help us develop a high-level view into the mission
// We will implement the Model by hooking up mission items on the steps and then flatten them all.

class Step : public QObject
{
    Q_OBJECT

public:
    enum Type {
        WAYPOINT,
        SPRAY,         // hold spray enabled between 2 step
        HOLD_YAW,      // hold yaw unchanged at a value until next HOLD_YAW
        HOLD_ALTITUDE, // same as HOLD_YAW, but for altitude
    };
    // NOTE: implementing limitation
    // HOLDs should be put before WAYPOINT
    // for ex:
    //     this: SPRAY HOLD_YAW HOLD_ALTITUDE WAYPOINT
    // not this: HOLD_YAW SPRAY HOLD_ALTITUDE WAYPOINT

    virtual Type type() const = 0;
    virtual QString repr() const = 0;

    QList<MissionItem*> items;
};
QDebug operator<<(QDebug dbg, const Step &step);

class WaypointStep : public Step
{
    Q_OBJECT
public:
    WaypointStep(QGeoCoordinate coord_) { coord = coord_; };
    WaypointStep(QPointF pointf_) { pointf = pointf_; };
    QGeoCoordinate coord;
    QPointF pointf;
    Type type() const override final { return Step::Type::WAYPOINT; };
    QString repr() const override final;
};

class SprayStep : public Step
{
    Q_OBJECT
public:
    Type type() const override final { return Step::Type::SPRAY; };
    QString repr() const override final;
};

// Note: make sure HOLD_ value attain the settle value before next step to
// fulfill the synchronous requirement.
class HoldYawStep : public Step
{
    Q_OBJECT
public:
    HoldYawStep(double yaw_) { yaw = yaw_; };
    double yaw;
    Type type() const override final { return Step::Type::HOLD_YAW; };
    QString repr() const override final;
};

class HoldAltitudeStep : public Step
{
    Q_OBJECT
public:
    HoldAltitudeStep(double altitude_) { altitude = altitude_; };
    double altitude;
    Type type() const override final { return Step::Type::HOLD_ALTITUDE; };
    QString repr() const override final;
};

class MissionModel : public QObject
{
    Q_OBJECT

public:
    MissionModel(int startSeqNum, MAV_FRAME mavFrame, QObject* missionItemParent, QObject* parent);

    // Model formation
    void clearStep();
    void appendWaypoint(QGeoCoordinate coord);
    void appendSpray();
    void appendHoldYaw(double yaw);
    void appendHoldAltitude(double altitude);

    // Integrity constraints
    void setExclusionFences(QmlObjectListModel* fences);
//    void setInclusionFences(QList<QGCFencePolygon> fences); // unimplemented
    void setStartSeqNum(int value) { _startSeqNum = value; };
    void setMissionItemParent(QObject* value) { _missionItemParent = value; };

    void pregenIntegrate();
    void postgenIntegrate();

    QList<MissionItem*> generateItems();

    const QList<Step*>& steps() const { return _steps; };

private:
    void _clearHookedItems();
    void _processWaypoints();
    void _processHoldYaws();
    void _processSprays();
    void _processHoldAltitudes();
    void _integrateFences();
    void _integrateSequenceNumber();

    void _gen_PointF_From_Geo();
    void _gen_Geo_From_PointF(); // must be called after _gen_PointF_From_Geo() so _tangentOrigin is available
    QList<MissionItem*> _flatten();

    QList<Step*> _steps;

    QmlObjectListModel* _fences;
    int _startSeqNum;
    MAV_FRAME _mavFrame;
    QObject* _missionItemParent;

    QGeoCoordinate _tangentOrigin;
    double distance = 3.0; // TODO gridSpacing / 2

    bool _integrity = true;
};
QDebug operator<<(QDebug dbg, const MissionModel &model);
