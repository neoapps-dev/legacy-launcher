#include "Downloader.h"

#include <QNetworkReply>
#include <QNetworkRequest>

Downloader::Downloader(QObject *parent)
    : QObject(parent)
    , m_nam(new QNetworkAccessManager(this))
    , m_reply(nullptr)
    , m_cancelled(false)
{}

void Downloader::download(const QString &url, const QString &destPath) {
    m_cancelled = false;
    m_file.setFileName(destPath);

    if (!m_file.open(QIODevice::WriteOnly)) {
        emit finished(false, tr("Cannot open file for writing: %1").arg(destPath));
        return;
    }

    QNetworkRequest req{QUrl(url)};
    req.setRawHeader("User-Agent", "LegacyLauncher/1.0");
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);

    m_reply = m_nam->get(req);
    connect(m_reply, &QNetworkReply::readyRead, this, &Downloader::onReadyRead);
    connect(m_reply, &QNetworkReply::finished, this, &Downloader::onFinished);
    connect(m_reply, &QNetworkReply::downloadProgress, this, &Downloader::onDownloadProgress);
}

void Downloader::cancel() {
    m_cancelled = true;
    if (m_reply) {
        m_reply->abort();
    }
}

void Downloader::onReadyRead() {
    if (m_reply && m_file.isOpen()) {
        m_file.write(m_reply->readAll());
    }
}

void Downloader::onFinished() {
    m_file.close();
    m_reply->deleteLater();

    if (m_cancelled) {
        QFile::remove(m_file.fileName());
        emit finished(false, tr("Cancelled"));
        return;
    }

    if (m_reply->error() != QNetworkReply::NoError) {
        QFile::remove(m_file.fileName());
        emit finished(false, m_reply->errorString());
        return;
    }

    emit finished(true, QString());
}

void Downloader::onDownloadProgress(qint64 received, qint64 total) {
    emit progressChanged(received, total);
}
