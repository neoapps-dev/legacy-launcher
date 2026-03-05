#pragma once

#include "Types.h"
#include <QObject>
#include <QNetworkAccessManager>
#include <QList>

class GitHubReleaseTracker : public QObject {
    Q_OBJECT
public:
    explicit GitHubReleaseTracker(QObject *parent = nullptr);

    void fetchReleases();
    QList<ReleaseInfo> cachedReleases() const;

signals:
    void releasesUpdated(QList<ReleaseInfo> releases);
    void fetchError(QString message);

private slots:
    void onReleasesReply();

private:
    QNetworkAccessManager *m_nam;
    QList<ReleaseInfo> m_releases;

    static constexpr const char *API_URL = "https://api.github.com/repos/smartcmd/MinecraftConsoles/releases";
    static constexpr const char *REPO = "smartcmd/MinecraftConsoles";

    ReleaseInfo parseRelease(const QJsonObject &obj);
    bool isNightlyRelease(const QString &tag, const QString &name);
};
