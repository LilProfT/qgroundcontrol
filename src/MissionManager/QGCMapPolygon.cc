/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "QGCMapPolygon.h"
#include "QGCGeo.h"
#include "JsonHelper.h"
#include "QGCQGeoCoordinate.h"
#include "QGCApplication.h"
#include "ShapeFileHelper.h"
#include "QGCLoggingCategory.h"
#include "PositionManager.h"

#include <QGeoRectangle>
#include <QDebug>
#include <QJsonArray>
#include <QLineF>
#include <QFile>
#include <QDomDocument>

const char* QGCMapPolygon::jsonPolygonKey = "polygon";

QGCMapPolygon::QGCMapPolygon(QObject* parent)
    : QObject               (parent)
    , _dirty                (false)
    , _centerDrag           (false)
    , _ignoreCenterUpdates  (false)
    , _interactive          (false)
    , _resetActive          (false)
{
    _init();
}

QGCMapPolygon::QGCMapPolygon(const QGCMapPolygon& other, QObject* parent)
    : QObject               (parent)
    , _dirty                (false)
    , _centerDrag           (false)
    , _ignoreCenterUpdates  (false)
    , _interactive          (false)
    , _resetActive          (false)
{
    *this = other;

    _init();
}

void QGCMapPolygon::_init(void)
{
    connect(&_polygonModel, &QmlObjectListModel::dirtyChanged, this, &QGCMapPolygon::_polygonModelDirtyChanged);
    connect(&_polygonModel, &QmlObjectListModel::countChanged, this, &QGCMapPolygon::_polygonModelCountChanged);

    connect(this, &QGCMapPolygon::pathChanged,  this, &QGCMapPolygon::_updateCenter);
    connect(this, &QGCMapPolygon::countChanged, this, &QGCMapPolygon::isValidChanged);
    connect(this, &QGCMapPolygon::countChanged, this, &QGCMapPolygon::isEmptyChanged);
}

const QGCMapPolygon& QGCMapPolygon::operator=(const QGCMapPolygon& other)
{
    clear();

    QVariantList vertices = other.path();
    QList<QGeoCoordinate> rgCoord;
    for (const QVariant& vertexVar: vertices) {
        rgCoord.append(vertexVar.value<QGeoCoordinate>());
    }
    appendVertices(rgCoord);

    setDirty(true);

    return *this;
}

void QGCMapPolygon::clear(void)
{
    // Bug workaround, see below
    while (_polygonPath.count() > 1) {
        _polygonPath.takeLast();
    }
    emit pathChanged();

    // Although this code should remove the polygon from the map it doesn't. There appears
    // to be a bug in QGCMapPolygon which causes it to not be redrawn if the list is empty. So
    // we work around it by using the code above to remove all but the last point which in turn
    // will cause the polygon to go away.
    _polygonPath.clear();

    _polygonModel.clearAndDeleteContents();

    emit cleared();

    setDirty(true);
}

void QGCMapPolygon::adjustVertex(int vertexIndex, const QGeoCoordinate coordinate)
{
    _polygonPath[vertexIndex] = QVariant::fromValue(coordinate);
    _polygonModel.value<QGCQGeoCoordinate*>(vertexIndex)->setCoordinate(coordinate);
    if (!_centerDrag) {
        // When dragging center we don't signal path changed until all vertices are updated
        emit pathChanged();
    }
    setDirty(true);
}

void QGCMapPolygon::setDirty(bool dirty)
{
    if (_dirty != dirty) {
        _dirty = dirty;
        if (!dirty) {
            _polygonModel.setDirty(false);
        }
        emit dirtyChanged(dirty);
    }
}

QGeoCoordinate QGCMapPolygon::_coordFromPointF(const QPointF& point) const
{
    QGeoCoordinate coord;

    if (_polygonPath.count() > 0) {
        QGeoCoordinate tangentOrigin = _polygonPath[0].value<QGeoCoordinate>();
        convertNedToGeo(-point.y(), point.x(), 0, tangentOrigin, &coord);
    }

    return coord;
}

QPointF QGCMapPolygon::_pointFFromCoord(const QGeoCoordinate& coordinate) const
{
    if (_polygonPath.count() > 0) {
        double y, x, down;
        QGeoCoordinate tangentOrigin = _polygonPath[0].value<QGeoCoordinate>();

        convertGeoToNed(coordinate, tangentOrigin, &y, &x, &down);
        return QPointF(x, -y);
    }

    return QPointF();
}

QPointF QGCMapPolygon::_pointFFromCoord(const QGeoCoordinate& coordinate, QGeoCoordinate& tangentOrigin) const
{
    if (_polygonPath.count() > 0) {
        double y, x, down;

        convertGeoToNed(coordinate, tangentOrigin, &y, &x, &down);
        return QPointF(x, -y);
    }

    return QPointF();
}

QPolygonF QGCMapPolygon::toPolygonF(QGeoCoordinate& tangentOrigin) const
{
    QPolygonF polygon;

    if (_polygonPath.count() > 2) {
        for (int i=0; i<_polygonPath.count(); i++) {
            polygon.append(_pointFFromCoord(_polygonPath[i].value<QGeoCoordinate>(), tangentOrigin));
        }
    }

    return polygon;
}

QPolygonF QGCMapPolygon::_toPolygonF(void) const
{
    QPolygonF polygon;

    if (_polygonPath.count() > 2) {
        for (int i=0; i<_polygonPath.count(); i++) {
            polygon.append(_pointFFromCoord(_polygonPath[i].value<QGeoCoordinate>()));
        }
    }

    return polygon;
}

bool QGCMapPolygon::containsCoordinate(const QGeoCoordinate& coordinate) const
{
    if (_polygonPath.count() > 2) {
        return _toPolygonF().containsPoint(_pointFFromCoord(coordinate), Qt::OddEvenFill);
    } else {
        return false;
    }
}

void QGCMapPolygon::setPath(const QList<QGeoCoordinate>& path)
{
    _polygonPath.clear();
    _polygonModel.clearAndDeleteContents();
    for(const QGeoCoordinate& coord: path) {
        _polygonPath.append(QVariant::fromValue(coord));
        _polygonModel.append(new QGCQGeoCoordinate(coord, this));
    }

    setDirty(true);
    emit pathChanged();
}

void QGCMapPolygon::setPath(const QVariantList& path)
{
    _polygonPath = path;

    _polygonModel.clearAndDeleteContents();
    for (int i=0; i<_polygonPath.count(); i++) {
        _polygonModel.append(new QGCQGeoCoordinate(_polygonPath[i].value<QGeoCoordinate>(), this));
    }

    setDirty(true);
    emit pathChanged();
}

void QGCMapPolygon::saveToJson(QJsonObject& json)
{
    QJsonValue jsonValue;

    JsonHelper::saveGeoCoordinateArray(_polygonPath, false /* writeAltitude*/, jsonValue);
    json.insert(jsonPolygonKey, jsonValue);

    json.insert("polygonOffsets", QJsonObject::fromVariantMap(_offsets));

    setDirty(false);
}

bool QGCMapPolygon::loadFromJson(const QJsonObject& json, bool required, QString& errorString)
{
    errorString.clear();
    clear();

    if (required) {
        QStringList requiredKeys;
        requiredKeys.append(jsonPolygonKey);
        requiredKeys.append("polygonOffsets");
        if (!JsonHelper::validateRequiredKeys(json, requiredKeys, errorString)) {
            return false;
        }
    } else if (!json.contains(jsonPolygonKey)) {
        return true;
    }

    if (!JsonHelper::loadGeoCoordinateArray(json[jsonPolygonKey], false /* altitudeRequired */, _polygonPath, errorString)) {
        return false;
    }

    for (int i=0; i<_polygonPath.count(); i++) {
        _polygonModel.append(new QGCQGeoCoordinate(_polygonPath[i].value<QGeoCoordinate>(), this));
    }

<<<<<<< HEAD
    _offsets = json["polygonOffsets"].toObject().toVariantMap();
=======
    QJsonValue offsetJson = json.value("polygonOffsets");
    if (offsetJson != QJsonValue::Undefined) _offsets = offsetJson.toObject().toVariantMap();
>>>>>>> ff293c6eb41722ca6819ea58d8e26a487e0c7f77

    setDirty(false);
    emit pathChanged();

    return true;
}

QList<QGeoCoordinate> QGCMapPolygon::coordinateList(void) const
{
    QList<QGeoCoordinate> coords;

    for (int i=0; i<_polygonPath.count(); i++) {
        coords.append(_polygonPath[i].value<QGeoCoordinate>());
    }

    return coords;
}

void QGCMapPolygon::splitPolygonSegment(int vertexIndex)
{
    int nextIndex = vertexIndex + 1;
    if (nextIndex > _polygonPath.length() - 1) {
        nextIndex = 0;
    }

    QGeoCoordinate firstVertex = _polygonPath[vertexIndex].value<QGeoCoordinate>();
    QGeoCoordinate nextVertex = _polygonPath[nextIndex].value<QGeoCoordinate>();

    double distance = firstVertex.distanceTo(nextVertex);
    double azimuth = firstVertex.azimuthTo(nextVertex);
    QGeoCoordinate newVertex = firstVertex.atDistanceAndAzimuth(distance / 2, azimuth);

    if (nextIndex == 0) {
        appendVertex(newVertex);
    } else {
        _polygonModel.insert(nextIndex, new QGCQGeoCoordinate(newVertex, this));
        _polygonPath.insert(nextIndex, QVariant::fromValue(newVertex));
        emit pathChanged();
    }
}

void QGCMapPolygon::appendVertex(const QGeoCoordinate& coordinate)
{
    _polygonPath.append(QVariant::fromValue(coordinate));
    _polygonModel.append(new QGCQGeoCoordinate(coordinate, this));
    emit pathChanged();
}

void QGCMapPolygon::appendVertices(const QList<QGeoCoordinate>& coordinates)
{
    QList<QObject*> objects;

    _beginResetIfNotActive();
    for (const QGeoCoordinate& coordinate: coordinates) {
        objects.append(new QGCQGeoCoordinate(coordinate, this));
        _polygonPath.append(QVariant::fromValue(coordinate));
    }
    _polygonModel.append(objects);
    _endResetIfNotActive();

    emit pathChanged();
}

void QGCMapPolygon::appendVertices(const QVariantList& varCoords)
{
    QList<QGeoCoordinate> rgCoords;
    for (const QVariant& varCoord: varCoords) {
        rgCoords.append(varCoord.value<QGeoCoordinate>());
    }
    appendVertices(rgCoords);
}

void QGCMapPolygon::_polygonModelDirtyChanged(bool dirty)
{
    if (dirty) {
        setDirty(true);
    }
}

void QGCMapPolygon::removeVertex(int vertexIndex)
{
    if (vertexIndex < 0 && vertexIndex > _polygonPath.length() - 1) {
        qWarning() << "Call to removePolygonCoordinate with bad vertexIndex:count" << vertexIndex << _polygonPath.length();
        return;
    }

    if (_polygonPath.length() <= 3) {
        // Don't allow the user to trash the polygon
        return;
    }

    QObject* coordObj = _polygonModel.removeAt(vertexIndex);
    coordObj->deleteLater();
    if(vertexIndex == _selectedVertexIndex) {
        selectVertex(-1);
    } else if (vertexIndex < _selectedVertexIndex) {
        selectVertex(_selectedVertexIndex - 1);
    } // else do nothing - keep current selected vertex

    _polygonPath.removeAt(vertexIndex);
    emit pathChanged();
}

void QGCMapPolygon::_polygonModelCountChanged(int count)
{
    emit countChanged(count);
}

void QGCMapPolygon::_updateCenter(void)
{
    if (!_ignoreCenterUpdates) {
        QGeoCoordinate center;

        if (_polygonPath.count() > 2) {
            QPointF centroid(0, 0);
            QPolygonF polygonF = _toPolygonF();
            for (int i=0; i<polygonF.count(); i++) {
                centroid += polygonF[i];
            }
            center = _coordFromPointF(QPointF(centroid.x() / polygonF.count(), centroid.y() / polygonF.count()));
        }
        if (_center != center) {
            _center = center;
            emit centerChanged(center);
        }
    }
}

void QGCMapPolygon::setCenter(QGeoCoordinate newCenter)
{
    if (newCenter != _center) {
        _ignoreCenterUpdates = true;

        // Adjust polygon vertices to new center
        double distance = _center.distanceTo(newCenter);
        double azimuth = _center.azimuthTo(newCenter);

        for (int i=0; i<count(); i++) {
            QGeoCoordinate oldVertex = _polygonPath[i].value<QGeoCoordinate>();
            QGeoCoordinate newVertex = oldVertex.atDistanceAndAzimuth(distance, azimuth);
            adjustVertex(i, newVertex);
        }

        if (_centerDrag) {
            // When center dragging, signals from adjustVertext are not sent. So we need to signal here when all adjusting is complete.
            emit pathChanged();
        }

        _ignoreCenterUpdates = false;

        _center = newCenter;
        emit centerChanged(newCenter);
    }
}

void QGCMapPolygon::setCenterDrag(bool centerDrag)
{
    if (centerDrag != _centerDrag) {
        _centerDrag = centerDrag;
        emit centerDragChanged(centerDrag);
    }
}

void QGCMapPolygon::setInteractive(bool interactive)
{
    if (_interactive != interactive) {
        _interactive = interactive;
        emit interactiveChanged(interactive);
    }
}

QGeoCoordinate QGCMapPolygon::vertexCoordinate(int vertex) const
{
    if (vertex >= 0 && vertex < _polygonPath.count()) {
        return _polygonPath[vertex].value<QGeoCoordinate>();
    } else {
        qWarning() << "QGCMapPolygon::vertexCoordinate bad vertex requested:count" << vertex << _polygonPath.count();
        return QGeoCoordinate();
    }
}

QList<QPointF> QGCMapPolygon::nedPolygon(void) const
{
    QList<QPointF>  nedPolygon;

    if (count() > 0) {
        QGeoCoordinate  tangentOrigin = vertexCoordinate(0);

        for (int i=0; i<_polygonModel.count(); i++) {
            double y, x, down;
            QGeoCoordinate vertex = vertexCoordinate(i);
            if (i == 0) {
                // This avoids a nan calculation that comes out of convertGeoToNed
                x = y = 0;
            } else {
                convertGeoToNed(vertex, tangentOrigin, &y, &x, &down);
            }
            nedPolygon += QPointF(x, y);
        }
    }

    return nedPolygon;
}


void QGCMapPolygon::offset(double distance)
{
    QList<QGeoCoordinate> rgNewPolygon;

    // I'm sure there is some beautiful famous algorithm to do this, but here is a brute force method

    if (count() > 2) {
        // Convert the polygon to NED
        QList<QPointF> rgNedVertices = nedPolygon();

        // Walk the edges, offsetting by the specified distance
        QList<QLineF> rgOffsetEdges;
        for (int i=0; i<rgNedVertices.count(); i++) {
            int     lastIndex = i == rgNedVertices.count() - 1 ? 0 : i + 1;
            QLineF  offsetEdge;
            QLineF  originalEdge(rgNedVertices[i], rgNedVertices[lastIndex]);

            QLineF workerLine = originalEdge;
            workerLine.setLength(distance);
            workerLine.setAngle(workerLine.angle() - 90.0);
            offsetEdge.setP1(workerLine.p2());

            workerLine.setPoints(originalEdge.p2(), originalEdge.p1());
            workerLine.setLength(distance);
            workerLine.setAngle(workerLine.angle() + 90.0);
            offsetEdge.setP2(workerLine.p2());

            rgOffsetEdges.append(offsetEdge);
        }

        // Intersect the offset edges to generate new vertices
        QPointF         newVertex;
        QGeoCoordinate  tangentOrigin = vertexCoordinate(0);
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
                return;
            }
            QGeoCoordinate coord;
            convertNedToGeo(newVertex.y(), newVertex.x(), 0, tangentOrigin, &coord);
            rgNewPolygon.append(coord);
        }
    }

    // Update internals
    _beginResetIfNotActive();
    clear();
    appendVertices(rgNewPolygon);
    _endResetIfNotActive();
}

bool QGCMapPolygon::loadKMLOrSHPFile(const QString& file)
{
    QString errorString;
    QList<QGeoCoordinate> rgCoords;
    if (!ShapeFileHelper::loadPolygonFromFile(file, rgCoords, errorString)) {
        qgcApp()->showAppMessage(errorString);
        return false;
    }

    _beginResetIfNotActive();
    clear();
    appendVertices(rgCoords);
    _endResetIfNotActive();

    return true;
}

double QGCMapPolygon::area(void) const
{
    // https://www.mathopenref.com/coordpolygonarea2.html

    if (_polygonPath.count() < 3) {
        return 0;
    }

    double coveredArea = 0.0;
    QList<QPointF> nedVertices = nedPolygon();
    for (int i=0; i<nedVertices.count(); i++) {
        if (i != 0) {
            coveredArea += nedVertices[i - 1].x() * nedVertices[i].y() - nedVertices[i].x() * nedVertices[i -1].y();
        } else {
            coveredArea += nedVertices.last().x() * nedVertices[i].y() - nedVertices[i].x() * nedVertices.last().y();
        }
    }
    return 0.5 * fabs(coveredArea);
}

void QGCMapPolygon::verifyClockwiseWinding(void)
{
    if (_polygonPath.count() <= 2) {
        return;
    }

    double sum = 0;
    for (int i=0; i<_polygonPath.count(); i++) {
        QGeoCoordinate coord1 = _polygonPath[i].value<QGeoCoordinate>();
        QGeoCoordinate coord2 = (i == _polygonPath.count() - 1) ? _polygonPath[0].value<QGeoCoordinate>() : _polygonPath[i+1].value<QGeoCoordinate>();

        sum += (coord2.longitude() - coord1.longitude()) * (coord2.latitude() + coord1.latitude());
    }

    if (sum < 0.0) {
        // Winding is counter-clockwise and needs reversal

        QList<QGeoCoordinate> rgReversed;
        for (const QVariant& varCoord: _polygonPath) {
            rgReversed.prepend(varCoord.value<QGeoCoordinate>());
        }

        _beginResetIfNotActive();
        clear();
        appendVertices(rgReversed);
        _endResetIfNotActive();
    }
}

void QGCMapPolygon::beginReset(void)
{
    _resetActive = true;
    _polygonModel.beginReset();
}

void QGCMapPolygon::endReset(void)
{
    _resetActive = false;
    _polygonModel.endReset();
    emit pathChanged();
    emit centerChanged(_center);
}

void QGCMapPolygon::_beginResetIfNotActive(void)
{
    if (!_resetActive) {
        beginReset();
    }
}

void QGCMapPolygon::_endResetIfNotActive(void)
{
    if (!_resetActive) {
        endReset();
    }
}

QDomElement QGCMapPolygon::kmlPolygonElement(KMLDomDocument& domDocument)
{
#if 0
    <Polygon id="ID">
      <!-- specific to Polygon -->
      <extrude>0</extrude>                       <!-- boolean -->
      <tessellate>0</tessellate>                 <!-- boolean -->
      <altitudeMode>clampToGround</altitudeMode>
            <!-- kml:altitudeModeEnum: clampToGround, relativeToGround, or absolute -->
            <!-- or, substitute gx:altitudeMode: clampToSeaFloor, relativeToSeaFloor -->
      <outerBoundaryIs>
        <LinearRing>
          <coordinates>...</coordinates>         <!-- lon,lat[,alt] -->
        </LinearRing>
      </outerBoundaryIs>
      <innerBoundaryIs>
        <LinearRing>
          <coordinates>...</coordinates>         <!-- lon,lat[,alt] -->
        </LinearRing>
      </innerBoundaryIs>
    </Polygon>
#endif

    QDomElement polygonElement = domDocument.createElement("Polygon");

    domDocument.addTextElement(polygonElement, "altitudeMode", "clampToGround");

    QDomElement outerBoundaryIsElement = domDocument.createElement("outerBoundaryIs");
    QDomElement linearRingElement = domDocument.createElement("LinearRing");

    outerBoundaryIsElement.appendChild(linearRingElement);
    polygonElement.appendChild(outerBoundaryIsElement);

    QString coordString;
    for (const QVariant& varCoord : _polygonPath) {
        coordString += QStringLiteral("%1\n").arg(domDocument.kmlCoordString(varCoord.value<QGeoCoordinate>()));
    }
    coordString += QStringLiteral("%1\n").arg(domDocument.kmlCoordString(_polygonPath.first().value<QGeoCoordinate>()));
    domDocument.addTextElement(linearRingElement, "coordinates", coordString);

    return polygonElement;
}

void QGCMapPolygon::setTraceMode(bool traceMode)
{
    if (traceMode != _traceMode) {
        _traceMode = traceMode;
        emit traceModeChanged(traceMode);
    }
}

void QGCMapPolygon::setShowAltColor(bool showAltColor){
    if (showAltColor != _showAltColor) {
        _showAltColor = showAltColor;
        emit showAltColorChanged(showAltColor);
    }
}

void QGCMapPolygon::selectVertex(int index)
{
    if(index == _selectedVertexIndex) return;   // do nothing

    if(-1 <= index && index < count()) {
        _selectedVertexIndex = index;
    } else {
        if (!qgcApp()->runningUnitTests()) {
            qCWarning(ParameterManagerLog)
                    << QString("QGCMapPolygon: Selected vertex index (%1) is out of bounds! "
                               "Polygon vertices indexes range is [%2..%3].").arg(index).arg(0).arg(count()-1);
        }
        _selectedVertexIndex = -1;   // deselect vertex
    }

    emit selectedVertexChanged(_selectedVertexIndex);
}

typedef struct {
    bool modified = false;
    QPointF point;
} PointInfo_t;

void proceedAvoidance(const QPointF& vertex, QList<PointInfo_t>& path, qreal* avoidDistance) {
    QLineF line(path.last().point, vertex);
    *avoidDistance += line.length();
    PointInfo_t toAppend;
    toAppend.modified = true;
    toAppend.point = vertex;
    path.append(toAppend);
}

QList<QGCMapPolygon::CoordInfo_t> QGCMapPolygon::followEdges(const QList<QGCMapPolygon::CoordInfo_t>& path, float distance)
{
#ifdef Q_OS_ANDROID
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsometimes-uninitialized"
#endif

    if (count() < 3) {
        qWarning() << "QGCMapPolygon::followEdges less than 3 vertices";
    };

    // the name `ned` here is legacy, NED is different from QPointF, see below
    // TODO rename it to pathF, also everything with `ned` prefix here
    QList<PointInfo_t> nedPath;

    // brute force save an altitude
    // suppose all altitudes are the same
    qreal baseAltitude = path[0].coord.altitude();
    _polygonPath[0].value<QGeoCoordinate>().setAltitude(baseAltitude);

    // convert path from Geo to PointF
    for (int i=0; i < path.count(); i++) {
        PointInfo_t toAppend;
        toAppend.modified = path[i].modified;
        toAppend.point = _pointFFromCoord(path[i].coord);
        nedPath.append(toAppend);
    }

    verifyClockwiseWinding();

    // list of vertices of the extend polygon
    QPolygonF nedExtendedPolygon;
    // [COPY `QGCMapPolygon::offset`]
    // but don't modified this polygon like the original
    // we just need QPolygonF to use here
    // yep, QPolygonF, not the fucking nedPolygon, they are different

    // >>>>>>>> DIFF HERE <<<<<<<<<
    QPolygonF rgNedVertices = _toPolygonF();
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
    QGeoCoordinate  tangentOrigin = vertexCoordinate(0);
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
        nedExtendedPolygon.append(newVertex);
        // >>>>>>>> DIFF HERE <<<<<<<<<
    };
    // [/COPY]

    if (nedExtendedPolygon.containsPoint(nedPath.first().point, Qt::OddEvenFill) || nedExtendedPolygon.containsPoint(nedPath.last().point, Qt::OddEvenFill)) {
        qWarning() << "QGCMapPolygon::followEdges >>> impossible <<<";
        QList<QGCMapPolygon::CoordInfo_t> empty;
        return empty;
    };

    // now let modify the path
    QList<PointInfo_t> nedModifiedPath;
    PointInfo_t currentPoint, previousPoint;
    nedModifiedPath.append(nedPath.first());
    bool modifying = false; // just 2 states
    PointInfo_t startFollowPoint;
    int startFollowEdgeIndex; // the edge that intersects with our path line

    // the list change while looping, so don't calc length beforehand
    //                      vvvvvvv
    for (int i=1; i<nedPath.count(); i++) {
        //   ^
        // the index change too, you can see it with
        // qDebug() << i;

        currentPoint = nedPath[i];
        previousPoint = nedPath[i-1];

        if (!modifying) {
            if (!nedExtendedPolygon.containsPoint(currentPoint.point, Qt::OddEvenFill)) {
                // whether the line cross the polygon ?
                // ATTENTION convex polygon please
                QLineF pathLine(previousPoint.point, currentPoint.point);
                QPointF first;
                QPointF second;
                for (int j=1; j<=nedExtendedPolygon.count(); j++) {
                    int j_ = (j == nedExtendedPolygon.count()) ? 0 : j; // handle last edge (count - 1, 0)
                    QLineF edge(nedExtendedPolygon[j-1], nedExtendedPolygon[j_]);
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
                    PointInfo_t toAppend;
                    toAppend.modified = true;
                    toAppend.point = fakePoint;
                    nedPath.insert(i, toAppend);

                    // hold index to process fakePoint
                    i--;
                    continue;
                };

                // the point is already valid and reachable
                // so don't modified
                nedModifiedPath.append(currentPoint);
                continue;
            } else {
                // start following
                modifying = true;
                // find intersected edge and point
                QLineF pathLine(previousPoint.point, currentPoint.point);
                for (int j=1; j<=nedExtendedPolygon.count(); j++) {
                    int j_ = (j == nedExtendedPolygon.count()) ? 0 : j; // handle last edge (count - 1, 0)
                    QLineF edge(nedExtendedPolygon[j-1], nedExtendedPolygon[j_]);
                    QPointF intersectPoint;
                    if (pathLine.intersect(edge, &intersectPoint) == QLineF::BoundedIntersection) {
                        startFollowPoint.modified = true;
                        startFollowPoint.point = intersectPoint;
                        startFollowEdgeIndex = j_;
                        break;
                    };
                };
            };
        } else { // following here
            // of course don't collect inbound path point
            if (nedExtendedPolygon.containsPoint(currentPoint.point, Qt::OddEvenFill)) continue;

            // but wait until we exit
            // end following
            modifying = false;

            // find intersected edge and point
            PointInfo_t endFollowPoint;
            int endFollowEdgeIndex;
            previousPoint = nedPath[i-1];
            QLineF pathLine(previousPoint.point, currentPoint.point);
            for (int j=1; j<=nedExtendedPolygon.count(); j++) {
                int j_ = (j == nedExtendedPolygon.count()) ? 0 : j; // handle last edge (count - 1, 0)
                QLineF edge(nedExtendedPolygon[j-1], nedExtendedPolygon[j_]);
                QPointF intersectPoint;
                if (pathLine.intersect(edge, &intersectPoint) == QLineF::BoundedIntersection) {
                    endFollowPoint.modified = true;
                    endFollowPoint.point = intersectPoint;
                    endFollowEdgeIndex = j_;
                    break;
                };
            };

            QList<PointInfo_t> clockwisePath;
            qreal clockwiseAvoidDistance = 0;
            QList<PointInfo_t> invClockwisePath;
            qreal invClockwiseAvoidDistance = 0;

            // now collecting
            {
                // clockwise: collect start vertex but not end vertex
                clockwisePath.append(startFollowPoint);

                // 2 phase mean passing index 0
                bool twoPhase = (startFollowEdgeIndex > endFollowEdgeIndex);
                if (twoPhase) {
                    for (int j=startFollowEdgeIndex; j<nedExtendedPolygon.count(); j++)
                    {
                        proceedAvoidance(nedExtendedPolygon[j], clockwisePath, &clockwiseAvoidDistance);
                    }
                    for (int j=0; j<endFollowEdgeIndex; j++)
                    {
                        proceedAvoidance(nedExtendedPolygon[j], clockwisePath, &clockwiseAvoidDistance);
                    }
                } else {
                    for (int j=startFollowEdgeIndex; j<endFollowEdgeIndex; j++)
                    {
                        proceedAvoidance(nedExtendedPolygon[j], clockwisePath, &clockwiseAvoidDistance);
                    }
                }

                clockwisePath.append(endFollowPoint);
                QLineF line(clockwisePath.last().point, endFollowPoint.point);
                clockwiseAvoidDistance += line.length();
                qDebug() << "clockwiseAvoidDistance: " << clockwiseAvoidDistance;
            };
            {
                // inverse clockwise: collect end vertex but not start vertex
                invClockwisePath.append(startFollowPoint);

                bool twoPhase = (startFollowEdgeIndex < endFollowEdgeIndex);
                if (twoPhase) {
                    for (int j=startFollowEdgeIndex-1; j>=0; j--)
                    {
                        proceedAvoidance(nedExtendedPolygon[j], invClockwisePath, &invClockwiseAvoidDistance);
                    }
                    for (int j=nedExtendedPolygon.count()-1; j>=endFollowEdgeIndex; j--)
                    {
                        proceedAvoidance(nedExtendedPolygon[j], invClockwisePath, &invClockwiseAvoidDistance);
                    }
                } else {
                    for (int j=startFollowEdgeIndex-1; j>=endFollowEdgeIndex; j--)
                    {
                        proceedAvoidance(nedExtendedPolygon[j], invClockwisePath, &invClockwiseAvoidDistance);
                    }
                }

                invClockwisePath.append(endFollowPoint);
                QLineF line(invClockwisePath.last().point, endFollowPoint.point);
                invClockwiseAvoidDistance += line.length();
                qDebug() << "invClockwiseAvoidDistance: " << invClockwiseAvoidDistance;
            };

            bool clockwise = clockwiseAvoidDistance <= invClockwiseAvoidDistance;
            nedModifiedPath << (clockwise ? clockwisePath : invClockwisePath);

            nedModifiedPath.append(currentPoint);
        }
    }
    
    // convert modified path from QPointF to Geo
    QList<CoordInfo_t> modifiedPath;
    for (int i=0; i < nedModifiedPath.count(); i++) {
        CoordInfo_t toAppend;
        toAppend.modified = nedModifiedPath[i].modified;
        toAppend.coord = _coordFromPointF(nedModifiedPath[i].point);

        // brute force set same altitude for all
        toAppend.coord.setAltitude(baseAltitude);

        modifiedPath.append(toAppend);
    };

    return modifiedPath;
#ifdef Q_OS_ANDROID
#pragma GCC diagnostic pop
#endif
}

void QGCMapPolygon::setPositionFromVehicle(void) 
{
    QGeoCoordinate coordinate = qgcApp()->toolbox()->multiVehicleManager()->activeVehicle()->coordinate();
    _polygonPath.append(QVariant::fromValue(coordinate));
    _polygonModel.append(new QGCQGeoCoordinate(coordinate, this));
    emit pathChanged();
}

void QGCMapPolygon::setPositionFromGCS(void)
{
    QGeoCoordinate coordinate = qgcApp()->toolbox()->qgcPositionManager()->gcsPosition();
    _polygonPath.append(QVariant::fromValue(coordinate));
    _polygonModel.append(new QGCQGeoCoordinate(coordinate, this));
    emit pathChanged();
}
