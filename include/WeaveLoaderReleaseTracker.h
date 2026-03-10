#pragma once

#include "Types.h"
#include <QObject>
#include <QNetworkAccessManager>
#include <QList>

class WeaveLoaderReleaseTracker : public QObject {
    Q_OBJECT
public:
    explicit WeaveLoaderReleaseTracker(QObject *parent = nullptr);

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

    static constexpr const char *API_URL = "https://api.github.com/repos/Jacobwasbeast/LegacyWeaveLoader/releases";
    static constexpr const char *REPO = "Jacobwasbeast/LegacyWeaveLoader";

    ReleaseInfo parseRelease(const QJsonObject &obj);
};
