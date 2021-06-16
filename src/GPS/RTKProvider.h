/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#pragma once

#include <QString>
#include <QThread>
#include <QByteArray>

#include <atomic>

#include "GPSPositionMessage.h"
#include <QTcpSocket>
#include <QTcpServer>

#include <QTimer>
#include <QGeoCoordinate>
#include "Drivers/src/rtcm.h"

/**
 ** class RTKProvider
 * opens a GPS device and handles the protocol
 */
class NTRIPTCPLink : public QThread
{
    Q_OBJECT

public:
    NTRIPTCPLink(const QString& hostAddress, int port, const QString& user, const QString& passwd, const QString& mntpnt, QObject* parent);
    ~NTRIPTCPLink();
    bool close();
    void setCoordinate(QGeoCoordinate coordinate);
    void startTimer(int interval);
    QGeoCoordinate      vhcPosition;

public slots:
    void timerSlot();
signals:
    void gotRTCMData(QByteArray data);
    void error(const QString errorMsg);

protected:
    void run(void) final;

signals:
    void connected                      ();
    void disconnected                   ();

private slots:
    void _readBytes(void);
    void _newConnection(void);

private:
    enum class NTRIPState {
            uninitialised,
            waiting_for_http_response,
            waiting_for_rtcm_header,
            accumulating_rtcm_packet,
        };

    void _hardwareConnect(void);
    int  _calcNMEAChecksum(const char *buf, int cnt );
    void _parseLine(const QString& line);
    void _parse(const QByteArray &buffer);
    void _socketDisconnected(void);
    QString         _hostAddress;
    int             _port;
    QString         _user;
    QString         _passwd;
    QString         _mntpnt;

    QTcpSocket*     _tcpSocket =   nullptr;
    QTcpServer*     _tcpServer = nullptr;
    bool            _serverMode = true;
    RTCMParsing*    _rtcm_parsing{nullptr};
    NTRIPState      _state;
};

class RTKProvider : public QThread
{
    Q_OBJECT
public:

    RTKProvider(const QString& hostAddress,
                int port, const QString& user, const QString& passwd, const QString& mntpnt,
                const std::atomic_bool& requestStop);
    ~RTKProvider();
    QGeoCoordinate          vhcPosition;
    bool                    syncInProgress;
    /**
     * this is called by the callback method
     */
    void gotRTCMData(QByteArray data);
    void setCoordinate(QGeoCoordinate coordinate);
signals:
    void positionUpdate(GPSPositionMessage message);
    void RTCMDataUpdate(QByteArray message);
    void satelliteInfoUpdate(GPSSatelliteMessage message);
    void surveyInStatus(float duration, float accuracyMM, double latitude, double longitude, float altitude, bool valid, bool active);
protected:
    void run();

private:
    QString                 _hostAddress;
    int                     _port;
    QString                 _user;
    QString                 _passwd;
    QString                 _mntpnt;
    const std::atomic_bool& _requestStop;
    int                     _count = 0;
    NTRIPTCPLink*           _tcpLink = nullptr;
};
