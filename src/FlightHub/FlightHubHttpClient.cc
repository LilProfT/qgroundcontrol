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

FlightHubHttpClient::FlightHubHttpClient(QObject *parent) : QObject(parent), _getAccessTokenManager(new QNetworkAccessManager(this)), _getUserAccessTokenManager(new QNetworkAccessManager(this)),
    _publishTelemetryManager(new QNetworkAccessManager(this)), _publishStatManager(new QNetworkAccessManager(this)), _publishPlanManager(new QNetworkAccessManager(this)),
    _publishOfflinePlanManager(new QNetworkAccessManager(this)),
    _fetchPlansManager(new QNetworkAccessManager(this)),
    _downloadPlanFileManager(new QNetworkAccessManager(this))

{

    connect(_getAccessTokenManager, &QNetworkAccessManager::finished, this, &FlightHubHttpClient::_onGetDeviceAccessTokenFinished);

    connect(_publishTelemetryManager, &QNetworkAccessManager::finished, this, &FlightHubHttpClient::_onPublishTelemetryFinished);

    connect(_getUserAccessTokenManager, &QNetworkAccessManager::finished, this, &FlightHubHttpClient::_onGetUserAccessTokenFinished);
    connect(_publishStatManager, &QNetworkAccessManager::finished, this, &FlightHubHttpClient::_onPublishStatFinished);
    connect(_publishPlanManager, &QNetworkAccessManager::finished, this, &FlightHubHttpClient::_onPublishPlanFinished);
    connect(_fetchPlansManager, &QNetworkAccessManager::finished, this , &FlightHubHttpClient::_onFetchPlansFinished);
    connect(_downloadPlanFileManager, &QNetworkAccessManager::finished, this , &FlightHubHttpClient::_onDownloadPlanFileFinished);
}

void FlightHubHttpClient::_onFetchPlansFinished(QNetworkReply *reply){
    QByteArray body =reply->readAll();
    QList<PlanItem*> array{};
    if (!reply->error()){
        QJsonDocument doc = QJsonDocument::fromJson(body);
        auto resp = doc.object();
        auto data = resp["data"].toArray();

        foreach(auto item, data){
            auto itemObj = item.toObject();
            auto id = itemObj["id"].toInt();
            auto name = itemObj["fileName"].toString();

            PlanItem* planItem = new PlanItem(id, name);
            array.append(planItem);


        }

    }



    emit fetchedPlans(array);
}

QString FlightHubHttpClient:: _getFilename(const std::string header) {
    std::string ascii;
    const std::string q1 { R"(filename=")" };
    if (const auto pos = header.find(q1) ; pos != std::string::npos){
        const auto len =  pos + q1.size();
        const std::string q2 { R"(")" };
        if ( const auto pos = header.find(q2, len); pos != std::string::npos )
        {
            ascii = header.substr(len, pos - len);
        }
    }
    std::string utf8;

    const std::string u { R"(UTF-8'')" };
    if ( const auto pos = header.find(u); pos != std::string::npos )
    {
        utf8 = header.substr(pos + u.size());
    }

    return QString::fromStdString(ascii);
}

void FlightHubHttpClient::_onDownloadPlanFileFinished(QNetworkReply *reply){
    auto content =  reply->readAll();

    if (!reply->error()){

        auto folderPath =  qgcApp()->toolbox()->settingsManager()->appSettings()->missionSavePath();
        QDir dir(folderPath);
        auto  header =  reply->rawHeader("Content-Disposition");
        auto filename = _getFilename(QString::fromLatin1(header).toStdString());
        auto fileInfo =  dir.filePath(filename);

        QFile writeFile(fileInfo);
        if (writeFile.open(QIODevice::WriteOnly))
        {
            QTextStream stream(&writeFile);


            stream << content << Qt::endl;
        }
        emit downloadPlanFileFinished(fileInfo);
    }

    reply->deleteLater();
}

void FlightHubHttpClient::downloadPlanFile(const int id) {
    qCWarning(FlightHubHttpClientLog) << "fetched plans client id" <<id;
    QString domain = _hostAddress;
    auto url =QUrl( domain + "/authorizeddevices/retrieveplanfile");
    QNetworkRequest request;
    request.setUrl(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    auto token = "Bearer " + _getDeviceAccessToken();
    request.setRawHeader(QString("Authorization").toUtf8(), token.toUtf8());
    QJsonObject dataRequest;
    dataRequest["planID"] = id;

    QJsonDocument doc;
    doc.setObject(dataRequest);

    QByteArray data =  doc.toJson();

    _downloadPlanFileManager->post(request, data);
}


void FlightHubHttpClient::fetchPlans(const QString& search,double longitude, double latitude) {
    qCWarning(FlightHubHttpClientLog) << "fetch plans";
    QString domain = _hostAddress;
    auto url =QUrl( domain + "/authorizeddevices/retrieveplans");
    QUrlQuery query;
    query.addQueryItem("search", search);
    if (longitude>0){
        query.addQueryItem("longitude", QString::number(longitude));
    }
    if (latitude>0){
        query.addQueryItem("latitude", QString::number(latitude));
    }
    url.setQuery(query.query());
    QNetworkRequest request;
    request.setUrl(url);
    auto token = "Bearer " + _getDeviceAccessToken();
    request.setRawHeader(QString("Authorization").toUtf8(), token.toUtf8());

    _fetchPlansManager->get(request);

}
void FlightHubHttpClient::_onPublishStatFinished(QNetworkReply *reply)
{
    qCWarning(FlightHubHttpClientLog) << reply->readAll();
    if (reply->error())
    {
        if (reply->error() == QNetworkReply::NetworkError::HostNotFoundError)
        {
            auto folderPath = qgcApp()->toolbox()->settingsManager()->appSettings()->resumeSavePath();
            QDir dir(folderPath);
            dir.cdUp();
            auto filePath = dir.filePath("offline_stats.json");

            QFile readFile(filePath);
            QString text = "[]";
            if (readFile.exists())
            {
                if (readFile.open(QIODevice::ReadOnly | QIODevice::Text))
                {
                    text = readFile.readAll();
                }
            }
            QJsonDocument doc = QJsonDocument::fromJson(text.toUtf8());
            QJsonArray array = doc.array();
            _currentStat["deviceAccessToken"] = _deviceAccessToken;

            array.append(_currentStat);

            QFile writeFile(filePath);

            if (writeFile.open(QIODevice::WriteOnly))
            {
                QTextStream stream(&writeFile);
                QJsonDocument doc;
                doc.setArray(array);

                stream << doc.toJson() << Qt::endl;
            }
        }
    }
}

void FlightHubHttpClient::_onPublishPlanFinished(QNetworkReply *reply)
{
    qCWarning(FlightHubHttpClientLog) << reply->readAll();

    if (reply->error())
    {
        if (reply->error() == QNetworkReply::NetworkError::HostNotFoundError)
        {
            QString uploadFileName = _currentPlan["filename"].toString();
            auto folderPath = qgcApp()->toolbox()->settingsManager()->appSettings()->missionSavePath() + "/sync";
            QDir dir(folderPath);
            if (!dir.exists())
            {
                dir.mkpath(".");
            }

            auto filePath = dir.filePath(uploadFileName + "." + QString::number(QDateTime::currentMSecsSinceEpoch()) + ".json");
            qCWarning(FlightHubHttpClientLog) << "Writing file" << filePath << folderPath << dir.absolutePath();
            QFile writeFile(filePath);
            if (writeFile.open(QIODevice::WriteOnly))
            {
                QTextStream stream(&writeFile);
                QJsonDocument doc;
                doc.setObject(_currentPlan);
                stream << doc.toJson() << Qt::endl;
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

    _currentStat = obj;
    _publishStatManager->post(request, data);
}
void FlightHubHttpClient::publishPlan(const QJsonDocument &json, const QGeoCoordinate &coordinate, const double &area, const QString &filename)
{

    //     Upload plan file to server

    QFileInfo fileInfo(filename);
    QString uploadFileName = fileInfo.fileName();
    QJsonObject plan;
    plan["data"] = json.object();
    plan["longitude"] = coordinate.longitude();
    plan["latitude"] = coordinate.latitude();
    plan["area"] = area;
    plan["filename"] = uploadFileName;

    _currentPlan = plan;

    QNetworkRequest request;
    auto token = "Bearer " + _getDeviceAccessToken();
    request.setRawHeader(QString("Authorization").toUtf8(), token.toUtf8());
    QString domain = _hostAddress;
    QString url = domain + "/authorizeddevices/me/plans";
    request.setUrl(QUrl(url));
    QHttpMultiPart *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);

    QHttpPart longitudePart;
    longitudePart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"longitude\""));
    longitudePart.setBody(QByteArray::number(coordinate.longitude()));

    QHttpPart latitudePart;
    latitudePart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"latitude\""));
    latitudePart.setBody(QByteArray::number(coordinate.latitude()));

    QHttpPart areaPart;
    areaPart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"area\""));
    areaPart.setBody(QByteArray::number(area));

    QHttpPart filePart;
    filePart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"file\"; filename=\"" + uploadFileName + "\""));
    filePart.setBody(json.toJson());

    multiPart->append(filePart);
    multiPart->append(longitudePart);
    multiPart->append(latitudePart);
    multiPart->append(areaPart);

    _publishPlanManager->post(request, multiPart);
}
void FlightHubHttpClient::publishOfflinePlan(const QJsonDocument &json, const QGeoCoordinate &coordinate, const double &area, const QString &filename, const QString &localFilename)
{
    //     Upload plan file to server
    qCWarning(FlightHubHttpClientLog) << "client publish offline" << localFilename;
    QString uploadFileName = filename;
    QNetworkRequest request;
    auto token = "Bearer " + _getDeviceAccessToken();
    request.setRawHeader(QString("Authorization").toUtf8(), token.toUtf8());
    QString domain = _hostAddress;
    QString url = domain + "/authorizeddevices/me/plans";
    request.setUrl(QUrl(url));
    QHttpMultiPart *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);

    QHttpPart longitudePart;
    longitudePart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"longitude\""));
    longitudePart.setBody(QByteArray::number(coordinate.longitude()));

    QHttpPart latitudePart;
    latitudePart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"latitude\""));
    latitudePart.setBody(QByteArray::number(coordinate.latitude()));

    QHttpPart areaPart;
    areaPart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"area\""));
    areaPart.setBody(QByteArray::number(area));

    QHttpPart filePart;
    filePart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"file\"; filename=\"" + uploadFileName + "\""));
    filePart.setBody(json.toJson());

    multiPart->append(filePart);
    multiPart->append(longitudePart);
    multiPart->append(latitudePart);
    multiPart->append(areaPart);
    auto conn = std::make_shared<QMetaObject::Connection>();
    *conn = connect(_publishOfflinePlanManager, &QNetworkAccessManager::finished, this, [ conn, localFilename](QNetworkReply *reply)
    {
        QObject::disconnect(*conn);
        qCWarning(FlightHubHttpClientLog) << reply->readAll() << localFilename;

        if (!reply -> error()) {
            QFileInfo fileInfo(localFilename);
            if (fileInfo.exists()){
                QFile file(fileInfo.absoluteFilePath());
                file.remove();
            }
        }
    });
    _publishOfflinePlanManager->post(request, multiPart);
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
