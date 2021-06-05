#include <QPointF>

#include "MissionModel.h"
#include "QGCGeo.h"

MissionModel::MissionModel(int startSeqNum, MAV_FRAME mavFrame, QObject* missionItemParent, QObject* parent)
    : QObject            (parent),
      _startSeqNum       (startSeqNum),
      _mavFrame          (mavFrame),
      _missionItemParent (missionItemParent)
{}

void MissionModel::clearStep() {
    _steps.clear();
}

void MissionModel::appendWaypoint(QGeoCoordinate coord) {
    Step* step = new WaypointStep(coord);
    _steps.append(step);
}

void MissionModel::appendSpray() {
    Step* step = new SprayStep();
    _steps.append(step);
}

void MissionModel::appendHoldYaw(double value) {
    Step* step = new HoldYawStep(value);
    _steps.append(step);
}

void MissionModel::appendHoldAltitude(double value) {
    Step* step = new HoldAltitudeStep(value);
    _steps.append(step);
}

void MissionModel::setExclusionFences(QmlObjectListModel* fences)
{
    _fences = fences;
    _integrity = false;
}

void MissionModel::pregenIntegrate()
{
    if (_integrity) return;

    _gen_PointF_From_Geo();
    _integrateFences();
    _gen_Geo_From_PointF();

    _integrity = true;
}

void MissionModel::postgenIntegrate()
{
    _integrateSequenceNumber();
}

QList<MissionItem*> MissionModel::generateItems()
{
    pregenIntegrate();
    _clearHookedItems();

    // The order of method calls is important
    _processWaypoints();
    _processHoldYaws();
    _processSprays();
    // stop generate new waypoint item here
    _processHoldAltitudes();

    postgenIntegrate();

    return _flatten();
}

void MissionModel::_clearHookedItems()
{
    for (Step* step: _steps) {
        step->items.clear();
    }
}

void MissionModel::_processWaypoints()
{
    for (Step* step: _steps) {
        if (step->type() == Step::Type::WAYPOINT) {
            QGeoCoordinate coordinate = qobject_cast<WaypointStep*>(step)->coord;
            MissionItem* item = new MissionItem(
                        0,   // set it later
                        MAV_CMD_NAV_WAYPOINT,
                        _mavFrame,
                        0.0, // hold time
                        0.0,                                         // No acceptance radius specified
                        0.0,                                         // Pass through waypoint
                        std::numeric_limits<double>::quiet_NaN(),    // Yaw unchanged
                        coordinate.latitude(),
                        coordinate.longitude(),
                        coordinate.altitude(),
                        true,                                        // autoContinue
                        false,                                       // isCurrentItem
                        _missionItemParent
            );
            step->items.append(item);
        }
    }
}

void MissionModel::_processHoldYaws()
{
    double yaw = qQNaN();
    bool toChange = false;

    for (Step* step: _steps) {
        if (step->type() == Step::Type::HOLD_YAW) {
            yaw = qobject_cast<HoldYawStep*>(step)->yaw;
            toChange = true;
        } else if ((step->type() == Step::Type::WAYPOINT) && toChange) {
            MissionItem* changeYawItem = new MissionItem(
                        0,   // set it later
                        MAV_CMD_CONDITION_YAW,
                        _mavFrame,
                        yaw,
                        0.0, 0.0, 0.0, 0.0, 0.0, 0.0,                // empty
                        true,                                        // autoContinue
                        false,                                       // isCurrentItem
                        _missionItemParent
            );
            step->items.append(changeYawItem);

            // hold at position for 5 sec, for yaw to finish changing
            QGeoCoordinate coordinate = qobject_cast<WaypointStep*>(step)->coord;
            MissionItem* holdingWaypointItem = new MissionItem(
                        0,   // set it later
                        MAV_CMD_NAV_WAYPOINT,
                        _mavFrame,
                        5.0, // hold time
                        0.0,                                         // No acceptance radius specified
                        0.0,                                         // Pass through waypoint
                        std::numeric_limits<double>::quiet_NaN(),    // Yaw unchanged
                        coordinate.latitude(),
                        coordinate.longitude(),
                        coordinate.altitude(),
                        true,                                        // autoContinue
                        false,                                       // isCurrentItem
                        _missionItemParent
            );
            step->items.append(holdingWaypointItem);
            toChange = false;
        }
    }
}

void MissionModel::_processSprays()
{
    bool spraying = false;
    bool passed = false;
    Step* lastWaypoint = nullptr;

    for (Step* step: _steps) {
        if ((!spraying) && (step->type() == Step::Type::SPRAY)) {
            MissionItem* enableItem = new MissionItem(0,   // set it later
                                                216, // MAV_CMD_DO_SPRAYER,
                                                _mavFrame,
                                                1.0, // enable
                                                0.0, 0.0, 0.0, 0.0, 0.0, 0.0,           // empty
                                                true,                                        // autoContinue
                                                false,                                       // isCurrentItem
                                                _missionItemParent);
            step->items.append(enableItem);
            spraying = true;
            passed = false;
        }

        if (spraying && passed && (step->type() == Step::Type::WAYPOINT)) {
            MissionItem* disableItem = new MissionItem(0,   // set it later
                                                216, // MAV_CMD_DO_SPRAYER,
                                                _mavFrame,
                                                0.0, // disable
                                                0.0, 0.0, 0.0, 0.0, 0.0, 0.0,           // empty
                                                true,                                        // autoContinue
                                                false,                                       // isCurrentItem
                                                _missionItemParent);
            lastWaypoint->items.append(disableItem);
            spraying = false;
        }

        if (spraying && (step->type() == Step::Type::WAYPOINT)) {
            lastWaypoint = step;
            passed = true;
        }

        if (spraying && (step->type() == Step::Type::SPRAY)) {
            passed = false;
        }
    }

    // disable after last waypoint
    MissionItem* disableItem = new MissionItem(0,   // set it later
                                        216, // MAV_CMD_DO_SPRAYER,
                                        _mavFrame,
                                        0.0, // disable
                                        0.0, 0.0, 0.0, 0.0, 0.0, 0.0,           // empty
                                        true,                                        // autoContinue
                                        false,                                       // isCurrentItem
                                        _missionItemParent);
    lastWaypoint->items.append(disableItem);
}

void MissionModel::_processHoldAltitudes()
{
    double altitude = qQNaN();
    for (Step* step: _steps) {
        if (step->type() == Step::Type::HOLD_ALTITUDE) {
            altitude = qobject_cast<HoldAltitudeStep*>(step)->altitude;
        }

        if (qIsNaN(altitude)) continue;

        for (MissionItem* item: step->items) {
            if (item->command() == MAV_CMD_NAV_WAYPOINT) {
                item->setParam7(altitude);
            }
        }
    }
}



void MissionModel::_integrateSequenceNumber()
{
    int seqNum = _startSeqNum;
    for (Step* step: _steps) {
        for (MissionItem* item: step->items) {
            item->setSequenceNumber(seqNum++);
        }
    }
}

void MissionModel::_gen_PointF_From_Geo()
{
    bool isFirst = true;
    for (Step* step: _steps) {
        if (step->type() == Step::Type::WAYPOINT) {
            WaypointStep* waypoint = qobject_cast<WaypointStep*>(step);
            if (waypoint->coord.isValid()) {
                if (isFirst) {
                    _tangentOrigin = waypoint->coord;
                    isFirst = false;
                } else {
                    double y, x, down;
                    convertGeoToNed(waypoint->coord, _tangentOrigin, &y, &x, &down);
                    QPointF  pointf(x, -y);
                    waypoint->pointf = pointf;
                }
            }
        }
    }
}

void MissionModel::_gen_Geo_From_PointF()
{
    for (Step* step: _steps) {
        if (step->type() == Step::Type::WAYPOINT) {
            WaypointStep* waypoint = qobject_cast<WaypointStep*>(step);
            if (!waypoint->pointf.isNull()) {
                convertNedToGeo(-waypoint->pointf.y(), waypoint->pointf.x(), 0, _tangentOrigin, &(waypoint->coord));
            }
        }
    }
}

QList<MissionItem*> MissionModel::_flatten()
{
    QList<MissionItem*> result;
    for (Step* step: _steps) {
        result << step->items;
    }
    return result;
}

QString WaypointStep::repr() const
{
    QString res("WAYPOINT");
    return res;
}

QString SprayStep::repr() const
{
    QString res("SPRAY");
    return res;
}

QString HoldYawStep::repr() const
{
    QString res("HOLD_YAW");
    return res;
}

QString HoldAltitudeStep::repr() const
{
    QString res("HOLD_ALTITUDE");
    return res;
}

QDebug operator<<(QDebug debug, const Step &step)
{
    QDebugStateSaver saver(debug);
    debug << step.repr();
    return debug;
}

QDebug operator<<(QDebug debug, const QList<Step*> &steps)
{
    QDebugStateSaver saver(debug);
    for (Step* step: steps) {
        debug << *step;
    }
    return debug;
}

QDebug operator<<(QDebug debug, const MissionModel &model)
{
    debug << "MissionModel: ";
    debug << model.steps();
    return debug;
}
