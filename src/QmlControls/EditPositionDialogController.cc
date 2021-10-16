/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "EditPositionDialogController.h"
#include "QGCGeo.h"
#include "QGCApplication.h"

const char*  EditPositionDialogController::_latitudeFactName =      "Latitude";
const char*  EditPositionDialogController::_longitudeFactName =     "Longitude";
const char*  EditPositionDialogController::_zoneFactName =          "Zone";
const char*  EditPositionDialogController::_hemisphereFactName =    "Hemisphere";
const char*  EditPositionDialogController::_eastingFactName =       "Easting";
const char*  EditPositionDialogController::_northingFactName =      "Northing";
const char*  EditPositionDialogController::_mgrsFactName =          "MGRS";

QMap<QString, FactMetaData*> EditPositionDialogController::_metaDataMap;

EditPositionDialogController::EditPositionDialogController(void)
    : _latitudeFact     (0, _latitudeFactName,      FactMetaData::valueTypeDouble)
    , _longitudeFact    (0, _longitudeFactName,     FactMetaData::valueTypeDouble)
    , _zoneFact         (0, _zoneFactName,          FactMetaData::valueTypeUint8)
    , _hemisphereFact   (0, _hemisphereFactName,    FactMetaData::valueTypeUint8)
    , _eastingFact      (0, _eastingFactName,       FactMetaData::valueTypeDouble)
    , _northingFact     (0, _northingFactName,      FactMetaData::valueTypeDouble)
    , _mgrsFact         (0, _mgrsFactName,          FactMetaData::valueTypeString)
{
    if (_metaDataMap.isEmpty()) {
        _metaDataMap = FactMetaData::createMapFromJsonFile(QStringLiteral(":/json/EditPositionDialog.FactMetaData.json"), nullptr /* QObject parent */);
    }

    _latitudeFact.setMetaData   (_metaDataMap[_latitudeFactName]);
    _longitudeFact.setMetaData  (_metaDataMap[_longitudeFactName]);
    _zoneFact.setMetaData       (_metaDataMap[_zoneFactName]);
    _hemisphereFact.setMetaData (_metaDataMap[_hemisphereFactName]);
    _eastingFact.setMetaData    (_metaDataMap[_eastingFactName]);
    _northingFact.setMetaData   (_metaDataMap[_northingFactName]);
    _mgrsFact.setMetaData       (_metaDataMap[_mgrsFactName]);
}

void EditPositionDialogController::setCoordinate(QGeoCoordinate coordinate)
{
    if (coordinate != _coordinate) {
        _coordinate = coordinate;
        emit coordinateChanged(coordinate);
    }
}

void EditPositionDialogController::initValues(void)
{
    _latitudeFact.setRawValue(_coordinate.latitude());
    _longitudeFact.setRawValue(_coordinate.longitude());

    _beginCoordinate.setLatitude(_coordinate.latitude());
    _beginCoordinate.setLongitude(_coordinate.longitude());
    _beginCoordinate.setAltitude(_coordinate.altitude());

    double easting, northing;
    int zone = convertGeoToUTM(_coordinate, easting, northing);
    if (zone >= 1 && zone <= 60) {
        _zoneFact.setRawValue(zone);
        _hemisphereFact.setRawValue(_coordinate.latitude() < 0);
        _eastingFact.setRawValue(easting);
        _northingFact.setRawValue(northing);
    }
    QString mgrs = convertGeoToMGRS(_coordinate);
    if (!mgrs.isEmpty()) {
        _mgrsFact.setRawValue(mgrs);
    }
}

void EditPositionDialogController::setFromGeo(void)
{
    _coordinate.setLatitude(_latitudeFact.rawValue().toDouble());
    _coordinate.setLongitude(_longitudeFact.rawValue().toDouble());
    emit coordinateChanged(_coordinate);
}

void EditPositionDialogController::setFromUTM(void)
{
    qDebug() << _eastingFact.rawValue().toDouble() << _northingFact.rawValue().toDouble() << _zoneFact.rawValue().toInt() << (_hemisphereFact.rawValue().toInt() == 1);
    if (convertUTMToGeo(_eastingFact.rawValue().toDouble(), _northingFact.rawValue().toDouble(), _zoneFact.rawValue().toInt(), _hemisphereFact.rawValue().toInt() == 1, _coordinate)) {
        qDebug() << _eastingFact.rawValue().toDouble() << _northingFact.rawValue().toDouble() << _zoneFact.rawValue().toInt() << (_hemisphereFact.rawValue().toInt() == 1) << _coordinate;
        emit coordinateChanged(_coordinate);
    } else {
        initValues();
    }
}

void EditPositionDialogController::setFromMGRS(void)
{
    if (convertMGRSToGeo(_mgrsFact.rawValue().toString(), _coordinate)) {
        emit coordinateChanged(_coordinate);
    } else {
        initValues();
    }
}

void EditPositionDialogController::setFromVehicle(void)
{
    _coordinate = qgcApp()->toolbox()->multiVehicleManager()->activeVehicle()->coordinate();
    emit coordinateChanged(_coordinate);
}

#define MOVE_STEP 0.5

void EditPositionDialogController::moveVertexUp(bool record)
{
    _coordinate = _coordinate.atDistanceAndAzimuth(MOVE_STEP, 0);
    emit coordinateChanged(_coordinate);
    if (record) _stackUndo.append(Move::UP);
}

void EditPositionDialogController::moveVertexLeft(bool record)
{
    _coordinate = _coordinate.atDistanceAndAzimuth(MOVE_STEP, -90);
    emit coordinateChanged(_coordinate);
    if (record) _stackUndo.append(Move::LEFT);
}

void EditPositionDialogController::moveVertexRight(bool record)
{
    _coordinate = _coordinate.atDistanceAndAzimuth(MOVE_STEP, 90);
    emit coordinateChanged(_coordinate);
    if (record) _stackUndo.append(Move::RIGHT);
}

void EditPositionDialogController::moveVertexDown(bool record)
{
    _coordinate = _coordinate.atDistanceAndAzimuth(MOVE_STEP, 180);
    emit coordinateChanged(_coordinate);
    if (record) _stackUndo.append(Move::DOWN);
}

void EditPositionDialogController::resetMoveVertex()
{
    _coordinate = _beginCoordinate;
    _stackUndo.clear();
    emit coordinateChanged(_coordinate);

}

void EditPositionDialogController::undoMoveVertex(void)
{
    if (!_stackUndo.isEmpty())
    {
        int moveSignal = _stackUndo.takeLast();
        if      (moveSignal == Move::UP)    moveVertexDown(false);
        else if (moveSignal == Move::DOWN)  moveVertexUp(false);
        else if (moveSignal == Move::LEFT)  moveVertexRight(false);
        else if (moveSignal == Move::RIGHT) moveVertexLeft(false);
    }
}

void EditPositionDialogController::commitVertex(void)
{
    _stackUndo.clear();
    emit coordinateChangeDone();
}
