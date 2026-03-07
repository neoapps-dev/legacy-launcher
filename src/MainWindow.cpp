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
  setMinimumSize(1000, 700);
  resize(1100, 750);

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
    QMainWindow {
        background-color: #2b2b2b;
    }
    QWidget#centralPanel {
        background-color: #1e1e1e;
    }
    
    /* Sidebar */
    QWidget#sidebar {
        background-color: #212121;
        border-right: 1px solid #111111;
    }
    QPushButton#sidebarBtn {
        text-align: left;
        padding: 12px 20px;
        background: transparent;
        color: #aaaaaa;
        border: none;
        font-weight: bold;
        font-size: 13px;
        border-left: 4px solid transparent;
    }
    QPushButton#sidebarBtn:hover {
        background-color: rgba(255, 255, 255, 0.05);
        color: #ffffff;
    }
    QPushButton#sidebarBtn:checked {
        background-color: rgba(255, 255, 255, 0.1);
        color: #ffffff;
        border-left: 4px solid #4ecdc4;
    }

    /* Top Nav (Tabs) */
    QWidget#topNav {
        background-color: #2b2b2b;
        border-bottom: 2px solid #111111;
    }
    QPushButton#tabBtn {
        background: transparent;
        border: none;
        padding: 15px 20px;
        color: #999999;
        font-size: 14px;
        font-weight: bold;
        border-bottom: 3px solid transparent;
    }
    QPushButton#tabBtn:hover {
        color: #ffffff;
        background-color: rgba(255,255,255,0.02);
    }
    QPushButton#tabBtn:checked {
        color: #ffffff;
        border-bottom: 3px solid #3c8527;
    }

    /* Play Tab */
    QWidget#playTabContent {
        background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #113311, stop:1 #1e1e1e);
    }
    QWidget#playBottomBar {
        background-color: rgba(30, 30, 30, 220);
        border-top: 1px solid #111111;
        border-bottom: 1px solid #111111;
    }
    QComboBox#instanceCombo {
        background-color: #333333;
        color: #ffffff;
        border: 1px solid #555555;
        padding: 8px 12px;
        font-size: 14px;
        font-weight: bold;
        border-radius: 2px;
    }
    QComboBox#instanceCombo::drop-down {
        border: 0px;
    }
    QPushButton#playBtn {
        background-color: #3c8527;
        color: white;
        font-size: 22px;
        font-weight: 800;
        letter-spacing: 2px;
        padding: 12px 60px;
        border: none;
        border-radius: 2px;
        box-shadow: 0 4px 6px rgba(0,0,0,0.3);
    }
    QPushButton#playBtn:hover {
        background-color: #4ba631;
    }
    QPushButton#playBtn:pressed {
        background-color: #316c20;
    }
    QPushButton#playBtn:disabled {
        background-color: #555555;
        color: #888888;
    }
    QPushButton#playBtnStop {
        background-color: #aa0000;
        color: white;
        font-size: 22px;
        font-weight: 800;
        letter-spacing: 2px;
        padding: 12px 60px;
        border: none;
        border-radius: 2px;
    }
    QPushButton#playBtnStop:hover {
        background-color: #cc0000;
    }

    /* Installations Tab */
    QScrollArea#installationsScroll {
        background: transparent;
        border: none;
    }
    QWidget#scrollContents {
        background: transparent;
    }
    QPushButton#addInstanceBtn {
        background-color: #333333;
        color: white;
        border: 1px solid #555555;
        padding: 8px 16px;
        font-weight: bold;
        border-radius: 2px;
    }
    QPushButton#addInstanceBtn:hover {
        background-color: #444444;
        border-color: #777777;
    }
    QPushButton#refreshBtn {
        background-color: transparent;
        color: #aaaaaa;
        border: 1px solid #444444;
        padding: 8px 16px;
        font-weight: bold;
        border-radius: 2px;
    }
    QPushButton#refreshBtn:hover {
        background-color: rgba(255,255,255,0.05);
        color: white;
    }

    /* InstanceWidget Card Styling */
    QFrame#instanceCard {
        background-color: #333333;
        border-radius: 4px;
        border: 1px solid #111111;
    }
    QFrame#instanceCard:hover {
        background-color: #3a3a3a;
    }
    QPushButton#launchBtn, QPushButton#settingsBtn, QPushButton#updateBtn, QPushButton#deleteBtn {
        background-color: transparent;
        border: 1px solid #555555;
        color: #ffffff;
        padding: 6px 12px;
        border-radius: 2px;
    }
    QPushButton#launchBtn:hover, QPushButton#settingsBtn:hover, QPushButton#updateBtn:hover, QPushButton#deleteBtn:hover {
        background-color: #555555;
    }
    QPushButton#launchBtn {
        background-color: #3c8527;
        border: none;
        font-weight: bold;
    }
    QPushButton#launchBtn:hover {
        background-color: #4ba631;
    }
    QLabel#statusBadge {
        background-color: #2b6a9e;
        color: white;
        border-radius: 3px;
        padding: 2px 6px;
        font-weight: bold;
        font-size: 11px;
    }
    QLabel { color: #dddddd; }
    QLabel#statusBadge { color: white; }
  )";
  qApp->setStyleSheet(css);
}

void MainWindow::setupUi() {
  m_centralWidget = new QWidget(this);
  m_centralWidget->setObjectName("centralPanel");
  setCentralWidget(m_centralWidget);

  QHBoxLayout *mainLayout = new QHBoxLayout(m_centralWidget);
  mainLayout->setContentsMargins(0, 0, 0, 0);
  mainLayout->setSpacing(0);

  // Sidebar Layout
  QWidget *sidebar = new QWidget();
  sidebar->setObjectName("sidebar");
  sidebar->setFixedWidth(240);
  QVBoxLayout *sidebarLayout = new QVBoxLayout(sidebar);
  sidebarLayout->setContentsMargins(0, 0, 0, 0);
  sidebarLayout->setSpacing(0);

  // Top profile section in sidebar
  QString username = qEnvironmentVariable("USER");
  if (username.isEmpty()) username = qEnvironmentVariable("USERNAME");
  if (username.isEmpty()) username = tr("Player");
  
  QLabel *profileLabel = new QLabel(tr("\n  %1\n  Local Account").arg(username));
  profileLabel->setStyleSheet("color: #cccccc; font-size: 11px; padding: 15px; border-bottom: 1px solid #111;");
  sidebarLayout->addWidget(profileLabel);

  // Navigation Links in Sidebar
  m_sidebarGroup = new QButtonGroup(this);
  m_sidebarGroup->setExclusive(true);

  QPushButton *btnNews = new QPushButton(tr("News"));
  btnNews->setObjectName("sidebarBtn");
  btnNews->setCheckable(true);
  
  QPushButton *btnLegacy = new QPushButton(tr("Minecraft: Legacy Console"));
  btnLegacy->setObjectName("sidebarBtn");
  btnLegacy->setCheckable(true);
  btnLegacy->setChecked(true);

  m_sidebarGroup->addButton(btnNews, 0);
  m_sidebarGroup->addButton(btnLegacy, 1);
  sidebarLayout->addSpacing(10);
  sidebarLayout->addWidget(btnNews);
  sidebarLayout->addWidget(btnLegacy);
  sidebarLayout->addStretch();
  
  QLabel *versionLabel = new QLabel(tr("Settings"));
  versionLabel->setStyleSheet("color: #666666; font-size: 11px; padding: 15px;");
  sidebarLayout->addWidget(versionLabel);

  mainLayout->addWidget(sidebar);

  QWidget *rightArea = new QWidget();
  QVBoxLayout *rightLayout = new QVBoxLayout(rightArea);
  rightLayout->setContentsMargins(0, 0, 0, 0);
  rightLayout->setSpacing(0);

  // Nav Header
  QWidget *topNav = new QWidget();
  topNav->setObjectName("topNav");
  topNav->setFixedHeight(55);
  QHBoxLayout *topNavLayout = new QHBoxLayout(topNav);
  topNavLayout->setContentsMargins(0, 0, 0, 0);
  topNavLayout->setSpacing(0);

  QLabel *gameTitle = new QLabel(tr(" MINECRAFT LEGACY CONSOLE "));
  gameTitle->setStyleSheet("color: white; font-weight: 900; font-size: 13px; letter-spacing: 1px; padding-left: 20px; padding-right: 20px;");
  topNavLayout->addWidget(gameTitle);

  m_tabGroup = new QButtonGroup(this);
  m_tabGroup->setExclusive(true);

  QPushButton *tabPlay = new QPushButton(tr("Play"));
  tabPlay->setObjectName("tabBtn");
  tabPlay->setCheckable(true);
  tabPlay->setChecked(true);
  m_tabGroup->addButton(tabPlay, 0);
  topNavLayout->addWidget(tabPlay);

  QPushButton *tabInstallations = new QPushButton(tr("Installations"));
  tabInstallations->setObjectName("tabBtn");
  tabInstallations->setCheckable(true);
  m_tabGroup->addButton(tabInstallations, 1);
  topNavLayout->addWidget(tabInstallations);

  QPushButton *tabSkins = new QPushButton(tr("Skins"));
  tabSkins->setObjectName("tabBtn");
  tabSkins->setCheckable(true);
  m_tabGroup->addButton(tabSkins, 2);
  topNavLayout->addWidget(tabSkins);
  
  QPushButton *tabPatch = new QPushButton(tr("Patch Notes"));
  tabPatch->setObjectName("tabBtn");
  tabPatch->setCheckable(true);
  m_tabGroup->addButton(tabPatch, 3);
  topNavLayout->addWidget(tabPatch);
  
  topNavLayout->addStretch();
  rightLayout->addWidget(topNav);

  // Stacked Widget covering the rest
  m_mainStack = new QStackedWidget();

  // --- PLAY TAB ---
  m_playTab = new QWidget();
  m_playTab->setObjectName("playTabContent");
  QVBoxLayout *playLayout = new QVBoxLayout(m_playTab);
  playLayout->setContentsMargins(0, 0, 0, 0);
  playLayout->addStretch();
  
  QWidget *playBottomBar = new QWidget();
  playBottomBar->setObjectName("playBottomBar");
  QHBoxLayout *bottomBarLayout = new QHBoxLayout(playBottomBar);
  bottomBarLayout->setContentsMargins(40, 20, 40, 20);
  
  m_instanceCombo = new QComboBox();
  m_instanceCombo->setObjectName("instanceCombo");
  m_instanceCombo->setFixedWidth(280);
  m_instanceCombo->setFixedHeight(45);
  bottomBarLayout->addWidget(m_instanceCombo);
  
  bottomBarLayout->addStretch();

  m_playBtn = new QPushButton(tr("PLAY"));
  m_playBtn->setObjectName("playBtn");
  m_playBtn->setCursor(Qt::PointingHandCursor);
  bottomBarLayout->addWidget(m_playBtn);
  
  bottomBarLayout->addStretch();

  m_playStatusLabel = new QLabel("");
  m_playStatusLabel->setStyleSheet("color: white; font-weight: bold; width: 280px; text-align: right;");
  bottomBarLayout->addWidget(m_playStatusLabel);
  
  playLayout->addWidget(playBottomBar);
  m_mainStack->addWidget(m_playTab);

  // --- INSTALLATIONS TAB ---
  m_installationsTab = new QWidget();
  QVBoxLayout *instLayout = new QVBoxLayout(m_installationsTab);
  instLayout->setContentsMargins(40, 30, 40, 30);
  
  QHBoxLayout *instToolbar = new QHBoxLayout();
  m_addBtn = new QPushButton(tr("New Installation"));
  m_addBtn->setObjectName("addInstanceBtn");
  m_addBtn->setFocusPolicy(Qt::StrongFocus);
  instToolbar->addWidget(m_addBtn);
  
  instToolbar->addStretch();
  
  m_refreshBtn = new QPushButton(tr("Refresh Releases"));
  m_refreshBtn->setObjectName("refreshBtn");
  instToolbar->addWidget(m_refreshBtn);
  instLayout->addLayout(instToolbar);
  instLayout->addSpacing(20);
  
  m_scrollArea = new QScrollArea();
  m_scrollArea->setObjectName("installationsScroll");
  m_scrollArea->setWidgetResizable(true);
  m_scrollArea->setFocusPolicy(Qt::NoFocus);
  m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  
  QWidget *scrollContents = new QWidget();
  scrollContents->setObjectName("scrollContents");
  m_instanceListLayout = new QVBoxLayout(scrollContents);
  m_instanceListLayout->setAlignment(Qt::AlignTop);
  m_instanceListLayout->setSpacing(8);
  m_instanceListLayout->setContentsMargins(0, 0, 8, 0);
  
  m_emptyLabel = new QLabel(tr("No installations found.\nClick \"New Installation\" to get started."));
  m_emptyLabel->setAlignment(Qt::AlignCenter);
  m_emptyLabel->setStyleSheet("color: #888888; font-size: 14px;");
  m_instanceListLayout->addWidget(m_emptyLabel);
  m_scrollArea->setWidget(scrollContents);
  
  instLayout->addWidget(m_scrollArea, 1);
  m_mainStack->addWidget(m_installationsTab);
  
  // Empty dummy tabs for Skins/Patch Notes
  m_mainStack->addWidget(new QWidget()); // Skins
  m_mainStack->addWidget(new QWidget()); // Patch Notes

  rightLayout->addWidget(m_mainStack);
  mainLayout->addWidget(rightArea);

  // Connections
  connect(m_tabGroup, &QButtonGroup::idClicked, this, &MainWindow::onTabNavClicked);
  connect(m_playBtn, &QPushButton::clicked, this, &MainWindow::onPlayButtonClicked);
  connect(m_addBtn, &QPushButton::clicked, this, &MainWindow::onAddInstance);
  connect(m_refreshBtn, &QPushButton::clicked, this, &MainWindow::onRefreshReleases);
  connect(m_instanceCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), 
          this, [this](int){ updatePlayButtonState(); });
}

void MainWindow::onTabNavClicked(int id) {
  if (id < m_mainStack->count()) {
      m_mainStack->setCurrentIndex(id);
  }
}

void MainWindow::onPlayButtonClicked() {
    if (m_instanceCombo->count() == 0) return;
    QString id = m_instanceCombo->currentData().toString();
    
    if (m_launchManager->isRunning(id)) {
        onStopRequested(id);
    } else {
        onLaunchRequested(id);
    }
}

void MainWindow::updatePlayButtonState() {
  if (m_instanceCombo->count() == 0) {
    m_playBtn->setEnabled(false);
    m_playBtn->setText(tr("PLAY"));
    m_playBtn->setObjectName("playBtn");
    m_playBtn->style()->unpolish(m_playBtn);
    m_playBtn->style()->polish(m_playBtn);
    m_playStatusLabel->setText("");
    return;
  }
  
  QString id = m_instanceCombo->currentData().toString();
  if (m_launchManager->isRunning(id)) {
    m_playBtn->setText(tr("STOP"));
    m_playBtn->setObjectName("playBtnStop");
    m_playStatusLabel->setText(tr("Game is running..."));
  } else {
    m_playBtn->setText(tr("PLAY"));
    m_playBtn->setObjectName("playBtn");
    m_playStatusLabel->setText("");
  }
  m_playBtn->setEnabled(true);
  
  // Force stylesheet re-evaluation to swap styling
  m_playBtn->style()->unpolish(m_playBtn);
  m_playBtn->style()->polish(m_playBtn);
}


void MainWindow::rebuildInstanceList() {
  QString selectedId;
  if (m_instanceCombo->count() > 0) {
    selectedId = m_instanceCombo->currentData().toString();
  }

  for (InstanceWidget *w : m_instanceWidgets) {
    m_instanceListLayout->removeWidget(w);
    w->deleteLater();
  }
  m_instanceWidgets.clear();
  
  m_instanceCombo->blockSignals(true);
  m_instanceCombo->clear();

  QList<Instance> instances = m_instanceManager->instances();
  m_emptyLabel->setVisible(instances.isEmpty());

  for (const Instance &inst : instances) {
    // Play tab combo box
    m_instanceCombo->addItem(inst.name, inst.id);
    if (!inst.installedTag.isEmpty()) {
       m_instanceCombo->setItemData(m_instanceCombo->count() - 1, 
           inst.name + " (" + inst.installedTag + ")", Qt::ToolTipRole);
    }

    // Installations Tab widget
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

  m_instanceCombo->blockSignals(false);

  int idx = m_instanceCombo->findData(selectedId);
  if (idx >= 0) {
      m_instanceCombo->setCurrentIndex(idx);
  }

  updatePlayButtonState();
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
  updatePlayButtonState();
  statusBar()->showMessage(tr("Instance \"%1\" started.").arg(id));
}

void MainWindow::onInstanceStopped(const QString &id, int exitCode) {
  if (InstanceWidget *w = widgetForId(id))
    w->setRunning(false);
  updatePlayButtonState();
  statusBar()->showMessage(
      tr("Instance stopped (exit code %1).").arg(exitCode));
}

void MainWindow::onInstanceError(const QString &id, const QString &error) {
  if (InstanceWidget *w = widgetForId(id))
    w->setRunning(false);
  updatePlayButtonState();
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
