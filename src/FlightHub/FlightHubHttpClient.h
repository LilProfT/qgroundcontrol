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
#include <QJsonObject>
#include <QNetworkAccessManager>
#include "QGCLoggingCategory.h"
Q_DECLARE_LOGGING_CATEGORY(FlightHubHttpClientLog)


class FlightHubHttpClient : public QObject
{
    Q_OBJECT
public:
    FlightHubHttpClient(QObject *parent);
    ~FlightHubHttpClient();

    void setParams(const QString &hostAddress, const QString &deviceToken ,const QString &userHostAddress, const QString &email, const QString &password,const QString &deviceCode);
public slots:
    void init();
    void publishTelemetry(QJsonObject obj);
    void publishStat(QJsonObject obj);

private slots:
    void _onGetDeviceAccessTokenFinished(QNetworkReply * reply);
    void _onPublishTelemetryFinished(QNetworkReply * reply);
    void _onGetUserAccessTokenFinished(QNetworkReply * reply);
    void _onPublishStatFinished(QNetworkReply * reply);
signals:
    void parameterReadyClientAvailableChanged(bool isReady);


private:

    QString _getDeviceAccessToken();
    QString _getUserAccessToken();
    QNetworkAccessManager *_getAccessTokenManager = nullptr;
    QNetworkAccessManager *_getUserAccessTokenManager = nullptr;
    QNetworkAccessManager *_publishTelemetryManager = nullptr;
    QNetworkAccessManager *_publishStatManager = nullptr;
    QString _hostAddress;
    QString _deviceCode;
    QString _userHostAddress;
    QString _deviceAccessToken ;
    QString _userAccessToken ;
    QString _deviceToken;
    QString _email;
    QString _password;
    const QString _nullToken = QString();

    QJsonObject user;

    bool _triedGetToken = false;
    bool _triedGetUserToken = false;
};
