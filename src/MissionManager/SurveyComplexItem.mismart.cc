#include "SurveyComplexItem.h"
#include "JsonHelper.h"
#include "MissionController.h"
#include "QGCGeo.h"
#include "QGCQGeoCoordinate.h"
#include "SettingsManager.h"
#include "AppSettings.h"
#include "PlanMasterController.h"
#include "QGCApplication.h"
#include "TakeoffMissionItem.h"

#include <QPolygonF>
#include <QSignalBlocker>
#include <QTimer>

void SurveyComplexItem::_updateSprayFlowRate(void)
{
    qDebug() << "update spray flow rate";
    double gridSpacing = _cameraCalc.adjustedFootprintSide()->rawValue().toDouble();
    double applicationRate = _applicationRateFact.property("value").toFloat();
    double velocity = _velocityFact.property("value").toFloat();
    _sprayFlowRateFact.setProperty("value", 0.006 * applicationRate * gridSpacing * velocity);
}

void SurveyComplexItem::_optimize_Angle_EntryPoint(void)
{

    const QSignalBlocker blocker(_gridAngleFact); // don't fire _rebuildTransects automatically
    QGeoCoordinate A, B;
    double min_distance = INFINITY;
    double best_angle = 0;

    if (_surveyAreaPolygon.count() < 3) return;

    if (_surveyAreaPolygon.traceMode()) {
        A = _surveyAreaPolygon.pathModel().value<QGCQGeoCoordinate*>(0)->coordinate();
        B = _surveyAreaPolygon.pathModel().value<QGCQGeoCoordinate*>(1)->coordinate();

        double angle = A.azimuthTo(B);
        qDebug() << "trace mode angle: " << angle;
        _gridAngleFact.setProperty("value", angle);
        _rebuildTransects();
        _optimize_EntryPoint();

        return;
    }

    for (int i=0; i < _surveyAreaPolygon.count(); i++) {
        int i_A = i;
        int i_B;

        if (i != _surveyAreaPolygon.count() - 1) {
            i_B = i + 1;
        } else { // A last point and B first point
            i_B = 0;
        }

        qDebug() << "(" << i_A << "," << i_B << ")";

        A = _surveyAreaPolygon.pathModel().value<QGCQGeoCoordinate*>(i_A)->coordinate();
        B = _surveyAreaPolygon.pathModel().value<QGCQGeoCoordinate*>(i_B)->coordinate();

        double angle = A.azimuthTo(B);
        qDebug() << "angle: " << angle;
        _gridAngleFact.setProperty("value", angle);
        _rebuildTransects();
        qDebug() << "distance: " << _complexDistance;
        if (_complexDistance < min_distance) {
            qDebug() << "min changed: " << _complexDistance;
            min_distance = _complexDistance;
            best_angle = angle;
        };
    };

    qDebug() << "best angle: " << best_angle;
    _gridAngleFact.setProperty("value", best_angle);
    _rebuildTransects();

    _optimize_EntryPoint();
}

void SurveyComplexItem::_updateAngle() {
    if (_isEdgeIndexFromFile) {
        rotateAngle();
        rotateEntryPoint();
        _isEdgeIndexFromFile = false;
    }
}

void SurveyComplexItem::_optimize_EntryPoint(void) {
    QGeoCoordinate takeoffWaypoint = _masterController->missionController()->takeoffMissionItem()->launchCoordinate();;
    if (!takeoffWaypoint.isValid()) {
        qDebug() << "takeoff invalid";
        return;
    };
    qDebug() << "takeoff: " << takeoffWaypoint;
    double min_distance = INFINITY;
    int start_entryPoint = _entryPoint;
    int best_entryPoint;

    do {
        qDebug() << "enter coord valid: " << _coordinate.isValid();
        qDebug() << "enter: " << _coordinate;
        double distance = takeoffWaypoint.distanceTo(_coordinate);
        qDebug() << " _entryPoint: " << _entryPoint << "distance: " << distance;
        if (distance < min_distance) {
            min_distance = distance;
            best_entryPoint = _entryPoint;
        };
        rotateEntryPoint();
    } while (_entryPoint != start_entryPoint);

    qDebug() << "best entryPoint: " << best_entryPoint;

    while (_entryPoint != best_entryPoint) rotateEntryPoint();
    qDebug() << "entryPoint after adjusted: " << _entryPoint;
}

void SurveyComplexItem::optimize(void) {
    _optimize_Angle_EntryPoint();
}

void SurveyComplexItem::_toggleAutoOptimize(QVariant value) {
    bool enable = value.value<bool>();
    if (enable) {
        connect(&_timer_optimize_Angle_EntryPoint,       &QTimer::timeout,                           this, &SurveyComplexItem::_optimize_Angle_EntryPoint);
        connect(_masterController->missionController()->takeoffMissionItem(), &TakeoffMissionItem::launchCoordinateChanged, this, &SurveyComplexItem::_optimize_EntryPoint);
    } else {
        disconnect(&_timer_optimize_Angle_EntryPoint,       &QTimer::timeout,                           this, &SurveyComplexItem::_optimize_Angle_EntryPoint);
        disconnect(_masterController->missionController()->takeoffMissionItem(), &TakeoffMissionItem::launchCoordinateChanged, this, &SurveyComplexItem::_optimize_EntryPoint);
    };
}

void SurveyComplexItem::_buildAndAppendMissionItems (QList<MissionItem*>& items, QObject* missionItemParent)
{
    // [COPY] from TransectStyleComplexItem, simplify, remove all the image capture feature
    qCDebug(TransectStyleComplexItemLog) << "_buildAndAppendMissionItems >>> mismart <<<";

    _model.setMissionItemParent(missionItemParent);
    items.append(_model.generateItems());
}

void SurveyComplexItem::rotateAngle(void)
{
    qDebug() << " rotateAngle _isEdgeIndexFromFile: " << _isEdgeIndexFromFile ;

    if (!_isEdgeIndexFromFile) {
        _edgeIndex += 1;
        if (_edgeIndex == _surveyAreaPolygon.count()) _edgeIndex = 0;

    }
    int i_A = _edgeIndex;
    int i_B = (_edgeIndex == _surveyAreaPolygon.count() - 1) ? 0 : _edgeIndex + 1;
    QGeoCoordinate A, B;
    A = _surveyAreaPolygon.pathModel().value<QGCQGeoCoordinate*>(i_A)->coordinate();
    B = _surveyAreaPolygon.pathModel().value<QGCQGeoCoordinate*>(i_B)->coordinate();

    double angle = A.azimuthTo(B);
    _gridAngleFact.setProperty("value", angle);

    _angleEdge.clear();
    _angleEdge.append(QVariant::fromValue(A));
    _angleEdge.append(QVariant::fromValue(B));
    emit angleEdgeChanged();
}

double SurveyComplexItem::getYaw(void)
{
    if (_transects.count() == 0) return 0;

    QGeoCoordinate first  = _transects[0][0].coord;
    QGeoCoordinate second = _transects[0][1].coord;

    return first.azimuthTo(second);
}

void SurveyComplexItem::_calcFeaturedWidth(void)
{
    double gridAngle = _gridAngleFact.rawValue().toDouble();
    double azimuth = gridAngle + 90;

    if (_surveyAreaPolygon.count() < 2) return;

    QGeoCoordinate tangentOrigin = _surveyAreaPolygon.pathModel().value<QGCQGeoCoordinate*>(0)->coordinate();
    QList<QLineF> crossvectors;
    QList<QPointF> vertices;
    for (int i=0; i<_surveyAreaPolygon.count(); i++) {
        QGeoCoordinate vertex = _surveyAreaPolygon.pathModel().value<QGCQGeoCoordinate*>(i)->coordinate();
        QGeoCoordinate crosspoint = vertex.atDistanceAndAzimuth(1, azimuth);

        double y, x, down;
        convertGeoToNed(vertex, tangentOrigin, &y, &x, &down);
        QPointF vertexf(x, -y);
        convertGeoToNed(crosspoint, tangentOrigin, &y, &x, &down);
        QPointF crosspointf(x, -y);
        QLineF crossvector(vertexf, crosspointf);
        crossvectors << crossvector;
        vertices << vertexf;
    }

    QList<QLineF> edges;
    for (int i=0; i<vertices.count()-1; i++) {
        QLineF edge(vertices[i], vertices[i+1]);
        edges << edge;
    }
    QLineF edge(vertices[vertices.count()-1], vertices[0]);
    edges << edge;

    double featuredWidth = 0;
    QLineF featuredLine;
    for (const QLineF& crossvector: crossvectors) {
        for (const QLineF& edge: edges) {
            QPointF intersectPoint;
            crossvector.intersect(edge, &intersectPoint);

            // check bound in edge
            QLineF first(edge.p1(), intersectPoint);
            QLineF second(edge.p2(), intersectPoint);
            if (qMax(first.length(), second.length()) < edge.length()) {
                QLineF crossline(crossvector.p1(), intersectPoint);
                double l = crossline.length();
                if (l > featuredWidth) {
                    featuredWidth = l;
                    featuredLine = crossline;
                }
            }
        }
    }

    QGeoCoordinate first;
    convertNedToGeo(-featuredLine.p1().y(), featuredLine.p1().x(), 0, tangentOrigin, &first);
    QGeoCoordinate second;
    convertNedToGeo(-featuredLine.p2().y(), featuredLine.p2().x(), 0, tangentOrigin, &second);
    _featuredLine.clear();
    _featuredLine << first << second;

    _cameraCalc.setFeaturedWidth(featuredWidth);
}
