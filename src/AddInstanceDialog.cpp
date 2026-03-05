#include "AddInstanceDialog.h"

#include "GitHubReleaseTracker.h"
#include "Downloader.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QUuid>
#include <QDir>
#include <QProcess>
#include <QStandardPaths>

AddInstanceDialog::AddInstanceDialog(const QList<ProtonInstallation> &protons, QWidget *parent)
    : QDialog(parent)
    , m_protons(protons)
    , m_tracker(new GitHubReleaseTracker(this))
    , m_downloader(new Downloader(this))
{
    setWindowTitle(tr("Add Instance"));
    setMinimumWidth(700);
    setupUi();

    connect(m_tracker, &GitHubReleaseTracker::releasesUpdated, this, &AddInstanceDialog::onReleasesUpdated);
    connect(m_tracker, &GitHubReleaseTracker::fetchError, this, &AddInstanceDialog::onFetchError);
    connect(m_downloader, &Downloader::progressChanged, this, &AddInstanceDialog::onDownloadProgress);
    connect(m_downloader, &Downloader::finished, this, &AddInstanceDialog::onDownloadFinished);

    m_statusLabel->setText(tr("Fetching releases..."));
    m_installBtn->setEnabled(false);
    m_tracker->fetchReleases();
}

void AddInstanceDialog::setupUi() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    QFormLayout *form = new QFormLayout();

    m_nameEdit = new QLineEdit(tr("My Instance"));
    form->addRow(tr("Instance Name:"), m_nameEdit);

    QHBoxLayout *pathLayout = new QHBoxLayout();
    m_installPathEdit = new QLineEdit();
    m_installPathEdit->setPlaceholderText(
        QStandardPaths::writableLocation(QStandardPaths::HomeLocation) + "/.local/share/LegacyLauncher/instances/");
    m_browseBtn = new QPushButton(tr("Browse..."));
    pathLayout->addWidget(m_installPathEdit);
    pathLayout->addWidget(m_browseBtn);
    form->addRow(tr("Install Path:"), pathLayout);

    m_releaseCombo = new QComboBox();
    m_releaseCombo->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Fixed);
    form->addRow(tr("Version:"), m_releaseCombo);

    m_protonCombo = new QComboBox();
    for (const ProtonInstallation &p : m_protons) {
        m_protonCombo->addItem(p.name, p.path);
    }
    if (m_protons.isEmpty()) {
        m_protonCombo->addItem(tr("No Proton installations found"));
        m_protonCombo->setEnabled(false);
    }
    form->addRow(tr("Proton:"), m_protonCombo);

    m_usernameEdit = new QLineEdit();
    m_usernameEdit->setPlaceholderText(tr("Player"));
    form->addRow(tr("Username:"), m_usernameEdit);

    mainLayout->addLayout(form);

    m_statusLabel = new QLabel();
    mainLayout->addWidget(m_statusLabel);

    m_progressBar = new QProgressBar();
    m_progressBar->setVisible(false);
    mainLayout->addWidget(m_progressBar);

    QHBoxLayout *btnLayout = new QHBoxLayout();
    m_cancelBtn = new QPushButton(tr("Cancel"));
    m_installBtn = new QPushButton(tr("Install"));
    m_installBtn->setDefault(true);
    btnLayout->addStretch();
    btnLayout->addWidget(m_cancelBtn);
    btnLayout->addWidget(m_installBtn);
    mainLayout->addLayout(btnLayout);

    connect(m_browseBtn, &QPushButton::clicked, this, &AddInstanceDialog::onBrowseInstallPath);
    connect(m_installBtn, &QPushButton::clicked, this, &AddInstanceDialog::onInstallClicked);
    connect(m_cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
}

void AddInstanceDialog::onReleasesUpdated(QList<ReleaseInfo> releases) {
    m_releases = releases;
    m_releaseCombo->clear();

    for (const ReleaseInfo &r : releases) {
        QString label = r.tag;
        if (r.isNightly) label += tr(" [Nightly]");
        label += " — " + r.publishedAt.toLocalTime().toString("yyyy-MM-dd HH:mm");
        m_releaseCombo->addItem(label, r.tag);
    }

    if (releases.isEmpty()) {
        m_statusLabel->setText(tr("No releases found."));
    } else {
        m_statusLabel->setText(tr("Ready."));
        m_installBtn->setEnabled(true);
    }
}

void AddInstanceDialog::onFetchError(QString msg) {
    m_statusLabel->setText(tr("Error fetching releases: %1").arg(msg));
}

void AddInstanceDialog::onBrowseInstallPath() {
    QString dir = QFileDialog::getExistingDirectory(this, tr("Select Install Directory"),
        QDir::homePath() + "/.local/share/LegacyLauncher/instances");
    if (!dir.isEmpty()) {
        m_installPathEdit->setText(dir);
    }
}

void AddInstanceDialog::onInstallClicked() {
    if (m_releases.isEmpty() || m_releaseCombo->currentIndex() < 0) return;

    QString name = m_nameEdit->text().trimmed();
    if (name.isEmpty()) {
        QMessageBox::warning(this, tr("Input Error"), tr("Please enter an instance name."));
        return;
    }

    QString basePath = m_installPathEdit->text().trimmed();
    if (basePath.isEmpty()) {
        basePath = QStandardPaths::writableLocation(QStandardPaths::HomeLocation)
                   + "/.local/share/LegacyLauncher/instances";
    }

    QString id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    QString installPath = basePath + "/" + id;

    if (!QDir().mkpath(installPath)) {
        QMessageBox::critical(this, tr("Error"), tr("Could not create directory: %1").arg(installPath));
        return;
    }

    const ReleaseInfo &release = m_releases[m_releaseCombo->currentIndex()];

    m_result.id = id;
    m_result.name = name;
    m_result.installPath = installPath;
    m_result.installedTag = release.tag;
    m_result.installedAt = QDateTime::currentDateTime();
    m_result.username = m_usernameEdit->text().trimmed();
    m_result.headlessMode = false;
    m_result.autoPort = 0;

    int protonIdx = m_protonCombo->currentIndex();
    if (protonIdx >= 0 && protonIdx < m_protons.size()) {
        m_result.protonId = m_protons[protonIdx].path;
    }

    m_installBtn->setEnabled(false);
    m_progressBar->setVisible(true);
    m_progressBar->setRange(0, 0);
    m_statusLabel->setText(tr("Downloading..."));

    QString zipPath = installPath + "/LCEWindows64.zip";
    m_downloader->download(release.downloadUrl, zipPath);
}

void AddInstanceDialog::onDownloadProgress(qint64 received, qint64 total) {
    if (total > 0) {
        m_progressBar->setRange(0, 100);
        m_progressBar->setValue(static_cast<int>((received * 100) / total));
        m_statusLabel->setText(tr("Downloading... %1 / %2")
            .arg(formatSize(received))
            .arg(formatSize(total)));
    }
}

void AddInstanceDialog::onDownloadFinished(bool success, QString error) {
    if (!success) {
        m_statusLabel->setText(tr("Download failed: %1").arg(error));
        m_installBtn->setEnabled(true);
        m_progressBar->setVisible(false);
        return;
    }

    m_statusLabel->setText(tr("Extracting..."));
    m_progressBar->setRange(0, 0);

    QString zipPath = m_result.installPath + "/LCEWindows64.zip";
    extractZip(zipPath, m_result.installPath);

    QFile::remove(zipPath);

    m_statusLabel->setText(tr("Done!"));
    m_progressBar->setVisible(false);

    accept();
}

void AddInstanceDialog::extractZip(const QString &zipPath, const QString &destDir) {
    QProcess proc;
    proc.start("unzip", {"-o", zipPath, "-d", destDir});
    proc.waitForFinished(-1);
}

QString AddInstanceDialog::formatSize(qint64 bytes) {
    if (bytes < 1024) return QString("%1 B").arg(bytes);
    if (bytes < 1024 * 1024) return QString("%1 KiB").arg(bytes / 1024);
    return QString("%1 MiB").arg(bytes / (1024 * 1024));
}

Instance AddInstanceDialog::createdInstance() const {
    return m_result;
}
