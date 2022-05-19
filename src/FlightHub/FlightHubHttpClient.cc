// /****************************************************************************
//  *
//  * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
//  *
//  * QGroundControl is licensed according to the terms in the file
//  * COPYING.md in the root of the source code directory.
//  *
//  ****************************************************************************/

// #include "FlightHubMqtt.h"
#include "QGCApplication.h"
#include <QDebug>
#include "FlightHubHttpClient.h"
#include "SettingsManager.h"
#include <QFile>
QGC_LOGGING_CATEGORY(FlightHubHttpClientLog, "FlightHubHttpClientLog")



FlightHubHttpClient::FlightHubHttpClient(QObject *parent) : QObject(parent), _getAccessTokenManager(new QNetworkAccessManager(this)), _getUserAccessTokenManager(new QNetworkAccessManager(this)), _publishTelemetryManager(new QNetworkAccessManager(this)), _publishStatManager(new QNetworkAccessManager(this))

{

    connect(_getAccessTokenManager, &QNetworkAccessManager::finished, this, &FlightHubHttpClient::_onGetDeviceAccessTokenFinished);

    connect(_publishTelemetryManager, &QNetworkAccessManager::finished, this, &FlightHubHttpClient::_onPublishTelemetryFinished);

    connect(_getUserAccessTokenManager, &QNetworkAccessManager::finished, this, &FlightHubHttpClient::_onGetUserAccessTokenFinished);
    connect(_publishStatManager, &QNetworkAccessManager::finished, this, &FlightHubHttpClient::_onPublishStatFinished);
}
void FlightHubHttpClient::_onPublishStatFinished(QNetworkReply *reply)
{
    qCWarning(FlightHubHttpClientLog) << reply->readAll();
    if (reply->error())
    {
        if (reply->error() == QNetworkReply::NetworkError::HostNotFoundError)
        {
            auto folderPath = qgcApp()->toolbox()->settingsManager()->appSettings()->resumeSavePath();
            QDir  dir(folderPath);
            dir.cdUp();
            auto filePath = dir.filePath("offline_stats.json");

            QFile readFile (filePath);
            QString text = "[]";
            if (readFile.exists()){
                if (readFile.open(QIODevice::ReadOnly | QIODevice::Text)){
                    text = readFile.readAll();
                }
            }
            QJsonDocument doc = QJsonDocument::fromJson(text.toUtf8());
            QJsonArray array = doc.array();
            _currentStat["deviceAccessToken"] =  _deviceAccessToken;

            array.append(_currentStat);

            QFile writeFile(filePath);


            if (writeFile.open(QIODevice::WriteOnly)){
                QTextStream stream(&writeFile);
                QJsonDocument doc;
                doc.setArray(array);
                qCWarning(FlightHubHttpClientLog) << "Writing file"<< doc.toJson();
                stream << doc.toJson() <<Qt::endl;
            }


        }
    }
}
void FlightHubHttpClient::_onGetUserAccessTokenFinished(QNetworkReply *reply)
{

    if (reply->error())
    {
        return;
    }

    QJsonDocument json = QJsonDocument::fromJson(reply->readAll());
    auto data = json.object().value("data").toObject();
    auto accessToken = data["accessToken"].toString();
    _userAccessToken = accessToken;
    _user = data["user"].toObject();

    if (_deviceAccessToken.isEmpty() || _deviceAccessToken.isNull())
    {
        return;
    }
    emit parameterReadyClientAvailableChanged(true);
}
FlightHubHttpClient::~FlightHubHttpClient()
{
}

void FlightHubHttpClient::init()
{

    _getUserAccessToken();
    _getDeviceAccessToken();
}

QString FlightHubHttpClient::_getUserAccessToken()
{
    if (!_triedGetUserToken)
    {
        _triedGetUserToken = true;
        QString domain = _userHostAddress;
        qCWarning(FlightHubHttpClientLog) << "login" << domain << _email << _password;
        if (domain.isEmpty() || domain.isNull())
        {
            _userAccessToken = _nullToken;

            return _userAccessToken;
        }
        QNetworkRequest request;
        auto url = domain + "/auth/login";

        request.setUrl(QUrl(url));
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
        QJsonObject obj;
        obj["email"] = _email;
        obj["password"] = _password;
        QJsonDocument doc(obj);
        QByteArray data = doc.toJson();

        qCWarning(FlightHubHttpClientLog) << "login" << url << _email << _password << data;
        _getUserAccessTokenManager->post(request, data);
    }
    return _userAccessToken;
}

void FlightHubHttpClient::publishStat(QJsonObject obj)
{
    obj["accessToken"] = _userAccessToken;
    obj["pilotName"] = _user["email"].toString();
    auto now = QDateTime::currentDateTimeUtc().toString("yyyy-MM-ddThh:mm:ss");
    obj["flightTime"] = now;

    QJsonDocument doc;
    doc.setObject(obj);

    QNetworkRequest request;
    QString domain = _hostAddress;

    auto url = domain + "/authorizeddevices/me/flightstats";

    request.setUrl(QUrl(url));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    auto token = "Bearer " + _getDeviceAccessToken();
    request.setRawHeader(QString("Authorization").toUtf8(), token.toUtf8());

    QByteArray data = doc.toJson();
    QDateTime date = QDateTime::currentDateTime();
    QString formattedTime = date.toString("dd.MM.yyyy hh:mm:ss");
    QByteArray formattedTimeMsg = formattedTime.toLocal8Bit();
    auto conn = std::make_shared<QMetaObject::Connection>();
    _currentStat = obj;
    _publishStatManager->post(request, data);
}

void FlightHubHttpClient::publishTelemetry(QJsonObject obj)
{
    obj["accessToken"] = _userAccessToken;
    QJsonDocument doc;
    doc.setObject(obj);
    QString dataToString(doc.toJson());

    QNetworkRequest request;
    QString domain = _hostAddress;

    auto url = domain + "/authorizeddevices/me/telemetryrecords";

    request.setUrl(QUrl(url));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    auto token = "Bearer " + _getDeviceAccessToken();
    request.setRawHeader(QString("Authorization").toUtf8(), token.toUtf8());

    QByteArray data = doc.toJson();

    QDateTime date = QDateTime::currentDateTime();
    QString formattedTime = date.toString("dd.MM.yyyy hh:mm:ss");
    QByteArray formattedTimeMsg = formattedTime.toLocal8Bit();
    auto conn = std::make_shared<QMetaObject::Connection>();
    _publishTelemetryManager->post(request, data);
}
void FlightHubHttpClient::_onGetDeviceAccessTokenFinished(QNetworkReply *reply)
{
    if (reply->error())
    {

        return;
    }

    QJsonDocument json = QJsonDocument::fromJson(reply->readAll());
    auto data = json.object().value("data").toObject();
    auto accessToken = data["accessToken"].toString();
    _deviceAccessToken = accessToken;
    qCWarning(FlightHubHttpClientLog) << "Retrivce token" << _deviceAccessToken;
    if (_userAccessToken.isEmpty() || _userAccessToken.isNull())
    {
        return;
    }
    emit parameterReadyClientAvailableChanged(true);
}

void FlightHubHttpClient::_onPublishTelemetryFinished(QNetworkReply *reply)
{
}

QString FlightHubHttpClient::_getDeviceAccessToken()
{
    if (!_triedGetToken)
    {
        _triedGetToken = true;
        QString domain = _hostAddress;
        if (domain.isEmpty() || domain.isNull())
        {
            _deviceAccessToken = _nullToken;

            return _deviceAccessToken;
        }

        QString token = _deviceToken;
        if (token.isEmpty() || token.isNull())
        {
            _deviceAccessToken = _nullToken;

            return _deviceAccessToken;
        }

        QNetworkRequest request;
        auto url = domain + "/auth/GenerateDeviceToken";
        request.setUrl(QUrl(url));
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
        QJsonObject obj;
        obj["deviceToken"] = token;
        obj["deviceCode"] = _deviceCode;

        QJsonDocument doc(obj);
        QByteArray data = doc.toJson();

        qCWarning(FlightHubHttpClientLog) << "Retrivce device token" << data;
        _getAccessTokenManager->post(request, data);
    }
    return _deviceAccessToken;
}

void FlightHubHttpClient::setParams(const QString &hostAddress, const QString &deviceToken, const QString &userHostAddress, const QString &email, const QString &password, const QString &deviceCode)
{
    _hostAddress = hostAddress;
    _deviceToken = deviceToken;
    _userHostAddress = userHostAddress;
    _email = email;
    _password = password;
    _deviceCode = deviceCode;
}
