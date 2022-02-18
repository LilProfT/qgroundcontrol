/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


// #pragma once

// #include <QString>
// #include <QByteArray>
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
