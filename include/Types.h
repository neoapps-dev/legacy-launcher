#pragma once

#include <QString>
#include <QDateTime>

struct ProtonInstallation {
    QString name;
    QString path;
    QString protonExecutable;
    bool isFlatpak;
};

struct ReleaseInfo {
    QString tag;
    QString name;
    QDateTime publishedAt;
    QString downloadUrl;
    bool isNightly;
};

struct Instance {
    QString id;
    QString name;
    QString installPath;
    QString protonId;
    QString installedTag;
    QDateTime installedAt;
    QString username;
    bool headlessMode;
    QString autoIp;
    int autoPort;
    QDateTime lastRun;
    bool weaveLoaderEnabled;
    QString weaveLoaderTag;
    QDateTime weaveLoaderInstalledAt;
};
