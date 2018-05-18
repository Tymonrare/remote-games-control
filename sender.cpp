/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   sender.cpp
 * Author: x
 * 
 * Created on March 5, 2017, 12:38 PM
 */

#include <QtCore/qbytearray.h>
#include <QtCore/qiodevice.h>
#include <QtWidgets/QSpinBox>
#include <QtCore/qstring.h>

#include "sender.h"
#include "qdom.h"

#include <QGuiApplication>
#include <QtQml/QQmlApplicationEngine>
#include <QtQml/QQmlEngine>
#include <QtQml/QQmlContext>

Sender::Sender(QWidget *parent)
    : QWidget(parent)
{
    instance = this;
    widget.setupUi(this);
    widget.statusLabel->setText(tr("Ready to broadcast datagrams on port 45454"));
    
    udpSocket = new QUdpSocket(this);
    udpSocket->bind(45455, QUdpSocket::ShareAddress);
    
    connect(widget.updateServersButton, SIGNAL(clicked()), this, SLOT(startBroadcasting()));
    connect(udpSocket, SIGNAL(readyRead()),
            this, SLOT(processPendingDatagrams()));
    
    QList<QHostAddress> ipAddressesList = QNetworkInterface::allAddresses();
    // use the first non-localhost IPv4 address
    for (int i = 0; i < ipAddressesList.size(); ++i) {
        if (ipAddressesList.at(i) != QHostAddress::LocalHost &&
            ipAddressesList.at(i).toIPv4Address()) {
            ipAddress = ipAddressesList.at(i).toString();
            break;
        }
    }
    // if we did not find one, use IPv4 localhost
    
    if (ipAddress.isEmpty()) {
        QHostAddress addr = QHostAddress(QHostAddress::LocalHost);
        ipAddress = addr.toString();
    }

    auto cdir = QDir(QDir::currentPath() + "/" +"res/cache");
    if(!cdir.exists())
    cdir.mkdir(".");


    QFile dataFile("res/client_cache.xml");
    if(!dataFile.exists()){
        dataFile.open(QIODevice::Truncate | QIODevice::WriteOnly);
        QTextStream stream( &dataFile );
        stream << xmlOrigin;
        dataFile.close();
    }
    startBroadcasting();

    setWindowTitle(tr("client"));
}

void Sender::showFullscreenClient(){
    QQmlApplicationEngine *view = new QQmlApplicationEngine(QUrl("qrc:/res/TestClient_copy.qml"));
    view->rootContext()->setContextProperty("applicationDirPath",
                                            QGuiApplication::applicationDirPath());
    view->addImportPath("qrc:/import");
    QObject *item = *(view->rootObjects().begin());

    QVariant returnedValue;
    for(auto i : gamesList){
        QFileInfo file;
        if(i.previewID > 0)
            file = QFileInfo(QDir::currentPath() + tr("/res/cache/%1.jpg").arg(i.previewID));

        QString imgpath = "";
        bool hideName = false;
        if(i.previewID > 0 && file.exists()){
            imgpath = tr("res/cache/%1.jpg").arg(i.previewID);
            hideName = true;
        }
        else if(i.previewPath.length()){
            imgpath = tr("res/cache/%1").arg(i.previewPath.c_str());
        }
        QMetaObject::invokeMethod(item, "addNewGame",
                      Q_RETURN_ARG(QVariant, returnedValue),
                      Q_ARG(QVariant, i.name.c_str()),
                      Q_ARG(QVariant, i.time),
                      Q_ARG(QVariant, i.price_is_fixed),
                      Q_ARG(QVariant, i.price),
                      Q_ARG(QVariant, i.timeStep),
                      Q_ARG(QVariant, i.minTime),
                      Q_ARG(QVariant, i.maxTime),
                      Q_ARG(QVariant, imgpath),
                      Q_ARG(QVariant, hideName));
    }
    QObject::connect(item, SIGNAL(play(QVariant, QVariant, QVariant)), this, SLOT(beginGameWithTime(QVariant,QVariant,QVariant)));
    QObject::connect(item, SIGNAL(stop()), this, SLOT(stopAllGames()));

}

void Sender::startBroadcasting()
{
    if(useLocalConnection){
        QByteArray datagram = "req frm: 127.0.0.1";
        udpSocket->writeDatagram(datagram.data(), datagram.size(),
                     QHostAddress::LocalHost, 45454);
        return;
    }

    QByteArray datagram = "req frm: ";
    datagram.append(ipAddress);
    widget.statusLabel->setText(tr("broadcasting... %1").arg(ipAddress));
    udpSocket->writeDatagram(datagram.data(), datagram.size(),
                             QHostAddress::Broadcast, 45454);
}

void Sender::processPendingDatagrams()
{
//! [2]
    while (udpSocket->hasPendingDatagrams()) {
        QByteArray rdatagram;
        rdatagram.resize(udpSocket->pendingDatagramSize());
        udpSocket->readDatagram(rdatagram.data(), rdatagram.size());
        widget.statusLabel->setText(tr("Received datagram: \"%1\"")
                             .arg(rdatagram.data()));
        
        parseDatagram(rdatagram.toStdString());
    }
//! [2]
}


void Sender::parseDatagram(const std::string data){
    if(data.substr(0, 8) == std::string("acc frm:")){
            if(!serversSet.insert(data.substr(9, data.length()-1).c_str()).second) return;
            QLayoutItem *child;
            while ((child = widget.serversListLayout->takeAt(0)) != 0)  {
            //...
            widget.serversListLayout->removeItem(child);
            delete child->widget();
            }
            
            insertServerInfo();
        }
    if(data.substr(0, 8) == std::string("accfile:")){
            QString errorStr;
            int errorLine;
            int errorColumn;
            printf(data.c_str());
            QByteArray gameData;
            gameData.append(data.substr(9, data.length()-1).c_str());
            if (!domDocument.setContent(gameData, true, &errorStr, &errorLine,
                                &errorColumn)) {
                    printf(
                                 (tr("Parse error at line %1, column %2:\n%3")
                                 .arg(errorLine)
                                 .arg(errorColumn)
                                 .arg(errorStr)).toStdString().c_str());
                return;
            }
            parseXMLData();
    }
}

//temporally global
QVBoxLayout *tvbox;

void Sender::insertServerInfo(){
     for(auto i : serversSet){
                auto LayoutWidget = new QWidget(widget.serversListLayout->widget());
                QHBoxLayout *box = new QHBoxLayout(LayoutWidget);
                QLabel *addrLabel = new QLabel(i);
                QLabel *timeLabel = new QLabel("pending");
                QPushButton *breakButton = new QPushButton("break");
                //breakButton->setEnabled(false);
                box->addWidget(addrLabel);
                box->addWidget(timeLabel);
                box->addWidget(breakButton);
                widget.serversListLayout->addWidget(LayoutWidget);
                
                connect(breakButton, &QPushButton::clicked, [=]() {
                    QByteArray datagram = "stp gme: ";
                    widget.statusLabel->setText(tr("request stop game on %1").arg(i));
                    udpSocket->writeDatagram(datagram.data(), datagram.size(),
                               QHostAddress(i), 45454);
                });

                QWidget *tab = new QWidget();
                widget.tabWidget->addTab(tab, i);
                tvbox = new QVBoxLayout(tab);
                
                QByteArray datagram = "reqfile: ";
                datagram.append(ipAddress);
                widget.statusLabel->setText(tr("request file from... %1").arg(i));
                udpSocket->writeDatagram(datagram.data(), datagram.size(),
                            QHostAddress(i), 45454);
     }
}

bool Sender::parseXMLData()
{
    QFile dataFile("res/client_cache.xml");
    //open .xml data
    if (!dataFile.open(QFile::ReadWrite | QFile::Text)) {
        return false;
    }

    QString errorStr;
    int errorLine;
    int errorColumn;
    if (!cacheDocument.setContent((QIODevice*)&dataFile, true, &errorStr, &errorLine,
                                &errorColumn)) {
        return false;
    }
    dataFile.close();

    QDomElement root = domDocument.documentElement();
    if (root.tagName() != "xbel") {
//        QMessageBox::information(window(), tr("DOM Bookmarks"),
//                                 tr("The file is not an XBEL file."));
        return false;
    } else if (root.hasAttribute("version")
               && root.attribute("version") != "1.0") {
//        QMessageBox::information(window(), tr("DOM Bookmarks"),
//                                 tr("The file is not an XBEL version 1.0 "
//                                    "file."));
        return false;
    }
    
    QDomElement child = root.firstChildElement("game");
    while (!child.isNull()) {
        parseGameElement(child);
        child = child.nextSiblingElement("game");
    }
    child = root.firstChildElement("folder");
    while (!child.isNull()) {
        child = child.firstChildElement("game");
        while (!child.isNull()) {
            parseGameElement(child);
            child = child.nextSiblingElement("game");
        }

        child = child.nextSiblingElement("folder");
    }

    downloader.queueDownloading();

    if(loadFullscreen)
        showFullscreenClient();

    if (!dataFile.open(QIODevice::Truncate | QIODevice::WriteOnly)) {
        return false;
    }

    QTextStream stream( &dataFile );
    stream << cacheDocument.toString();
    dataFile.close();


    return true;
}
//#include "../ui_fu.h"
bool Sender::parseGameElement(const QDomElement &element)
{
    if(element.hasAttribute("hidden") && element.attribute("hidden").toInt()) return false;

    auto gname = element.attribute("name");
    auto child = cacheDocument.documentElement().firstChildElement("game");
    bool exist = false;
    while (!child.isNull()) {
        if(child.attribute("name") == gname) {exist = true; break;}
        child = child.nextSiblingElement("game");
    }


    auto LayoutWidget = new QWidget(tvbox->widget());
    QHBoxLayout *box = new QHBoxLayout(LayoutWidget);

    QSpinBox *spinBox = new QSpinBox();
    spinBox->setRange(element.firstChildElement("time").attribute("min").toInt(),
                      element.firstChildElement("time").attribute("max").toInt());
    spinBox->setValue(element.firstChildElement("time").attribute("default").toInt());

    if(element.firstChildElement("time").attribute("fixed") == "true")
        spinBox->setDisabled(true);

    QPushButton *playButton = new QPushButton("play");
    QLabel *nameLabel = new QLabel(element.attribute("name"));
    box->addWidget(nameLabel);
    box->addWidget(spinBox);
    box->addWidget(playButton);
    tvbox->addWidget(LayoutWidget);

    connect(playButton, &QPushButton::clicked, [=]() {
        beginGameWithTime(spinBox->value(), 0, element.attribute("name"));
    });
    GameData game;
    game.name = element.attribute("name").toStdString();
    game.time = element.firstChildElement("time").attribute("default").toInt();
    game.price = element.firstChildElement("time").attribute("price").toInt();
    game.minTime = element.firstChildElement("time").attribute("min").toInt();
    game.maxTime = element.firstChildElement("time").attribute("max").toInt();
    if(element.firstChildElement("time").hasAttribute("step"))
        game.timeStep = element.firstChildElement("time").attribute("step").toInt();
    if(element.firstChildElement("time").attribute("fixed") == "true")
        game.price_is_fixed = true;
    if(element.hasAttribute("steamID") || element.hasAttribute("preview")){
        int id;
        bool ok = true;

        if(element.hasAttribute("steamID")) {
            game.steamkill = true;
            game.steamID = element.attribute("steamID").toInt();
            id = element.attribute("steamID").toInt();
        }
        else{
            element.attribute("preview").toInt(&ok);
            if(ok) id = element.attribute("preview").toInt();
        }
        game.previewID = id;

        QFileInfo file(QDir::currentPath() + tr("/res/cache/%1.jpg").arg(id));
        if(!file.exists() && ok)
            downloader.appendToDownloads(downloadData(
                tr("http://cdn.akamai.steamstatic.com/steam/apps/%1/header.jpg").arg(id),
                QDir::currentPath() + "/res/cache",
                tr("%1.jpg").arg(id)));
        else if(!ok){
            file = QFileInfo(element.attribute("preview"));
            QFileInfo chfile = QFileInfo(QDir::currentPath() + tr("/res/cache/%1.%2").arg(game.name.c_str(), file.completeSuffix()));
            if(file.exists()){
                if(chfile.exists()) QFile::remove(chfile.absoluteFilePath());
                QFile::copy(file.absoluteFilePath(), chfile.absoluteFilePath());
            }
            game.previewPath = chfile.fileName().toStdString();
        }
    }


    if(!exist){
                cacheDocument.documentElement().appendChild(element.cloneNode());
    }

    gamesList.push_back(game);

    return true;
}
void Sender::beginGameDefaultGame(QVariant time){
    beginGameWithTime(time.toInt(), 0, "oculus plane");
}

void Sender::stopAllGames(){
    for(auto i : serversSet){
        QByteArray datagram = "stp gme: ";
        widget.statusLabel->setText(tr("request stop game on %1").arg(i));
        udpSocket->writeDatagram(datagram.data(), datagram.size(),
                                 QHostAddress(i), 45454);
    }
}

void Sender::beginGameWithTime(QVariant time, QVariant server, QVariant game){
    beginGameWithTime(time.toInt(), server.toInt(), game.toString());
}

void Sender::beginGameWithTime(int time, const int server, const QString game){
    QByteArray datagram = "srt gme: ";
    datagram.append("/:name:/" + game + "/:time:/" + tr("%1").arg(time));
    widget.statusLabel->setText(tr("request starting game on %1").arg(*serversSet.begin()));
    udpSocket->writeDatagram(datagram.data(), datagram.size(),
                             QHostAddress(*serversSet.begin()), 45454);
}
