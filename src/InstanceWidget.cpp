#include "InstanceWidget.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFrame>
#include <QLabel>

InstanceWidget::InstanceWidget(const Instance &instance, QWidget *parent)
    : QWidget(parent)
    , m_instance(instance)
    , m_running(false)
    , m_updateAvailable(false)
{
    setupUi();
    refresh();
}

void InstanceWidget::setupUi() {
    QFrame *card = new QFrame(this);
    card->setFrameShape(QFrame::StyledPanel);
    card->setFrameShadow(QFrame::Raised);

    QHBoxLayout *cardLayout = new QHBoxLayout(card);
    cardLayout->setContentsMargins(12, 8, 12, 8);
    cardLayout->setSpacing(12);

    QVBoxLayout *infoLayout = new QVBoxLayout();
    infoLayout->setSpacing(2);

    m_nameLabel = new QLabel();
    QFont nameFont = m_nameLabel->font();
    nameFont.setBold(true);
    m_nameLabel->setFont(nameFont);
    infoLayout->addWidget(m_nameLabel);

    m_versionLabel = new QLabel();
    QFont versionFont = m_versionLabel->font();
    versionFont.setPointSize(versionFont.pointSize() - 1);
    m_versionLabel->setFont(versionFont);
    m_versionLabel->setEnabled(false);
    infoLayout->addWidget(m_versionLabel);

    m_lastRunLabel = new QLabel();
    QFont lastRunFont = m_lastRunLabel->font();
    lastRunFont.setPointSize(lastRunFont.pointSize() - 1);
    m_lastRunLabel->setFont(lastRunFont);
    m_lastRunLabel->setEnabled(false);
    infoLayout->addWidget(m_lastRunLabel);

    cardLayout->addLayout(infoLayout, 1);

    m_statusBadge = new QLabel();
    m_statusBadge->setAlignment(Qt::AlignCenter);
    m_statusBadge->setVisible(false);
    cardLayout->addWidget(m_statusBadge);

    QHBoxLayout *actionsLayout = new QHBoxLayout();
    actionsLayout->setSpacing(4);

    m_updateBtn = new QPushButton(tr("Update"));
    m_updateBtn->setVisible(false);
    actionsLayout->addWidget(m_updateBtn);

    m_settingsBtn = new QPushButton(tr("Settings"));
    m_settingsBtn->setAutoDefault(false);
    actionsLayout->addWidget(m_settingsBtn);

    m_deleteBtn = new QPushButton(tr("Delete"));
    m_deleteBtn->setAutoDefault(false);
    actionsLayout->addWidget(m_deleteBtn);

    m_launchBtn = new QPushButton(tr("Launch"));
    m_launchBtn->setSizePolicy(QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Fixed);
    actionsLayout->addWidget(m_launchBtn);

    cardLayout->addLayout(actionsLayout);

    QVBoxLayout *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(8, 4, 8, 4);
    rootLayout->addWidget(card);

    connect(m_launchBtn, &QPushButton::clicked, this, [this]() {
        if (m_running) emit stopRequested(m_instance.id);
        else emit launchRequested(m_instance.id);
    });
    connect(m_settingsBtn, &QPushButton::clicked, this, [this]() {
        emit settingsRequested(m_instance.id);
    });
    connect(m_updateBtn, &QPushButton::clicked, this, [this]() {
        emit updateRequested(m_instance.id);
    });
    connect(m_deleteBtn, &QPushButton::clicked, this, [this]() {
        emit deleteRequested(m_instance.id);
    });
}

void InstanceWidget::setInstance(const Instance &instance) {
    m_instance = instance;
    refresh();
}

void InstanceWidget::setRunning(bool running) {
    m_running = running;
    refresh();
}

void InstanceWidget::setUpdateAvailable(bool available, const QString &newTag) {
    m_updateAvailable = available;
    m_latestTag = newTag;
    m_updateBtn->setVisible(available);
    refresh();
}

QString InstanceWidget::instanceId() const {
    return m_instance.id;
}

void InstanceWidget::refresh() {
    m_nameLabel->setText(m_instance.name);
    m_versionLabel->setText(tr("Version: %1").arg(
        m_instance.installedTag.isEmpty() ? tr("Unknown") : m_instance.installedTag));

    if (m_instance.lastRun.isValid()) {
        m_lastRunLabel->setText(tr("Last run: %1").arg(
            m_instance.lastRun.toLocalTime().toString("yyyy-MM-dd HH:mm")));
        m_lastRunLabel->setVisible(true);
    } else {
        m_lastRunLabel->setVisible(false);
    }

    if (m_running) {
        m_statusBadge->setText(tr("Running"));
        m_statusBadge->setVisible(true);
        m_launchBtn->setText(tr("Stop"));
        m_settingsBtn->setEnabled(false);
        m_deleteBtn->setEnabled(false);
    } else {
        m_statusBadge->setVisible(false);
        m_launchBtn->setText(tr("Launch"));
        m_settingsBtn->setEnabled(true);
        m_deleteBtn->setEnabled(true);
    }

    if (m_updateAvailable && !m_latestTag.isEmpty()) {
        m_updateBtn->setText(tr("Update to %1").arg(m_latestTag));
    }
}
