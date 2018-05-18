/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   receiver.cpp
 * Author: x
 * 
 * Created on March 5, 2017, 12:09 PM
 */

#include <QtCore/qiodevice.h>
#include <QtCore/qstringlist.h>
#include <QtCore/QDateTime>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QSettings>
#include <QPushButton>
#include <QDesktopServices>
#include <QtConcurrent>
#include <QFileDialog>

#include "receiver.h"
#include "3rd/xlsx/xlsxdocument.h"

void logNewGame(int time, float price, const QString &name, bool legal = true);
QXlsx::Document xlsx;

Receiver::Receiver(QWidget *parent)
    : QWidget(parent)
{
    instance = this;

    widget.setupUi(this);
    gameProcess = nullptr;

//! [0]
    udpSocket = new QUdpSocket(this);
    udpSocket->bind(45454, QUdpSocket::ShareAddress);
//! [0]

//! [1]
    connect(udpSocket, SIGNAL(readyRead()),
            this, SLOT(processPendingDatagrams()));

    connect(widget.exportButton, &QToolButton::clicked, [=]() {
        auto file = QFileDialog::getSaveFileName(nullptr, "Save file", "./log.xlsx", ".xlsx");
        if(!file.length()) return;
        if(file.indexOf(".xlsx") < 0) file.append(".xlsx");
        xlsx.saveAs(file);
    });

//! [1]
    widget.statusLabel->setText(tr("wait client on port 45454"));
    
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
    
    setWindowTitle(tr("server"));
    
    {
    QFile dataFile("res/games.xml");
    if(!dataFile.exists()){
        dataFile.open(QIODevice::Truncate | QIODevice::WriteOnly);
        QTextStream stream( &dataFile );
        stream << xmlOrigin;
        dataFile.close();
    }
    }

    parseXMLData();
    gameIsActive = false;
    QTimer *timer = new QTimer();
    timer->start(60000);
    connect(timer, &QTimer::timeout, [&]() {
        if(gameIsActive || gameProcess) return;
        QtConcurrent::run(QThreadPool::globalInstance(), [&](){
            if(gameIsActive || gameProcess) return;
            if(steamKill()) logNewGame(0,0,"illegal",false);

            QString process;
            for (auto it = games.begin(); it != games.end(); ++it) {
                QRegExp rx("\\w+.exe");
                rx.indexIn(it->second.path.c_str());

                if(!rx.cap().length()) continue;

                QProcess cmd;
                cmd.start("cmd /c tasklist /FI \"IMAGENAME eq " +  rx.cap() + "\"");
                cmd.waitForFinished(-1); // will wait forever until finished
                QByteArray out = cmd.readAllStandardOutput();
                QRegExp rx2(rx.cap() + ".*\\d{1,}");
                rx2.indexIn(out);
                if(rx2.cap().length()){
                    logNewGame(0,0,"illegal",false);

                    if(process.length())
                        process += " & ";
                    process += "taskkill /F /IM ";
                    process += rx.cap();
                }
            }
            if(process.length()){
                system(process.toStdString().c_str());
                qInfo() << "illegal start: " << process << ". Killing. Logged";
            }
        });
    });

    logNewGame(0, 0, "startup", true);
}

bool Receiver::recursiveKill(int uid){
    bool killed = false;
    {
        QProcess process;
        QString cmd(tr("cmd /c wmic process where (ParentProcessId=%1) get Caption,ProcessId").arg(uid));
        process.start(cmd);
        process.waitForFinished(-1); // will wait forever until finished
        QByteArray out = process.readAllStandardOutput();

        QRegularExpression reA("[\\w.]+\\.exe\\s+\\d{1,}");

        QRegularExpressionMatchIterator i = reA.globalMatch(out);
        qInfo() << "ignore list:" << steamKillIgnore;
        while (i.hasNext()) {
            QRegularExpressionMatch match = i.next();
            if (match.hasMatch()) {
                auto m = match.captured(0);

                QRegExp rx(steamKillIgnore);
                rx.setCaseSensitivity(Qt::CaseInsensitive);
                if(rx.indexIn(m) < 0){
                    int childuid = QRegularExpression("\\s+\\d{1,}").match(m).captured().toInt();
                    recursiveKill(childuid);

                    system(tr("taskkill /pid %1").arg(childuid).toStdString().c_str());
                    qInfo() << "killing steam game " << m;
                    killed = true;
                }
            }
        }
    }
    return killed;
}

bool Receiver::recursiveKill(const QString killprocess){
    int uid;
    bool parseOk;
    int tmpuid = killprocess.toInt(&parseOk);
    if(!parseOk) {
        QProcess process;
        process.start("cmd /c tasklist /FI \"IMAGENAME eq " + killprocess + "\"");
        process.waitForFinished(-1); // will wait forever until finished
        QByteArray out = process.readAllStandardOutput();
        QRegExp rx(killprocess + ".*(\\d{1,})");
        rx.indexIn(out);
        uid = QRegularExpression("\\d{1,}").match(rx.cap()).captured().toInt();
    }
    else uid = tmpuid;

    return recursiveKill(uid);
}

bool Receiver::steamKill(){
    return recursiveKill("Steam.exe");
}

void Receiver::processPendingDatagrams()
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
bool parseXMLLogData(QTreeWidget *widget)
{

    int totalprice = 0;
    QTime totaltime(0,0,0);
    QFile dataFile("res/log.xml");
    //open .xml data
    if (!dataFile.open(QFile::ReadOnly | QFile::Text)) {
        return false;
    }

    QString errorStr;
    int errorLine;
    int errorColumn;
     QDomDocument domDocument;
    if (!domDocument.setContent((QIODevice*)&dataFile, true, &errorStr, &errorLine,
                                &errorColumn)) {
        return false;
    }

    QDomElement root = domDocument.documentElement();
    if (root.tagName() != "xbel") {
        return false;
    } else if (root.hasAttribute("version")
               && root.attribute("version") != "1.0") {
        return false;
    }

    int xcelCount = 2;
    xlsx.setColumnWidth(1,1,20);
    xlsx.setColumnWidth(4,4,30);
    xlsx.write(1, 1, "Дата");
    xlsx.write(1, 2, "Время");
    xlsx.write(1, 3, "Цена");
    xlsx.write(1, 4, "Название");

    QDomElement child = root.firstChildElement("game");
    while (!child.isNull()) {
        QTreeWidgetItem *item = new QTreeWidgetItem(widget);
        item->setText(0, child.attribute("date"));
        item->setText(1, child.attribute("playtime"));
        item->setText(2, child.attribute("price"));
        item->setText(3, child.attribute("name"));


        if(!child.attribute("legal").toInt()){
            item->setBackgroundColor(2, QColor(255,0,0));
            item->setBackgroundColor(3, QColor(255,0,0));
        }

        if(child.attribute("name") != "startup"){
            xlsx.write(xcelCount, 1, child.attribute("date"));
            xlsx.write(xcelCount, 2, QTime::fromString(child.attribute("playtime"), "mm:ss"));
            xlsx.write(xcelCount, 3, child.attribute("price").toInt());
            xlsx.write(xcelCount, 4, child.attribute("name"));
            xcelCount++;
        }

        totalprice += child.attribute("price").toInt();
        totaltime = totaltime.addSecs(QTime(0,0,0).secsTo(QTime::fromString(child.attribute("playtime"), "mm:ss")));
        child = child.nextSiblingElement("game");
    }

    QTreeWidgetItem *item = new QTreeWidgetItem(widget);
    item->setText(0, "total");
    item->setText(1, totaltime.toString("mm:ss"));
    char t[100];
    sprintf(t, "%d", totalprice);
    item->setText(2, t);


    return true;
}

void logNewGame(int time, float price, const QString &name, bool legal){
    QFile dataFile("res/log.xml");
    if(!dataFile.exists()){
        dataFile.open(QIODevice::Truncate | QIODevice::WriteOnly);
        QTextStream stream( &dataFile );
        stream << xmlOrigin;
        dataFile.close();
    }

    //open .xml data
    if (!dataFile.open(QFile::ReadWrite | QFile::Text)) {
        return;
    }

    QString errorStr;
    int errorLine;
    int errorColumn;
    QDomDocument domDocument;
    if (!domDocument.setContent((QIODevice*)&dataFile, true, &errorStr, &errorLine,
                                &errorColumn)) {
        return;
    }
    dataFile.close();

    QDomElement root = domDocument.documentElement();
    if (root.tagName() != "xbel") {

        return;
    } else if (root.hasAttribute("version")
               && root.attribute("version") != "1.0") {

        return;
    }

    QDomElement game = domDocument.createElement("game");
    game.setAttribute("date", QDateTime::currentDateTime().toString("dd.MM.yy hh:mm:ss"));
    game.setAttribute("playtime", QDateTime::fromTime_t(time).toUTC().toString("mm:ss"));
    game.setAttribute("price", price);
    game.setAttribute("legal", legal);
    game.setAttribute("name", name);
    root.appendChild(game);
    if (!dataFile.open(QIODevice::Truncate | QIODevice::WriteOnly)) {
        return;
    }

    QTextStream stream( &dataFile );
    stream << domDocument.toString();
    dataFile.close();
}

void Receiver::beginGame(const std::string data){
    if(gameProcess)
        parseDatagram("stp gme:");

    auto name = data.substr(data.find("/:name:/")+8,
            data.find_first_of("/:", data.find("/:name:/")+8)-8);
    
    QString qargtime = data.substr(data.find("/:time:/")+8,
            data.find_first_of("/:", data.find("/:time:/")+8)-8).c_str();

    auto game = games.at(name);
    
    game.time = qargtime.toInt();
    
    QStringList arglist;
    for(auto i : game.arguments) 
    {
        if(!game.timeArg.empty() && game.timeArg == i.first)
            i.second = game.time;
        
        arglist.append(QString(i.first.c_str()) + tr(" %1 ").arg(i.second));
        printf((QString(i.first.c_str()) + tr(" %1 ").arg(i.second)).toStdString().c_str());
    }

    float p;
    if(game.price_is_fixed)
        p = game.price;
    else
        p = game.price*((float)game.time)/60;

    logNewGame(game.time, p, game.name.c_str());
    activeGame = game;

    killTimer.setSingleShot(true);
    killTimer.start((game.time+game.startDelay)*1000);
    connect(&killTimer, &QTimer::timeout, [=]() {
        killTimer.stop();
        qInfo() << "time over, killing";
        parseDatagram("stp gme:");
    });

    gameIsActive = true;

    if(game.steamkill){
        qInfo() << "starting steam game " << game.steamID;
        QSettings settings("HKEY_CURRENT_USER\\SOFTWARE\\Valve\\Steam", QSettings::NativeFormat);
        qInfo() << "request steam in " << settings.value("SteamExe").toString();
        system((
                "\"" + settings.value("SteamExe").toString() + "\""
                + tr(" -applaunch %1").arg(game.steamID))
               .toStdString().c_str()
               );
        return;
    }

    activeGame.taskkill = "taskkill /F /IM " + activeGame.taskkill;
    gameProcess = new QProcess(this);
    gameProcess->start(QString(game.path.c_str()), arglist);
}

void Receiver::parseDatagram(const std::string data){
    if(data.substr(0, 8) == std::string("req frm:")){
            QByteArray tdatagram = "acc frm: ";
            if(useLocalConnection)
                tdatagram.append("127.0.0.1");
            else
                tdatagram.append(ipAddress);
            widget.statusLabel->setText(tr("return own address... %1").arg(ipAddress));
            udpSocket->writeDatagram(tdatagram.data(), tdatagram.size(),
                             QHostAddress(data.substr(9, data.length()-1).c_str()), 45455);
    }
    
    if(data.substr(0, 8) == std::string("reqfile:")){
        QFile dataFile("res/games.xml");
        //open .xml data
        if (!dataFile.open(QFile::ReadOnly | QFile::Text)) {
            return;
        }
        
        QByteArray tdatagram = "accfile: ";
        
        tdatagram.append(dataFile.readAll());
        
        widget.statusLabel->setText(tr("return game data"));
        udpSocket->writeDatagram(tdatagram.data(), tdatagram.size(),
                        QHostAddress(data.substr(9, data.length()-1).c_str()), 45455);
    }
    
    if(data.substr(0, 8) == std::string("srt gme:")){
        beginGame(data.substr(9, data.length()-1));
    }
    if(data.substr(0, 8) == std::string("stp gme:") && (gameProcess || activeGame.steamkill)) {
        killTimer.stop();
        gameIsActive = false;
        widget.statusLabel->setText(tr("killing game process: %1").arg(activeGame.name.c_str()));
        if(activeGame.steamkill){
            steamKill();
        }

        if(activeGame.systemkill)
            system(activeGame.taskkill.c_str());

        if(gameProcess){
            recursiveKill(gameProcess->processId());
            gameProcess->terminate();
            delete gameProcess;
            gameProcess = nullptr;
        }

    }
}

void Receiver::updateDomElement(QTreeWidgetItem *item, int column)
{
    QDomElement element = domElementForItem.value(item);

    bool toIntIsOk = true;
    item->data(column,0).toInt(&toIntIsOk);

    if(item->flags().testFlag(Qt::ItemIsTristate))
        element.setAttribute(item->data(0,0).toString(), item->checkState(column) == Qt::Checked ? "true" : "false");
    else if(toIntIsOk)
        element.setAttribute(item->data(0,0).toString(), item->data(column,0).toInt());
    else
        element.setAttribute(item->data(0,0).toString(), item->data(column,0).toString());
    QFile dataFile("res/games.xml");

    if (!dataFile.open(QIODevice::Truncate | QIODevice::WriteOnly)) {
        return;
    }

    QTextStream stream( &dataFile );
    stream << domDocument.toString();
    dataFile.close();
}

bool Receiver::parseXMLData()
{
    parseXMLLogData(widget.treeWidget_2);
    QFile dataFile("res/games.xml");
    //open .xml data
    if (!dataFile.open(QFile::ReadOnly | QFile::Text)) {
        return false;
    }

    QString errorStr;
    int errorLine;
    int errorColumn;
    if (!domDocument.setContent((QIODevice*)&dataFile, true, &errorStr, &errorLine,
                                &errorColumn)) {
                    printf(
                                 (tr("Parse error at line %1, column %2:\n%3")
                                 .arg(errorLine)
                                 .arg(errorColumn)
                                 .arg(errorStr)).toStdString().c_str());
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

    //init new
    if(child.isNull()){
        auto el = domDocument.createElement("folder");
        el.setAttribute("path", "games");
        root.appendChild(el);
    }
    child = root.firstChildElement("killignore");
    if(child.isNull()){
        steamKillIgnore = "steam|htc|nw\.exe|vrmonitor|vrserver|vrdashboard|vrcompositor|vive";
        auto el = domDocument.createElement("killignore");
        el.setAttribute("notkill", steamKillIgnore);
        root.appendChild(el);
    }
    else{
        auto child = root.firstChildElement("killignore");
        steamKillIgnore = child.attribute("notkill");
    }

    child = root.firstChildElement("folder");
    while (!child.isNull()) {
        parseFolder(&child);

        child = child.firstChildElement("game");
        while (!child.isNull()) {
            parseGameElement(child);
            child = child.nextSiblingElement("game");
        }

        child = child.nextSiblingElement("folder");
    }

    connect(widget.treeWidget, SIGNAL(itemChanged(QTreeWidgetItem*,int)),
            this, SLOT(updateDomElement(QTreeWidgetItem*,int)));

    if (!dataFile.open(QIODevice::Truncate | QIODevice::WriteOnly)) {
        return false;
    }

    QTextStream stream( &dataFile );
    stream << domDocument.toString();
    dataFile.close();

    return true;
}

void Receiver::parseFolder(QDomElement *folder){
        QDir gamesdir(folder->attribute("path"));
        if(!gamesdir.exists()) return;

        auto list = gamesdir.entryInfoList();
        for(int i = 2;i < list.length();i++){
            auto gname = list.at(i).baseName().split(".")[0];

            auto cont = false;
            auto child = folder->firstChildElement("game");
            while (!child.isNull()) {
                if(child.attribute("name") == gname){
                    cont = true;
                    break;
                }
                child = child.nextSiblingElement("game");
            }
            if(cont) continue;

            QString apath = list.at(i).absoluteFilePath();
            QDomElement n = domDocument.createElement("game");
            n.setAttribute("name", gname);
            n.setAttribute("path", apath);

            QDomElement t = domDocument.createElement("time");
            t.setAttribute("min", 60);
            t.setAttribute("default", 360);
            t.setAttribute("max", 720);
            t.setAttribute("delay", 0);
            t.setAttribute("price", 60);
            t.setAttribute("fixed", false);
            t.setAttribute("step", 150);
            n.appendChild(t);

            int lth = apath.length()-1;
            if(apath.toStdString().substr(lth-3, lth) == ".url"){
                qInfo() << "steam .url finded";

                QFile link(apath);
                link.open(QIODevice::ReadOnly | QIODevice::Text);
                QString rawlink(link.readAll());
                QRegExp rx("URL=steam://rungameid/(\\d{1,})");
                rx.indexIn(rawlink);
                n.setAttribute("steamID", rx.cap(1));
            }

            folder->appendChild(n);
        }
        auto child = folder->firstChildElement("game");
        while (!child.isNull()) {
            bool found = false;
            for(int i = 2;i < list.length();i++){
                auto gname = list.at(i).baseName().split(".")[0];
                if(child.attribute("name") == gname){
                    found = true;
                    break;
                }
            }
            child.setAttribute("hidden", !found);
            child = child.nextSiblingElement("game");
        }
    }

bool Receiver::parseGameElement(const QDomElement &element)
{
    if(element.hasAttribute("hidden") && element.attribute("hidden").toInt()) return false;
    GameData game;
    game.name = element.attribute("name").toStdString();
    game.path = element.attribute("path").toStdString();

    int lth = game.path.length()-1;
    if(game.path.substr(lth-3, lth) == ".lnk"){
        QFileInfo info(QString(game.path.c_str()));
        game.path = info.symLinkTarget().toStdString();
    }

    if(game.systemkill = element.hasAttribute("taskkill"))
        game.taskkill = element.attribute("taskkill").toStdString();

    QTreeWidgetItem *item = new QTreeWidgetItem(widget.treeWidget);
    item->setText(0, element.attribute("name"));

    if(element.hasAttribute("steamID")){
        game.steamID = element.attribute("steamID").toInt();
        game.steamkill = true;
    }
    else{
        QTreeWidgetItem *argitem = new QTreeWidgetItem(item);
        argitem->setFlags(item->flags() | Qt::ItemIsEditable);
        argitem->setText(0, "preview");
        argitem->setText(1, element.attribute("preview"));
        argitem->setText(2, "Картинка-превью для игры, путь к картинке на диске или steamID");
        domElementForItem.insert(argitem,element);
        /*
        QPushButton *button = new QPushButton("...");
        widget.treeWidget->setItemWidget(argitem, 1, button);

        connect(button, &QPushButton::clicked, [=]() {
        });
        */
    }

    {//time values
    QDomElement child = element.firstChildElement("time");
    QTreeWidgetItem *argitem = new QTreeWidgetItem(item);
    argitem->setText(0, "time");
    argitem->setText(2, "Параметры игрового времени");
    
    QTreeWidgetItem *mini = new QTreeWidgetItem(argitem);
    mini->setFlags(item->flags() | Qt::ItemIsEditable);
    domElementForItem.insert(mini,child);
    mini->setText(0, "min");
    mini->setText(1, child.attribute("min"));
    mini->setText(2, "Минимальное время, задаваемое оператором");

    QTreeWidgetItem *def = new QTreeWidgetItem(argitem);
    def->setFlags(item->flags() | Qt::ItemIsEditable);
    domElementForItem.insert(def,child);
    def->setText(0, "default");
    def->setText(1, child.attribute("default"));
    def->setText(2, "Значение по умолчанию");
    
    QTreeWidgetItem *maxi = new QTreeWidgetItem(argitem);
    maxi->setFlags(item->flags() | Qt::ItemIsEditable);
    domElementForItem.insert(maxi,child);
    maxi->setText(0, "max");
    maxi->setText(1, child.attribute("max")); 
    maxi->setText(2, "Максимальное время, задаваемое оператором");

    QTreeWidgetItem *delay = new QTreeWidgetItem(argitem);
    delay->setFlags(item->flags() | Qt::ItemIsEditable);
    domElementForItem.insert(delay,child);
    delay->setText(0, "delay");
    delay->setText(1, child.attribute("delay"));
    game.startDelay = child.attribute("delay").toInt();
    delay->setText(2, "Добавочное время, не учитываемое в стоимости");

     QTreeWidgetItem *step = new QTreeWidgetItem(argitem);
     step->setFlags(item->flags() | Qt::ItemIsEditable);
     domElementForItem.insert(step,child);
     step->setText(0, "step");
     step->setText(1, child.attribute("step"));
     step->setText(2, "Шаг установки времени оператором");
     game.timeStep = child.attribute("step").toInt();

    if(child.hasAttribute("argument")){
        QTreeWidgetItem *arg = new QTreeWidgetItem(argitem);
        arg->setText(0, "argument");
        arg->setText(1, child.attribute("argument"));
        arg->setText(2, "Аргумент, передаваемый в игру");
        arg->setFlags(item->flags() | Qt::ItemIsEditable);
        domElementForItem.insert(arg,child);
        game.arguments.insert(std::pair<std::string, int>
                (child.attribute("argument").toStdString(), 
                child.attribute("default").toInt()));
        game.timeArg = child.attribute("argument").toStdString();
    }
    game.time = child.attribute("default").toInt();

    QTreeWidgetItem *min = new QTreeWidgetItem(argitem);
    min->setFlags(item->flags() | Qt::ItemIsEditable );
    domElementForItem.insert(min,child);
    min->setText(0, "price");
    min->setText(1, child.attribute("price"));
    min->setText(2, "Стоимость минуты");

    QTreeWidgetItem *fixed = new QTreeWidgetItem(argitem);
    fixed->setFlags(item->flags() | Qt::ItemIsUserCheckable| Qt::ItemIsTristate);
    domElementForItem.insert(fixed,child);
    fixed->setText(0, "fixed");
    fixed->setCheckState(1, child.attribute("fixed") == "true" ? Qt::Checked : Qt::Unchecked);
    fixed->setText(2, "Запрещает оператору менять время");

    if(child.attribute("fixed") == "true"){
        game.price_is_fixed = true;
        game.price = child.attribute("price").toInt();
    }
    else
        game.price = child.attribute("price").toInt();
    }

    QDomElement child = element.firstChildElement("argument");
    while (!child.isNull()) {
        QTreeWidgetItem *argitem = new QTreeWidgetItem(item);
        argitem->setText(0, child.attribute("key"));
        
        QDomElement valchild = child.firstChildElement("value");
        QTreeWidgetItem *mini = new QTreeWidgetItem(argitem);
        mini->setText(0, "min");
        mini->setFlags(item->flags() | Qt::ItemIsEditable);
        domElementForItem.insert(mini,valchild);
        mini->setText(1, valchild.attribute("min"));
    
        QTreeWidgetItem *maxi = new QTreeWidgetItem(argitem);
        maxi->setText(0, "max");
        maxi->setFlags(item->flags() | Qt::ItemIsEditable);
        domElementForItem.insert(maxi,valchild);
        maxi->setText(1, valchild.attribute("max"));
        
        QTreeWidgetItem *def = new QTreeWidgetItem(argitem);
        def->setText(0, "default");
        def->setFlags(item->flags() | Qt::ItemIsEditable);
        domElementForItem.insert(def,valchild);
        def->setText(1, valchild.attribute("default"));
        
        game.arguments.insert(std::pair<std::string, int>
                (child.attribute("key").toStdString(), 
                valchild.attribute("default").toInt()));
        
        child = child.nextSiblingElement("argument");
    }
    games.insert(std::pair<std::string, GameData>(game.name, game));
    return true;
}
