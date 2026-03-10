#include "GitHubReleaseTracker.h"

#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

GitHubReleaseTracker::GitHubReleaseTracker(QObject *parent)
    : QObject(parent)
    , m_nam(new QNetworkAccessManager(this))
{}

void GitHubReleaseTracker::fetchReleases() {
    QNetworkRequest req{QUrl(API_URL)};
    req.setRawHeader("Accept", "application/vnd.github+json");
    req.setRawHeader("User-Agent", "LegacyLauncher/1.0");

    QNetworkReply *reply = m_nam->get(req);
    connect(reply, &QNetworkReply::finished, this, &GitHubReleaseTracker::onReleasesReply);
}

void GitHubReleaseTracker::onReleasesReply() {
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

ReleaseInfo GitHubReleaseTracker::parseRelease(const QJsonObject &obj) {
    ReleaseInfo r;
    r.tag = obj["tag_name"].toString();
    r.name = obj["name"].toString();
    r.body = obj["body"].toString();
    r.publishedAt = QDateTime::fromString(obj["published_at"].toString(), Qt::ISODate);
    r.isNightly = isNightlyRelease(r.tag, r.name);

    QString downloadBase = QString("https://github.com/%1/releases/download/%2/LCEWindows64.zip")
        .arg(REPO)
        .arg(r.tag);
    r.downloadUrl = downloadBase;

    return r;
}

bool GitHubReleaseTracker::isNightlyRelease(const QString &tag, const QString &name) {
    QString lTag = tag.toLower();
    QString lName = name.toLower();
    return lTag.contains("nightly") || lName.contains("nightly")
        || lTag.contains("dev") || lName.contains("dev")
        || lTag.contains("snapshot") || lName.contains("snapshot");
}

QList<ReleaseInfo> GitHubReleaseTracker::cachedReleases() const {
    return m_releases;
}
