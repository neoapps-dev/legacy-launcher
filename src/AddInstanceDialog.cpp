#include "AddInstanceDialog.h"

#include "GitHubReleaseTracker.h"
#include "WeaveLoaderReleaseTracker.h"
#include "Downloader.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QFileDialog>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QPushButton>
#include <QProgressBar>
#include <QScrollArea>
#include <QMessageBox>
#include <QUuid>
#include <QDir>
#include <QProcess>
#include <QStandardPaths>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QCoreApplication>

AddInstanceDialog::AddInstanceDialog(const QList<ProtonInstallation> &protons, QWidget *parent)
    : QWidget(parent)
    , m_protons(protons)
    , m_tracker(new GitHubReleaseTracker(this))
    , m_weaveLoaderTracker(new WeaveLoaderReleaseTracker(this))
    , m_downloader(new Downloader(this))
    , m_downloadingWeaveLoader(false)
    , m_installingDotNet(false)
{
    setObjectName("addInstancePage");
    setWindowTitle(tr("Add Instance"));
    setMinimumWidth(700);
    setupUi();

    connect(m_tracker, &GitHubReleaseTracker::releasesUpdated, this, &AddInstanceDialog::onReleasesUpdated);
    connect(m_tracker, &GitHubReleaseTracker::fetchError, this, &AddInstanceDialog::onFetchError);
    connect(m_weaveLoaderTracker, &WeaveLoaderReleaseTracker::releasesUpdated, this, &AddInstanceDialog::onWeaveLoaderReleasesUpdated);
    connect(m_weaveLoaderTracker, &WeaveLoaderReleaseTracker::fetchError, this, &AddInstanceDialog::onWeaveLoaderFetchError);
    connect(m_downloader, &Downloader::progressChanged, this, &AddInstanceDialog::onDownloadProgress);
    connect(m_downloader, &Downloader::finished, this, &AddInstanceDialog::onDownloadFinished);

    m_statusLabel->setText(tr("Fetching releases..."));
    m_installBtn->setEnabled(false);
    m_weaveLoaderCombo->setEnabled(false);
    m_tracker->fetchReleases();
    m_weaveLoaderTracker->fetchReleases();
}

void AddInstanceDialog::setupUi() {
    setStyleSheet(R"(
        QWidget#addInstancePage {
            background-color: #313338;
        }
        QLabel {
            color: #b9bbbe;
            font-size: 12px;
            font-weight: bold;
            text-transform: uppercase;
        }
        QLabel#titleLabel {
            color: white;
            font-size: 20px;
            font-weight: 800;
            text-transform: none;
        }
        QLineEdit, QComboBox {
            background-color: #1e1f22;
            color: white;
            border: 1px solid #101113;
            border-radius: 4px;
            padding: 12px;
            font-size: 14px;
        }
        QLineEdit:hover, QComboBox:hover {
            border: 1px solid #4f545c;
        }
        QLineEdit:focus, QComboBox:focus {
            border: 1px solid #5865f2;
        }
        QComboBox::drop-down {
            border: none;
        }
        QPushButton {
            font-weight: bold;
            border-radius: 3px;
        }
        QPushButton#installBtn {
            background-color: #2e8b57;
            color: white;
            padding: 10px 24px;
            border: none;
        }
        QPushButton#installBtn:hover {
            background-color: #3cb371;
        }
        QPushButton#installBtn:disabled {
            background-color: #555555;
            color: #888888;
        }
        QPushButton#cancelBtn {
            background-color: transparent;
            color: white;
            padding: 10px 24px;
            border: 1px solid #4f545c;
        }
        QPushButton#cancelBtn:hover {
            background-color: rgba(255, 255, 255, 0.05);
        }
        QPushButton#browseBtn {
            background-color: #2b2d31;
            color: #b9bbbe;
            border: none;
            padding: 10px 16px;
        }
        QPushButton#browseBtn:hover {
            color: white;
        }
        QPushButton#closeBtn {
            background-color: transparent;
            color: #b9bbbe;
            border: 1px solid #4f545c;
            border-radius: 16px;
            font-size: 16px;
        }
        QPushButton#closeBtn:hover {
            background-color: #4f545c;
            color: white;
        }
    )");

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    QWidget *headerWidget = new QWidget();
    QHBoxLayout *headerLayout = new QHBoxLayout(headerWidget);
    headerLayout->setContentsMargins(40, 40, 40, 10);
    
    headerLayout->addStretch();
    QLabel *titleLabel = new QLabel(tr("Create new installation"));
    titleLabel->setObjectName("titleLabel");
    titleLabel->setAlignment(Qt::AlignCenter);
    headerLayout->addWidget(titleLabel);
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
    scrollArea->setStyleSheet("QScrollArea { border: none; background-color: transparent; } QScrollBar { background: #2b2d31; width: 8px; } QScrollBar::handle { background: #1e1f22; border-radius: 4px; }");
    
    QWidget *contentWidget = new QWidget();
    contentWidget->setStyleSheet("background-color: transparent;");
    QVBoxLayout *contentLayout = new QVBoxLayout(contentWidget);
    contentLayout->setContentsMargins(100, 20, 100, 40);
    contentLayout->setSpacing(24);

    QLabel *iconLabel = new QLabel();
    iconLabel->setPixmap(QPixmap(":/packaging/icon.png").scaled(64, 64, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    iconLabel->setAlignment(Qt::AlignCenter);
    contentLayout->addWidget(iconLabel);
    contentLayout->addSpacing(10);

    QVBoxLayout *nameLayout = new QVBoxLayout();
    nameLayout->setSpacing(8);
    QLabel *nameLbl = new QLabel(tr("NAME"));
    m_nameEdit = new QLineEdit();
    m_nameEdit->setText(tr("unnamed installation"));
    nameLayout->addWidget(nameLbl);
    nameLayout->addWidget(m_nameEdit);
    contentLayout->addLayout(nameLayout);

    QVBoxLayout *verLayout = new QVBoxLayout();
    verLayout->setSpacing(8);
    QLabel *verLbl = new QLabel(tr("VERSION"));
    m_releaseCombo = new QComboBox();
    verLayout->addWidget(verLbl);
    verLayout->addWidget(m_releaseCombo);
    contentLayout->addLayout(verLayout);

    QVBoxLayout *pathContainerLayout = new QVBoxLayout();
    pathContainerLayout->setSpacing(8);
    QLabel *pathLbl = new QLabel(tr("GAME DIRECTORY"));
    pathContainerLayout->addWidget(pathLbl);
    
    QHBoxLayout *pathLayout = new QHBoxLayout();
    pathLayout->setSpacing(0);
    m_installPathEdit = new QLineEdit();
    m_installPathEdit->setPlaceholderText(tr("<Use default directory>"));
    m_installPathEdit->setStyleSheet("QLineEdit { border-top-right-radius: 0; border-bottom-right-radius: 0; }");
    m_browseBtn = new QPushButton(tr("BROWSE"));
    m_browseBtn->setObjectName("browseBtn");
    m_browseBtn->setStyleSheet("QPushButton { border-top-left-radius: 0; border-bottom-left-radius: 0; }");
    pathLayout->addWidget(m_installPathEdit);
    pathLayout->addWidget(m_browseBtn);
    pathContainerLayout->addLayout(pathLayout);
    contentLayout->addLayout(pathContainerLayout);

    QVBoxLayout *protonLayout = new QVBoxLayout();
    protonLayout->setSpacing(8);
    QLabel *protonLbl = new QLabel(tr("PROTON"));
    m_protonCombo = new QComboBox();
    for (const ProtonInstallation &p : m_protons) {
        m_protonCombo->addItem(p.name, p.path);
    }
    if (m_protons.isEmpty()) {
        m_protonCombo->addItem(tr("No Proton installations found"));
        m_protonCombo->setEnabled(false);
    }
    protonLayout->addWidget(protonLbl);
    protonLayout->addWidget(m_protonCombo);
    contentLayout->addLayout(protonLayout);

    QVBoxLayout *userLayout = new QVBoxLayout();
    userLayout->setSpacing(8);
    QLabel *userLbl = new QLabel(tr("USERNAME"));
    m_usernameEdit = new QLineEdit();
    m_usernameEdit->setPlaceholderText(tr("Player"));
    userLayout->addWidget(userLbl);
    userLayout->addWidget(m_usernameEdit);
    contentLayout->addLayout(userLayout);
    
    contentLayout->addStretch();
    
    form->addRow(tr("Username:"), m_usernameEdit);

    QGroupBox *weaveGroup = new QGroupBox(tr("Weave Loader (Optional)"));
    QVBoxLayout *weaveLayout = new QVBoxLayout(weaveGroup);

    m_weaveLoaderCheck = new QCheckBox(tr("Enable Weave Loader"));
    m_weaveLoaderCheck->setChecked(false);
    weaveLayout->addWidget(m_weaveLoaderCheck);

    m_weaveLoaderCombo = new QComboBox();
    m_weaveLoaderCombo->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Fixed);
    weaveLayout->addWidget(m_weaveLoaderCombo);

    connect(m_weaveLoaderCheck, &QCheckBox::stateChanged, this, &AddInstanceDialog::onWeaveLoaderCheckChanged);

    mainLayout->addLayout(form);
    mainLayout->addWidget(weaveGroup);

    m_statusLabel = new QLabel();
    m_statusLabel->setStyleSheet("color: #b9bbbe; text-transform: none; font-weight: normal;");
    contentLayout->addWidget(m_statusLabel);

    m_progressBar = new QProgressBar();
    m_progressBar->setVisible(false);
    m_progressBar->setFixedHeight(4);
    m_progressBar->setTextVisible(false);
    m_progressBar->setStyleSheet("QProgressBar { background: #1e1f22; border: none; } QProgressBar::chunk { background: #2e8b57; }");
    contentLayout->addWidget(m_progressBar);

    scrollArea->setWidget(contentWidget);
    mainLayout->addWidget(scrollArea);

    QWidget *footerWidget = new QWidget();
    footerWidget->setStyleSheet("background-color: transparent; border-top: 1px solid rgba(255, 255, 255, 0.05);");
    QHBoxLayout *btnLayout = new QHBoxLayout(footerWidget);
    btnLayout->setContentsMargins(60, 20, 60, 30);
    
    m_cancelBtn = new QPushButton(tr("Cancel"));
    m_cancelBtn->setObjectName("cancelBtn");
    m_installBtn = new QPushButton(tr("Create"));
    m_installBtn->setObjectName("installBtn");
    m_installBtn->setDefault(true);
    
    btnLayout->addStretch();
    btnLayout->addWidget(m_cancelBtn);
    btnLayout->addWidget(m_installBtn);
    mainLayout->addWidget(footerWidget);

    connect(m_browseBtn, &QPushButton::clicked, this, &AddInstanceDialog::onBrowseInstallPath);
    connect(m_installBtn, &QPushButton::clicked, this, &AddInstanceDialog::onInstallClicked);
    connect(m_cancelBtn, &QPushButton::clicked, this, [this]{ emit finished(false); });
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
    }
    checkInstallReady();
}

void AddInstanceDialog::onFetchError(QString msg) {
    m_statusLabel->setText(tr("Error fetching releases: %1").arg(msg));
}

void AddInstanceDialog::onWeaveLoaderReleasesUpdated(QList<ReleaseInfo> releases) {
    m_weaveLoaderReleases = releases;
    m_weaveLoaderCombo->clear();

    for (const ReleaseInfo &r : releases) {
        QString label = r.tag + " — " + r.publishedAt.toLocalTime().toString("yyyy-MM-dd HH:mm");
        m_weaveLoaderCombo->addItem(label, r.tag);
    }

    m_weaveLoaderCombo->setEnabled(!releases.isEmpty());
    checkInstallReady();
}

void AddInstanceDialog::onWeaveLoaderFetchError(QString msg) {
    qWarning() << "WeaveLoader fetch error:" << msg;
}

void AddInstanceDialog::onWeaveLoaderCheckChanged(int state) {
    m_weaveLoaderCombo->setEnabled(state == Qt::Checked && !m_weaveLoaderReleases.isEmpty());
    checkInstallReady();
}

void AddInstanceDialog::checkInstallReady() {
    bool ready = !m_releases.isEmpty() && m_releaseCombo->currentIndex() >= 0;
    if (m_weaveLoaderCheck->isChecked()) {
        ready = ready && m_weaveLoaderCombo->currentIndex() >= 0;
    }
    m_installBtn->setEnabled(ready);
    if (ready) {
        m_statusLabel->setText(tr("Ready."));
    }
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

    m_result.weaveLoaderEnabled = m_weaveLoaderCheck->isChecked();
    m_result.weaveLoaderTag = "";
    m_result.weaveLoaderInstalledAt = QDateTime();

    if (m_result.weaveLoaderEnabled && m_weaveLoaderCombo->currentIndex() >= 0) {
        int wlIdx = m_weaveLoaderCombo->currentIndex();
        if (wlIdx >= 0 && wlIdx < m_weaveLoaderReleases.size()) {
            m_result.weaveLoaderTag = m_weaveLoaderReleases[wlIdx].tag;
        }
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
    if (m_downloadingWeaveLoader) {
        if (!success) {
            QMessageBox::critical(this, tr("Download Failed"), error);
            m_downloadingWeaveLoader = false;
            m_progressBar->setVisible(false);
            return;
        }

        m_statusLabel->setText(tr("Extracting Weave Loader..."));
        m_progressBar->setRange(0, 0);

        QString wlZipPath = m_result.installPath + "/WeaveLoader.zip";
        extractZip(wlZipPath, m_result.installPath);
        QFile::remove(wlZipPath);

        m_downloadingWeaveLoader = false;

        int protonIdx = m_protonCombo->currentIndex();
        if (protonIdx >= 0 && protonIdx < m_protons.size()) {
            m_selectedProton = m_protons[protonIdx];
            installDotNetRuntime(m_result.installPath, m_selectedProton);
            return;
        }

        createWeaveLoaderJson(m_result.installPath);
        m_statusLabel->setText(tr("Done!"));
        m_progressBar->setVisible(false);
        accept();
        return;
    }

    m_statusLabel->setText(tr("Extracting..."));
    m_progressBar->setRange(0, 0);

    QString zipPath = m_result.installPath + "/LCEWindows64.zip";
    extractZip(zipPath, m_result.installPath);

    QFile::remove(zipPath);

    if (m_result.weaveLoaderEnabled && m_weaveLoaderCombo->currentIndex() >= 0) {
        int wlIdx = m_weaveLoaderCombo->currentIndex();
        if (wlIdx >= 0 && wlIdx < m_weaveLoaderReleases.size()) {
            const ReleaseInfo &wlRelease = m_weaveLoaderReleases[wlIdx];
            m_result.weaveLoaderTag = wlRelease.tag;
            m_result.weaveLoaderInstalledAt = QDateTime::currentDateTime();

            m_statusLabel->setText(tr("Downloading Weave Loader..."));
            QString wlZipPath = m_result.installPath + "/WeaveLoader.zip";
            m_downloadingWeaveLoader = true;
            m_downloader->download(wlRelease.downloadUrl, wlZipPath);
            return;
        }
    }

    m_statusLabel->setText(tr("Done!"));
    m_progressBar->setVisible(false);

    emit finished(true);
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

void AddInstanceDialog::createWeaveLoaderJson(const QString &installPath) {
    QJsonObject obj;
    obj["GameExePath"] = ".\\Minecraft.Client.exe";

    QJsonDocument doc(obj);
    QString jsonPath = installPath + "/weaveloader.json";
    QFile file(jsonPath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(doc.toJson(QJsonDocument::Indented));
        file.close();
    }
}

void AddInstanceDialog::installDotNetRuntime(const QString &installPath, const ProtonInstallation &proton) {
    QString dotNetUrl = "https://builds.dotnet.microsoft.com/dotnet/WindowsDesktop/8.0.24/windowsdesktop-runtime-8.0.24-win-x64.exe";
    QString tempPath = QDir::tempPath() + "/windowsdesktop-runtime-8.0.24-win-x64.exe";

    m_statusLabel->setText(tr("Downloading .NET Runtime..."));
    m_progressBar->setRange(0, 0);
    m_installingDotNet = true;

    connect(m_downloader, &Downloader::finished, this, &AddInstanceDialog::onDotNetDownloadFinished);
    m_downloader->download(dotNetUrl, tempPath);
}

void AddInstanceDialog::onDotNetDownloadFinished(bool success, QString errorMsg) {
    disconnect(m_downloader, &Downloader::finished, this, &AddInstanceDialog::onDotNetDownloadFinished);

    if (!success) {
        QMessageBox::critical(this, tr("Download Failed"), tr("Failed to download .NET runtime: %1").arg(errorMsg));
        m_installingDotNet = false;
        m_progressBar->setVisible(false);
        accept();
        return;
    }

    QString tempPath = QDir::tempPath() + "/windowsdesktop-runtime-8.0.24-win-x64.exe";
    QString prefixPath = m_result.installPath + "/proton-prefix";
    QDir().mkpath(prefixPath);

    QProcess *proc = new QProcess(this);

    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert("STEAM_COMPAT_DATA_PATH", prefixPath);
    env.insert("STEAM_COMPAT_CLIENT_INSTALL_PATH", m_selectedProton.path + "/../..");
    proc->setProcessEnvironment(env);

    m_statusLabel->setText(tr("Installing .NET Runtime..."));
    m_progressBar->setRange(0, 0);

    bool isFlatpakLauncher = QFileInfo("/app/bin/legacy-launcher").exists();
    QString actualProtonPath = m_selectedProton.path;

    if (isFlatpakLauncher && m_selectedProton.isFlatpak) {
        QString flatpakSteamPath = QDir::homePath() + "/.var/app/com.valvesoftware.Steam";
        if (m_selectedProton.path.startsWith(flatpakSteamPath)) {
            QString relativePath = m_selectedProton.path.mid(flatpakSteamPath.length());
            QStringList possibleHostPaths = {
                QDir::homePath() + "/.local/share/Steam" + relativePath,
                QDir::homePath() + "/.steam/steam" + relativePath,
                QDir::homePath() + "/.steam/root" + relativePath
            };
            for (const QString &hostPath : possibleHostPaths) {
                if (QFileInfo(hostPath).exists()) {
                    actualProtonPath = hostPath;
                    break;
                }
            }
            env.insert("STEAM_COMPAT_CLIENT_INSTALL_PATH", actualProtonPath + "/../..");
        }
    }

    if (m_selectedProton.isFlatpak && !isFlatpakLauncher) {
        proc->setProgram("flatpak");
        proc->setArguments({"run", "--command=" + m_selectedProton.protonExecutable,
                           "com.valvesoftware.Steam", "run", tempPath});
    } else if (isFlatpakLauncher) {
        QString wrapperPath = QDir::homePath() + "/.local/share/LegacyLauncher/dotnet-install-wrapper.sh";
        QFile wrapperFile(wrapperPath);
        if (wrapperFile.open(QIODevice::WriteOnly)) {
            QTextStream out(&wrapperFile);
            out << "#!/bin/bash\n";
            out << "export LD_LIBRARY_PATH=/usr/lib:/usr/lib64:/lib:/lib64:$LD_LIBRARY_PATH\n";
            out << "export STEAM_COMPAT_DATA_PATH=\"" << prefixPath << "\"\n";
            out << "export STEAM_COMPAT_CLIENT_INSTALL_PATH=\"" << actualProtonPath << "/../..\"\n";
            out << "exec \"" << actualProtonPath << "/proton\" run \"" << tempPath << "\" \"$@\"\n";
            wrapperFile.close();
            QProcess::execute("chmod", {"+x", wrapperPath});
        }
        proc->setProgram("flatpak-spawn");
        proc->setArguments({"--host", wrapperPath});
    } else {
        bool isAppImage = QFileInfo(QCoreApplication::applicationDirPath() + "/../Libs").exists() ||
                          QCoreApplication::applicationDirPath().contains(".AppImage");

        if (isAppImage) {
            QString wrapperPath = QDir::homePath() + "/.local/share/LegacyLauncher/appimage-dotnet-wrapper.sh";
            QFile wrapperFile(wrapperPath);
            if (wrapperFile.open(QIODevice::WriteOnly)) {
                QTextStream out(&wrapperFile);
                out << "#!/bin/bash\n";
                out << "export LD_LIBRARY_PATH=\"" << QCoreApplication::applicationDirPath() << "/../usr/lib:" << QCoreApplication::applicationDirPath() << "/usr/lib:$LD_LIBRARY_PATH\"\n";
                out << "export STEAM_COMPAT_DATA_PATH=\"" << prefixPath << "\"\n";
                out << "export STEAM_COMPAT_CLIENT_INSTALL_PATH=\"" << actualProtonPath << "/../..\"\n";
                out << "exec \"" << actualProtonPath << "/proton\" run \"" << tempPath << "\" \"$@\"\n";
                wrapperFile.close();
                QProcess::execute("chmod", {"+x", wrapperPath});
            }
            proc->setProgram(wrapperPath);
            proc->setArguments({});
        } else {
            proc->setProgram(actualProtonPath + "/proton");
            proc->setArguments({"run", tempPath});
        }
    }

    connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &AddInstanceDialog::onDotNetInstallFinished);
    proc->start();

    if (!proc->waitForStarted(3000)) {
        proc->deleteLater();
        QMessageBox::critical(this, tr("Installation Failed"), tr("Failed to start .NET runtime installer"));
        m_installingDotNet = false;
        m_progressBar->setVisible(false);
        accept();
    }
}

void AddInstanceDialog::onDotNetInstallFinished(int exitCode, QProcess::ExitStatus) {
    QProcess *proc = qobject_cast<QProcess *>(sender());
    proc->deleteLater();

    QString tempPath = QDir::tempPath() + "/windowsdesktop-runtime-8.0.24-win-x64.exe";
    QFile::remove(tempPath);

    m_installingDotNet = false;

    if (exitCode != 0) {
        qWarning() << ".NET runtime installer exited with code:" << exitCode;
    }

    createWeaveLoaderJson(m_result.installPath);
    m_statusLabel->setText(tr("Done!"));
    m_progressBar->setVisible(false);
    accept();
}
