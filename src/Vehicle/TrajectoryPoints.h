/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "QmlObjectListModel.h"

#include <QGeoCoordinate>

class Vehicle;

class TrajectoryPoints : public QObject
{
    Q_OBJECT

public:
    TrajectoryPoints(Vehicle* vehicle, QObject* parent = nullptr);

    Q_INVOKABLE QVariantList list(void) const { return _points; }

    void start  (void);
    void stop   (void);
    void pointAddedFromfile(QGeoCoordinate coordinate);
    void updateEnterPoint(QGeoCoordinate coordinate) { _enterPoint = coordinate; };
    void updateExitPoint(QGeoCoordinate coordinate) { _exitPoint = coordinate; };
    void updateResumePoint(QGeoCoordinate coordinate) ;
    void updatePausePoint(QGeoCoordinate coordinate) ;

public slots:
    void clear  (void);

signals:
    void pointAdded     (QGeoCoordinate coordinate);
    void updateLastPoint(QGeoCoordinate coordinate);
    void pointsCleared  (void);
    void pointEnterAdded     (QGeoCoordinate coordinate);
    void updateEnterLastPoint(QGeoCoordinate coordinate);
    void pointExitAdded     (QGeoCoordinate coordinate);
    void updateExitLastPoint(QGeoCoordinate coordinate);
    void exitPointsCleared  (void);

private slots:
    void _vehicleCoordinateChanged(QGeoCoordinate coordinate);

private:
    Vehicle*        _vehicle;
    QVariantList    _points;
    QGeoCoordinate  _lastPoint;
    QGeoCoordinate  _enterPoint;
    QGeoCoordinate  _exitPoint;

    double          _lastAzimuth;
    double          _reachEnterPoint;
    double          _reachExitPoint;

    static constexpr double _distanceTolerance = 2.0;
    static constexpr double _azimuthTolerance = 1.5;
};
