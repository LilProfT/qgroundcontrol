#include <QPointF>
#include <QLineF>
#include <QPolygonF>

#include "MissionModel.h"

typedef struct {
    QPointF point;
    float coord;
} IntersectInfo;
Q_DECLARE_METATYPE(IntersectInfo)

QList<IntersectInfo> _intersectLinePolygon(const QLineF& line, const QPolygonF& polygon);

typedef struct {
    float distance;
    QList<QPointF> path;
} AvoidanceInfo;
Q_DECLARE_METATYPE(AvoidanceInfo)

AvoidanceInfo proceedAvoidance(bool clockwise, const QPolygonF& extendedFence, const IntersectInfo& first, const IntersectInfo& second);

class Ring
{
public:
    Ring(int start, int stop) { _start = start; _stop = stop; _current = start; }
    int current() { return _current; }
    void set(int value) { _current = value; }
    virtual int next() = 0;
    virtual int after(float mid) = 0;
    virtual int before(float mid) = 0;
    void setAfter (float mid) { _current = after (mid); }
    void setBefore(float mid) { _current = before(mid); }
protected:
    int _start;
    int _stop;
    int _current;
};

class IncrementRing : public Ring
{
public:
    IncrementRing(int start, int stop)
        : Ring(start, stop)
    {}
    int next() final {
        if (_current == _stop) {
            _current = _start;
            return _stop;
        } else {
            return _current++;
        }
    }

    int after(float mid) final {
        if (mid > _stop) return _start;
        return qCeil(mid);
    }

    int before(float mid) final {
        return qFloor(mid);
    }
};

class DecrementRing : public Ring
{
public:
    DecrementRing(int start, int stop)
        : Ring(start, stop)
    {}
    int next() final {
        if (_current == _start) {
            _current = _stop;
            return _start;
        } else {
            return _current--;
        }
    }

    int after(float mid) final {
        return qFloor(mid);
    }

    int before(float mid) final {
        if (mid > _stop) return _start;
        return qCeil(mid);
    }
};

void MissionModel::_integrateFences()
{
//    qDebug() << "pre fence: " << *this;

    QList<Step*> inputSteps;
    inputSteps << _steps;
    QList<Step*> outputSteps;
    outputSteps << _steps;
    for (int i=0; i < _fences->count(); i++) {
        outputSteps.clear();

        QGCFencePolygon* fence = qobject_cast<QGCFencePolygon*>(_fences->get(i));
        if (fence->inclusion()) continue;
        fence->verifyClockwiseWinding();

        QPolygonF extendedFence;
        // [COPY `QGCMapPolygon::offset`]
        // but don't modified this polygon like the original
        // we just need QPolygonF to use here

        // >>>>>>>> DIFF HERE <<<<<<<<<
        QPolygonF rgNedVertices = fence->toPolygonF(_tangentOrigin);
        const double distance = _avoidDistance;
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
        // >>>>>>>> DIFF HERE <<<<<<<<<
        QGeoCoordinate  tangentOrigin = _tangentOrigin;
        // >>>>>>>> DIFF HERE <<<<<<<<<
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

        QList<int> waypointIndices;
        QList<int> sprayIndices;
        const int l = inputSteps.count();

        // filtering
        for (int i=0; i<l; i++) {
            Step* step = inputSteps[i];
            if      (step->type() == Step::Type::WAYPOINT) waypointIndices.append(i);
            else if (step->type() == Step::Type::SPRAY)    sprayIndices.append(i);
        }

        // test fence valid
        QPointF first = qobject_cast<WaypointStep*>(inputSteps[waypointIndices.first()])->pointf;
        QPointF last  = qobject_cast<WaypointStep*>(inputSteps[waypointIndices.last() ])->pointf;

        if (extendedFence.containsPoint(first, Qt::OddEvenFill) ||
            extendedFence.containsPoint(last , Qt::OddEvenFill))
        {
            qWarning() << "fence invalid, skip";
            outputSteps.clear();
            outputSteps << inputSteps;
            continue;
        }

        // traversing
        const int lw = waypointIndices.count();
        const int ls = sprayIndices.count();
        int w = 0; // index on waypointIndices
        int s = 0; // index on sprayIndices
        bool isPrevInbound;
        bool spraying = false;
        QList<Step*> holdQueue;
        IntersectInfo enterIntersect;

        for (int i=0; i<l; i++) {
            Step* curr = inputSteps[i];
            int iw = (w == lw) ? -1 : waypointIndices[w];
            int is = (s == ls) ? -1 : sprayIndices[s];

            if ((i != iw) && (i != is)) { // not waypoint or spray, i.e. HOLD
                // TODO if you add a different Step archtype than HOLD, please check it here
                if (spraying) {
                    holdQueue.append(curr);
                } else {
                    outputSteps.append(curr);
                };
                continue;
            }

            if (i == is) {
                spraying = true;
                s++;
            }

            if ((i == iw) && (w == 0)) { // first waypoint
                outputSteps.append(curr);
                isPrevInbound = false;
                spraying = false;
                w++;
                continue;
            }

            if (i == iw) { // second to last waypoint
                Step* prev = inputSteps[waypointIndices[w-1]];
                const WaypointStep* currWP = qobject_cast<WaypointStep*>(curr);
                const WaypointStep* prevWP = qobject_cast<WaypointStep*>(prev);
                const QPointF& cp = currWP->pointf;
                const QPointF& pp = prevWP->pointf;
                const bool isCurrInbound = extendedFence.containsPoint(cp, Qt::OddEvenFill);

                // for readability
                const bool isPrevOutbound = !isPrevInbound;
                const bool isCurrOutbound = !isCurrInbound;

                if (isCurrOutbound && isPrevOutbound) {
                    QLineF path(pp, cp);
                    auto intersectPoints = _intersectLinePolygon(path, extendedFence);

                    if (intersectPoints.isEmpty()) { // not intersect, path valid
                        if (spraying) {
                            outputSteps.append(new SprayStep());
                            outputSteps << holdQueue; // release the queue
                            holdQueue.clear();
                        };
                        outputSteps.append(curr);
                    } else {
                        // intersect, assume convex polygon, we have 2 intersect points
                        IntersectInfo first  = intersectPoints.first();
                        IntersectInfo second = intersectPoints.last();

                        QLineF cross(first.point, second.point);
                        if (qAbs(path.angle() - cross.angle()) > 90) { // swap if is inverse (180 deg)
                            const IntersectInfo temp = second;
                            second = first;
                            first = temp;
                        } // now we are sure this order: prev -> first -> second -> curr

                        // proceed avoidance
                        const AvoidanceInfo clockwiseAvoidance    = proceedAvoidance(true , extendedFence, first, second);
                        const AvoidanceInfo invClockwiseAvoidance = proceedAvoidance(false, extendedFence, first, second);

                        bool clockwise = clockwiseAvoidance.distance < invClockwiseAvoidance.distance;
                        const AvoidanceInfo& avoidance = clockwise ? clockwiseAvoidance : invClockwiseAvoidance;
                        QList<Step*> avoidWaypoints;
                        for (int j=0; j <avoidance.path.count(); j++) {
                            avoidWaypoints.append(new WaypointStep(avoidance.path[j]));
                        }

                        // collect all                           already have WAYPOINT (prev)
                        if (spraying) outputSteps.append(new SprayStep()); // SPRAY
                        outputSteps << avoidWaypoints;                     // WAYPOINTs (avoidance)
                        if (spraying) outputSteps.append(new SprayStep()); // SPRAY
                        outputSteps << holdQueue; // release the queue     // HOLD_*s
                        holdQueue.clear();
                        outputSteps.append(curr);                          // WAYPOINT (current)
                    }
                } else if (isPrevOutbound && isCurrInbound) { // enter
                    QLineF path(pp, cp);
                    enterIntersect = _intersectLinePolygon(path, extendedFence).first();
                    if (spraying) outputSteps.append(new SprayStep()); // SPRAY
                } else if (isPrevInbound && isCurrOutbound) { // exit
                    QLineF path(pp, cp);
                    auto exitIntersect = _intersectLinePolygon(path, extendedFence).first();

                    // proceed avoidance
                    const AvoidanceInfo clockwiseAvoidance    = proceedAvoidance(true , extendedFence, enterIntersect, exitIntersect);
                    const AvoidanceInfo invClockwiseAvoidance = proceedAvoidance(false, extendedFence, enterIntersect, exitIntersect);

                    bool clockwise = clockwiseAvoidance.distance < invClockwiseAvoidance.distance;
                    const AvoidanceInfo& avoidance = clockwise ? clockwiseAvoidance : invClockwiseAvoidance;
                    QList<Step*> avoidWaypoints;
                    for (int j=0; j <avoidance.path.count(); j++) {
                        avoidWaypoints.append(new WaypointStep(avoidance.path[j]));
                    }

                    // collect all                           already have WAYPOINT (prev)
                    if (spraying) outputSteps.append(new SprayStep()); // SPRAY
                    outputSteps << avoidWaypoints;                     // WAYPOINTs (avoidance)
                    if (spraying) outputSteps.append(new SprayStep()); // SPRAY
                    outputSteps << holdQueue; // release the queue     // HOLD_*s
                    holdQueue.clear();
                    outputSteps.append(curr);                          // WAYPOINT (current)
                }

                isPrevInbound = isCurrInbound;
                spraying = false;
                w++;
                continue;
            }
        }

        inputSteps.clear();
        inputSteps << outputSteps;
    }

    _steps.clear();
    _steps << outputSteps;

//    qDebug() << "post fence: " << *this;
}

QList<IntersectInfo> _intersectLinePolygon(const QLineF& line, const QPolygonF& polygon) {
    // return intersection information
    // which is, a list of intersected points (.point) with their respective coordinate (.coord)
    // in 1D vertex-index space, for ex:
    //     .point A has .coord of c=1.5
    //     1 < c=1.5 < 2
    //     => A is on the edge whose terminals are polygon[1] and polygon[2]
    // This coordinate could be used as bound condition to iterate over polygon's vertices.
    // For ex, assume firstIntersectCoord < secondIntersectCoord:
    //     int first = qCeil(firstIntersectCoord);
    //     int last = qFloor(secondIntersectCoord);
    //     for (int i=first; i<last; i++) // do something with vertices between 2 intersect points

    QList<IntersectInfo> result;
    for (int j=1; j <= polygon.count(); j++) {
        int j_ = (j == polygon.count()) ? 0 : j; // last edge (count - 1, 0)
        QLineF edge(polygon[j-1], polygon[j_]);
        QPointF intersectPoint;
        if (line.intersect(edge, &intersectPoint) == QLineF::BoundedIntersection) {
            IntersectInfo info;
            info.point = intersectPoint;
            info.coord = j-0.5; // only x.5
            result.append(info);
        };
    };
    return result;
}


AvoidanceInfo proceedAvoidance(bool clockwise, const QPolygonF& extendedFence, const IntersectInfo& first, const IntersectInfo& second) {
    AvoidanceInfo result;
    result.path.append(first.point);

    Ring* ring;
    if (clockwise) {
        ring = new IncrementRing(0, extendedFence.count()-1);
    } else {
        ring = new DecrementRing(0, extendedFence.count()-1);
    }

    for (ring->setAfter(first.coord); ring->current() != ring->after(second.coord); ring->next()) {
        result.path.append(extendedFence[ring->current()]);
    }

    result.path.append(second.point);

    result.distance = 0;

    for (int j=0; j<result.path.count() - 1; j++) {
        QLineF line(result.path[j], result.path[j+1]);
        result.distance += line.length();
    }

    return result;
}
