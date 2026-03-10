#include "UpdateInstanceDialog.h"

#include "Downloader.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QProgressBar>
#include <QScrollArea>
#include <QMessageBox>
#include <QDir>
#include <QProcess>

UpdateInstanceDialog::UpdateInstanceDialog(const Instance &instance,
                                           const QList<ReleaseInfo> &releases,
                                           QWidget *parent)
    : QWidget(parent)
    , m_instance(instance)
    , m_releases(releases)
    , m_downloader(new Downloader(this))
{
    setObjectName("updateInstancePage");
    setWindowTitle(tr("Update Instance"));
    setMinimumWidth(550);
    setAttribute(Qt::WA_StyledBackground);
    setupUi();

    connect(m_downloader, &Downloader::progressChanged, this, &UpdateInstanceDialog::onDownloadProgress);
    connect(m_downloader, &Downloader::finished, this, &UpdateInstanceDialog::onDownloadFinished);
}

void UpdateInstanceDialog::setupUi() {

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    QWidget *headerWidget = new QWidget();
    QHBoxLayout *headerLayout = new QHBoxLayout(headerWidget);
    headerLayout->setContentsMargins(40, 40, 40, 10);
    
    headerLayout->addStretch();
    QLabel *titleLabel = new QLabel(tr("Change Version"));
    titleLabel->setObjectName("titleLabel");
    titleLabel->setAlignment(Qt::AlignCenter);
    headerLayout->addWidget(titleLabel, 0, Qt::AlignCenter);
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
    scrollArea->setObjectName("updateScrollArea");

    QWidget *contentWidget = new QWidget();
    contentWidget->setObjectName("contentWidget");
    QVBoxLayout *contentLayout = new QVBoxLayout(contentWidget);
    contentLayout->setContentsMargins(100, 20, 100, 40);
    contentLayout->setSpacing(24);

    QLabel *iconLabel = new QLabel();
    iconLabel->setPixmap(QPixmap(":/packaging/icon.png").scaled(64, 64, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    iconLabel->setAlignment(Qt::AlignCenter);
    contentLayout->addWidget(iconLabel);
    contentLayout->addSpacing(10);

    QVBoxLayout *nameLayout = new QVBoxLayout();
    nameLayout->setSpacing(4);
    QLabel *nameLbl = new QLabel(tr("NAME"));
    nameLbl->setObjectName("formLabel");
    m_nameLabel = new QLabel(m_instance.name);
    m_nameLabel->setObjectName("valLabel");
    nameLayout->addWidget(nameLbl);
    nameLayout->addWidget(m_nameLabel);
    contentLayout->addLayout(nameLayout);

    QVBoxLayout *currVerLayout = new QVBoxLayout();
    currVerLayout->setSpacing(4);
    QLabel *currVerLbl = new QLabel(tr("CURRENT VERSION"));
    currVerLbl->setObjectName("formLabel");
    m_currentVersionLabel = new QLabel(m_instance.installedTag.isEmpty() ? tr("Unknown") : m_instance.installedTag);
    m_currentVersionLabel->setObjectName("valLabel");
    currVerLayout->addWidget(currVerLbl);
    currVerLayout->addWidget(m_currentVersionLabel);
    contentLayout->addLayout(currVerLayout);

    QVBoxLayout *targetVerLayout = new QVBoxLayout();
    targetVerLayout->setSpacing(4);
    QLabel *targetVerLbl = new QLabel(tr("TARGET VERSION"));
    targetVerLbl->setObjectName("formLabel");
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
    targetVerLayout->addWidget(targetVerLbl);
    targetVerLayout->addWidget(m_targetVersionCombo);
    contentLayout->addLayout(targetVerLayout);

    QLabel *pathLabel = new QLabel(tr("Install path: %1").arg(m_instance.installPath));
    pathLabel->setObjectName("infoLabel");
    pathLabel->setWordWrap(true);
    contentLayout->addWidget(pathLabel);

    QLabel *noteLabel = new QLabel(tr("Your proton prefix, saves, name, and all settings will be preserved."));
    noteLabel->setObjectName("infoLabel");
    noteLabel->setWordWrap(true);
    contentLayout->addWidget(noteLabel);

    contentLayout->addStretch();
    
    m_statusLabel = new QLabel();
    m_statusLabel->setObjectName("updateStatusLabel");
    contentLayout->addWidget(m_statusLabel);

    m_progressBar = new QProgressBar();
    m_progressBar->setVisible(false);
    m_progressBar->setObjectName("updateProgressBar");
    m_progressBar->setFixedHeight(4);
    m_progressBar->setTextVisible(false);
    contentLayout->addWidget(m_progressBar);

    scrollArea->setWidget(contentWidget);
    mainLayout->addWidget(scrollArea);

    QWidget *footerWidget = new QWidget();
    footerWidget->setObjectName("modalFooter");
    QHBoxLayout *btnLayout = new QHBoxLayout(footerWidget);
    btnLayout->setContentsMargins(60, 20, 60, 30);
    
    m_cancelBtn = new QPushButton(tr("Cancel"));
    m_cancelBtn->setObjectName("updateCancelBtn");
    m_updateBtn = new QPushButton(tr("Update"));
    m_updateBtn->setObjectName("updateConfirmBtn");
    m_updateBtn->setDefault(true);
    
    btnLayout->addStretch();
    btnLayout->addWidget(m_cancelBtn);
    btnLayout->addWidget(m_updateBtn);
    mainLayout->addWidget(footerWidget);

    connect(m_updateBtn, &QPushButton::clicked, this, &UpdateInstanceDialog::onUpdateClicked);
    connect(m_cancelBtn, &QPushButton::clicked, this, [this]{ emit finished(false); });

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

    emit finished(true);
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
