#pragma once

#include "Types.h"
#include <QWidget>
#include <QList>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QProgressBar>
#include <QLabel>
#include <QStackedWidget>

class GitHubReleaseTracker;
class Downloader;

class AddInstanceDialog : public QWidget {
    Q_OBJECT
public:
    explicit AddInstanceDialog(const QList<ProtonInstallation> &protons, QWidget *parent = nullptr);

    Instance createdInstance() const;

signals:
    void finished(bool accepted);

private slots:
    void onReleasesUpdated(QList<ReleaseInfo> releases);
    void onFetchError(QString msg);
    void onInstallClicked();
    void onDownloadProgress(qint64 received, qint64 total);
    void onDownloadFinished(bool success, QString error);
    void onBrowseInstallPath();

private:
    QList<ProtonInstallation> m_protons;
    QList<ReleaseInfo> m_releases;
    GitHubReleaseTracker *m_tracker;
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

    void setupUi();
    void extractZip(const QString &zipPath, const QString &destDir);
    QString formatSize(qint64 bytes);
};
