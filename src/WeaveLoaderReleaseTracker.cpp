#include "WeaveLoaderReleaseTracker.h"

#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

WeaveLoaderReleaseTracker::WeaveLoaderReleaseTracker(QObject *parent)
    : QObject(parent)
    , m_nam(new QNetworkAccessManager(this))
{}

void WeaveLoaderReleaseTracker::fetchReleases() {
    QNetworkRequest req{QUrl(API_URL)};
    req.setRawHeader("Accept", "application/vnd.github+json");
    req.setRawHeader("User-Agent", "LegacyLauncher/1.0");

    QNetworkReply *reply = m_nam->get(req);
    connect(reply, &QNetworkReply::finished, this, &WeaveLoaderReleaseTracker::onReleasesReply);
}

void WeaveLoaderReleaseTracker::onReleasesReply() {
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        emit fetchError(reply->errorString());
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
    QJsonArray arr = doc.array();

    m_releases.clear();
    for (const QJsonValue &val : arr) {
        if (val.isObject()) {
            m_releases.append(parseRelease(val.toObject()));
        }
    }

    std::sort(m_releases.begin(), m_releases.end(), [](const ReleaseInfo &a, const ReleaseInfo &b) {
        return a.publishedAt > b.publishedAt;
    });

    emit releasesUpdated(m_releases);
}

ReleaseInfo WeaveLoaderReleaseTracker::parseRelease(const QJsonObject &obj) {
    ReleaseInfo r;
    r.tag = obj["tag_name"].toString();
    r.name = obj["name"].toString();
    r.publishedAt = QDateTime::fromString(obj["published_at"].toString(), Qt::ISODate);
    r.isNightly = false;

    QString titleWithDashes = r.name;
    titleWithDashes.replace(" ", "-");
    QString downloadBase = QString("https://github.com/%1/releases/download/%2/WeaveLoader.%3.zip")
        .arg(REPO)
        .arg(r.tag)
        .arg(titleWithDashes);
    r.downloadUrl = downloadBase;

    return r;
}

QList<ReleaseInfo> WeaveLoaderReleaseTracker::cachedReleases() const {
    return m_releases;
}
