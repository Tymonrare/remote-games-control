#include "mainwindow.h"

#include <QApplication>
#include "receiver.h"
#include "sender.h"
#include "logindialog.h"

#include <QGuiApplication>
#include <QtQml/QQmlApplicationEngine>
#include <QtQml/QQmlEngine>

#include <QFileDialog>
#include <QInputDialog>
#include <QCryptographicHash>
#include <QShortcut>
#include <QMessageBox>

Receiver *receiver = nullptr;
Sender *sender = nullptr;
bool loadFullscreen = false;
bool useLocalConnection = false;

QApplication *application = nullptr;

const QString xmlOrigin = "<?xml version=\'1.0\' encoding=\'UTF-8\'?><!DOCTYPE xbel><xbel version=\"1.0\"></xbel>";

ShortcutsHandler::ShortcutsHandler(QWidget *parent)
    : QWidget(parent){

}

void saveDocument() {
 QFileDialog *dlg = new QFileDialog();
 dlg->open();
 QObject::connect(dlg, &QDialog::finished, [dlg](int result) {
 if (result) {
 QFile file(dlg->selectedFiles().first());
 // …
 }
 dlg->deleteLater();
 });

}

bool confirmPasswordDialog(QString ref){
    bool ok;
    QString text = QInputDialog::getText(nullptr, "QInputDialog::getText()",
                                         "Confirm password:", QLineEdit::Normal,
                                         QDir::home().dirName(), &ok);
    if (ok && !text.isEmpty() && text == ref){
        return true;
    }
    if(!ok) return false;
    confirmPasswordDialog(ref);
}

void loginAsAdmin(){
        LoginDialog* loginDialog = new LoginDialog();
        loginDialog->setUsername( "admin" ); // optional


        QObject::connect(loginDialog, &LoginDialog::acceptLogin, [=]( const QString& username, const QString& password) {
            bool admin = false;
            QSettings settings("HKEY_CURRENT_USER\\SOFTWARE\\RGC\\users", QSettings::NativeFormat);
            if(settings.contains(username)){
                QByteArray ba;
                ba.append(password);
                admin = settings.value(username).toString().toStdString() == ba.toBase64().toStdString();
            }
            else if(confirmPasswordDialog(password)){
                admin = username == "admin";
                QByteArray ba;
                ba.append(password);
                settings.setValue(username, ba.toBase64().toStdString().c_str());
            }

            if(!admin){
                loginDialog->hide();
                loginDialog->deleteLater();
                loginAsAdmin();
                return;
            }
            if(receiver)
                receiver->show();
            if(sender)
                sender->show();

        });
        loginDialog->exec();
}

void ShortcutsHandler::runAdmin(){
    qInfo() << "lof";
    loginAsAdmin();
}
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setQuitOnLastWindowClosed(false);
    application = &a;

    QSettings settings("HKEY_CURRENT_USER\\SOFTWARE\\RGC", QSettings::NativeFormat);
    if(!settings.contains("key") || settings.value("key") != "mjTCrueOR2otqEFaw$LGtVetNuN5oU~2z~@p~5kv1yqjGp3jKa0mb~iNlZn3B?nFPWbqufdxhcCAOlIamsREfvs5iSvrrIHMMBc") {
        QMessageBox messageBox;
        messageBox.critical(0,"Error","Ошибка проверки лицензии");
        messageBox.setFixedSize(500,200);
        return 1;
    }

    auto cdir = QDir(QDir::currentPath() + "/" +"res");
    if(!cdir.exists())
    cdir.mkdir(".");

    QCommandLineParser parser;
    parser.setSingleDashWordOptionMode(QCommandLineParser::ParseAsLongOptions);
    const QCommandLineOption clientOption("client");
    const QCommandLineOption serverOption("server");
    const QCommandLineOption fullscreenOption("fullscreen");
    const QCommandLineOption adminOption("admin");
    const QCommandLineOption showLogOption("gamesLog");

    parser.addOption(clientOption);
    parser.addOption(serverOption);
    parser.addOption(fullscreenOption);
    parser.addOption(adminOption);
    parser.addOption(showLogOption);

    parser.process(a);

    if(parser.isSet(fullscreenOption))
        loadFullscreen = true;

    useLocalConnection = parser.isSet(serverOption) && parser.isSet(clientOption);

    if(parser.isSet(serverOption)){
        receiver = new Receiver();
    }

    if(parser.isSet(clientOption)){
        sender = new Sender();
    }

    if(parser.isSet(adminOption)){
        loginAsAdmin();
    }

    //if(!parser.isSet(fullscreenOption)){
        //auto h = new ShortcutsHandler();
        //new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_Q), h, SLOT(runAdmin()), nullptr, Qt::ApplicationShortcut);
    //}

    //if(parser.isSet(showLogOption)){
        //auto window = new QWidget();
        //auto tree = new QTreeWidget(window);
        //window->showMaximized();
    //}

    return a.exec();
}

