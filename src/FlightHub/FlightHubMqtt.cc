// /****************************************************************************
//  *
//  * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
//  *
//  * QGroundControl is licensed according to the terms in the file
//  * COPYING.md in the root of the source code directory.
//  *
//  ****************************************************************************/


// #include "FlightHubMqtt.h"
// #include "QGCApplication.h"
// #include <QDebug>

// FlightHubMqtt::FlightHubMqtt(QObject *parent):QMqttClient(parent)
// {
// }

// FlightHubMqtt::~FlightHubMqtt()
// {
// }

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
