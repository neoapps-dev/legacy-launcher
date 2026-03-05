#include "ProtonDetector.h"

#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>
#include <QProcess>

QList<ProtonInstallation> ProtonDetector::detect() {
    QList<ProtonInstallation> result;
    result.append(detectNativeSteam());
    result.append(detectFlatpakSteam());
    return result;
}

QList<ProtonInstallation> ProtonDetector::detectNativeSteam() {
    QList<ProtonInstallation> result;
    QString home = QDir::homePath();

    QStringList steamRoots = {
        home + "/.steam/steam",
        home + "/.steam/root",
        home + "/.local/share/Steam"
    };

    for (const QString &root : steamRoots) {
        if (QDir(root).exists()) {
            auto found = scanSteamRoot(root, false);
            for (const auto &p : found) {
                bool dupe = false;
                for (const auto &existing : result) {
                    if (existing.path == p.path) { dupe = true; break; }
                }
                if (!dupe) result.append(p);
            }
        }
    }

    return result;
}

QList<ProtonInstallation> ProtonDetector::detectFlatpakSteam() {
    QList<ProtonInstallation> result;
    QString home = QDir::homePath();

    QStringList flatpakRoots = {
        home + "/.var/app/com.valvesoftware.Steam/data/Steam",
        home + "/.var/app/com.valvesoftware.Steam/.steam/steam"
    };

    for (const QString &root : flatpakRoots) {
        if (QDir(root).exists()) {
            auto found = scanSteamRoot(root, true);
            for (const auto &p : found) {
                bool dupe = false;
                for (const auto &existing : result) {
                    if (existing.path == p.path) { dupe = true; break; }
                }
                if (!dupe) result.append(p);
            }
        }
    }

    return result;
}

QList<ProtonInstallation> ProtonDetector::scanSteamRoot(const QString &steamRoot, bool isFlatpak) {
    QList<ProtonInstallation> result;

    QStringList searchPaths = {
        steamRoot + "/steamapps/common",
        steamRoot + "/compatibilitytools.d"
    };

    for (const QString &searchPath : searchPaths) {
        QDir dir(searchPath);
        if (!dir.exists()) continue;

        for (const QString &entry : dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
            QString fullPath = searchPath + "/" + entry;
            if (isValidProton(fullPath)) {
                ProtonInstallation p;
                p.name = entry + (isFlatpak ? " (Flatpak)" : "");
                p.path = fullPath;
                p.protonExecutable = fullPath + "/proton";
                p.isFlatpak = isFlatpak;
                result.append(p);
            }
        }
    }

    return result;
}

bool ProtonDetector::isValidProton(const QString &path) {
    return QFileInfo(path + "/proton").isExecutable();
}
