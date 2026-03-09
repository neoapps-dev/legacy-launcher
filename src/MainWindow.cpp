#include "MainWindow.h"

#include "AddInstanceDialog.h"
#include "GitHubReleaseTracker.h"
#include "InstanceManager.h"
#include "InstanceWidget.h"
#include "LaunchManager.h"
#include "ProtonDetector.h"
#include "SettingsDialog.h"
#include "UpdateInstanceDialog.h"

#include <QApplication>
#include <QCoreApplication>
#include <QDir>
#include <QEvent>
#include <QFont>
#include <QFontDatabase>
#include <QGraphicsDropShadowEffect>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), m_instanceManager(new InstanceManager(this)),
      m_launchManager(new LaunchManager(this)),
      m_releaseTracker(new GitHubReleaseTracker(this)) {
  setWindowTitle(tr("Legacy Launcher"));
  setMinimumSize(900, 650);
  resize(1000, 700);

  m_protons = ProtonDetector::detect();

  connect(m_instanceManager, &InstanceManager::instancesChanged, this,
          &MainWindow::onInstancesChanged);
  connect(m_releaseTracker, &GitHubReleaseTracker::releasesUpdated, this,
          &MainWindow::onReleasesUpdated);
  connect(m_launchManager, &LaunchManager::instanceStarted, this,
          &MainWindow::onInstanceStarted);
  connect(m_launchManager, &LaunchManager::instanceStopped, this,
          &MainWindow::onInstanceStopped);
  connect(m_launchManager, &LaunchManager::instanceError, this,
          &MainWindow::onInstanceError);

  applyGlobalStylesheet();
  setupUi();
  rebuildInstanceList();

  m_releaseTracker->fetchReleases();
  statusBar()->showMessage(tr("Checking for updates..."));

  installEventFilter(this);
}

void MainWindow::applyGlobalStylesheet() {
  QString css = R"(
        QLabel#headerTitle {
            font-size: 24px;
            font-weight: 700;
            padding: 0;
        }

        QLabel#headerSubtitle {
            font-size: 13px;
            padding: 0;
        }
    )";
  qApp->setStyleSheet(css);
}

void MainWindow::setupUi() {
  m_centralWidget = new QWidget(this);
  m_centralWidget->setObjectName("centralPanel");
  setCentralWidget(m_centralWidget);

  QVBoxLayout *rootLayout = new QVBoxLayout(m_centralWidget);
  rootLayout->setContentsMargins(28, 24, 28, 12);
  rootLayout->setSpacing(0);

  QVBoxLayout *headerBlock = new QVBoxLayout();
  headerBlock->setSpacing(4);

  QHBoxLayout *headerTop = new QHBoxLayout();
  headerTop->setSpacing(16);

  QLabel *logoLabel = new QLabel();
  QPixmap logoPixmap(":/packaging/icon.png");
  if (!logoPixmap.isNull()) {
    logoPixmap = logoPixmap.scaled(50, 50, Qt::KeepAspectRatio,
                                   Qt::SmoothTransformation);
    logoLabel->setPixmap(logoPixmap);
  }
  logoLabel->setFixedSize(50, 50);
  headerTop->addWidget(logoLabel);

  QVBoxLayout *titleBlock = new QVBoxLayout();
  titleBlock->setSpacing(4);

  QLabel *titleLabel = new QLabel(tr("Legacy Launcher"));
  titleLabel->setObjectName("headerTitle");
  titleBlock->addWidget(titleLabel);

  QLabel *subtitleLabel = new QLabel(tr("Minecraft Legacy Console Edition"));
  subtitleLabel->setObjectName("headerSubtitle");
  titleBlock->addWidget(subtitleLabel);

  titleBlock->addStretch();
  headerTop->addLayout(titleBlock);

  headerBlock->addLayout(headerTop);

  rootLayout->addLayout(headerBlock);
  rootLayout->addSpacing(20);

  QHBoxLayout *toolBar = new QHBoxLayout();
  toolBar->setSpacing(10);

  QLabel *instancesLabel = new QLabel(tr("Instances"));
  instancesLabel->setStyleSheet(
      "color: #8a8f9d; font-size: 12px; font-weight: 600; letter-spacing: 1px; "
      "text-transform: uppercase;");
  toolBar->addWidget(instancesLabel);
  toolBar->addStretch();

  m_refreshBtn = new QPushButton(tr("\xe2\x9f\xb3  Refresh"));
  m_refreshBtn->setObjectName("refreshBtn");
  m_refreshBtn->setFocusPolicy(Qt::StrongFocus);
  toolBar->addWidget(m_refreshBtn);

  m_addBtn = new QPushButton(tr("+  Add Instance"));
  m_addBtn->setObjectName("addInstanceBtn");
  m_addBtn->setFocusPolicy(Qt::StrongFocus);
  toolBar->addWidget(m_addBtn);

  rootLayout->addLayout(toolBar);
  rootLayout->addSpacing(12);

  m_scrollArea = new QScrollArea();
  m_scrollArea->setWidgetResizable(true);
  m_scrollArea->setFrameShape(QFrame::NoFrame);
  m_scrollArea->setFocusPolicy(Qt::NoFocus);
  m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

  QWidget *scrollContents = new QWidget();
  scrollContents->setObjectName("scrollContents");
  m_instanceListLayout = new QVBoxLayout(scrollContents);
  m_instanceListLayout->setAlignment(Qt::AlignTop);
  m_instanceListLayout->setSpacing(8);
  m_instanceListLayout->setContentsMargins(0, 0, 8, 0);

  m_emptyLabel = new QLabel(
      tr("No instances yet.\nClick \"+  Add Instance\" to get started."));
  m_emptyLabel->setObjectName("emptyStateLabel");
  m_emptyLabel->setAlignment(Qt::AlignCenter);
  m_instanceListLayout->addWidget(m_emptyLabel);

  m_scrollArea->setWidget(scrollContents);
  rootLayout->addWidget(m_scrollArea, 1);

  connect(m_addBtn, &QPushButton::clicked, this, &MainWindow::onAddInstance);
  connect(m_refreshBtn, &QPushButton::clicked, this,
          &MainWindow::onRefreshReleases);
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
    if (upd)
      w->setUpdateAvailable(true, latestTagForInstance(inst));

    connect(w, &InstanceWidget::launchRequested, this,
            &MainWindow::onLaunchRequested);
    connect(w, &InstanceWidget::stopRequested, this,
            &MainWindow::onStopRequested);
    connect(w, &InstanceWidget::settingsRequested, this,
            &MainWindow::onSettingsRequested);
    connect(w, &InstanceWidget::updateRequested, this,
            &MainWindow::onUpdateRequested);
    connect(w, &InstanceWidget::deleteRequested, this,
            &MainWindow::onDeleteRequested);

    m_instanceListLayout->addWidget(w);
    m_instanceWidgets.append(w);
  }

  updateFocusChain();
}

void MainWindow::updateFocusChain() {
  QWidget *prev = m_addBtn;
  setTabOrder(m_refreshBtn, m_addBtn);

  for (InstanceWidget *w : m_instanceWidgets) {
    QList<QPushButton *> buttons = w->findChildren<QPushButton *>();
    for (QPushButton *btn : buttons) {
      setTabOrder(prev, btn);
      prev = btn;
    }
  }
  if (prev != m_refreshBtn) {
    setTabOrder(prev, m_refreshBtn);
  }
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event) {
  if (event->type() == QEvent::KeyPress) {
    QKeyEvent *ke = static_cast<QKeyEvent *>(event);
    switch (ke->key()) {
    case Qt::Key_Up:
    case Qt::Key_Left:
      focusNextPrevChild(false);
      return true;
    case Qt::Key_Down:
    case Qt::Key_Right:
      focusNextPrevChild(true);
      return true;
    case Qt::Key_Return:
    case Qt::Key_Enter:
    case Qt::Key_Space: {
      QWidget *focused = QApplication::focusWidget();
      QPushButton *btn = qobject_cast<QPushButton *>(focused);
      if (btn) {
        btn->click();
        return true;
      }
      break;
    }
    default:
      break;
    }
  }
  return QMainWindow::eventFilter(obj, event);
}

void MainWindow::onAddInstance() {
  AddInstanceDialog dlg(m_protons, this);
  if (dlg.exec() == QDialog::Accepted) {
    m_instanceManager->addInstance(dlg.createdInstance());
  }
}

void MainWindow::onLaunchRequested(const QString &id) {
  Instance *inst = m_instanceManager->findById(id);
  if (!inst)
    return;

  ProtonInstallation proton = protonForId(inst->protonId);
  if (proton.path.isEmpty() && !m_protons.isEmpty()) {
    proton = m_protons.first();
  }

  if (proton.path.isEmpty()) {
    QMessageBox::critical(
        this, tr("No Proton"),
        tr("No Proton installation found. Please install Proton via Steam."));
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
  if (!inst)
    return;

  SettingsDialog dlg(*inst, m_protons, this);
  if (dlg.exec() == QDialog::Accepted) {
    m_instanceManager->updateInstance(dlg.updatedInstance());
  }
}

void MainWindow::onUpdateRequested(const QString &id) {
  Instance *inst = m_instanceManager->findById(id);
  if (!inst)
    return;

  UpdateInstanceDialog dlg(*inst, m_releases, this);
  if (dlg.exec() == QDialog::Accepted) {
    m_instanceManager->updateInstance(dlg.updatedInstance());
    statusBar()->showMessage(tr("Updated \"%1\" to %2.")
                                 .arg(inst->name)
                                 .arg(dlg.updatedInstance().installedTag));
  }
}

void MainWindow::onDeleteRequested(const QString &id) {
  Instance *inst = m_instanceManager->findById(id);
  if (!inst)
    return;

  QMessageBox::StandardButton ans = QMessageBox::question(
      this, tr("Delete Instance"),
      tr("Delete \"%1\"? This will remove all files at:\n%2")
          .arg(inst->name)
          .arg(inst->installPath));

  if (ans != QMessageBox::Yes)
    return;

  QDir dir(inst->installPath);
  dir.removeRecursively();

  m_instanceManager->removeInstance(id);
}

void MainWindow::onInstancesChanged() { rebuildInstanceList(); }

void MainWindow::onReleasesUpdated(QList<ReleaseInfo> releases) {
  m_releases = releases;

  for (InstanceWidget *w : m_instanceWidgets) {
    Instance *inst = m_instanceManager->findById(w->instanceId());
    if (!inst)
      continue;
    bool upd = isUpdateAvailable(*inst);
    w->setUpdateAvailable(upd, upd ? latestTagForInstance(*inst) : QString());
  }

  if (!releases.isEmpty()) {
    statusBar()->showMessage(
        tr("Latest release: %1 (%2)")
            .arg(releases.first().tag)
            .arg(releases.first().publishedAt.toLocalTime().toString(
                "yyyy-MM-dd")));
  } else {
    statusBar()->showMessage(tr("No releases found."));
  }
}

void MainWindow::onInstanceStarted(const QString &id) {
  if (InstanceWidget *w = widgetForId(id))
    w->setRunning(true);
  statusBar()->showMessage(tr("Instance started."));
}

void MainWindow::onInstanceStopped(const QString &id, int exitCode) {
  if (InstanceWidget *w = widgetForId(id))
    w->setRunning(false);
  statusBar()->showMessage(
      tr("Instance stopped (exit code %1).").arg(exitCode));
}

void MainWindow::onInstanceError(const QString &id, const QString &error) {
  if (InstanceWidget *w = widgetForId(id))
    w->setRunning(false);
  QMessageBox::warning(this, tr("Launch Error"), error);
}

void MainWindow::onRefreshReleases() {
  statusBar()->showMessage(tr("Checking for updates..."));
  m_releaseTracker->fetchReleases();
}

InstanceWidget *MainWindow::widgetForId(const QString &id) {
  for (InstanceWidget *w : m_instanceWidgets) {
    if (w->instanceId() == id)
      return w;
  }
  return nullptr;
}

ProtonInstallation MainWindow::protonForId(const QString &id) const {
  for (const ProtonInstallation &p : m_protons) {
    if (p.path == id)
      return p;
  }
  return {};
}

bool MainWindow::isUpdateAvailable(const Instance &inst) const {
  if (m_releases.isEmpty() || inst.installedTag.isEmpty())
    return false;
  const ReleaseInfo &latest = m_releases.first();
  return latest.tag != inst.installedTag &&
         latest.publishedAt > inst.installedAt;
}

QString MainWindow::latestTagForInstance(const Instance &) const {
  if (m_releases.isEmpty())
    return QString();
  return m_releases.first().tag;
}
