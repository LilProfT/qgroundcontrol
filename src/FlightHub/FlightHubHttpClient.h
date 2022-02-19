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
#include <QByteArray>
#include <QObject>
#include <QJsonDocument>
#include <QNetworkAccessManager>
#include "QGCLoggingCategory.h"
Q_DECLARE_LOGGING_CATEGORY(FlightHubHttpClientLog)
// #include <QtMqtt/QMqttClient>

// class FlightHubMqtt : public QMqttClient
// {
//     Q_OBJECT
// public:

//     FlightHubMqtt(QObject *parent);
//     ~FlightHubMqtt();

//     void init();
//     void initParams(const QString& hostAddress, int port, const QString& user, const QString& passwd);

// public slots:
//     void publishMsg(QByteArray message);

// private slots:
//     void updateLogStateChange();
//     void updateConnected();
//     void updateDisConnected();

// private:

//     QString _hostAddress;
//     int     _port;
//     QString _user;
//     QString _passwd;
// };

class FlightHubHttpClient : public QObject
{
    Q_OBJECT
public:
    FlightHubHttpClient(QObject *parent);
    ~FlightHubHttpClient();

    void setParams(const QString &hostAddress, const QString &deviceToken);
public slots:
    void init();
    void publishTelemetry(QJsonDocument doc);

private slots:
    void _onGetAccessTokenFinished(QNetworkReply * reply);
    void _onPublishTelemetryFinished(QNetworkReply * reply);

signals:
    void parameterReadyClientAvailableChanged(bool isReady);


private:

    QString _getAccessToken();
    QNetworkAccessManager *_getAccessTokenManager = nullptr;
    QString _hostAddress;
    QString _accessToken;
    QString _deviceToken;
    const QString _nullToken = QString();
    QNetworkAccessManager *_publishTelemetryManager = nullptr;
    bool _triedGetToken = false;
};
