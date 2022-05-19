#include <QPointF>

#include "QGCApplication.h"
#include "PlanMasterController.h"
#include "TrajectoryPoints.h"
#include "MissionModel.h"
#include "QGCGeo.h"

MissionModel::MissionModel(int startSeqNum, MAV_FRAME mavFrame, QObject* missionItemParent, QObject* parent)
    : QObject            (parent),
      _startSeqNum       (startSeqNum),
      _mavFrame          (mavFrame),
      _missionItemParent (missionItemParent),
      _trimStart         (0.0),
      _trimEnd           (0.0),
      _trimResume        (0.0),
      _resumeCoord       (),
      _useResumeCoord    (false)
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

int MissionModel::endSeqNum() const
{
    int result = 0;
    for (Step* step: _steps) {
        if (step->type() == Step::Type::WAYPOINT) {
            result += 1;
        } else if (step->type() == Step::Type::SPRAY) {
            result += 1; // FIXME: WRONG HERE, we don't know before gen
        } else if (step->type() == Step::Type::HOLD_YAW) {
            result += 2;
        }
    }
    return result;
}

void MissionModel::pregenIntegrate()
{
    if (_integrity) return;

    _gen_PointF_From_Geo();
    _integrateFences();
    qDebug() << "after fences" << _steps.count();
    qDebug() << "after fences" << _steps;
    _gen_Geo_From_PointF();
    _integrateTrim();
    qDebug() << "after trim" << _steps.count();
    _gen_Geo_From_PointF();

    _integrity = true;
}

void MissionModel::postgenIntegrate()
{
    _integrateSequenceNumber();
}

QList<MissionItem*> MissionModel::generateItems()
{
    qDebug() << "before pregen" << _steps.count();

    pregenIntegrate();
    _clearHookedItems();

    qDebug() << "after pregen" << _steps.count();

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

            // hold at position for 2 sec
            QGeoCoordinate coordinate = qobject_cast<WaypointStep*>(step)->coord;
            MissionItem* holdingWaypointItem = new MissionItem(
                        0,   // set it later
                        MAV_CMD_NAV_WAYPOINT,
                        _mavFrame,
                        2.0, // hold time
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

            MissionItem* anotherChangeYawItem = new MissionItem(
                        0,   // set it later
                        MAV_CMD_CONDITION_YAW,
                        _mavFrame,
                        yaw,
                        0.0, 0.0, 0.0, 0.0, 0.0, 0.0,                // empty
                        true,                                        // autoContinue
                        false,                                       // isCurrentItem
                        _missionItemParent
            );
            step->items.append(anotherChangeYawItem);

            toChange = false;
        }
    }
}

void MissionModel::_processSprays()
{
    bool spraying = false;
    bool passed = false;
    Step* lastWaypoint = nullptr;

    int max = 1750;
    int min  = 1050;
    double ratio = (max - min) / 100;
    int pwm = min + (int) (_centrifugalRPM * ratio);

    // find out the correct firstWaypoint
    Step* firstWaypoint = nullptr;
    for (int i=0; i<_steps.count(); i++) {
        if (_steps[i]->type() == Step::Type::WAYPOINT) {
            firstWaypoint = _steps[i];
            break;
        }
    }
    MissionItem* setServoItem = new MissionItem(0,   // set it later
                                                MAV_CMD_DO_SET_SERVO, // MAV_CMD_DO_SET_SERVO,
                                                _mavFrame,
                                                9, // servo 9
                                                pwm, 0.0, 0.0, 0.0, 0.0, 0.0,           // empty
                                                true,                                        // autoContinue
                                                false,                                       // isCurrentItem
                                                _missionItemParent);
    if (firstWaypoint) firstWaypoint->items.append(setServoItem);

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

    // find out the correct lastWaypoint
    // the current value should be ok, but we go on to ensure it
    for (int i=_steps.count()-1; i>=0; i--) {
        if (_steps[i]->type() == Step::Type::WAYPOINT) {
            lastWaypoint = _steps[i];
            break;
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
    if (lastWaypoint) lastWaypoint->items.append(disableItem);

    setServoItem = new MissionItem(0,   // set it later
                                   MAV_CMD_DO_SET_SERVO, // MAV_CMD_DO_SET_SERVO,
                                   _mavFrame,
                                   9, // servo 9
                                   min, 0.0, 0.0, 0.0, 0.0, 0.0,           // empty
                                   true,                                        // autoContinue
                                   false,                                       // isCurrentItem
                                   _missionItemParent);
    _steps[_steps.count() - 1]->items.append(setServoItem);
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
                qDebug() << "modify altitude";
                item->setParam7(altitude);
            }
        }
    }
}

double MissionModel::sprayLength() const
{
    double result = 0.0;
    WaypointStep* prev;
    bool isFirst = true;
    bool spraying = false;
    for (Step* step: _steps) {
        if (step->type() == Step::Type::WAYPOINT) {
            if (isFirst) {
                prev = qobject_cast<WaypointStep*>(step);
                isFirst = false;
            } else {
                WaypointStep* curr = qobject_cast<WaypointStep*>(step);
                if (spraying) result += prev->coord.distanceTo(curr->coord);
                prev = curr;
                spraying = false;
            }
        } else if (step->type() == Step::Type::SPRAY) {
            spraying = true;
        }
    }
    return result;
}

double MissionModel::_totalLength() const
{
    double result = 0.0;
    WaypointStep* prev;
    bool isFirst = true;
    for (Step* step: _steps) {
        if (step->type() == Step::Type::WAYPOINT) {
            if (isFirst) {
                prev = qobject_cast<WaypointStep*>(step);
                isFirst = false;
            } else {
                WaypointStep* curr = qobject_cast<WaypointStep*>(step);
                result += prev->coord.distanceTo(curr->coord);
                prev = curr;
            }
        }
    }
    return result;
}

void MissionModel::_integrateTrim()
{
    const double total = _totalLength();
    if (_trimStart + _trimEnd > 100) _trimEnd = 99.9 - _trimStart;
    double trimStartAtMeter = _trimStart/100.0*total;
    if (!_useResumeCoord) trimStartAtMeter = qMax(trimStartAtMeter, _trimResume/100.0*total);
    double trimEndAtMeter = (100-_trimEnd)/100.0*total;

    if (!_useResumeCoord) qDebug() << "trim resume used by model" << _trimResume;

//    TrajectoryPoints* trajectoryPoints = qgcApp()->toolbox()->planMasterControllerFlyView()->managerVehicle()->property("trajectoryPoints").value<TrajectoryPoints*>();

    double length = 0.0;
    WaypointStep* prev;
    bool isFirst = true;
    bool spraying = false;
    bool trimmedStart = false;
    bool trimmedEnd = false;
    QList<Step*> output;
    bool doFindResumePoint = _useResumeCoord && _resumeCoord.isValid();
    bool passedResume = !doFindResumePoint;
    qDebug() << _steps;
    for (Step* step: _steps) {
        if (step->type() == Step::Type::WAYPOINT) {
            if (isFirst) {
                prev = qobject_cast<WaypointStep*>(step);
                isFirst = false;
            } else {
                WaypointStep* curr = qobject_cast<WaypointStep*>(step);
                double prevLength = length;
                length += prev->coord.distanceTo(curr->coord);


                if (length > trimStartAtMeter) {
                    double azimuth = prev->coord.azimuthTo(curr->coord);
                    if (!trimmedStart) {
                        double distance = trimStartAtMeter - prevLength;
                        QGeoCoordinate newCoord = prev->coord.atDistanceAndAzimuth(distance, azimuth);
                        output << new WaypointStep(newCoord);
                        trimmedStart = true;
                    }
                    if (length < trimEndAtMeter) {
                        if (!passedResume) {
//                            trajectoryPoints->pointAddedFromfile(prev->coord);

                            QLineF flyLine(prev->pointf, curr->pointf);
                            QGeoCoordinate testFirst = _resumeCoord.atDistanceAndAzimuth(_avoidDistance, azimuth+90);
                            double y, x, down;
                            convertGeoToNed(testFirst, _tangentOrigin, &y, &x, &down);
                            QPointF  testFirstF(x, -y);

                            QGeoCoordinate testSecond = _resumeCoord.atDistanceAndAzimuth(_avoidDistance, azimuth-90);
                            convertGeoToNed(testSecond, _tangentOrigin, &y, &x, &down);
                            QPointF  testSecondF(x, -y);

                            QLineF testLine(testFirstF, testSecondF);
                            QPointF intersectPoint;
                            auto result = testLine.intersects(flyLine, &intersectPoint);

                            if (result == QLineF::BoundedIntersection) {
                                //assert(output[0]->type() == Step::Type::HOLD_YAW);
                                //assert(output[1]->type() == Step::Type::WAYPOINT);
                                output.takeAt(1); // WORKAROUND: remove first waypoint - second step
                                QGeoCoordinate intersectCoord;
                                convertNedToGeo(-intersectPoint.y(), intersectPoint.x(), prev->nedDown, _tangentOrigin, &intersectCoord);
                                output << new WaypointStep(intersectCoord);
//                                trajectoryPoints->pointAddedFromfile(intersectCoord);
                                passedResume = true;
                                _trimResume = (prevLength + prev->coord.distanceTo(intersectCoord))/total*100.0;
                                qDebug() << "trim resume gen by model" << _trimResume;
                            }
                        }

                        if (passedResume) {
                            if (spraying) output << new SprayStep();
                            output << step;
                        }
                    } else if (!trimmedEnd) {
                        double distance = trimEndAtMeter - prevLength;
                        double azimuth = prev->coord.azimuthTo(curr->coord);
                        QGeoCoordinate newCoord = prev->coord.atDistanceAndAzimuth(distance, azimuth);
                        if (spraying) output << new SprayStep();
                        output << new WaypointStep(newCoord);
                        trimmedEnd = true;
                    }
                }
                prev = curr;
                spraying = false;
            }
        } else if (step->type() == Step::Type::SPRAY) {
            spraying = true;
        } else {
            output << step;
        }
    }

    _steps.clear();
    _steps << output;
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
                    QPointF pointf(0, 0);
                    waypoint->pointf = pointf;
                    waypoint->nedDown = 0;

                    isFirst = false;
                } else {
                    double y, x, down;
                    convertGeoToNed(waypoint->coord, _tangentOrigin, &y, &x, &down);
                    QPointF  pointf(x, -y);
                    waypoint->pointf = pointf;
                    waypoint->nedDown = down;
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
                convertNedToGeo(-waypoint->pointf.y(), waypoint->pointf.x(), waypoint->nedDown, _tangentOrigin, &(waypoint->coord));
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

void MissionModel::backup()
{
    _backupSteps.clear();
    for (Step* step: _steps) {
        if (step->type() == Step::Type::WAYPOINT)           _backupSteps << new WaypointStep(*qobject_cast<WaypointStep*>(step));
        else if (step->type() == Step::Type::SPRAY)         _backupSteps << new SprayStep();
        else if (step->type() == Step::Type::HOLD_YAW)      _backupSteps << new HoldYawStep(*qobject_cast<HoldYawStep*>(step));
        else if (step->type() == Step::Type::HOLD_ALTITUDE) _backupSteps << new HoldAltitudeStep(*qobject_cast<HoldAltitudeStep*>(step));
    }
}

void MissionModel::restore()
{
    _steps.clear();
    _steps << _backupSteps;
}

WaypointStep::WaypointStep(WaypointStep& other) {
    coord = QGeoCoordinate(other.coord);
    pointf = QPointF(other.pointf);
    nedDown = other.nedDown;
}

HoldYawStep::HoldYawStep(HoldYawStep& other) {
    yaw = other.yaw;
}

HoldAltitudeStep::HoldAltitudeStep(HoldAltitudeStep& other) {
    altitude = other.altitude;
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
