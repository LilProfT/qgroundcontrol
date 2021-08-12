/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "TrajectoryPoints.h"
#include "Vehicle.h"

TrajectoryPoints::TrajectoryPoints(Vehicle* vehicle, QObject* parent)
    : QObject       (parent)
    , _vehicle      (vehicle)
    , _lastAzimuth  (qQNaN())
{
    connect(_vehicle, &Vehicle::pointAddedFromfile, this, &TrajectoryPoints::pointAddedFromfile);
    connect(_vehicle, &Vehicle::clearTrajectoryPoint, this, &TrajectoryPoints::clear);

    _enterPoint.setLatitude(-35.3635283621613);
    _enterPoint.setLongitude(149.1642707053213);
    _exitPoint.setLatitude(-35.363367756072925);
    _exitPoint.setLongitude(149.16492230227956);

    _reachEnterPoint = false;
}

void TrajectoryPoints::_vehicleCoordinateChanged(QGeoCoordinate coordinate)
{   
    if (isSprayTrigger() != _isSprayTrigger) {
        _isSprayTrigger = isSprayTrigger();
        qCWarning(VehicleLog()) << "_isSprayTrigger: " << _isSprayTrigger << "_lastPoint:" << _lastPoint;

        emit pointSprayTrigger(_isSprayTrigger, _lastPoint);
        _lastPoint = QGeoCoordinate();
    }

    if (_lastPoint.isValid()) {
        double distance = _lastPoint.distanceTo(coordinate);
        if (distance > _distanceTolerance) {
            //-- Update flight distance
            _vehicle->updateFlightDistance(distance);

            //Mismart: Throw in a sprayed area update function
            if (_isSprayTrigger) {
                _vehicle->updateAreaSprayed(distance);
            }

            // Vehicle has moved far enough from previous point for an update
            double newAzimuth = _lastPoint.azimuthTo(coordinate);
            if (qIsNaN(_lastAzimuth) || qAbs(newAzimuth - _lastAzimuth) > _azimuthTolerance) {
                // The new position IS NOT colinear with the last segment. Append the new position to the list.
                _lastAzimuth = _lastPoint.azimuthTo(coordinate);
                _lastPoint = coordinate;
                _points.append(QVariant::fromValue(coordinate));
                emit pointAdded(coordinate);
            } else {
                // The new position IS colinear with the last segment. Don't add a new point, just update
                // the last point to be the new position.
                _lastPoint = coordinate;
                _points[_points.count() - 1] = QVariant::fromValue(coordinate);
                emit updateLastPoint(coordinate);
            }
        }
    } else {
        // Add the very first trajectory point to the list
        _lastPoint = coordinate;
        _points.append(QVariant::fromValue(coordinate));
        emit pointAdded(coordinate);
    }
}

void TrajectoryPoints::sprayTrigger()
{
    if (_isTrigger)
        _isTrigger = false;
    else
        _isTrigger = true;
    qCWarning(VehicleLog()) << "_isTrigger: " << _isTrigger;
}

bool TrajectoryPoints::isSprayTrigger()
{
    return _isTrigger;
    //return _vehicle->_areaSprayedStart();
}

void TrajectoryPoints::start(void)
{
    //clear();
    _reachEnterPoint = false;
    _reachExitPoint = false;

    connect(_vehicle, &Vehicle::coordinateChanged, this, &TrajectoryPoints::_vehicleCoordinateChanged);

}

void TrajectoryPoints::stop(void)
{
    disconnect(_vehicle, &Vehicle::coordinateChanged, this, &TrajectoryPoints::_vehicleCoordinateChanged);
}

void TrajectoryPoints::pointAddedFromfile(QGeoCoordinate coordinate)
{
    qWarning(MissionManagerLog) << "pointAddedFromfile coordinate :" << coordinate;

    if (_lastPoint.isValid()) {
        double distance = _lastPoint.distanceTo(coordinate);
        if (distance > _distanceTolerance) {
            //-- Update flight distance
//            _vehicle->updateFlightDistance(distance);

//            //Mismart: Throw in a sprayed area update function
//            if (_vehicle->_areaSprayedStart()) {
//                _vehicle->updateAreaSprayed(distance);
//            }

            // Vehicle has moved far enough from previous point for an update
            double newAzimuth = _lastPoint.azimuthTo(coordinate);
            if (qIsNaN(_lastAzimuth) || qAbs(newAzimuth - _lastAzimuth) > _azimuthTolerance) {
                // The new position IS NOT colinear with the last segment. Append the new position to the list.
                _lastAzimuth = _lastPoint.azimuthTo(coordinate);
                _lastPoint = coordinate;
                _points.append(QVariant::fromValue(coordinate));
                emit pointAdded(coordinate);
            } else {
                // The new position IS colinear with the last segment. Don't add a new point, just update
                // the last point to be the new position.
                _lastPoint = coordinate;
                _points[_points.count() - 1] = QVariant::fromValue(coordinate);
                emit updateLastPoint(coordinate);
            }
        }
    } else {
        // Add the very first trajectory point to the list
        _lastPoint = coordinate;
        pointSprayTrigger(true, _lastPoint);
        QTimer::singleShot(5000, this, &TrajectoryPoints::_timerSlot);

        _points.append(QVariant::fromValue(coordinate));
        emit pointAdded(coordinate);
    }
}

void TrajectoryPoints::_timerSlot()
{
    _lastPoint = QGeoCoordinate();
    pointSprayTrigger(false, _lastPoint);
}

void TrajectoryPoints::updatePausePoint(QGeoCoordinate coordinate)
{
    _reachExitPoint = true;
    _vehicleCoordinateChanged(coordinate);
}

void TrajectoryPoints::updateResumePoint(QGeoCoordinate coordinate)
{
    _reachExitPoint = false;
    _vehicleCoordinateChanged(coordinate);
    //emit exitPointsCleared();
}
void TrajectoryPoints::clear(void)
{
    _points.clear();
    _lastPoint = QGeoCoordinate();
    _lastAzimuth = qQNaN();
    emit pointsCleared();
}
