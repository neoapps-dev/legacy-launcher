#pragma once

#include "Types.h"
#include <QWidget>
#include <QList>
#include <QComboBox>
#include <QPushButton>
#include <QProgressBar>
#include <QLabel>

class GitHubReleaseTracker;
class Downloader;

class UpdateInstanceDialog : public QWidget {
    Q_OBJECT
public:
    explicit UpdateInstanceDialog(const Instance &instance,
                                  const QList<ReleaseInfo> &releases,
                                  QWidget *parent = nullptr);

    Instance updatedInstance() const;

signals:
    void finished(bool accepted);

private slots:
    void onUpdateClicked();
    void onDownloadProgress(qint64 received, qint64 total);
    void onDownloadFinished(bool success, QString error);

private:
    Instance m_instance;
    QList<ReleaseInfo> m_releases;
    Downloader *m_downloader;

    QLabel *m_nameLabel;
    QLabel *m_currentVersionLabel;
    QComboBox *m_targetVersionCombo;
    QPushButton *m_updateBtn;
    QPushButton *m_cancelBtn;
    QProgressBar *m_progressBar;
    QLabel *m_statusLabel;

    void setupUi();
    void extractZip(const QString &zipPath, const QString &destDir);
    QString formatSize(qint64 bytes);
};
