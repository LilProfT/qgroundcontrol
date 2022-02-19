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

#include <QObject>
#include <QDir>
#include <QTimer>
#include <QQueue>

#include "UASInterface.h"
#include "QGCLoggingCategory.h"
#include <QGeoCoordinate>
#include "QGCMAVLink.h"
#include "FlightHubHttpClient.h"
#include "QJsonObject"
#include "QJsonArray"
Q_DECLARE_LOGGING_CATEGORY(FlightHubManagerLog)

class Vehicle;

class FlightHubManager : public QGCTool
{
    Q_OBJECT

    friend class Vehicle;

public:
    FlightHubManager(QGCApplication *app, QGCToolbox *toolbox);
    // FlightHubManager(Vehicle* vehicle);
    ~FlightHubManager();
    void setToolbox(QGCToolbox *toolbox) final;

    /// Downloads the specified file.
    ///     @param fromURI  File to download from vehicle, fully qualified path. May be in the format "mftp://[;comp=<id>]..." where the component id is specified.
    ///                     If component id is not specified MAV_COMP_ID_AUTOPILOT1 is used.
    ///     @param toDir    Local directory to download file to
    /// @return true: download has started, false: error, no download
    /// Signals downloadComplete, commandError, commandProgress
    // bool download(const QString& fromURI, const QString& toDir);
    void startTimer(int interval);

signals:
    void publishTelemetry(QJsonDocument doc);
public slots:
    void timerSlot();

private slots:
    void _vehicleCoordinatedChanged(const QGeoCoordinate& coordinate);
    void _vehicleReady(bool isReady);
    void _clientReady(bool isReady);
private:

    QString _handleAltitude(const mavlink_message_t &message);
    QString _handleGpsRawInt(const mavlink_message_t &message);
    QString _handleBatteryStatus(const mavlink_message_t &message);
    void _handleHighLatency(const mavlink_message_t &message);
    void _handleHighLatency2(const mavlink_message_t &message);

    Vehicle *_vehicle;
    // FlightHubMqtt   *_flightHubMQtt     = nullptr;
    FlightHubHttpClient *_flightHubHttpClient = nullptr;
    QThread _clientThread;
    QJsonArray _positionArray;
};
