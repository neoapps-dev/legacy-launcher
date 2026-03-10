#include "SettingsDialog.h"
#include "WeaveLoaderReleaseTracker.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QDialogButtonBox>
#include <QLabel>

SettingsDialog::SettingsDialog(Instance instance, const QList<ProtonInstallation> &protons, QWidget *parent)
    : QDialog(parent)
    , m_instance(instance)
    , m_protons(protons)
    , m_weaveLoaderTracker(new WeaveLoaderReleaseTracker(this))
{
    setWindowTitle(tr("Instance Settings — %1").arg(instance.name));
    setMinimumWidth(500);
    setupUi();

    connect(m_weaveLoaderTracker, &WeaveLoaderReleaseTracker::releasesUpdated, this, &SettingsDialog::onWeaveLoaderReleasesUpdated);
    connect(m_weaveLoaderTracker, &WeaveLoaderReleaseTracker::fetchError, this, &SettingsDialog::onWeaveLoaderFetchError);

    m_weaveLoaderTracker->fetchReleases();
}

void SettingsDialog::setupUi() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    QFormLayout *form = new QFormLayout();

    m_nameEdit = new QLineEdit(m_instance.name);
    form->addRow(tr("Name:"), m_nameEdit);

    m_usernameEdit = new QLineEdit(m_instance.username);
    m_usernameEdit->setPlaceholderText(tr("Player"));
    form->addRow(tr("Username:"), m_usernameEdit);

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
    form->addRow(tr("Proton:"), m_protonCombo);

    mainLayout->addLayout(form);

    QGroupBox *serverGroup = new QGroupBox(tr("Server / Headless"));
    QVBoxLayout *serverLayout = new QVBoxLayout(serverGroup);
    QFormLayout *serverForm = new QFormLayout();

    m_headlessCheck = new QCheckBox(tr("Enable headless / server mode (-server)"));
    m_headlessCheck->setChecked(m_instance.headlessMode);
    serverLayout->addWidget(m_headlessCheck);

    m_ipEdit = new QLineEdit(m_instance.autoIp);
    m_ipEdit->setPlaceholderText(tr("Leave blank to disable auto-connect"));
    serverForm->addRow(tr("Auto-connect IP:"), m_ipEdit);

    m_portSpin = new QSpinBox();
    m_portSpin->setRange(0, 65535);
    m_portSpin->setValue(m_instance.autoPort);
    m_portSpin->setSpecialValueText(tr("None"));
    serverForm->addRow(tr("Auto-connect Port:"), m_portSpin);

    serverLayout->addLayout(serverForm);
    mainLayout->addWidget(serverGroup);

    QGroupBox *weaveGroup = new QGroupBox(tr("Weave Loader"));
    QVBoxLayout *weaveLayout = new QVBoxLayout(weaveGroup);

    m_weaveLoaderCheck = new QCheckBox(tr("Enable Weave Loader"));
    m_weaveLoaderCheck->setChecked(m_instance.weaveLoaderEnabled);
    weaveLayout->addWidget(m_weaveLoaderCheck);

    m_weaveLoaderCombo = new QComboBox();
    m_weaveLoaderCombo->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Fixed);
    weaveLayout->addWidget(m_weaveLoaderCombo);

    m_weaveLoaderStatusLabel = new QLabel(tr("Fetching versions..."));
    m_weaveLoaderStatusLabel->setEnabled(false);
    weaveLayout->addWidget(m_weaveLoaderStatusLabel);

    connect(m_weaveLoaderCheck, &QCheckBox::stateChanged, this, &SettingsDialog::onWeaveLoaderCheckChanged);

    mainLayout->addWidget(weaveGroup);

    QLabel *pathLabel = new QLabel(tr("Install path: %1").arg(m_instance.installPath));
    pathLabel->setEnabled(false);
    pathLabel->setWordWrap(true);
    mainLayout->addWidget(pathLabel);

    QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttons, &QDialogButtonBox::accepted, this, &SettingsDialog::onAccept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(buttons);
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

    accept();
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
