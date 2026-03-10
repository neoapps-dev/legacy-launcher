#include "SettingsDialog.h"
#include "WeaveLoaderReleaseTracker.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QDialogButtonBox>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QSpinBox>
#include <QPushButton>
#include <QScrollArea>

SettingsDialog::SettingsDialog(Instance instance, const QList<ProtonInstallation> &protons, QWidget *parent)
    : QWidget(parent)
    , m_instance(instance)
    , m_protons(protons)
    , m_weaveLoaderTracker(new WeaveLoaderReleaseTracker(this))
{
    setObjectName("settingsPage");
    setWindowTitle(tr("Edit installation"));
    setMinimumWidth(700);
    setupUi();

    connect(m_weaveLoaderTracker, &WeaveLoaderReleaseTracker::releasesUpdated, this, &SettingsDialog::onWeaveLoaderReleasesUpdated);
    connect(m_weaveLoaderTracker, &WeaveLoaderReleaseTracker::fetchError, this, &SettingsDialog::onWeaveLoaderFetchError);

    m_weaveLoaderTracker->fetchReleases();
}

void SettingsDialog::setupUi() {
    setStyleSheet(R"(
        QWidget#settingsPage {
            background-color: #313338;
        }
        QLabel {
            color: #b9bbbe;
            font-size: 12px;
            font-weight: bold;
            text-transform: uppercase;
        }
        QLabel#titleLabel {
            color: white;
            font-size: 20px;
            font-weight: 800;
            text-transform: none;
        }
        QLabel#infoLabel {
            text-transform: none;
            font-weight: normal;
            font-size: 13px;
        }
        QLineEdit, QComboBox, QSpinBox {
            background-color: #1e1f22;
            color: white;
            border: 1px solid #101113;
            border-radius: 4px;
            padding: 12px;
            font-size: 14px;
        }
        QLineEdit:hover, QComboBox:hover, QSpinBox:hover {
            border: 1px solid #4f545c;
        }
        QLineEdit:focus, QComboBox:focus, QSpinBox:focus {
            border: 1px solid #5865f2;
        }
        QComboBox::drop-down {
            border: none;
        }
        QGroupBox {
            color: #b9bbbe;
            font-weight: bold;
            border: 1px solid #4f545c;
            border-radius: 4px;
            margin-top: 1ex; /* leave space at the top for the title */
            padding-top: 15px;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            subcontrol-position: top left;
            padding: 0 3px;
        }
        QCheckBox {
            color: #b9bbbe;
            font-size: 14px;
            text-transform: none;
            font-weight: normal;
        }
        QCheckBox::indicator {
            width: 16px;
            height: 16px;
            border-radius: 3px;
            border: 1px solid #b9bbbe;
        }
        QCheckBox::indicator:checked {
            background-color: #3ba55d;
            border: none;
            image: url(:/packaging/icon.png); /* A placeholder for checkmark */
        }
        QPushButton {
            font-weight: bold;
            border-radius: 3px;
        }
        QPushButton#saveBtn {
            background-color: #2e8b57;
            color: white;
            padding: 10px 24px;
            border: none;
        }
        QPushButton#saveBtn:hover {
            background-color: #3cb371;
        }
        QPushButton#cancelBtn {
            background-color: transparent;
            color: white;
            padding: 10px 24px;
            border: 1px solid #4f545c;
        }
        QPushButton#cancelBtn:hover {
            background-color: rgba(255, 255, 255, 0.05);
        }
        QPushButton#closeBtn {
            background-color: transparent;
            color: #b9bbbe;
            border: 1px solid #4f545c;
            border-radius: 16px;
            font-size: 16px;
        }
        QPushButton#closeBtn:hover {
            background-color: #4f545c;
            color: white;
        }
    )");

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    QWidget *headerWidget = new QWidget();
    QHBoxLayout *headerLayout = new QHBoxLayout(headerWidget);
    headerLayout->setContentsMargins(40, 40, 40, 10);
    
    headerLayout->addStretch();
    QLabel *titleLabel = new QLabel(tr("Edit installation"));
    titleLabel->setObjectName("titleLabel");
    titleLabel->setAlignment(Qt::AlignCenter);
    headerLayout->addWidget(titleLabel);
    headerLayout->addStretch();
    
    QPushButton *closeBtn = new QPushButton("✕");
    closeBtn->setObjectName("closeBtn");
    closeBtn->setFixedSize(32, 32);
    connect(closeBtn, &QPushButton::clicked, this, [this]{ emit finished(false); });
    headerLayout->addWidget(closeBtn);
    
    mainLayout->addWidget(headerWidget);

    QScrollArea *scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setStyleSheet("QScrollArea { border: none; background-color: transparent; } QScrollBar { background: #2b2d31; width: 8px; } QScrollBar::handle { background: #1e1f22; border-radius: 4px; }");

    QWidget *contentWidget = new QWidget();
    contentWidget->setStyleSheet("background-color: transparent;");
    QVBoxLayout *contentLayout = new QVBoxLayout(contentWidget);
    contentLayout->setContentsMargins(100, 20, 100, 40);
    contentLayout->setSpacing(24);

    QLabel *iconLabel = new QLabel();
    iconLabel->setPixmap(QPixmap(":/packaging/icon.png").scaled(64, 64, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    iconLabel->setAlignment(Qt::AlignCenter);
    contentLayout->addWidget(iconLabel);
    contentLayout->addSpacing(10);

    QVBoxLayout *nameLayout = new QVBoxLayout();
    nameLayout->setSpacing(8);
    QLabel *nameLbl = new QLabel(tr("NAME"));
    m_nameEdit = new QLineEdit(m_instance.name);
    nameLayout->addWidget(nameLbl);
    nameLayout->addWidget(m_nameEdit);
    contentLayout->addLayout(nameLayout);

    QVBoxLayout *userLayout = new QVBoxLayout();
    userLayout->setSpacing(8);
    QLabel *userLbl = new QLabel(tr("USERNAME"));
    m_usernameEdit = new QLineEdit(m_instance.username);
    m_usernameEdit->setPlaceholderText(tr("Player"));
    userLayout->addWidget(userLbl);
    userLayout->addWidget(m_usernameEdit);
    contentLayout->addLayout(userLayout);

    QVBoxLayout *protonLayout = new QVBoxLayout();
    protonLayout->setSpacing(8);
    QLabel *protonLbl = new QLabel(tr("PROTON"));
    m_protonCombo = new QComboBox();
    int selectedProton = 0;
    for (int i = 0; i < m_protons.size(); ++i) {
        m_protonCombo->addItem(m_protons[i].name, m_protons[i].path);
        if (m_protons[i].path == m_instance.protonId) selectedProton = i;
    }
    if (m_protons.isEmpty()) {
        m_protonCombo->addItem(tr("No Proton found"));
        m_protonCombo->setEnabled(false);
    } else {
        m_protonCombo->setCurrentIndex(selectedProton);
    }
    protonLayout->addWidget(protonLbl);
    protonLayout->addWidget(m_protonCombo);
    contentLayout->addLayout(protonLayout);

    QVBoxLayout *headlessLayout = new QVBoxLayout();
    headlessLayout->setSpacing(8);
    QLabel *headlessLbl = new QLabel(tr("HEADLESS / SERVER MODE"));
    headlessLayout->addWidget(headlessLbl);
    m_headlessCheck = new QCheckBox(tr("Enable headless / server mode (-server)"));
    m_headlessCheck->setChecked(m_instance.headlessMode);
    m_headlessCheck->setStyleSheet("QCheckBox { color: white; margin-top: 5px; }");
    headlessLayout->addWidget(m_headlessCheck);
    contentLayout->addLayout(headlessLayout);

    QVBoxLayout *ipLayout = new QVBoxLayout();
    ipLayout->setSpacing(8);
    QLabel *ipLbl = new QLabel(tr("AUTO-CONNECT IP"));
    m_ipEdit = new QLineEdit(m_instance.autoIp);
    m_ipEdit->setPlaceholderText(tr("Leave blank to disable auto-connect"));
    ipLayout->addWidget(ipLbl);
    ipLayout->addWidget(m_ipEdit);
    contentLayout->addLayout(ipLayout);

    QVBoxLayout *portLayout = new QVBoxLayout();
    portLayout->setSpacing(8);
    QLabel *portLbl = new QLabel(tr("AUTO-CONNECT PORT"));
    m_portSpin = new QSpinBox();
    m_portSpin->setRange(0, 65535);
    m_portSpin->setValue(m_instance.autoPort);
    m_portSpin->setSpecialValueText(tr("None"));
    portLayout->addWidget(portLbl);
    portLayout->addWidget(m_portSpin);
    contentLayout->addLayout(portLayout);

    QVBoxLayout *weaveLayoutSection = new QVBoxLayout();
    weaveLayoutSection->setSpacing(8);
    QLabel *weaveLbl = new QLabel(tr("WEAVE LOADER"));
    weaveLayoutSection->addWidget(weaveLbl);

    m_weaveLoaderCheck = new QCheckBox(tr("Enable Weave Loader"));
    m_weaveLoaderCheck->setChecked(m_instance.weaveLoaderEnabled);
    m_weaveLoaderCheck->setStyleSheet("QCheckBox { color: white; margin-top: 5px; }");
    weaveLayoutSection->addWidget(m_weaveLoaderCheck);

    m_weaveLoaderCombo = new QComboBox();
    weaveLayoutSection->addWidget(m_weaveLoaderCombo);

    m_weaveLoaderStatusLabel = new QLabel(tr("Fetching versions..."));
    m_weaveLoaderStatusLabel->setStyleSheet("color: #b9bbbe; text-transform: none; font-weight: normal;");
    weaveLayoutSection->addWidget(m_weaveLoaderStatusLabel);
    contentLayout->addLayout(weaveLayoutSection);

    connect(m_weaveLoaderCheck, &QCheckBox::stateChanged, this, &SettingsDialog::onWeaveLoaderCheckChanged);

    QLabel *pathLabel = new QLabel(tr("Install path: %1").arg(m_instance.installPath));
    pathLabel->setObjectName("infoLabel");
    pathLabel->setWordWrap(true);
    contentLayout->addWidget(pathLabel);

    contentLayout->addStretch();
    scrollArea->setWidget(contentWidget);
    mainLayout->addWidget(scrollArea);

    QWidget *footerWidget = new QWidget();
    footerWidget->setStyleSheet("background-color: transparent; border-top: 1px solid rgba(255, 255, 255, 0.05);");
    QHBoxLayout *btnLayout = new QHBoxLayout(footerWidget);
    btnLayout->setContentsMargins(60, 20, 60, 30);
    
    QPushButton *cancelBtn = new QPushButton(tr("Cancel"));
    cancelBtn->setObjectName("cancelBtn");
    QPushButton *saveBtn = new QPushButton(tr("Save"));
    saveBtn->setObjectName("saveBtn");
    saveBtn->setDefault(true);
    
    btnLayout->addStretch();
    btnLayout->addWidget(cancelBtn);
    btnLayout->addWidget(saveBtn);
    mainLayout->addWidget(footerWidget);

    connect(saveBtn, &QPushButton::clicked, this, &SettingsDialog::onAccept);
    connect(cancelBtn, &QPushButton::clicked, this, [this]{ emit finished(false); });
}

void SettingsDialog::onAccept() {
    m_instance.name = m_nameEdit->text().trimmed();
    m_instance.username = m_usernameEdit->text().trimmed();
    m_instance.headlessMode = m_headlessCheck->isChecked();
    m_instance.autoIp = m_ipEdit->text().trimmed();
    m_instance.autoPort = m_portSpin->value();

    int idx = m_protonCombo->currentIndex();
    if (idx >= 0 && idx < m_protons.size()) {
        m_instance.protonId = m_protons[idx].path;
    }
    bool wasEnabled = m_instance.weaveLoaderEnabled;
    m_instance.weaveLoaderEnabled = m_weaveLoaderCheck->isChecked();

    if (m_instance.weaveLoaderEnabled && m_weaveLoaderCombo->currentIndex() >= 0) {
        int wlIdx = m_weaveLoaderCombo->currentIndex();
        if (wlIdx >= 0 && wlIdx < m_weaveLoaderReleases.size()) {
            QString newTag = m_weaveLoaderReleases[wlIdx].tag;
            if (newTag != m_instance.weaveLoaderTag) {
                m_instance.weaveLoaderTag = newTag;
                m_instance.weaveLoaderInstalledAt = QDateTime::currentDateTime();
            }
        }
    } else {
        m_instance.weaveLoaderEnabled = false;
        m_instance.weaveLoaderTag = "";
        m_instance.weaveLoaderInstalledAt = QDateTime();
    }
    emit finished(true);
}

void SettingsDialog::onWeaveLoaderReleasesUpdated(QList<ReleaseInfo> releases) {
    m_weaveLoaderReleases = releases;
    m_weaveLoaderCombo->clear();

    for (const ReleaseInfo &r : releases) {
        QString label = r.tag + " — " + r.publishedAt.toLocalTime().toString("yyyy-MM-dd HH:mm");
        if (r.tag == m_instance.weaveLoaderTag) {
            label += tr(" (current)");
        }
        m_weaveLoaderCombo->addItem(label, r.tag);
    }

    if (!m_instance.weaveLoaderTag.isEmpty()) {
        for (int i = 0; i < m_weaveLoaderReleases.size(); ++i) {
            if (m_weaveLoaderReleases[i].tag == m_instance.weaveLoaderTag) {
                m_weaveLoaderCombo->setCurrentIndex(i);
                break;
            }
        }
    }

    m_weaveLoaderCombo->setEnabled(!releases.isEmpty());
    m_weaveLoaderStatusLabel->setText(releases.isEmpty() ? tr("No versions found") : tr("%1 versions available").arg(releases.size()));
}

void SettingsDialog::onWeaveLoaderFetchError(QString msg) {
    m_weaveLoaderStatusLabel->setText(tr("Error fetching versions"));
    qWarning() << "WeaveLoader fetch error:" << msg;
}

void SettingsDialog::onWeaveLoaderCheckChanged(int state) {
    m_weaveLoaderCombo->setEnabled(state == Qt::Checked && !m_weaveLoaderReleases.isEmpty());
}

Instance SettingsDialog::updatedInstance() const {
    return m_instance;
}
