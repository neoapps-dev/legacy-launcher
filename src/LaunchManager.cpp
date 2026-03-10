#include "LaunchManager.h"

#include <QDir>
#include <QFileInfo>
#include <QDebug>
#include <QCoreApplication>

LaunchManager::LaunchManager(QObject *parent)
    : QObject(parent)
{}

void LaunchManager::continueLaunch() {
    const Instance &instance = m_currentInstance;
    const ProtonInstallation &proton = m_currentProton;
    QString gameExe = m_currentGameExe;

    if (isRunning(instance.id)) {
        emit instanceError(instance.id, tr("Instance already running"));
        return;
    }

    QString prefixPath = protonPrefixPath(instance);
    QDir().mkpath(prefixPath);

    QProcess *proc = new QProcess(this);
    proc->setProperty("instanceId", instance.id);

    QStringList args = buildGameArgs(instance);

    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert("STEAM_COMPAT_DATA_PATH", prefixPath);
    env.insert("STEAM_COMPAT_CLIENT_INSTALL_PATH", proton.path + "/../..");

    bool isFlatpakLauncher = QFileInfo("/app/bin/legacy-launcher").exists();
    QString actualProtonPath = proton.path;
    QString actualGameExe = gameExe;

    if (isFlatpakLauncher && proton.isFlatpak) {
        QString flatpakSteamPath = QDir::homePath() + "/.var/app/com.valvesoftware.Steam";
        if (proton.path.startsWith(flatpakSteamPath)) {
            QString relativePath = proton.path.mid(flatpakSteamPath.length());
            QStringList possibleHostPaths = {
                QDir::homePath() + "/.local/share/Steam" + relativePath,
                QDir::homePath() + "/.steam/steam" + relativePath,
                QDir::homePath() + "/.steam/root" + relativePath
            };
            for (const QString &hostPath : possibleHostPaths) {
                if (QFileInfo(hostPath).exists()) {
                    actualProtonPath = hostPath;
                    break;
                }
            }

            QString instanceRelativePath = instance.installPath.mid(QDir::homePath().length());
            QStringList possibleInstanceHostPaths = {
                QDir::homePath() + "/.local/share/LegacyLauncher" + instanceRelativePath,
                QDir::homePath() + instanceRelativePath
            };
            QString exeToCheck = instance.weaveLoaderEnabled ? "/WeaveLoader.exe" : "/Minecraft.Client.exe";
            for (const QString &hostPath : possibleInstanceHostPaths) {
                if (QFileInfo(hostPath + exeToCheck).exists()) {
                    actualGameExe = hostPath + exeToCheck;
                    break;
                }
            }

            env.insert("STEAM_COMPAT_CLIENT_INSTALL_PATH",
                actualProtonPath + "/../..");
        }
    }

    proc->setProcessEnvironment(env);

    QStringList fullArgs;

    if (proton.isFlatpak && !isFlatpakLauncher) {
        proc->setProgram("flatpak");
        fullArgs << "run" << "--command=" + proton.protonExecutable
                 << "com.valvesoftware.Steam"
                 << "run" << actualGameExe;
    } else if (isFlatpakLauncher) {
        QString wrapperPath = QDir::homePath() + "/.local/share/LegacyLauncher/flatpak-wrapper.sh";
        QFile wrapperFile(wrapperPath);
        if (wrapperFile.open(QIODevice::WriteOnly)) {
            QTextStream out(&wrapperFile);
            out << "#!/bin/bash\n";
            out << "export LD_LIBRARY_PATH=/usr/lib:/usr/lib64:/lib:/lib64:$LD_LIBRARY_PATH\n";
            out << "export STEAM_COMPAT_DATA_PATH=\"" << prefixPath << "\"\n";
            out << "export STEAM_COMPAT_CLIENT_INSTALL_PATH=\"" << actualProtonPath << "/../..\"\n";
            out << "exec \"" << actualProtonPath << "/proton\" run \"" << actualGameExe << "\" \"$@\"\n";
            wrapperFile.close();
            QProcess::execute("chmod", {"+x", wrapperPath});
        }

        proc->setProgram("flatpak-spawn");
        fullArgs << "--host" << wrapperPath;
    } else {
        bool isAppImage = QFileInfo(QCoreApplication::applicationDirPath() + "/../Libs").exists() ||
                          QCoreApplication::applicationDirPath().contains(".AppImage");

        if (isAppImage) {
            QString wrapperPath = QDir::homePath() + "/.local/share/LegacyLauncher/appimage-wrapper.sh";
            QFile wrapperFile(wrapperPath);
            if (wrapperFile.open(QIODevice::WriteOnly)) {
                QTextStream out(&wrapperFile);
                out << "#!/bin/bash\n";
                out << "export LD_LIBRARY_PATH=\"" << QCoreApplication::applicationDirPath() << "/../usr/lib:" << QCoreApplication::applicationDirPath() << "/usr/lib:$LD_LIBRARY_PATH\"\n";
                out << "export STEAM_COMPAT_DATA_PATH=\"" << prefixPath << "\"\n";
                out << "export STEAM_COMPAT_CLIENT_INSTALL_PATH=\"" << actualProtonPath << "/../..\"\n";
                out << "exec \"" << actualProtonPath << "/proton\" run \"" << actualGameExe << "\" \"$@\"\n";
                wrapperFile.close();
                QProcess::execute("chmod", {"+x", wrapperPath});
            }
            proc->setProgram(wrapperPath);
            fullArgs.clear();
        } else {
            proc->setProgram(actualProtonPath + "/proton");
            fullArgs << "run" << actualGameExe;
        }
    }

    fullArgs.append(args);
    proc->setArguments(fullArgs);

    qDebug() << "Launching with:" << proc->program() << fullArgs;
    qDebug() << "Environment:" << env.toStringList();

    connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &LaunchManager::onProcessFinished);
    connect(proc, &QProcess::errorOccurred, this, &LaunchManager::onProcessError);

    m_processes[instance.id] = proc;
    proc->start();

    if (!proc->waitForStarted(3000)) {
        m_processes.remove(instance.id);
        proc->deleteLater();
        emit instanceError(instance.id, tr("Failed to start process"));
        return;
    }

    emit instanceStarted(instance.id);
}

bool LaunchManager::launch(const Instance &instance, const ProtonInstallation &proton) {
    if (isRunning(instance.id)) return false;

    if (instance.weaveLoaderEnabled) {
        m_currentGameExe = instance.installPath + "/WeaveLoader.exe";
    } else {
        m_currentGameExe = instance.installPath + "/Minecraft.Client.exe";
    }

    m_currentInstance = instance;
    m_currentProton = proton;

    QString prefixPath = protonPrefixPath(instance);
    QDir().mkpath(prefixPath);

    continueLaunch();
    return true;
}

bool LaunchManager::isRunning(const QString &instanceId) const {
    return m_processes.contains(instanceId) &&
           m_processes[instanceId]->state() != QProcess::NotRunning;
}

void LaunchManager::terminate(const QString &instanceId) {
    if (m_processes.contains(instanceId)) {
        m_processes[instanceId]->terminate();
    }
}

void LaunchManager::onProcessFinished(int exitCode, QProcess::ExitStatus) {
    QProcess *proc = qobject_cast<QProcess *>(sender());
    QString id = proc->property("instanceId").toString();
    qDebug() << "Process finished with exit code:" << exitCode;
    m_processes.remove(id);
    proc->deleteLater();
    emit instanceStopped(id, exitCode);
}

void LaunchManager::onProcessError(QProcess::ProcessError error) {
    QProcess *proc = qobject_cast<QProcess *>(sender());
    QString id = proc->property("instanceId").toString();
    QString msg;
    switch (error) {
        case QProcess::FailedToStart: msg = tr("Failed to start"); break;
        case QProcess::Crashed: msg = tr("Crashed"); break;
        default: msg = tr("Unknown error"); break;
    }
    m_processes.remove(id);
    proc->deleteLater();
    emit instanceError(id, msg);
}

QString LaunchManager::protonPrefixPath(const Instance &instance) const {
    return instance.installPath + "/proton-prefix";
}

QStringList LaunchManager::buildGameArgs(const Instance &instance) const {
    QStringList args;
    if (!instance.username.isEmpty()) {
        args << "-name" << instance.username;
    }

    if (instance.headlessMode) {
        args << "-server";
    }

    if (!instance.autoIp.isEmpty()) {
        args << "-ip" << instance.autoIp;
    }

    if (instance.autoPort > 0) {
        args << "-port" << QString::number(instance.autoPort);
    }

    return args;
}
