#pragma once

#include "Types.h"
#include "Downloader.h"
#include <QDialog>
#include <QList>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QProgressBar>
#include <QLabel>
#include <QStackedWidget>
#include <QCheckBox>
#include <QProcess>

class GitHubReleaseTracker;
class WeaveLoaderReleaseTracker;

class AddInstanceDialog : public QDialog {
    Q_OBJECT
public:
    explicit AddInstanceDialog(const QList<ProtonInstallation> &protons, QWidget *parent = nullptr);

    Instance createdInstance() const;

private slots:
    void onReleasesUpdated(QList<ReleaseInfo> releases);
    void onWeaveLoaderReleasesUpdated(QList<ReleaseInfo> releases);
    void onFetchError(QString msg);
    void onWeaveLoaderFetchError(QString msg);
    void onInstallClicked();
    void onDownloadProgress(qint64 received, qint64 total);
    void onDownloadFinished(bool success, QString error);
    void onBrowseInstallPath();
    void onWeaveLoaderCheckChanged(int state);
    void onDotNetDownloadFinished(bool success, QString error);
    void onDotNetInstallFinished(int exitCode, QProcess::ExitStatus);

private:
    QList<ProtonInstallation> m_protons;
    QList<ReleaseInfo> m_releases;
    QList<ReleaseInfo> m_weaveLoaderReleases;
    GitHubReleaseTracker *m_tracker;
    WeaveLoaderReleaseTracker *m_weaveLoaderTracker;
    Downloader *m_downloader;
    Instance m_result;

    QLineEdit *m_nameEdit;
    QLineEdit *m_installPathEdit;
    QPushButton *m_browseBtn;
    QComboBox *m_releaseCombo;
    QComboBox *m_protonCombo;
    QLineEdit *m_usernameEdit;
    QPushButton *m_installBtn;
    QProgressBar *m_progressBar;
    QLabel *m_statusLabel;
    QPushButton *m_cancelBtn;
    QCheckBox *m_weaveLoaderCheck;
    QComboBox *m_weaveLoaderCombo;
    bool m_downloadingWeaveLoader;
    bool m_installingDotNet;
    ProtonInstallation m_selectedProton;

    void setupUi();
    void extractZip(const QString &zipPath, const QString &destDir);
    QString formatSize(qint64 bytes);
    void checkInstallReady();
    void createWeaveLoaderJson(const QString &installPath);
    void installDotNetRuntime(const QString &installPath, const ProtonInstallation &proton);
};
