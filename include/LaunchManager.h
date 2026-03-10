#pragma once

#include "Types.h"
#include <QObject>
#include <QProcess>
#include <QMap>

class LaunchManager : public QObject {
    Q_OBJECT
public:
    explicit LaunchManager(QObject *parent = nullptr);

    bool launch(const Instance &instance, const ProtonInstallation &proton);
    bool isRunning(const QString &instanceId) const;
    void terminate(const QString &instanceId);

signals:
    void instanceStarted(QString instanceId);
    void instanceStopped(QString instanceId, int exitCode);
    void instanceError(QString instanceId, QString error);

private slots:
    void onProcessFinished(int exitCode, QProcess::ExitStatus status);
    void onProcessError(QProcess::ProcessError error);

private:
    QMap<QString, QProcess *> m_processes;
    Instance m_currentInstance;
    ProtonInstallation m_currentProton;
    QString m_currentGameExe;

    QString protonPrefixPath(const Instance &instance) const;
    QStringList buildGameArgs(const Instance &instance) const;
    void continueLaunch();
};
