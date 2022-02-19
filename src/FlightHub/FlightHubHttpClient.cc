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
QGC_LOGGING_CATEGORY(FlightHubHttpClientLog, "FlightHubHttpClientLog")

FlightHubHttpClient::FlightHubHttpClient(QObject *parent) : QObject(parent),
    _getAccessTokenManager(new QNetworkAccessManager(this)),
    _publishTelemetryManager(new QNetworkAccessManager(this))
{
    connect(_getAccessTokenManager, &QNetworkAccessManager::finished , this,  &FlightHubHttpClient::_onGetAccessTokenFinished);
    connect(_publishTelemetryManager, &QNetworkAccessManager::finished , this,  &FlightHubHttpClient::_onPublishTelemetryFinished);
}

FlightHubHttpClient::~FlightHubHttpClient()
{
}

void FlightHubHttpClient::init()
{
    _getAccessToken();
}

void FlightHubHttpClient::publishTelemetry(QJsonDocument doc)
{

    QString dataToString(doc.toJson());

    //    qCWarning(FlightHubHttpClientLog) << "Publish telemetry" << dataToString;
    QNetworkRequest request;
    QString domain = _hostAddress;

    auto url = domain + "/authorizeddevices/me/telemetryrecords";

    request.setUrl(QUrl(url));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    auto token = "Bearer " + _getAccessToken();
    request.setRawHeader(QString("Authorization").toUtf8(), token.toUtf8());

    QByteArray data = doc.toJson();
    QDateTime date = QDateTime::currentDateTime();
    QString formattedTime = date.toString("dd.MM.yyyy hh:mm:ss");
    QByteArray formattedTimeMsg = formattedTime.toLocal8Bit();
    qCWarning(FlightHubHttpClientLog) << "Publish telemetry"<< formattedTime;
    auto conn = std::make_shared<QMetaObject::Connection>();
    _publishTelemetryManager->post(request, data);
}
void FlightHubHttpClient::_onGetAccessTokenFinished(QNetworkReply * reply){
    if (reply->error())
    {
        emit parameterReadyClientAvailableChanged(false);
    }
    else
    {
        QJsonDocument json = QJsonDocument::fromJson(reply->readAll());
        auto data = json.object().value("data").toObject();
        auto accessToken = data["accessToken"].toString();
        _accessToken = accessToken;
        qCWarning(FlightHubHttpClientLog)<<"Retrivce token" << _accessToken;
        emit parameterReadyClientAvailableChanged(true);
    }
}

void FlightHubHttpClient::_onPublishTelemetryFinished(QNetworkReply * reply){
    qCWarning(FlightHubHttpClientLog)<<"reply"<<reply->readAll();
    if (reply->error())
    {
        qCWarning(FlightHubHttpClientLog)<<"error"<<reply->readAll();
    }
    else
    {
        qCWarning(FlightHubHttpClientLog)<<"oh yeah"<<reply->readAll();
    }
}

QString FlightHubHttpClient::_getAccessToken()
{
    if (!_triedGetToken)
    {
        qCWarning(FlightHubHttpClientLog) << "Get token";

        _triedGetToken = true;
        QString domain = _hostAddress;
        if (domain.isEmpty() || domain.isNull())
        {
            _accessToken = _nullToken;

            return _accessToken;
        }

        QString token = _deviceToken;
        if (token.isEmpty() || token.isNull())
        {
            _accessToken = _nullToken;

            return _accessToken;
        }

        QNetworkRequest request;
        auto url = domain + "/auth/GenerateDeviceToken";
        request.setUrl(QUrl(url));
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
        QJsonObject obj;
        obj["deviceToken"] = token;
        QJsonDocument doc(obj);
        QByteArray data = doc.toJson();


        _getAccessTokenManager->post(request, data);
    }
    return _accessToken;
}

void FlightHubHttpClient::setParams(const QString &hostAddress, const QString &deviceToken)
{
    _hostAddress = hostAddress;
    _deviceToken = deviceToken;
}

// void FlightHubMqtt::initParams(const QString& hostAddress, int port, const QString& user, const QString& passwd)
// {
//     _hostAddress = hostAddress;
//     _port = port;
//     _user = user;
//     _passwd = passwd;
// }

// void FlightHubMqtt::init()
// {
//     this->setHostname(_hostAddress);
//     this->setPort(_port);
//     this->setUsername(_user);
//     this->setPassword(_passwd);
//     this->setClientId("bb8f4af0-d7c9-11ea-8f7f-59cfb267c8ff");
//     this->setKeepAlive(3600);

//     connect(this, &FlightHubMqtt::stateChanged, this, &FlightHubMqtt::updateLogStateChange);
//     connect(this, &FlightHubMqtt::connected, this, &FlightHubMqtt::updateConnected);
//     connect(this, &FlightHubMqtt::disconnected, this, &FlightHubMqtt::updateDisConnected);

//     connect(this, &FlightHubMqtt::messageReceived, this, [](const QByteArray &message, const QMqttTopicName &topic) {
//         const QString content = QDateTime::currentDateTime().toString()
//                 + QLatin1String(" Received Topic: ")
//                 + topic.name()
//                 + QLatin1String(" Message: ")
//                 + message
//                 + QLatin1Char('\n');
//     });

//     connect(this, &FlightHubMqtt::pingResponseReceived, this, []() {
//         const QString content = QDateTime::currentDateTime().toString()
//                 + QLatin1String(" PingResponse")
//                 + QLatin1Char('\n');
//     });

//     this->connectToHost();
// }

// void FlightHubMqtt::updateLogStateChange()
// {
//     const QString content = QDateTime::currentDateTime().toString()
//             + QLatin1String(": State Change")
//             + QString::number(this->state())
//             + QLatin1Char('\n');

//     qCDebug(RTKGPSLog) << "updateLogStateChange: " << content;
// }

// void FlightHubMqtt::updateConnected()
// {
//     const QString content = QDateTime::currentDateTime().toString()
//             + QLatin1String(": State Change")
//             + QString::number(this->state())
//             + QLatin1Char('\n');

//     qCDebug(RTKGPSLog) << "updateConnected: " << content;
// //    if(this->state() == QMqttClient::Connected) {
// //        QByteArray messagePub;
// //        QString topicPub;
// //        topicPub = "v1/devices/me/telemetry";
// //        QString telemetry ("{\"temperature\":42}");
// //        messagePub = telemetry.toUtf8();
// //        this->publish(topicPub, messagePub);
// //    }
// }

// void FlightHubMqtt::updateDisConnected()
// {
//     //this->connectToHost();
// }

// void FlightHubMqtt:: publishMsg(QByteArray message)
// {
//     QString topicPub;
//     if(this->state() == QMqttClient::Connected) {
//         topicPub = "v1/devices/me/telemetry";

//         this->publish(topicPub, message);
//     }
// }
