/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#include "RTKProvider.h"
#include "QGCLoggingCategory.h"
#include "QGCApplication.h"
#include "SettingsManager.h"
#include "qdatetime.h"
#include <QtMath>
#include <stdint.h>

#define GPS_RECEIVE_TIMEOUT 1200

#include <QDebug>

//#define SIMULATE_RTCM_OUTPUT //if defined, generate simulated RTCM messages
//additionally make sure to call connectGPS(""), eg. from QGCToolbox.cc
#define MISMART_RTK true

void RTKProvider::run()
{
#ifdef SIMULATE_RTCM_OUTPUT
    const int fakeMsgLengths[3] = { 30, 170, 240 };
    uint8_t* fakeData = new uint8_t[fakeMsgLengths[2]];
    while (!_requestStop) {
        for (int i = 0; i < 3; ++i) {
            gotRTCMData((uint8_t*) fakeData, fakeMsgLengths[i]);
            msleep(4);
        }
        msleep(100);
    }
    delete[] fakeData;
#endif /* SIMULATE_RTCM_OUTPUT */
    while (!_requestStop) {
        if (_tcpLink == nullptr) {
            _tcpLink = new NTRIPTCPLink(_hostAddress, _port, _user, _passwd, _mntpnt, this->parent());
            connect(_tcpLink, &NTRIPTCPLink::gotRTCMData,  this, &RTKProvider::gotRTCMData,  Qt::QueuedConnection);
            _tcpLink->startTimer(1000);
        }

        if (_tcpLink->vhcPosition != vhcPosition)
            _tcpLink->setCoordinate(vhcPosition);
    }

    qCDebug(RTKGPSLog) << "Exiting RTKGPS thread";
}

RTKProvider::RTKProvider(const QString& hostAddress, int port, const QString& user, const QString& passwd, const QString& mntpnt,
                         const std::atomic_bool& requestStop
                         )
    : _hostAddress                   (hostAddress)
    , _port                     (port)
    , _user                     (user)
    , _passwd                     (passwd)
    , _mntpnt                     (mntpnt)
    , _requestStop            (requestStop)

{
}

RTKProvider::~RTKProvider()
{
}

void RTKProvider::gotRTCMData(QByteArray data)
{
    if (!syncInProgress) {
        qCWarning(RTKGPSLog) << "RTCMDataUpdate";
        emit RTCMDataUpdate(data);
    }
}

//------------------------------------------------------------------------------//

NTRIPTCPLink::NTRIPTCPLink(const QString& hostAddress, int port, const QString& user, const QString& passwd, const QString& mntpnt, QObject* parent)
    : QThread       (parent)
    , _hostAddress  (hostAddress)
    , _port                     (port)
    , _user                     (user)
    , _passwd                     (passwd)
    , _mntpnt                     (mntpnt)
{
    moveToThread(this);
    start();

    if (!_rtcm_parsing) {
        _rtcm_parsing = new RTCMParsing();
    }
    _rtcm_parsing->reset();
    _state = NTRIPState::uninitialised;
}

NTRIPTCPLink::~NTRIPTCPLink(void)
{
    if (_tcpSocket)
    {
        QObject::disconnect(_tcpSocket, &QTcpSocket::readyRead, this, &NTRIPTCPLink::_readBytes);
        _tcpSocket->disconnectFromHost();
        _tcpSocket->deleteLater();
        _tcpSocket = nullptr;
    }
    quit();
    wait();
}

void NTRIPTCPLink::startTimer(int interval)
{
    QTimer::singleShot(interval, this, &NTRIPTCPLink::timerSlot);
}

void NTRIPTCPLink::timerSlot()
{
    qCWarning(RTKGPSLog) << "timerSlot";
    if (_tcpSocket != nullptr && vhcPosition.longitude() == vhcPosition.longitude()) {
        double latdms = (int) vhcPosition.latitude() + (vhcPosition.latitude() - (int) vhcPosition.latitude()) * .6f;
        double lngdms = (int) vhcPosition.longitude() + (vhcPosition.longitude() - (int) vhcPosition.longitude()) * .6f;
        auto time = QDateTime::currentDateTimeUtc();
        QString sTime = time.toString("HHmmss");
        QString GPGGA = QString( "$GP%0,%1.00,%2,%3,%4,%5,%6,%7,%8,%9,%10,%11,%12,%13,%14")
                .arg("GGA")
                .arg(sTime)
                .arg(QString::number(qAbs(latdms) * 100, 'f', 8))
                .arg(vhcPosition.latitude() < 0 ? "S" : "N")
                .arg(QString::number(qAbs(lngdms) * 100, 'f', 8))
                .arg(vhcPosition.longitude() < 0 ? "W" : "E")
                .arg(1)
                .arg(10)
                .arg(1)
                .arg(0) //alt
                .arg("M")
                .arg(0)
                .arg("M")
                .arg("0.0")
                .arg("0");

        QString hexadecimal;
        //QString s = QString("$GPGGA,172814.0,3723.46587704,N,12202.26957864,W,2,6,1.2,18.893,M,-25.669,M,2.0,0031");
        //hexadecimal.setNum(calc_NMEA_Checksum (s.toStdString().c_str(), s.length()),16);

        hexadecimal.setNum(_calcNMEAChecksum(GPGGA.toStdString().c_str(), GPGGA.length()), 16);
        QString nmea = QString("%0*%1").arg(GPGGA).arg(hexadecimal);
        qCWarning(RTKGPSLog) << nmea;

        _tcpSocket->write(nmea.toUtf8());
    }
    startTimer(2000);
}

void NTRIPTCPLink::run(void)
{
    _hardwareConnect();
    exec();
}

bool NTRIPTCPLink::close()
{
    bool res = (_tcpSocket || _tcpServer);
    if(_tcpSocket) {
        qCDebug(RTKGPSLog) << "Close RTKGPS TCP socket on port" << _tcpSocket->localPort();
        _tcpSocket->close();
        _tcpSocket->deleteLater();
        _tcpSocket = nullptr;
    }
    if(_tcpServer) {
        qCDebug(RTKGPSLog) << "Close RTKGPS TCP server on port" << _tcpServer->serverPort();;
        _tcpServer->close();
        _tcpServer->deleteLater();
        _tcpServer = nullptr;
    }
    return res;
}

void NTRIPTCPLink::setCoordinate(QGeoCoordinate coordinate)
{
    vhcPosition = coordinate;
}

void NTRIPTCPLink::_hardwareConnect()
{
    close();
    _serverMode = _hostAddress == QHostAddress::AnyIPv4;
    if(_serverMode) {
        if(!_tcpServer) {
            qCDebug(RTKGPSLog) << "Listen for Taisync TCP on port" << _port;
            _tcpServer = new QTcpServer(this);
            QObject::connect(_tcpServer, &QTcpServer::newConnection, this, &NTRIPTCPLink::_newConnection);
            _tcpServer->listen(QHostAddress::AnyIPv4, _port);
        }
    } else {
        _tcpSocket = new QTcpSocket();
        QObject::connect(_tcpSocket, &QIODevice::readyRead, this, &NTRIPTCPLink::_readBytes);
        qCDebug(RTKGPSLog) << "Connecting to" << _hostAddress;
        _tcpSocket->connectToHost(_hostAddress, _port);
        //-- TODO: This has to be removed. It's blocking the main thread.
        if (!_tcpSocket->waitForConnected(1000)) {
            close();
            return;
        }

        QString userpwd = QString("%1:%2").arg(_user).arg(_passwd);
        QString base64 = userpwd.toUtf8().toBase64();

        if (_user.isEmpty() && _passwd.isEmpty()) {
            QString message = QString("GET %1/%2 HTTP/1.1\r\n"
                                "Ntrip-Version: Ntrip/2.0\r\n"
                                "User-Agent: NTRIP %3\r\n"
                                "Connection: close\r\n"
                                "\r\n"
                                "%4\r\n"
                                )
                    .arg("").arg(_mntpnt).arg("MiSmart RTK").arg("RTCM");

            _tcpSocket->write(message.toUtf8());
        } else {
            QString message = QString("GET %1/%2 HTTP/1.1\r\n"
                                "Ntrip-Version: Ntrip/2.0\r\n"
                                "User-Agent: NTRIP %3\r\n"
                                "Authorization: Basic %4\r\n"
                                "Connection: close\r\n"
                                "\r\n"
                                "%5\r\n"
                                )
                    .arg("").arg(_mntpnt).arg("MiSmart RTK").arg(base64).arg("RTCM");
            qCWarning(RTKGPSLog) << message;

            _tcpSocket->write(message.toUtf8());
            _state = NTRIPState::waiting_for_http_response;
        }

        emit connected();
    }
}

int NTRIPTCPLink::_calcNMEAChecksum(const char *buf, int cnt )
{
    char character;
    int checksum = 0;
    int i;              // loop counter
    for (i = 0; i < cnt; ++i)
    {
        character = buf[i];
        switch(character)
        {
        case '$':
            // Ignore the dollar sign
            break;
        case '*':
            // Stop processing before the asterisk
            i = cnt;
            continue;
        default:
            // Is this the first value for the checksum?
            if (checksum == 0)
            {
                // Yes. Set the checksum to the value
                checksum = character;
            }
            else
            {
                // No. XOR the checksum with this character's value
                checksum = checksum ^ character;
            }
            break;
        }
    }

    // Return the checksum
    return (checksum);

} // calc_NEMA_Checksum()

void NTRIPTCPLink::_parse(const QByteArray &buffer)
{
    for(const u_int8_t& byte : buffer){
        if(_state == NTRIPState::waiting_for_rtcm_header){
            if(byte != RTCM3_PREAMBLE)
                continue;
            _state = NTRIPState::accumulating_rtcm_packet;
        }
        if(_rtcm_parsing->addByte(byte)){
            _state = NTRIPState::waiting_for_rtcm_header;
            QByteArray message((char*)_rtcm_parsing->message(), static_cast<int>(_rtcm_parsing->messageLength()));
            //TODO: Restore the following when upstreamed in Driver repo
            //uint16_t id = _rtcm_parsing->messageId();
            u_int16_t id = ((u_int8_t)message[3] << 4) | ((u_int8_t)message[4] >> 4);
            qCWarning(RTKGPSLog) << QString::fromStdString(message.toStdString());

            emit gotRTCMData(message);
            qCWarning(NTRIPLog) << "Sending " << id << "of size " << message.length();

            _rtcm_parsing->reset();
        }
    }
}

void NTRIPTCPLink::_readBytes(void)
{
    if (_tcpSocket) {
        QByteArray bytes = _tcpSocket->readAll();
        QString response = QString::fromStdString(bytes.toStdString());

        if(_state == NTRIPState::waiting_for_http_response) {
            QString line = _tcpSocket->readLine();
            if (line.contains("200")){
                _state = NTRIPState::waiting_for_rtcm_header;
            } else {
                qCWarning(NTRIPLog) << "Server responded with " << line;
                // TODO: Handle failure. Reconnect?
                // Just move into parsing mode and hope for now.
                _state = NTRIPState::waiting_for_rtcm_header;
            }
        }
        _parse(bytes);
    }
}

void NTRIPTCPLink::_newConnection(void)
{
    qCDebug(RTKGPSLog) << "New Taisync TCP Connection on port" << _tcpServer->serverPort();
    if(_tcpSocket) {
        _tcpSocket->close();
        _tcpSocket->deleteLater();
    }
    _tcpSocket = _tcpServer->nextPendingConnection();
    if(_tcpSocket) {
        QObject::connect(_tcpSocket, &QIODevice::readyRead, this, &NTRIPTCPLink::_readBytes);
        QObject::connect(_tcpSocket, &QAbstractSocket::disconnected, this, &NTRIPTCPLink::_socketDisconnected);
        emit connected();
    } else {
        qCWarning(RTKGPSLog) << "New Taisync TCP Connection provided no socket";
    }
}

//-----------------------------------------------------------------------------
void NTRIPTCPLink::_socketDisconnected()
{
    qCDebug(RTKGPSLog) << "Taisync TCP Connection Closed on port" << _tcpSocket->localPort();
    if(_tcpSocket) {
        _tcpSocket->close();
        _tcpSocket->deleteLater();
        _tcpSocket = nullptr;
        emit disconnected();
    }
}
