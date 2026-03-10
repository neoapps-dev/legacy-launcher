#pragma once

#include "Types.h"
#include <QWidget>
#include <QLineEdit>
#include <QCheckBox>
#include <QSpinBox>
#include <QComboBox>
#include <QList>
#include <QLabel>

class WeaveLoaderReleaseTracker;

class SettingsDialog : public QWidget {
    Q_OBJECT
public:
    explicit SettingsDialog(Instance instance, const QList<ProtonInstallation> &protons, QWidget *parent = nullptr);

    Instance updatedInstance() const;

signals:
    void finished(bool accepted);

private slots:
    void onAccept();
    void onWeaveLoaderReleasesUpdated(QList<ReleaseInfo> releases);
    void onWeaveLoaderFetchError(QString msg);
    void onWeaveLoaderCheckChanged(int state);

private:
    Instance m_instance;
    QList<ProtonInstallation> m_protons;
    QList<ReleaseInfo> m_weaveLoaderReleases;
    WeaveLoaderReleaseTracker *m_weaveLoaderTracker;

    QLineEdit *m_nameEdit;
    QLineEdit *m_usernameEdit;
    QComboBox *m_protonCombo;
    QCheckBox *m_headlessCheck;
    QLineEdit *m_ipEdit;
    QSpinBox *m_portSpin;
    QCheckBox *m_weaveLoaderCheck;
    QComboBox *m_weaveLoaderCombo;
    QLabel *m_weaveLoaderStatusLabel;

    void setupUi();
};
