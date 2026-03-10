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
#include <QStackedLayout>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>
#include <QTimer>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), m_instanceManager(new InstanceManager(this)),
      m_launchManager(new LaunchManager(this)),
      m_releaseTracker(new GitHubReleaseTracker(this)),
      m_isTabAnimating(false) {
  setWindowTitle(tr("Legacy Launcher"));
  setMinimumSize(1000, 600);

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
  QFile file(":/assets/main.qss");
  if (file.open(QFile::ReadOnly | QFile::Text)) {
    QTextStream ts(&file);
    QString style = ts.readAll();
    qApp->setStyleSheet(style);
  }
}


void MainWindow::setupUi() {
  m_rootStack = new QStackedWidget(this);
  m_rootStack->setObjectName("rootStack");
  setCentralWidget(m_rootStack);

  QWidget *launcherPage = new QWidget(this);
  launcherPage->setObjectName("launcherPage");
  
  QHBoxLayout *mainLayout = new QHBoxLayout(launcherPage);
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
  
  QWidget *userCard = new QWidget();
  userCard->setObjectName("userCard");
  QHBoxLayout *userCardLayout = new QHBoxLayout(userCard);
  userCardLayout->setContentsMargins(20, 25, 20, 20);
  userCardLayout->setSpacing(12);

  QLabel *avatarLabel = new QLabel();
  avatarLabel->setPixmap(QPixmap(":/assets/icon_profile.jpg").scaled(40, 40, Qt::KeepAspectRatio, Qt::SmoothTransformation));
  userCardLayout->addWidget(avatarLabel);

  QVBoxLayout *userInfoLayout = new QVBoxLayout();
  userInfoLayout->setSpacing(2);
  
  QHBoxLayout *nameRow = new QHBoxLayout();
  nameRow->setContentsMargins(0, 0, 0, 0);
  nameRow->setSpacing(5);
  QLabel *nameLabel = new QLabel(username);
  nameLabel->setStyleSheet("color: white; font-weight: bold; font-size: 14px;");
  nameRow->addWidget(nameLabel);
  
  nameRow->addStretch();
  
  userInfoLayout->addLayout(nameRow);

  QLabel *accountLabel = new QLabel(tr("Local account"));
  accountLabel->setStyleSheet("color: #aaaaaa; font-size: 12px;");
  userInfoLayout->addWidget(accountLabel);

  userCardLayout->addLayout(userInfoLayout);
  userCardLayout->addStretch();

  sidebarLayout->addWidget(userCard);

  // Navigation Links in Sidebar
  m_sidebarGroup = new QButtonGroup(this);
  m_sidebarGroup->setExclusive(true);
  
  QPushButton *btnLegacy = new QPushButton();
  btnLegacy->setObjectName("sidebarBtn");
  btnLegacy->setCheckable(true);
  btnLegacy->setChecked(true);
  btnLegacy->setFixedHeight(54);
  
  QHBoxLayout *btnLegLayout = new QHBoxLayout(btnLegacy);
  btnLegLayout->setContentsMargins(15, 5, 5, 5);
  btnLegLayout->setSpacing(12);
  
  QLabel *legIcon = new QLabel();
  legIcon->setPixmap(QPixmap(":/packaging/icon.png").scaled(32, 32, Qt::KeepAspectRatio, Qt::SmoothTransformation));
  legIcon->setAttribute(Qt::WA_TransparentForMouseEvents);
  btnLegLayout->addWidget(legIcon);
  
  QVBoxLayout *legTextLayout = new QVBoxLayout();
  legTextLayout->setContentsMargins(0, 4, 0, 4);
  legTextLayout->setSpacing(0);
  QLabel *lblTop = new QLabel("MINECRAFT");
  lblTop->setStyleSheet("color: #aaaaaa; font-size: 10px; font-weight: bold; background: transparent;");
  lblTop->setAttribute(Qt::WA_TransparentForMouseEvents);
  QLabel *lblBottom = new QLabel(tr("Legacy Console"));
  lblBottom->setStyleSheet("color: white; font-size: 14px; font-weight: bold; background: transparent;");
  lblBottom->setAttribute(Qt::WA_TransparentForMouseEvents);
  legTextLayout->addWidget(lblTop);
  legTextLayout->addWidget(lblBottom);
  
  btnLegLayout->addLayout(legTextLayout);
  btnLegLayout->addStretch();
  
  m_sidebarGroup->addButton(btnLegacy, 1);
  sidebarLayout->addSpacing(10);
  sidebarLayout->addWidget(btnLegacy);
  sidebarLayout->addStretch();
  
  QPushButton *btnSettings = new QPushButton(tr("  Settings"));
  btnSettings->setObjectName("sidebarBtn");
  btnSettings->setCheckable(true);
  sidebarLayout->addWidget(btnSettings);
  m_sidebarGroup->addButton(btnSettings, 2);
  sidebarLayout->addSpacing(10);

  m_sidebarIndicator = new QWidget(sidebar);
  m_sidebarIndicator->setFixedWidth(4);
  m_sidebarIndicator->setFixedHeight(48);
  m_sidebarIndicator->setStyleSheet("background-color: #3BA55D; border-top-right-radius: 2px; border-bottom-right-radius: 2px; border: none;");
  m_sidebarIndicator->show();
  m_sidebarIndicator->raise();

  m_sidebarIndicatorAnim = new QPropertyAnimation(m_sidebarIndicator, "pos", this);
  m_sidebarIndicatorAnim->setDuration(250);
  m_sidebarIndicatorAnim->setEasingCurve(QEasingCurve::InOutQuad);

  QTimer::singleShot(100, this, [this]() {
      if (m_sidebarGroup && m_sidebarGroup->checkedButton()) {
          m_sidebarIndicator->move(0, m_sidebarGroup->checkedButton()->pos().y());
      }
  });

  connect(m_sidebarGroup, &QButtonGroup::buttonClicked, this, [this](QAbstractButton *button) {
      if (!m_sidebarIndicator) return;
      m_sidebarIndicatorAnim->stop();
      m_sidebarIndicatorAnim->setEndValue(QPoint(0, button->pos().y()));
      m_sidebarIndicatorAnim->start();
  });

  mainLayout->addWidget(sidebar);

  QWidget *rightArea = new QWidget();
  QVBoxLayout *rightLayout = new QVBoxLayout(rightArea);
  rightLayout->setContentsMargins(0, 0, 0, 0);
  rightLayout->setSpacing(0);

  // Nav Header
  QWidget *topNav = new QWidget();
  topNav->setObjectName("topNav");
  topNav->setFixedHeight(90);
  QVBoxLayout *topNavLayout = new QVBoxLayout(topNav);
  topNavLayout->setContentsMargins(20, 15, 20, 0);
  topNavLayout->setSpacing(10);

  QLabel *gameTitle = new QLabel(tr("MINECRAFT: LEGACY CONSOLE"));
  gameTitle->setStyleSheet("color: white; font-weight: 800; font-size: 13px; letter-spacing: 0.5px;");
  topNavLayout->addWidget(gameTitle);

  m_tabGroup = new QButtonGroup(this);
  m_tabGroup->setExclusive(true);

  QHBoxLayout *tabsLayout = new QHBoxLayout();
  tabsLayout->setSpacing(0);

  QPushButton *tabPlay = new QPushButton(tr("Play"));
  tabPlay->setObjectName("tabBtn");
  tabPlay->setCheckable(true);
  tabPlay->setChecked(true);
  m_tabGroup->addButton(tabPlay, 0);
  tabsLayout->addWidget(tabPlay);

  QPushButton *tabInstallations = new QPushButton(tr("Installations"));
  tabInstallations->setObjectName("tabBtn");
  tabInstallations->setCheckable(true);
  m_tabGroup->addButton(tabInstallations, 1);
  tabsLayout->addWidget(tabInstallations);

  QPushButton *tabPatch = new QPushButton(tr("Patch Notes"));
  tabPatch->setObjectName("tabBtn");
  tabPatch->setCheckable(true);
  m_tabGroup->addButton(tabPatch, 2);
  tabsLayout->addWidget(tabPatch);
  
  tabsLayout->addStretch();
  topNavLayout->addLayout(tabsLayout);
  
  rightLayout->addWidget(topNav);

  m_mainStack = new QStackedWidget();
  
  // --- PLAY TAB ---
  m_playTab = new QWidget();
  m_playTab->setObjectName("playTabContent");
  m_playTab->installEventFilter(this);

  QLabel *bgLabel = new QLabel(m_playTab);
  bgLabel->setObjectName("bgLabel");
  bgLabel->lower();
  bgLabel->show();

  QVBoxLayout *playLayout = new QVBoxLayout(m_playTab);
  playLayout->setContentsMargins(0, 0, 0, 0);
  
  QHBoxLayout *logoLayout = new QHBoxLayout();
  logoLayout->setContentsMargins(0, 40, 0, 0);
  QLabel *tabLogo = new QLabel();
  tabLogo->setPixmap(QPixmap(":/assets/logo.png").scaled(300, 112, Qt::KeepAspectRatio, Qt::SmoothTransformation));
  logoLayout->addStretch();
  logoLayout->addWidget(tabLogo);
  logoLayout->addStretch();
  playLayout->addLayout(logoLayout);

  playLayout->addStretch();
  
  QWidget *playBottomBar = new QWidget();
  playBottomBar->setObjectName("playBottomBar");
  playBottomBar->setFixedHeight(105);
  
  QGridLayout *playBarGrid = new QGridLayout(playBottomBar);
  playBarGrid->setContentsMargins(0, 0, 0, 0);

  QWidget *bgContainer = new QWidget();
  QVBoxLayout *bgLayout = new QVBoxLayout(bgContainer);
  bgLayout->setContentsMargins(0, 0, 0, 0);
  bgLayout->addStretch();
  
  QFrame *innerBarBg = new QFrame();
  innerBarBg->setObjectName("playBottomBarInner");
  innerBarBg->setFixedHeight(75);
  
  QHBoxLayout *bottomBarLayout = new QHBoxLayout(innerBarBg);
  bottomBarLayout->setContentsMargins(20, 0, 20, 0);
  
  m_instanceSelector = new QFrame();
  m_instanceSelector->setObjectName("instanceSelector");
  m_instanceSelector->setCursor(Qt::PointingHandCursor);
  m_instanceSelector->setFixedSize(280, 64);
  m_instanceSelector->installEventFilter(this);

  m_instanceCombo = new QComboBox(m_instanceSelector);
  m_instanceCombo->setObjectName("instanceCombo");
  m_instanceCombo->setGeometry(0, 0, 280, 64);
  m_instanceCombo->setFocusPolicy(Qt::NoFocus);
  
  QHBoxLayout *vLayout = new QHBoxLayout(m_instanceSelector);
  vLayout->setContentsMargins(12, 0, 12, 0);
  vLayout->setSpacing(12);
  
  QLabel *vIcon = new QLabel();
  vIcon->setPixmap(QPixmap(":/packaging/icon.png").scaled(32, 32, Qt::KeepAspectRatio, Qt::SmoothTransformation));
  vIcon->setAttribute(Qt::WA_TransparentForMouseEvents);
  vLayout->addWidget(vIcon);
  
  QVBoxLayout *vTextLayout = new QVBoxLayout();
  vTextLayout->setContentsMargins(0, 5, 0, 5);
  vTextLayout->setSpacing(2);
  
  QLabel *vTitle = new QLabel("");
  vTitle->setObjectName("selectedInstTitle");
  vTitle->setStyleSheet("color: white; font-weight: bold; font-size: 15px;");
  vTitle->setAttribute(Qt::WA_TransparentForMouseEvents);
  vTextLayout->addWidget(vTitle);
  
  QLabel *vText = new QLabel("");
  vText->setObjectName("selectedInstText");
  vText->setStyleSheet("color: #aaaaaa; font-size: 13px;");
  vText->setAttribute(Qt::WA_TransparentForMouseEvents);
  vTextLayout->addWidget(vText);
  
  vLayout->addLayout(vTextLayout);
  
  QLabel *vChevron = new QLabel("▾");
  vChevron->setStyleSheet("color: #aaaaaa; font-size: 14px;");
  vChevron->setAttribute(Qt::WA_TransparentForMouseEvents);
  vLayout->addWidget(vChevron);
  
  vLayout->addStretch();
  vLayout->addWidget(vChevron);
  
  bottomBarLayout->addWidget(m_instanceSelector, 0, Qt::AlignVCenter);
  bottomBarLayout->addStretch(1);

  m_playStatusLabel = new QLabel(username);
  m_playStatusLabel->setObjectName("playStatusLabel");
  m_playStatusLabel->setMinimumWidth(100);
  m_playStatusLabel->setMaximumWidth(260);
  m_playStatusLabel->setFixedHeight(60);
  m_playStatusLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
  bottomBarLayout->addWidget(m_playStatusLabel, 0, Qt::AlignRight);
  
  bgLayout->addWidget(innerBarBg);
  playBarGrid->addWidget(bgContainer, 0, 0);

  QVBoxLayout *btnLayout = new QVBoxLayout();
  btnLayout->setContentsMargins(0, 0, 0, 15);
  btnLayout->addStretch();
  QHBoxLayout *btnHLayout = new QHBoxLayout();
  btnHLayout->addStretch(1);
  m_playBtn = new QPushButton(tr("PLAY"));
  m_playBtn->setObjectName("playBtn");
  m_playBtn->setCursor(Qt::PointingHandCursor);
  m_playBtn->setFixedHeight(75);
  btnHLayout->addWidget(m_playBtn);
  btnHLayout->addStretch(1);
  btnLayout->addLayout(btnHLayout);
  
  playBarGrid->addLayout(btnLayout, 0, 0);
  
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
  m_scrollArea->setFrameShape(QFrame::NoFrame);
  m_scrollArea->setObjectName("installationsScrollArea");
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
  m_emptyLabel->setObjectName("emptyListLabel");
  m_instanceListLayout->addWidget(m_emptyLabel);
  m_scrollArea->setWidget(scrollContents);
  
  instLayout->addWidget(m_scrollArea, 1);
  m_mainStack->addWidget(m_installationsTab);
  
  // --- PATCH NOTES TAB ---
  m_patchTab = new QWidget();
  QVBoxLayout *patchLayout = new QVBoxLayout(m_patchTab);
  patchLayout->setContentsMargins(40, 30, 40, 30);
  
  m_patchNotesBox = new QTextBrowser();
  m_patchNotesBox->setOpenExternalLinks(true);
  m_patchNotesBox->setObjectName("patchNotesBox");
  patchLayout->addWidget(m_patchNotesBox);
  
  m_mainStack->addWidget(m_patchTab);

  rightLayout->addWidget(m_mainStack);
  mainLayout->addWidget(rightArea);

  m_rootStack->addWidget(launcherPage);

  // Connections
  connect(m_tabGroup, &QButtonGroup::idClicked, this, &MainWindow::onTabNavClicked);
  connect(m_playBtn, &QPushButton::clicked, this, &MainWindow::onPlayButtonClicked);
  connect(m_addBtn, &QPushButton::clicked, this, &MainWindow::onAddInstance);
  connect(m_refreshBtn, &QPushButton::clicked, this, &MainWindow::onRefreshReleases);
  connect(m_instanceCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), 
          this, [this](int){ updatePlayButtonState(); });
}

void MainWindow::onTabNavClicked(int id) {
  if (m_mainStack->currentIndex() == id || m_isTabAnimating) return;
  m_isTabAnimating = true;

  QPixmap snapshot = m_mainStack->grab();
  
  QLabel *overlay = new QLabel(m_mainStack->parentWidget());
  overlay->setPixmap(snapshot);
  overlay->setGeometry(m_mainStack->geometry());
  overlay->show();
  overlay->raise();

  m_mainStack->setCurrentIndex(id);

  QGraphicsOpacityEffect *eff = new QGraphicsOpacityEffect(overlay);
  overlay->setGraphicsEffect(eff);
  
  QPropertyAnimation *anim = new QPropertyAnimation(eff, "opacity");
  anim->setDuration(250);
  anim->setStartValue(1.0);
  anim->setEndValue(0.0);
  anim->setEasingCurve(QEasingCurve::InOutQuad);

  connect(anim, &QPropertyAnimation::finished, this, [=]() {
      overlay->deleteLater();
      m_isTabAnimating = false;
      anim->deleteLater();
  });
  
  anim->start();
}

void MainWindow::onPlayButtonClicked() {
    if (m_instanceCombo->count() == 0) return;

    QPropertyAnimation *a = new QPropertyAnimation(m_playBtn, "geometry");
    a->setDuration(100);
    QRect orig = m_playBtn->geometry();
    a->setEndValue(orig.adjusted(2, 2, -2, -2));
    a->setEasingCurve(QEasingCurve::OutQuad);
    connect(a, &QPropertyAnimation::finished, this, [this, orig]() {
        QPropertyAnimation *b = new QPropertyAnimation(m_playBtn, "geometry");
        b->setDuration(100);
        b->setStartValue(m_playBtn->geometry());
        b->setEndValue(orig);
        b->setEasingCurve(QEasingCurve::OutBack);
        connect(b, &QPropertyAnimation::finished, b, &QObject::deleteLater);
        b->start();
    });
    a->start();

    QString id = m_instanceCombo->currentData().toString();
    
    if (m_launchManager->isRunning(id)) {
        onStopRequested(id);
    } else {
        onLaunchRequested(id);
    }
}

void MainWindow::updatePlayButtonState() {
  QString username = qEnvironmentVariable("USER");
  if (username.isEmpty()) username = qEnvironmentVariable("USERNAME");
  if (username.isEmpty()) username = tr("Player");

  QLabel* instTitle = findChild<QLabel*>("selectedInstTitle");
  QLabel* instText = findChild<QLabel*>("selectedInstText");

  if (m_instanceCombo->count() == 0) {
    if(instTitle) instTitle->setText(tr("No instances"));
    if(instText) instText->setText("");

    m_playBtn->setEnabled(false);
    m_playBtn->setText(tr("PLAY"));
    m_playBtn->setObjectName("playBtn");
    m_playBtn->style()->unpolish(m_playBtn);
    m_playBtn->style()->polish(m_playBtn);
    m_playStatusLabel->setText("");
    return;
  }
  
  QString id = m_instanceCombo->currentData().toString();
  
  if (instTitle) instTitle->setText(m_instanceCombo->currentText());
  
  if (instText) {
      Instance *inst = m_instanceManager->findById(id);
      if (inst) {
          instText->setText(inst->installedTag.isEmpty() ? tr("Unknown version") : inst->installedTag);
          if (!inst->username.isEmpty()) {
              username = inst->username;
          }
      }
  }

  if (m_launchManager->isRunning(id)) {
    m_playBtn->setText(tr("STOP"));
    m_playBtn->setObjectName("playBtnStop");
    m_playStatusLabel->setText(tr("Playing as\n%1").arg(username));
  } else {
    m_playBtn->setText(tr("PLAY"));
    m_playBtn->setObjectName("playBtn");
    m_playStatusLabel->setText(username);
  }
  m_playBtn->setEnabled(true);
  
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
    m_instanceCombo->addItem(inst.name, inst.id);
    if (!inst.installedTag.isEmpty()) {
       m_instanceCombo->setItemData(m_instanceCombo->count() - 1, 
           inst.name + " (" + inst.installedTag + ")", Qt::ToolTipRole);
    }

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
  if (obj == m_playTab && event->type() == QEvent::Resize) {
      QLabel *bgLabel = m_playTab->findChild<QLabel*>("bgLabel");
      if (bgLabel && !m_playTab->size().isEmpty()) {
          QSize s = m_playTab->size();
          bgLabel->resize(s);
          QPixmap bg(":/assets/bg.jpg");
          QPixmap scaledBg = bg.scaled(s, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
          
          int x = (scaledBg.width() - s.width()) / 2;
          int y = (scaledBg.height() - s.height()) / 2;
          
          bgLabel->setPixmap(scaledBg.copy(x, y, s.width(), s.height()));
      }
  }

  if ((obj == m_instanceCombo || obj == m_instanceSelector) && 
      (event->type() == QEvent::MouseButtonPress || event->type() == QEvent::MouseButtonRelease)) {
      if (event->type() == QEvent::MouseButtonRelease) {
          m_instanceCombo->showPopup();
      }
      return true;
  }

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
  AddInstanceDialog *page = new AddInstanceDialog(m_protons, this);
  m_rootStack->addWidget(page);
  m_rootStack->setCurrentWidget(page);
  
  connect(page, &AddInstanceDialog::finished, this, [this, page](bool accepted) {
      if (accepted) {
          m_instanceManager->addInstance(page->createdInstance());
      }
      m_rootStack->removeWidget(page);
      page->deleteLater();
  });
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

  SettingsDialog *page = new SettingsDialog(*inst, m_protons, this);
  m_rootStack->addWidget(page);
  m_rootStack->setCurrentWidget(page);
  
  connect(page, &SettingsDialog::finished, this, [this, page](bool accepted) {
      if (accepted) {
          m_instanceManager->updateInstance(page->updatedInstance());
      }
      m_rootStack->removeWidget(page);
      page->deleteLater();
  });
}

void MainWindow::onUpdateRequested(const QString &id) {
  Instance *inst = m_instanceManager->findById(id);
  if (!inst)
    return;

  UpdateInstanceDialog *page = new UpdateInstanceDialog(*inst, m_releases, this);
  m_rootStack->addWidget(page);
  m_rootStack->setCurrentWidget(page);
  
  connect(page, &UpdateInstanceDialog::finished, this, [this, page, inst](bool accepted) {
      if (accepted) {
          m_instanceManager->updateInstance(page->updatedInstance());
          statusBar()->showMessage(tr("Updated \"%1\" to %2.")
                                       .arg(inst->name)
                                       .arg(page->updatedInstance().installedTag));
      }
      m_rootStack->removeWidget(page);
      page->deleteLater();
  });
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
                
    QString markdown;
    for (const ReleaseInfo &r : releases) {
        markdown += QString("## %1 (%2)\n").arg(r.name, r.tag);
        markdown += QString("*Published: %1*\n\n").arg(r.publishedAt.toLocalTime().toString("yyyy-MM-dd HH:mm"));
        markdown += r.body;
        markdown += "\n\n---\n\n";
    }
    m_patchNotesBox->setMarkdown(markdown);
  } else {
    statusBar()->showMessage(tr("No releases found."));
    m_patchNotesBox->setMarkdown("No patch notes found.");
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
