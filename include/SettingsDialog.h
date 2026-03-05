#pragma once

#include "Types.h"
#include <QDialog>
#include <QLineEdit>
#include <QCheckBox>
#include <QSpinBox>
#include <QComboBox>
#include <QList>

class SettingsDialog : public QDialog {
    Q_OBJECT
public:
    explicit SettingsDialog(Instance instance, const QList<ProtonInstallation> &protons, QWidget *parent = nullptr);

    Instance updatedInstance() const;

private slots:
    void onAccept();

private:
    Instance m_instance;
    QList<ProtonInstallation> m_protons;

    QLineEdit *m_nameEdit;
    QLineEdit *m_usernameEdit;
    QComboBox *m_protonCombo;
    QCheckBox *m_headlessCheck;
    QLineEdit *m_ipEdit;
    QSpinBox *m_portSpin;

    void setupUi();
};
