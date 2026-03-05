#include "MainWindow.h"

#include "InstanceManager.h"
#include "ProtonDetector.h"
#include "GitHubReleaseTracker.h"
#include "LaunchManager.h"
#include "AddInstanceDialog.h"
#include "SettingsDialog.h"
#include "InstanceWidget.h"
#include "Downloader.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QToolBar>
#include <QScrollArea>
#include <QLabel>
#include <QPushButton>
#include <QMessageBox>
#include <QProgressDialog>
#include <QDir>
#include <QProcess>
#include <QCoreApplication>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_instanceManager(new InstanceManager(this))
    , m_launchManager(new LaunchManager(this))
    , m_releaseTracker(new GitHubReleaseTracker(this))
{
    setWindowTitle(tr("Legacy Launcher"));
    setMinimumSize(800, 600);

    m_protons = ProtonDetector::detect();

    connect(m_instanceManager, &InstanceManager::instancesChanged, this, &MainWindow::onInstancesChanged);
    connect(m_releaseTracker, &GitHubReleaseTracker::releasesUpdated, this, &MainWindow::onReleasesUpdated);
    connect(m_launchManager, &LaunchManager::instanceStarted, this, &MainWindow::onInstanceStarted);
    connect(m_launchManager, &LaunchManager::instanceStopped, this, &MainWindow::onInstanceStopped);
    connect(m_launchManager, &LaunchManager::instanceError, this, &MainWindow::onInstanceError);

    setupUi();
    rebuildInstanceList();

    m_releaseTracker->fetchReleases();
    statusBar()->showMessage(tr("Checking for updates..."));
}

void MainWindow::setupUi() {
    m_centralWidget = new QWidget(this);
    setCentralWidget(m_centralWidget);

    QVBoxLayout *rootLayout = new QVBoxLayout(m_centralWidget);

    QHBoxLayout *topBar = new QHBoxLayout();
    QLabel *titleLabel = new QLabel(tr("Instances"));
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(titleFont.pointSize() + 2);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    topBar->addWidget(titleLabel);
    topBar->addStretch();

    m_refreshBtn = new QPushButton(tr("Check for Updates"));
    topBar->addWidget(m_refreshBtn);

    m_addBtn = new QPushButton(tr("Add Instance"));
    topBar->addWidget(m_addBtn);

    rootLayout->addLayout(topBar);

    m_scrollArea = new QScrollArea();
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setFrameShape(QFrame::NoFrame);

    QWidget *scrollContents = new QWidget();
    m_instanceListLayout = new QVBoxLayout(scrollContents);
    m_instanceListLayout->setAlignment(Qt::AlignTop);
    m_instanceListLayout->setSpacing(4);

    m_emptyLabel = new QLabel(tr("No instances yet. Click \"Add Instance\" to get started."));
    m_emptyLabel->setAlignment(Qt::AlignCenter);
    m_emptyLabel->setEnabled(false);
    m_instanceListLayout->addWidget(m_emptyLabel);

    m_scrollArea->setWidget(scrollContents);
    rootLayout->addWidget(m_scrollArea, 1);

    connect(m_addBtn, &QPushButton::clicked, this, &MainWindow::onAddInstance);
    connect(m_refreshBtn, &QPushButton::clicked, this, &MainWindow::onRefreshReleases);
}

void MainWindow::rebuildInstanceList() {
    for (InstanceWidget *w : m_instanceWidgets) {
        m_instanceListLayout->removeWidget(w);
        w->deleteLater();
    }
    m_instanceWidgets.clear();

    QList<Instance> instances = m_instanceManager->instances();
    m_emptyLabel->setVisible(instances.isEmpty());

    for (const Instance &inst : instances) {
        InstanceWidget *w = new InstanceWidget(inst, this);
        w->setRunning(m_launchManager->isRunning(inst.id));

        bool upd = isUpdateAvailable(inst);
        if (upd) w->setUpdateAvailable(true, latestTagForInstance(inst));

        connect(w, &InstanceWidget::launchRequested, this, &MainWindow::onLaunchRequested);
        connect(w, &InstanceWidget::stopRequested, this, &MainWindow::onStopRequested);
        connect(w, &InstanceWidget::settingsRequested, this, &MainWindow::onSettingsRequested);
        connect(w, &InstanceWidget::updateRequested, this, &MainWindow::onUpdateRequested);
        connect(w, &InstanceWidget::deleteRequested, this, &MainWindow::onDeleteRequested);

        m_instanceListLayout->addWidget(w);
        m_instanceWidgets.append(w);
    }
}

void MainWindow::onAddInstance() {
    AddInstanceDialog dlg(m_protons, this);
    if (dlg.exec() == QDialog::Accepted) {
        m_instanceManager->addInstance(dlg.createdInstance());
    }
}

void MainWindow::onLaunchRequested(const QString &id) {
    Instance *inst = m_instanceManager->findById(id);
    if (!inst) return;

    ProtonInstallation proton = protonForId(inst->protonId);
    if (proton.path.isEmpty() && !m_protons.isEmpty()) {
        proton = m_protons.first();
    }

    if (proton.path.isEmpty()) {
        QMessageBox::critical(this, tr("No Proton"), tr("No Proton installation found. Please install Proton via Steam."));
        return;
    }

    m_instanceManager->setLastRan(id);
    m_launchManager->launch(*inst, proton);
}

void MainWindow::onStopRequested(const QString &id) {
    m_launchManager->terminate(id);
}

void MainWindow::onSettingsRequested(const QString &id) {
    Instance *inst = m_instanceManager->findById(id);
    if (!inst) return;

    SettingsDialog dlg(*inst, m_protons, this);
    if (dlg.exec() == QDialog::Accepted) {
        m_instanceManager->updateInstance(dlg.updatedInstance());
    }
}

void MainWindow::onUpdateRequested(const QString &id) {
    Instance *inst = m_instanceManager->findById(id);
    if (!inst || m_releases.isEmpty()) return;

    performUpdate(id, m_releases.first());
}

void MainWindow::onDeleteRequested(const QString &id) {
    Instance *inst = m_instanceManager->findById(id);
    if (!inst) return;

    QMessageBox::StandardButton ans = QMessageBox::question(this,
        tr("Delete Instance"),
        tr("Delete \"%1\"? This will remove all files at:\n%2").arg(inst->name).arg(inst->installPath));

    if (ans != QMessageBox::Yes) return;

    QDir dir(inst->installPath);
    dir.removeRecursively();

    m_instanceManager->removeInstance(id);
}

void MainWindow::onInstancesChanged() {
    rebuildInstanceList();
}

void MainWindow::onReleasesUpdated(QList<ReleaseInfo> releases) {
    m_releases = releases;

    for (InstanceWidget *w : m_instanceWidgets) {
        Instance *inst = m_instanceManager->findById(w->instanceId());
        if (!inst) continue;
        bool upd = isUpdateAvailable(*inst);
        w->setUpdateAvailable(upd, upd ? latestTagForInstance(*inst) : QString());
    }

    if (!releases.isEmpty()) {
        statusBar()->showMessage(tr("Latest release: %1 (%2)")
            .arg(releases.first().tag)
            .arg(releases.first().publishedAt.toLocalTime().toString("yyyy-MM-dd")));
    } else {
        statusBar()->showMessage(tr("No releases found."));
    }
}

void MainWindow::onInstanceStarted(const QString &id) {
    if (InstanceWidget *w = widgetForId(id)) w->setRunning(true);
    statusBar()->showMessage(tr("Instance started."));
}

void MainWindow::onInstanceStopped(const QString &id, int exitCode) {
    if (InstanceWidget *w = widgetForId(id)) w->setRunning(false);
    statusBar()->showMessage(tr("Instance stopped (exit code %1).").arg(exitCode));
}

void MainWindow::onInstanceError(const QString &id, const QString &error) {
    if (InstanceWidget *w = widgetForId(id)) w->setRunning(false);
    QMessageBox::warning(this, tr("Launch Error"), error);
}

void MainWindow::onRefreshReleases() {
    statusBar()->showMessage(tr("Checking for updates..."));
    m_releaseTracker->fetchReleases();
}

InstanceWidget *MainWindow::widgetForId(const QString &id) {
    for (InstanceWidget *w : m_instanceWidgets) {
        if (w->instanceId() == id) return w;
    }
    return nullptr;
}

ProtonInstallation MainWindow::protonForId(const QString &id) const {
    for (const ProtonInstallation &p : m_protons) {
        if (p.path == id) return p;
    }
    return {};
}

void MainWindow::performUpdate(const QString &instanceId, const ReleaseInfo &release) {
    Instance *inst = m_instanceManager->findById(instanceId);
    if (!inst) return;

    QProgressDialog progress(tr("Downloading update..."), tr("Cancel"), 0, 100, this);
    progress.setWindowModality(Qt::WindowModal);
    progress.show();

    Downloader downloader;
    QString zipPath = inst->installPath + "/update.zip";

    bool done = false;
    bool success = false;
    QString errMsg;

    QObject::connect(&downloader, &Downloader::progressChanged, [&](qint64 recv, qint64 total) {
        if (total > 0) progress.setValue(static_cast<int>((recv * 100) / total));
    });
    QObject::connect(&downloader, &Downloader::finished, [&](bool ok, QString err) {
        done = true; success = ok; errMsg = err;
    });
    QObject::connect(&progress, &QProgressDialog::canceled, [&]() {
        downloader.cancel();
    });

    downloader.download(release.downloadUrl, zipPath);

    while (!done) {
        QCoreApplication::processEvents();
        if (progress.wasCanceled()) break;
    }

    if (!success) {
        QMessageBox::warning(this, tr("Update Failed"), errMsg);
        return;
    }

    QProcess proc;
    proc.start("unzip", {"-o", zipPath, "-d", inst->installPath});
    proc.waitForFinished(-1);
    QFile::remove(zipPath);

    inst->installedTag = release.tag;
    inst->installedAt = QDateTime::currentDateTime();
    m_instanceManager->updateInstance(*inst);

    statusBar()->showMessage(tr("Updated to %1.").arg(release.tag));
}

bool MainWindow::isUpdateAvailable(const Instance &inst) const {
    if (m_releases.isEmpty() || inst.installedTag.isEmpty()) return false;
    const ReleaseInfo &latest = m_releases.first();
    return latest.tag != inst.installedTag && latest.publishedAt > inst.installedAt;
}

QString MainWindow::latestTagForInstance(const Instance &) const {
    if (m_releases.isEmpty()) return QString();
    return m_releases.first().tag;
}
