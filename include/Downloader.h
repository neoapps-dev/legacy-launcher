#pragma once

#include <QObject>
#include <QNetworkAccessManager>
#include <QFile>

class Downloader : public QObject {
    Q_OBJECT
public:
    explicit Downloader(QObject *parent = nullptr);

    void download(const QString &url, const QString &destPath);
    void cancel();

signals:
    void progressChanged(qint64 received, qint64 total);
    void finished(bool success, QString errorMsg);

private slots:
    void onReadyRead();
    void onFinished();
    void onDownloadProgress(qint64 received, qint64 total);

private:
    QNetworkAccessManager *m_nam;
    QNetworkReply *m_reply;
    QFile m_file;
    bool m_cancelled;
};
