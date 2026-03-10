#pragma once

#include "Types.h"
#include <QMainWindow>
#include <QList>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QStatusBar>
#include <QStackedWidget>
#include <QComboBox>
#include <QButtonGroup>
#include <QStackedWidget>
#include <QTextBrowser>
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>

class InstanceManager;
class ProtonDetector;
class GitHubReleaseTracker;
class LaunchManager;
class InstanceWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

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

    void onPlayButtonClicked();
    void onTabNavClicked(int id);

private:
    InstanceManager *m_instanceManager;
    LaunchManager *m_launchManager;
    GitHubReleaseTracker *m_releaseTracker;
    QList<ProtonInstallation> m_protons;
    QList<ReleaseInfo> m_releases;

    QWidget *m_centralWidget;
    
    // Navigation
    QButtonGroup *m_tabGroup;
    QButtonGroup *m_sidebarGroup;
    QStackedWidget *m_mainStack;
    QWidget *m_sidebarIndicator;
    QPropertyAnimation *m_sidebarIndicatorAnim;
    
    bool m_isTabAnimating;

    // Play Tab
    QWidget *m_playTab;
    QComboBox *m_instanceCombo;
    QPushButton *m_playBtn;
    QLabel *m_playStatusLabel;

    // Patch Notes Tab
    QWidget *m_patchTab;
    QTextBrowser *m_patchNotesBox;

    // Installations Tab
    QWidget *m_installationsTab;
    QStackedWidget *m_rootStack;
    QVBoxLayout *m_instanceListLayout;
    QScrollArea *m_scrollArea;
    QLabel *m_emptyLabel;
    QPushButton *m_addBtn;
    QPushButton *m_refreshBtn;
    QList<InstanceWidget *> m_instanceWidgets;

    void setupUi();
    void applyGlobalStylesheet();
    void rebuildInstanceList();
    void updateFocusChain();
    void updatePlayButtonState();

    InstanceWidget *widgetForId(const QString &id);
    ProtonInstallation protonForId(const QString &id) const;
    QString latestTagForInstance(const Instance &inst) const;
    bool isUpdateAvailable(const Instance &inst) const;
};
