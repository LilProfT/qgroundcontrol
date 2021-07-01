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
    // The goal of this algorithm is to limit the number of trajectory points whic represent the vehicle path.
    // Fewer points means higher performance of map display.

    if (!_reachEnterPoint) {
        double dist = coordinate.distanceTo(_enterPoint);
        qWarning(MissionManagerLog) << "dist :" << dist;
        if (dist < 1) {
            _reachEnterPoint = true;
            qWarning(MissionManagerLog) << "_reachEnterPoint :" << _reachEnterPoint;

        }
    } else {
        double dist = coordinate.distanceTo(_exitPoint);
        qWarning(MissionManagerLog) << "dist :" << dist;
        if (dist < 1) {
            _reachExitPoint = true;
            qWarning(MissionManagerLog) << "_reachExitPoint :" << _reachExitPoint;
        }
    }


    if (_lastPoint.isValid()) {
        double distance = _lastPoint.distanceTo(coordinate);
        if (distance > _distanceTolerance) {
            //-- Update flight distance
            _vehicle->updateFlightDistance(distance);

            //Mismart: Throw in a sprayed area update function
            if (_vehicle->_areaSprayedStart()) {
                _vehicle->updateAreaSprayed(distance);
            }

            // Vehicle has moved far enough from previous point for an update
            double newAzimuth = _lastPoint.azimuthTo(coordinate);
            if (qIsNaN(_lastAzimuth) || qAbs(newAzimuth - _lastAzimuth) > _azimuthTolerance) {
                // The new position IS NOT colinear with the last segment. Append the new position to the list.
                _lastAzimuth = _lastPoint.azimuthTo(coordinate);
                _lastPoint = coordinate;
                _points.append(QVariant::fromValue(coordinate));
                //if (coordinate != _vehicle->homePosition())
                qWarning(MissionManagerLog) << "pointFreeAdded coordinate :" << coordinate;
                if (_reachEnterPoint) {
                    if (!_reachExitPoint)
                        emit pointAdded(coordinate);
                    else
                        emit pointExitAdded(coordinate);
                } else {
                    emit pointEnterAdded(coordinate);
                }

            } else {
                // The new position IS colinear with the last segment. Don't add a new point, just update
                // the last point to be the new position.
                _lastPoint = coordinate;
                _points[_points.count() - 1] = QVariant::fromValue(coordinate);
                qWarning(MissionManagerLog) << "updateLastPoint coordinate :" << coordinate;

                if (_reachEnterPoint) {
                    if (!_reachExitPoint)
                        emit updateLastPoint(coordinate);
                    else
                        emit updateExitLastPoint(coordinate);
                } else {
                    emit updateEnterLastPoint(coordinate);
                }

//                if (_reachEnterPoint)
//                    emit updateLastPoint(coordinate);
//                else
//                    emit updateFreeLastPoint(coordinate);
            }
        }
    } else {
        // Add the very first trajectory point to the list
        _lastPoint = coordinate;
        _points.append(QVariant::fromValue(coordinate));

        qWarning(MissionManagerLog) << "first pointFreeAdded coordinate :" << coordinate;

        if (_reachEnterPoint) {
            if (!_reachExitPoint)
                emit pointAdded(coordinate);
            else
                emit pointExitAdded(coordinate);
        } else {
            emit pointEnterAdded(coordinate);
        }

    }
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
            _vehicle->updateFlightDistance(distance);

            //Mismart: Throw in a sprayed area update function
            if (_vehicle->_areaSprayedStart()) {
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

void TrajectoryPoints::clear(void)
{
    _points.clear();
    _lastPoint = QGeoCoordinate();
    _lastAzimuth = qQNaN();
    emit pointsCleared();
}
