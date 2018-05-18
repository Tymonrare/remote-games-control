/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   sender.h
 * Author: x
 *
 * Created on March 5, 2017, 12:38 PM
 */

#ifndef SENDER_H
#define SENDER_H
#include <QtNetwork>
#include <QDomDocument>
#include <QtGui/qvector2d.h>
#include <set>
#include "global.h"
#include "httpdownloader.h"

#include "../ui_clientWindow.h"

class Sender : public QWidget
{
    Q_OBJECT
public slots:
    void beginGameDefaultGame(QVariant time);
    void beginGameWithTime(QVariant time, QVariant server, QVariant game);
    void stopAllGames();
public:
    Sender(QWidget *parent = 0);
    void beginGameWithTime(int time, const int server, const QString game);
    void showFullscreenClient();
    inline Sender *getInstence(){return instance;}

private slots:
    void startBroadcasting();
    void processPendingDatagrams();
    void parseDatagram(const std::string data);
    bool parseXMLData();
    bool parseGameElement(const QDomElement &element);

    void insertServerInfo();

private:
    Sender *instance;

    QUdpSocket *udpSocket;
    Ui::clientWindow widget;
    QString ipAddress;
    std::set<QString> serversSet;
    std::vector<GameData> gamesList;

    HttpDownloader downloader;

    QDomDocument domDocument, cacheDocument;
};

#endif /* SENDER_H */
