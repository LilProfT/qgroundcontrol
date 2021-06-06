#include "TransectStyleFenceSupportedComplexItem.h"
#include "PlanMasterController.h"

QGC_LOGGING_CATEGORY(TransectStyleFenceSupportedComplexItemLog, "TransectStyleFenceSupportedComplexItemLog")

TransectStyleFenceSupportedComplexItem::TransectStyleFenceSupportedComplexItem(PlanMasterController* masterController, bool flyView, QString settingsGroup, QObject* parent)
    : TransectStyleComplexItem  (masterController, flyView, settingsGroup, parent),
      _model                    (_sequenceNumber, followTerrain() || !_cameraCalc.distanceToSurfaceRelative() ? MAV_FRAME_GLOBAL : MAV_FRAME_GLOBAL_RELATIVE_ALT, this /* temporary missionItemParent */, this)
{
    auto fences = _masterController->geoFenceController()->polygons();
    connect(fences, &QmlObjectListModel::countChanged, this, &TransectStyleFenceSupportedComplexItem::_rebuildTransects);
    connect(fences, &QmlObjectListModel::countChanged, this, &TransectStyleFenceSupportedComplexItem::_listenFences);
}

void TransectStyleFenceSupportedComplexItem::_listenFences (void)
{
    auto fences = _masterController->geoFenceController()->polygons();
    for (int i=0; i < fences->count(); i++) {
        QGCFencePolygon* fence = qobject_cast<QGCFencePolygon*>(fences->get(i));
        connect(fence, &QGCMapPolygon::pathChanged, this, &TransectStyleFenceSupportedComplexItem::_rebuildTransects, Qt::UniqueConnection);
        connect(fence, &QGCFencePolygon::inclusionChanged, this, &TransectStyleFenceSupportedComplexItem::_rebuildTransects, Qt::UniqueConnection);
    }
}

void TransectStyleFenceSupportedComplexItem::_rebuildTransects (void)
{
    if (_ignoreRecalc) {
        return;
    }

    _rebuildTransectsPhase1();

    if (_followTerrain) {
        // Query the terrain data. Once available terrain heights will be calculated
        _queryTransectsPathHeightInfo();
        // We won't know min/max till were done
        _minAMSLAltitude = _maxAMSLAltitude = qQNaN();
    } else {
        // Not following terrain, just add requested altitude to coords
        double requestedAltitude = _cameraCalc.distanceToSurface()->rawValue().toDouble();

        for (int i=0; i<_transects.count(); i++) {
            QList<CoordInfo_t>& transect = _transects[i];

            for (int j=0; j<transect.count(); j++) {
                QGeoCoordinate& coord = transect[j].coord;

                coord.setAltitude(requestedAltitude);
            }
        }

        _minAMSLAltitude = _maxAMSLAltitude = requestedAltitude + (_cameraCalc.distanceToSurfaceRelative() ? _missionController->plannedHomePosition().altitude() : 0);
    }

    double requestedAltitude = _cameraCalc.distanceToSurface()->rawValue().toDouble();
    double yaw = getYaw();
    double gridSpacing = _cameraCalc.adjustedFootprintSide()->rawValue().toDouble();

    _model.setExclusionFences(_masterController->geoFenceController()->polygons());
    _model.setAvoidDistance(gridSpacing / 2);

    _model.clearStep();
    _model.appendHoldAltitude(requestedAltitude);
    _model.appendHoldYaw(yaw);

    for (const QList<CoordInfo_t>& transect: _transects) {
        _model.appendWaypoint(transect[0].coord);
        _model.appendSpray();
        _model.appendWaypoint(transect[1].coord);
    }

    _model.pregenIntegrate();

    // Calc bounding cube
    double north = 0.0;
    double south = 180.0;
    double east  = 0.0;
    double west  = 360.0;
    double bottom = 100000.;
    double top = 0.;
    // Generate the visuals transect representation
    _visualTransectPoints.clear();
    for (Step* step: _model.steps()) {
            if (step->type() != Step::Type::WAYPOINT) continue;
            QGeoCoordinate coord = qobject_cast<WaypointStep*>(step)->coord;
            _visualTransectPoints.append(QVariant::fromValue(coord));
            double lat = coord.latitude()  + 90.0;
            double lon = coord.longitude() + 180.0;
            north   = fmax(north, lat);
            south   = fmin(south, lat);
            east    = fmax(east,  lon);
            west    = fmin(west,  lon);
            bottom  = fmin(bottom, coord.altitude());
            top     = fmax(top, coord.altitude());
    }

    //-- Update bounding cube for airspace management control
    _setBoundingCube(QGCGeoBoundingCube(
                         QGeoCoordinate(north - 90.0, west - 180.0, bottom),
                         QGeoCoordinate(south - 90.0, east - 180.0, top)));
    emit visualTransectPointsChanged();

    _coordinate = _visualTransectPoints.count() ? _visualTransectPoints.first().value<QGeoCoordinate>() : QGeoCoordinate();
    _exitCoordinate = _visualTransectPoints.count() ? _visualTransectPoints.last().value<QGeoCoordinate>() : QGeoCoordinate();
    emit coordinateChanged(_coordinate);
    emit exitCoordinateChanged(_exitCoordinate);

    if (_isIncomplete) {
        _isIncomplete = false;
        emit isIncompleteChanged();
    }

    _recalcComplexDistance();
    _recalcCameraShots();

    emit lastSequenceNumberChanged(lastSequenceNumber());
    emit timeBetweenShotsChanged();
    emit additionalTimeDelayChanged();

    emit minAMSLAltitudeChanged();
    emit maxAMSLAltitudeChanged();

    emit _updateFlightPathSegmentsSignal();
    _amslEntryAltChanged();
    _amslExitAltChanged();
}

int TransectStyleFenceSupportedComplexItem::lastSequenceNumber(void) const
{
    if (_loadedMissionItems.count()) {
        // We have stored mission items, just use those
        return _sequenceNumber + _loadedMissionItems.count() - 1;
    } else if (_model.steps().isEmpty()) {
        // Polygon has not yet been set so we just return back a one item complex item for now
        return _sequenceNumber;
    } else {
        // We have to determine from transects
        return _model.endSeqNum();
    }
}
