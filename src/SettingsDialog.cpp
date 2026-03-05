#include "SettingsDialog.h"

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
{
    setWindowTitle(tr("Instance Settings — %1").arg(instance.name));
    setMinimumWidth(500);
    setupUi();
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

    accept();
}

Instance SettingsDialog::updatedInstance() const {
    return m_instance;
}
