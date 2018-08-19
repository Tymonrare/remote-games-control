#include "httpdownloader.h"
#include <QtWidgets>
#include <QtNetwork>
#include <QUrl>

ProgressDialog::ProgressDialog(const QUrl &url, QWidget *parent)
  : QProgressDialog(parent)
{
    setWindowTitle(tr("Download Progress"));
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setLabelText(tr("Downloading %1.").arg(url.toDisplayString()));
    setMinimum(0);
    setValue(0);
    setMinimumDuration(0);
}

void ProgressDialog::networkReplyProgress(qint64 bytesRead, qint64 totalBytes)
{
    setMaximum(totalBytes);
    setValue(bytesRead);
}

HttpDownloader::HttpDownloader()
{
#ifndef QT_NO_SSL
    connect(&qnam, &QNetworkAccessManager::sslErrors,
            this, &HttpDownloader::sslErrors);
#endif
}

void HttpDownloader::startRequest(const QUrl &requestedUrl)
{
    url = requestedUrl;
    httpRequestAborted = false;

    reply = qnam.get(QNetworkRequest(url));
    connect(reply, &QNetworkReply::finished, this, &HttpDownloader::httpFinished);
    connect(reply, &QIODevice::readyRead, this, &HttpDownloader::httpReadyRead);

    ProgressDialog *progressDialog = new ProgressDialog(url, nullptr);
    progressDialog->setAttribute(Qt::WA_DeleteOnClose);
    connect(progressDialog, &QProgressDialog::canceled, this, &HttpDownloader::cancelDownload);
    connect(reply, &QNetworkReply::downloadProgress, progressDialog, &ProgressDialog::networkReplyProgress);
    connect(reply, &QNetworkReply::finished, progressDialog, &ProgressDialog::hide);
    progressDialog->show();

}

void HttpDownloader::downloadFile(const QString &link, const QString &path, const QString &name)
{
    const QString urlSpec = link;
    if (urlSpec.isEmpty())
        return;

    const QUrl newUrl = QUrl::fromUserInput(urlSpec);
    if (!newUrl.isValid()) {
        QMessageBox::information(nullptr, tr("Error"),
                                 tr("Invalid URL: %1: %2").arg(urlSpec, newUrl.errorString()));
        return;
    }

    QString fileName = name;
    if (fileName.isEmpty())
        fileName = newUrl.fileName();
    if (fileName.isEmpty())
        fileName = path;
    QString downloadDirectory = QDir::cleanPath(path);
    if (!downloadDirectory.isEmpty() && QFileInfo(downloadDirectory).isDir())
        fileName.prepend(downloadDirectory + '/');
    if (QFile::exists(fileName)) {
        if (QMessageBox::question(nullptr, tr("Overwrite Existing File"),
                                  tr("There already exists a file called %1 in "
                                     "the current directory. Overwrite?").arg(fileName),
                                  QMessageBox::Yes|QMessageBox::No, QMessageBox::No)
            == QMessageBox::No)
            return;
        QFile::remove(fileName);
    }

    this->file = openFileForWrite(fileName);
    if (!this->file)
        return;

    // schedule the request
    startRequest(newUrl);
}

QFile *HttpDownloader::openFileForWrite(const QString &fileName)
{
    QScopedPointer<QFile> file(new QFile(fileName));
    if (!file->open(QIODevice::WriteOnly)) {
        QMessageBox::information(nullptr, tr("Error"),
                                 tr("Unable to save the file %1: %2.")
                                 .arg(QDir::toNativeSeparators(fileName),
                                      file->errorString()));
        return Q_NULLPTR;
    }
    return file.take();
}
void HttpDownloader::cancelDownload()
{
    qInfo() << "Download canceled.";
    httpRequestAborted = true;
    reply->abort();
}

void HttpDownloader::httpFinished()
{
    QFileInfo fi;
    if (file) {
        fi.setFile(file->fileName());
        file->close();
        delete file;
        file = Q_NULLPTR;
    }

    if (httpRequestAborted) {
        reply->deleteLater();
        reply = Q_NULLPTR;
        queueDownloading();
        return;
    }

    if (reply->error()) {
        QFile::remove(fi.absoluteFilePath());
        qInfo() << (tr("Download failed:\n%1.").arg(reply->errorString()));
        reply->deleteLater();
        reply = Q_NULLPTR;
        queueDownloading();
        return;
    }

    const QVariant redirectionTarget = reply->attribute(QNetworkRequest::RedirectionTargetAttribute);

    reply->deleteLater();
    reply = Q_NULLPTR;

    if (!redirectionTarget.isNull()) {
        const QUrl redirectedUrl = url.resolved(redirectionTarget.toUrl());
        if (QMessageBox::question(nullptr, tr("Redirect"),
                                  tr("Redirect to %1 ?").arg(redirectedUrl.toString()),
                                  QMessageBox::Yes | QMessageBox::No) == QMessageBox::No) {
            return;
        }
        file = openFileForWrite(fi.absoluteFilePath());
        if (!file) {
            queueDownloading();
            return;
        }
        startRequest(redirectedUrl);
        queueDownloading();
        return;
    }

    qInfo() << (tr("Downloaded %1 bytes to %2\nin\n%3")
                         .arg(fi.size()).arg(fi.fileName(), QDir::toNativeSeparators(fi.absolutePath())));
    queueDownloading();
}

void HttpDownloader::httpReadyRead()
{
    // this slot gets called every time the QNetworkReply has new data.
    // We read all of its new data and write it into the file.
    // That way we use less RAM than when reading it at the finished()
    // signal of the QNetworkReply
    if (file)
        file->write(reply->readAll());
}

#ifndef QT_NO_SSL
void HttpDownloader::sslErrors(QNetworkReply*,const QList<QSslError> &errors)
{
    QString errorString;
    foreach (const QSslError &error, errors) {
        if (!errorString.isEmpty())
            errorString += '\n';
        errorString += error.errorString();
    }

    if (QMessageBox::warning(nullptr, tr("SSL Errors"),
                             tr("One or more SSL errors has occurred:\n%1").arg(errorString),
                             QMessageBox::Ignore | QMessageBox::Abort) == QMessageBox::Ignore) {
        reply->ignoreSslErrors();
    }
}
#endif
