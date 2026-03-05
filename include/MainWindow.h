#pragma once

#include "Types.h"
#include <QMainWindow>
#include <QList>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QStatusBar>

class InstanceManager;
class ProtonDetector;
class GitHubReleaseTracker;
class LaunchManager;
class InstanceWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void onAddInstance();
    void onLaunchRequested(const QString &id);
    void onStopRequested(const QString &id);
    void onSettingsRequested(const QString &id);
    void onUpdateRequested(const QString &id);
    void onDeleteRequested(const QString &id);
    void onInstancesChanged();
    void onReleasesUpdated(QList<ReleaseInfo> releases);
    void onInstanceStarted(const QString &id);
    void onInstanceStopped(const QString &id, int exitCode);
    void onInstanceError(const QString &id, const QString &error);
    void onRefreshReleases();

private:
    InstanceManager *m_instanceManager;
    LaunchManager *m_launchManager;
    GitHubReleaseTracker *m_releaseTracker;
    QList<ProtonInstallation> m_protons;
    QList<ReleaseInfo> m_releases;

    QWidget *m_centralWidget;
    QVBoxLayout *m_instanceListLayout;
    QScrollArea *m_scrollArea;
    QLabel *m_emptyLabel;
    QPushButton *m_addBtn;
    QPushButton *m_refreshBtn;
    QList<InstanceWidget *> m_instanceWidgets;

    void setupUi();
    void rebuildInstanceList();
    InstanceWidget *widgetForId(const QString &id);
    ProtonInstallation protonForId(const QString &id) const;
    void performUpdate(const QString &instanceId, const ReleaseInfo &release);
    QString latestTagForInstance(const Instance &inst) const;
    bool isUpdateAvailable(const Instance &inst) const;
};
