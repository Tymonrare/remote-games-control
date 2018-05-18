#ifndef HTTPDOWNLOADER_H
#define HTTPDOWNLOADER_H

#include <QProgressDialog>
#include <QNetworkAccessManager>
#include <QUrl>
#include <queue>

QT_BEGIN_NAMESPACE
class QFile;
class QSslError;
class QAuthenticator;
class QNetworkReply;
QT_END_NAMESPACE

class ProgressDialog : public QProgressDialog {
    Q_OBJECT

public:
    explicit ProgressDialog(const QUrl &url, QWidget *parent = Q_NULLPTR);

public slots:
   void networkReplyProgress(qint64 bytesRead, qint64 totalBytes);
};

struct downloadData{
    downloadData(const QString &link, const QString &path, const QString &name) : link(link), path(path), name(name){}

    const QString link, path, name;
};

class HttpDownloader : public QObject
{
    Q_OBJECT
public:
    HttpDownloader();

    void downloadFile(const QString &link, const QString &path, const QString &name);
    inline void appendToDownloads(downloadData data){
       downloadQueue.push(data);
    }
    inline void queueDownloading(){
        if(!downloadQueue.empty()){
           downloadFile(downloadQueue.front().link, downloadQueue.front().path, downloadQueue.front().name);
           downloadQueue.pop();
        }
    }

private slots:
    void startRequest(const QUrl &requestedUrl);
    void cancelDownload();
    void httpFinished();
    void httpReadyRead();
#ifndef QT_NO_SSL
    void sslErrors(QNetworkReply*,const QList<QSslError> &errors);
#endif

private:
    QFile *openFileForWrite(const QString &fileName);

    QUrl url;
    QNetworkAccessManager qnam;
    QNetworkReply *reply;
    QFile *file;
    bool httpRequestAborted;

    std::queue<downloadData> downloadQueue;
};

#endif // HTTPDOWNLOADER_H
