#include "UpdateInstanceDialog.h"

#include "Downloader.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QMessageBox>
#include <QDir>
#include <QProcess>

UpdateInstanceDialog::UpdateInstanceDialog(const Instance &instance,
                                           const QList<ReleaseInfo> &releases,
                                           QWidget *parent)
    : QDialog(parent)
    , m_instance(instance)
    , m_releases(releases)
    , m_downloader(new Downloader(this))
{
    setWindowTitle(tr("Update Instance"));
    setMinimumWidth(550);
    setupUi();

    connect(m_downloader, &Downloader::progressChanged, this, &UpdateInstanceDialog::onDownloadProgress);
    connect(m_downloader, &Downloader::finished, this, &UpdateInstanceDialog::onDownloadFinished);
}

void UpdateInstanceDialog::setupUi() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(16);
    mainLayout->setContentsMargins(24, 24, 24, 24);

    QLabel *titleLabel = new QLabel(tr("Change Version"));
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(titleFont.pointSize() + 4);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    mainLayout->addWidget(titleLabel);

    QGroupBox *infoGroup = new QGroupBox(tr("Instance Info"));
    infoGroup->setObjectName("updateInfoGroup");
    QFormLayout *infoForm = new QFormLayout(infoGroup);
    infoForm->setSpacing(8);

    m_nameLabel = new QLabel(m_instance.name);
    QFont nameFont = m_nameLabel->font();
    nameFont.setBold(true);
    m_nameLabel->setFont(nameFont);
    infoForm->addRow(tr("Name:"), m_nameLabel);

    m_currentVersionLabel = new QLabel(
        m_instance.installedTag.isEmpty() ? tr("Unknown") : m_instance.installedTag);
    infoForm->addRow(tr("Current Version:"), m_currentVersionLabel);

    QLabel *pathLabel = new QLabel(m_instance.installPath);
    pathLabel->setWordWrap(true);
    pathLabel->setEnabled(false);
    infoForm->addRow(tr("Install Path:"), pathLabel);

    mainLayout->addWidget(infoGroup);

    QGroupBox *versionGroup = new QGroupBox(tr("Target Version"));
    versionGroup->setObjectName("updateVersionGroup");
    QVBoxLayout *versionLayout = new QVBoxLayout(versionGroup);
    versionLayout->setSpacing(8);

    m_targetVersionCombo = new QComboBox();
    m_targetVersionCombo->setObjectName("targetVersionCombo");
    for (const ReleaseInfo &r : m_releases) {
        QString label = r.tag;
        if (r.isNightly) label += tr(" [Nightly]");
        label += QString::fromUtf8(" \xe2\x80\x94 ") + r.publishedAt.toLocalTime().toString("yyyy-MM-dd HH:mm");
        bool isCurrent = (r.tag == m_instance.installedTag);
        if (isCurrent) label += tr(" (current)");
        m_targetVersionCombo->addItem(label, r.tag);
    }
    versionLayout->addWidget(m_targetVersionCombo);

    QLabel *noteLabel = new QLabel(tr("Your proton prefix, saves, name, and all settings will be preserved."));
    noteLabel->setWordWrap(true);
    noteLabel->setEnabled(false);
    versionLayout->addWidget(noteLabel);

    mainLayout->addWidget(versionGroup);

    m_statusLabel = new QLabel();
    m_statusLabel->setObjectName("updateStatusLabel");
    mainLayout->addWidget(m_statusLabel);

    m_progressBar = new QProgressBar();
    m_progressBar->setVisible(false);
    m_progressBar->setObjectName("updateProgressBar");
    mainLayout->addWidget(m_progressBar);

    mainLayout->addStretch();

    QHBoxLayout *btnLayout = new QHBoxLayout();
    m_cancelBtn = new QPushButton(tr("Cancel"));
    m_cancelBtn->setObjectName("updateCancelBtn");
    m_updateBtn = new QPushButton(tr("Update"));
    m_updateBtn->setObjectName("updateConfirmBtn");
    m_updateBtn->setDefault(true);
    btnLayout->addStretch();
    btnLayout->addWidget(m_cancelBtn);
    btnLayout->addWidget(m_updateBtn);
    mainLayout->addLayout(btnLayout);

    connect(m_updateBtn, &QPushButton::clicked, this, &UpdateInstanceDialog::onUpdateClicked);
    connect(m_cancelBtn, &QPushButton::clicked, this, &QDialog::reject);

    if (m_releases.isEmpty()) {
        m_updateBtn->setEnabled(false);
        m_statusLabel->setText(tr("No releases available."));
    }
}

void UpdateInstanceDialog::onUpdateClicked() {
    int idx = m_targetVersionCombo->currentIndex();
    if (idx < 0 || idx >= m_releases.size()) return;

    const ReleaseInfo &release = m_releases[idx];

    if (release.tag == m_instance.installedTag) {
        QMessageBox::StandardButton ans = QMessageBox::question(this,
            tr("Same Version"),
            tr("You already have %1 installed. Re-download and reinstall it?").arg(release.tag));
        if (ans != QMessageBox::Yes) return;
    }

    m_updateBtn->setEnabled(false);
    m_cancelBtn->setEnabled(false);
    m_targetVersionCombo->setEnabled(false);
    m_progressBar->setVisible(true);
    m_progressBar->setRange(0, 0);
    m_statusLabel->setText(tr("Downloading %1...").arg(release.tag));

    QString zipPath = m_instance.installPath + "/LCEWindows64.zip";
    m_downloader->download(release.downloadUrl, zipPath);
}

void UpdateInstanceDialog::onDownloadProgress(qint64 received, qint64 total) {
    if (total > 0) {
        m_progressBar->setRange(0, 100);
        m_progressBar->setValue(static_cast<int>((received * 100) / total));
        m_statusLabel->setText(tr("Downloading... %1 / %2")
            .arg(formatSize(received))
            .arg(formatSize(total)));
    }
}

void UpdateInstanceDialog::onDownloadFinished(bool success, QString error) {
    if (!success) {
        m_statusLabel->setText(tr("Download failed: %1").arg(error));
        m_updateBtn->setEnabled(true);
        m_cancelBtn->setEnabled(true);
        m_targetVersionCombo->setEnabled(true);
        m_progressBar->setVisible(false);
        return;
    }

    m_statusLabel->setText(tr("Extracting..."));
    m_progressBar->setRange(0, 0);

    QString zipPath = m_instance.installPath + "/LCEWindows64.zip";
    extractZip(zipPath, m_instance.installPath);
    QFile::remove(zipPath);

    int idx = m_targetVersionCombo->currentIndex();
    if (idx >= 0 && idx < m_releases.size()) {
        m_instance.installedTag = m_releases[idx].tag;
        m_instance.installedAt = QDateTime::currentDateTime();
    }

    m_statusLabel->setText(tr("Done!"));
    m_progressBar->setVisible(false);

    accept();
}

void UpdateInstanceDialog::extractZip(const QString &zipPath, const QString &destDir) {
    QProcess proc;
    proc.start("unzip", {"-o", zipPath, "-d", destDir});
    proc.waitForFinished(-1);
}

QString UpdateInstanceDialog::formatSize(qint64 bytes) {
    if (bytes < 1024) return QString("%1 B").arg(bytes);
    if (bytes < 1024 * 1024) return QString("%1 KiB").arg(bytes / 1024);
    return QString("%1 MiB").arg(bytes / (1024 * 1024));
}

Instance UpdateInstanceDialog::updatedInstance() const {
    return m_instance;
}
