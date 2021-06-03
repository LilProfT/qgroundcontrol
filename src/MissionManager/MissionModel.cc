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

void proceedAvoidance(const QPointF& vertex, QList<Step*>& steps, qreal* avoidDistance) {
    QLineF line(qobject_cast<WaypointStep*>(steps.last())->pointf, vertex);
    *avoidDistance += line.length();
    steps.append(new WaypointStep(vertex));
}

void MissionModel::_integrateFences()
{
    qDebug() << "pre fence: " << *this;

    QList<Step*> inputSteps;
    inputSteps << _steps;
    QList<Step*> outputSteps;
    for (int i=0; i < _fences->count(); i++) {
        QGCFencePolygon* fence = qobject_cast<QGCFencePolygon*>(_fences->get(i));
        if (fence->inclusion()) continue;
        fence->verifyClockwiseWinding();

        QPolygonF extendedFence;
        // [COPY `QGCMapPolygon::offset`]
        // but don't modified this polygon like the original
        // we just need QPolygonF to use here
        // yep, QPolygonF, not the fucking nedPolygon, they are different

        // >>>>>>>> DIFF HERE <<<<<<<<<
        QPolygonF rgNedVertices = fence->toPolygonF();
        // >>>>>>>> DIFF HERE <<<<<<<<<

        // Walk the edges, offsetting by the specified distance
        QList<QLineF> rgOffsetEdges;
        for (int i=0; i<rgNedVertices.count(); i++) {
            int     lastIndex = i == rgNedVertices.count() - 1 ? 0 : i + 1;
            QLineF  offsetEdge;
            QLineF  originalEdge(rgNedVertices[i], rgNedVertices[lastIndex]);

            QLineF workerLine = originalEdge;
            workerLine.setLength(distance);
            workerLine.setAngle(workerLine.angle() + 90.0);
            offsetEdge.setP1(workerLine.p2());

            workerLine.setPoints(originalEdge.p2(), originalEdge.p1());
            workerLine.setLength(distance);
            workerLine.setAngle(workerLine.angle() - 90.0);
            offsetEdge.setP2(workerLine.p2());

            rgOffsetEdges.append(offsetEdge);
        }

        // Intersect the offset edges to generate new vertices
        QPointF         newVertex;
        QGeoCoordinate  tangentOrigin = fence->vertexCoordinate(0);
        for (int i=0; i<rgOffsetEdges.count(); i++) {
            int prevIndex = i == 0 ? rgOffsetEdges.count() - 1 : i - 1;
    #if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
            auto intersect = rgOffsetEdges[prevIndex].intersect(rgOffsetEdges[i], &newVertex);
    #else
            auto intersect = rgOffsetEdges[prevIndex].intersects(rgOffsetEdges[i], &newVertex);
    #endif
            if (intersect == QLineF::NoIntersection) {
                // FIXME: Better error handling?
                qWarning("Intersection failed");
            }
            // >>>>>>>> DIFF HERE <<<<<<<<<
            extendedFence.append(newVertex);
            // >>>>>>>> DIFF HERE <<<<<<<<<
        };
        // [/COPY]

        // TODO if extendedFence contain first and last waypoint, it is invalid

        QPointF currentPoint, previousPoint;
        bool isFirst = true;
        bool spraying = false;
        bool sprayBefore = false;
        bool sprayAfter = false;
        bool modifying = false; // just 2 states
        QPointF startFollowPoint;
        int startFollowEdgeIndex; // the edge that intersects with our path line

        int previousPointIndex;

        // the list change while looping, so don't calc length beforehand
        //                      vvvvvvv
        for (int i=0; i<inputSteps.count(); i++) {
            //   ^
            // the index change too, you can see it with
            // qDebug() << i;

            Step* step = inputSteps[i];
            if (step->type() != Step::Type::WAYPOINT) {
                if (step->type() != Step::Type::SPRAY) {
                    outputSteps.append(step);
                } else {
                    spraying = true;
                }
                continue;
            }
            // now we are sure it's waypoint
            WaypointStep* waypoint = qobject_cast<WaypointStep*>(step);

            if (isFirst) {
                outputSteps.append(step);
                isFirst = false;
                previousPointIndex = i;
                continue;
            }

            currentPoint = waypoint->pointf;
            previousPoint = qobject_cast<WaypointStep*>(inputSteps[previousPointIndex])->pointf;

            if (!modifying) {
                if (!extendedFence.containsPoint(currentPoint, Qt::OddEvenFill)) {
                    // whether the line cross the polygon ?
                    // ATTENTION convex polygon please
                    QLineF pathLine(previousPoint, currentPoint);
                    QPointF first;
                    QPointF second;
                    for (int j=1; j<=extendedFence.count(); j++) {
                        int j_ = (j == extendedFence.count()) ? 0 : j; // handle last edge (count - 1, 0)
                        QLineF edge(extendedFence[j-1], extendedFence[j_]);
                        QPointF intersectPoint;
                        if (pathLine.intersect(edge, &intersectPoint) == QLineF::BoundedIntersection) {
                            if (first.isNull()) {
                                first = intersectPoint;
                                continue;
                            };
                            if (second.isNull()) {
                                second = intersectPoint;
                                continue;
                            };
                            break;
                        };
                    };

                    if ((!first.isNull()) && (!second.isNull())) {
                        // got the start and end following point
                        // we want to use the code below for the case of inbound point
                        // so let add a fake inbound point in between
                        QLineF cross(first, second);
                        QPointF fakePoint = cross.center();
                        Step* fakeWaypoint = new WaypointStep(fakePoint);
                        inputSteps.insert(i, fakeWaypoint);

                        sprayAfter = spraying;

                        // hold spraying state for fake point, don't set to false
                        // hold index to process fakePoint
                        i--;
                        continue;
                    };

                    // the point is already valid and reachable
                    // so don't modified
                    if (spraying) outputSteps.append(new SprayStep());
                    outputSteps.append(step);
                    previousPointIndex = i;
                    spraying = false;
                    continue;
                } else {
                    // start following
                    modifying = true;
                    // find intersected edge and point
                    QLineF pathLine(previousPoint, currentPoint);
                    for (int j=1; j<=extendedFence.count(); j++) {
                        int j_ = (j == extendedFence.count()) ? 0 : j; // handle last edge (count - 1, 0)
                        QLineF edge(extendedFence[j-1], extendedFence[j_]);
                        QPointF intersectPoint;
                        if (pathLine.intersect(edge, &intersectPoint) == QLineF::BoundedIntersection) {
                            startFollowPoint = intersectPoint;
                            startFollowEdgeIndex = j_;
                            break;
                        };
                    };

                    previousPointIndex = i;
                    sprayBefore = spraying;
                };
            } else { // following here
                // of course don't collect inbound path point
                if (extendedFence.containsPoint(currentPoint, Qt::OddEvenFill)) {
                    previousPointIndex = i;
                    spraying = false;
                    continue;
                }

                // but wait until we exit
                // end following
                modifying = false;

                // find intersected edge and point
                QPointF endFollowPoint;
                int endFollowEdgeIndex;
                previousPoint = qobject_cast<WaypointStep*>(inputSteps[previousPointIndex])->pointf;
                QLineF pathLine(previousPoint, currentPoint);
                for (int j=1; j<=extendedFence.count(); j++) {
                    int j_ = (j == extendedFence.count()) ? 0 : j; // handle last edge (count - 1, 0)
                    QLineF edge(extendedFence[j-1], extendedFence[j_]);
                    QPointF intersectPoint;
                    if (pathLine.intersect(edge, &intersectPoint) == QLineF::BoundedIntersection) {
                        endFollowPoint = intersectPoint;
                        endFollowEdgeIndex = j_;
                        break;
                    };
                };
                sprayAfter = sprayAfter || spraying;

                QList<Step*> clockwisePath;
                qreal clockwiseAvoidDistance = 0;
                QList<Step*> invClockwisePath;
                qreal invClockwiseAvoidDistance = 0;

                // now collecting
                {
                    // clockwise: collect start vertex but not end vertex
                    clockwisePath.append(new WaypointStep(startFollowPoint));

                    // 2 phase mean passing index 0
                    bool twoPhase = (startFollowEdgeIndex > endFollowEdgeIndex);
                    if (twoPhase) {
                        for (int j=startFollowEdgeIndex; j<extendedFence.count(); j++)
                        {
                            proceedAvoidance(extendedFence[j], clockwisePath, &clockwiseAvoidDistance);
                        }
                        for (int j=0; j<endFollowEdgeIndex; j++)
                        {
                            proceedAvoidance(extendedFence[j], clockwisePath, &clockwiseAvoidDistance);
                        }
                    } else {
                        for (int j=startFollowEdgeIndex; j<endFollowEdgeIndex; j++)
                        {
                            proceedAvoidance(extendedFence[j], clockwisePath, &clockwiseAvoidDistance);
                        }
                    }

                    clockwisePath.append(new WaypointStep(endFollowPoint));
                    QLineF line(qobject_cast<WaypointStep*>(clockwisePath.last())->pointf, endFollowPoint);
                    clockwiseAvoidDistance += line.length();
                    qDebug() << "clockwiseAvoidDistance: " << clockwiseAvoidDistance;
                };
                {
                    // inverse clockwise: collect end vertex but not start vertex
                    invClockwisePath.append(new WaypointStep(startFollowPoint));

                    bool twoPhase = (startFollowEdgeIndex < endFollowEdgeIndex);
                    if (twoPhase) {
                        for (int j=startFollowEdgeIndex-1; j>=0; j--)
                        {
                            proceedAvoidance(extendedFence[j], invClockwisePath, &invClockwiseAvoidDistance);
                        }
                        for (int j=extendedFence.count()-1; j>=endFollowEdgeIndex; j--)
                        {
                            proceedAvoidance(extendedFence[j], invClockwisePath, &invClockwiseAvoidDistance);
                        }
                    } else {
                        for (int j=startFollowEdgeIndex-1; j>=endFollowEdgeIndex; j--)
                        {
                            proceedAvoidance(extendedFence[j], invClockwisePath, &invClockwiseAvoidDistance);
                        }
                    }

                    invClockwisePath.append(new WaypointStep(endFollowPoint));
                    QLineF line(qobject_cast<WaypointStep*>(invClockwisePath.last())->pointf, endFollowPoint);
                    invClockwiseAvoidDistance += line.length();
                    qDebug() << "invClockwiseAvoidDistance: " << invClockwiseAvoidDistance;
                };

                if (sprayBefore) {
                    outputSteps.append(new SprayStep());
                    sprayBefore = false;
                }

                bool clockwise = clockwiseAvoidDistance <= invClockwiseAvoidDistance;
                outputSteps << (clockwise ? clockwisePath : invClockwisePath);

                if (sprayAfter) {
                    outputSteps.append(new SprayStep());
                    sprayAfter = false;
                }

                outputSteps.append(step);
            }

            spraying = false;
            previousPointIndex = i;
        }

        inputSteps.clear();
        inputSteps << outputSteps;
        outputSteps.clear();
    }

    qDebug() << "post fence: " << *this;
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
        result.append(step->items);
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

QDebug operator<<(QDebug debug, const MissionModel &model)
{
    debug << "MissionModel: ";
    for (Step* step: model.steps()) {
        debug << *step;
    }
    return debug;
}
