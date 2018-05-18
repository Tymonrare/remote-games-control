/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   receiver.h
 * Author: x
 *
 * Created on March 5, 2017, 12:09 PM
 */

#ifndef RECEIVER_H
#define RECEIVER_H
#include <QtNetwork>
#include <QDomDocument>
#include <QTreeWidget>
#include "../ui_serverWindow.h"
#include <map>
#include "global.h"
QT_USE_NAMESPACE

class Receiver : public QWidget
{
    Q_OBJECT

public:
    Receiver(QWidget *parent = 0);
    bool steamKill();

    //by name or by pid
    bool recursiveKill(const QString killprocess);
    bool recursiveKill(int uid);
    inline Receiver *getInstence(){return instance;}
private slots:
    void processPendingDatagrams();
    void parseDatagram(const std::string data);
    bool parseXMLData();
    bool parseGameElement(const QDomElement &element);
    void updateDomElement(QTreeWidgetItem *item, int column);
private:
    Receiver *instance;
    void beginGame(const std::string data);

    void parseFolder(QDomElement *folder);
    
    QUdpSocket *udpSocket;
    Ui::serverWindow widget;
    QString ipAddress;
    
    QProcess *gameProcess;

    QDomDocument domDocument;
    QHash<QTreeWidgetItem *, QDomElement> domElementForItem;
    GameData activeGame;
    bool gameIsActive;
    std::map<std::string, GameData> games;
    QString steamKillIgnore;
    QTimer killTimer;
};

#endif /* RECEIVER_H */
