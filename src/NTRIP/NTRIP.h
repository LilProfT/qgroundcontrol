/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "QGCToolbox.h"
#include "QmlObjectListModel.h"

#include <QThread>
#include <QTcpSocket>
#include <QTcpServer>
#include <QTimer>
#include <QGeoCoordinate>

#include "Drivers/src/rtcm.h"
#include "RTCM/RTCMMavlink.h"
#include "RTKProvider.h"

class NTRIP : public QGCTool {
    Q_OBJECT
    
public:
    NTRIP(QGCApplication* app, QGCToolbox* toolbox);
    ~NTRIP();

    // QGCTool overrides
    void setToolbox(QGCToolbox* toolbox) final;
    void connectGPSNTRIP(void);
    void disconnectGPS  (void);

signals:
    void onConnect();
    void onDisconnect();
    void surveyInStatus(float duration, float accuracyMM,  double latitude, double longitude, float altitude, bool valid, bool active);
    void satelliteUpdate(int numSats);

private slots:
    void GPSPositionUpdate(GPSPositionMessage msg);
    void GPSSatelliteUpdate(GPSSatelliteMessage msg);

public slots:
    void _tcpError          (const QString errorMsg);
    void _coordinateChanged(QGeoCoordinate coordinate);
    void syncInProgressChanged(bool syncInProgress);
    void sendComplete(bool error);

private slots:

private:
    RTCMMavlink*        _rtcmMavlink = nullptr;
    RTKProvider*        _rtkProvider = nullptr;
    std::atomic_bool    _requestRTKStop; ///< signals the thread to quit
    QGeoCoordinate      _vhcPosition;
    int                 _positionChanged = 0;
};
