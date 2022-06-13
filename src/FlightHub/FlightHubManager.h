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
#include "MissionItem.h"
#include <QNetworkAccessManager>
#include <QVariantList>
#include "PlanItem.h"

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

    Q_PROPERTY(QmlObjectListModel*  planList MEMBER _planList NOTIFY planListChanged)
    Q_PROPERTY(QString  downloadedFile MEMBER _downloadedFile NOTIFY donwloadedFileChanged)

    /// Downloads the specified file.
    ///     @param fromURI  File to download from vehicle, fully qualified path. May be in the format "mftp://[;comp=<id>]..." where the component id is specified.
    ///                     If component id is not specified MAV_COMP_ID_AUTOPILOT1 is used.
    ///     @param toDir    Local directory to download file to
    /// @return true: download has started, false: error, no download
    /// Signals downloadComplete, commandError, commandProgress
    // bool download(const QString& fromURI, const QString& toDir);
    void startTimer(int interval);
    void startUploadOfflineStatTimer(int interval);
    void startConnectFlightHubTimer(int interval);
    void startUploadOfflinePlanTimer(int interval);

    void uploadStatistic(QList<MissionItem *> items);

    void uploadPlanFile(const QJsonDocument& json,const QGeoCoordinate& coordinate,const double& area,const QString &filename );

    Q_INVOKABLE   void downloadPlanFileFromPlanView(const int index);

signals:
    void publishTelemetry(QJsonObject obj);
    void publishStat(QJsonObject obj);
    void publishPlan(const QJsonDocument& json,const QGeoCoordinate& coordinate,const double& area,const QString &filename );
    void publishOfflinePlan(const QJsonDocument& json,const QGeoCoordinate& coordinate,const double& area,const QString &filename, const QString& localFilename );
    void fetchPlans(const QString& search,double longitude, double latitude, bool isGetAll);
    void planListChanged                 (QmlObjectListModel* _planList);
    void donwloadedFileChanged(QString file);

    void downloadPlanFile(const int id);
public slots:
    void timerSlot();
    void uploadOfflineStatTimerSlot();
    void connectFlightHubTimerSlot();
    void uploadOfflinePlanTimerSlot();

private slots:
    void _onFetchedPlans(const QList<PlanItem*> plans);
    void _onVehicleCoordinatedChanged(const QGeoCoordinate& coordinate);
    void _onVehicleReady(bool isReady);
    void _onClientReady(bool isReady);
    void _onVehicleMissionCompleted();

    void _onVehicleSetSprayedArea(double area);

    void _uploadOfflineFinished(QNetworkReply * reply);

    void _downloadPlanFileFinished(const QString file);

private:
    Vehicle *_vehicle;

    FlightHubHttpClient *_flightHubHttpClient = nullptr;
    bool _clientReady = false;
    QThread _clientThread;
    QJsonArray _positionArray;
    QJsonArray _batteryArray;

    QNetworkAccessManager* _uploadOfflineManager = nullptr;

    int _oldAreaValue = 0;

    QmlObjectListModel* _planList = nullptr;
    QString _downloadedFile = "";
};
