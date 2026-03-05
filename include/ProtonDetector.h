#pragma once

#include "Types.h"
#include <QList>

class ProtonDetector {
public:
    static QList<ProtonInstallation> detect();

private:
    static QList<ProtonInstallation> detectNativeSteam();
    static QList<ProtonInstallation> detectFlatpakSteam();
    static QList<ProtonInstallation> scanSteamRoot(const QString &steamRoot, bool isFlatpak);
    static bool isValidProton(const QString &path);
};
