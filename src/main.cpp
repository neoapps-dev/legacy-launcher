#include "MainWindow.h"
#include "InstanceManager.h"
#include "LaunchManager.h"
#include "ProtonDetector.h"

#include <QApplication>
#include <QCommandLineParser>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("LegacyLauncher");
    app.setOrganizationName("LegacyLauncher");
    app.setApplicationVersion("1.0.0");

    QCommandLineParser parser;
    parser.setApplicationDescription("Legacy Launcher for Minecraft Legacy Console Edition");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addOption({"auto", "Launch the last ran instance without showing the launcher."});
    parser.process(app);

    bool autoMode = parser.isSet("auto");

    if (autoMode) {
        InstanceManager mgr;
        QString lastId = mgr.lastRanId();

        if (lastId.isEmpty()) {
            qWarning("No last-ran instance found.");
            return 1;
        }

        Instance *inst = mgr.findById(lastId);
        if (!inst) {
            qWarning("Last-ran instance not found.");
            return 1;
        }

        QList<ProtonInstallation> protons = ProtonDetector::detect();
        ProtonInstallation proton;
        for (const ProtonInstallation &p : protons) {
            if (p.path == inst->protonId) { proton = p; break; }
        }
        if (proton.path.isEmpty() && !protons.isEmpty()) proton = protons.first();

        if (proton.path.isEmpty()) {
            qWarning("No Proton installation available.");
            return 1;
        }

        LaunchManager launcher;
        mgr.setLastRan(lastId);
        launcher.launch(*inst, proton);

        return app.exec();
    }

    MainWindow window;
    window.show();

    return app.exec();
}
