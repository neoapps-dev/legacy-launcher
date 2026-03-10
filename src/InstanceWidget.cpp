#include "InstanceWidget.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFrame>
#include <QLabel>
#include <QGraphicsDropShadowEffect>

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
    m_card = new QFrame(this);
    m_card->setObjectName("instanceCard");
    
    m_shadow = new QGraphicsDropShadowEffect(m_card);
    m_shadow->setBlurRadius(15);
    m_shadow->setXOffset(0);
    m_shadow->setYOffset(4);
    m_shadow->setColor(QColor(0, 0, 0, 80));
    m_card->setGraphicsEffect(m_shadow);

    m_blurAnim = new QPropertyAnimation(m_shadow, "blurRadius", this);
    m_blurAnim->setDuration(200);
    
    m_offsetAnim = new QPropertyAnimation(m_shadow, "yOffset", this);
    m_offsetAnim->setDuration(200);

    QHBoxLayout *cardLayout = new QHBoxLayout(m_card);
    cardLayout->setContentsMargins(20, 16, 20, 16);
    cardLayout->setSpacing(16);

    QVBoxLayout *infoLayout = new QVBoxLayout();
    infoLayout->setSpacing(2);

    m_nameLabel = new QLabel();
    m_nameLabel->setObjectName("instanceName");
    infoLayout->addWidget(m_nameLabel);

    m_versionLabel = new QLabel();
    m_versionLabel->setObjectName("instanceVersion");
    infoLayout->addWidget(m_versionLabel);

    m_lastRunLabel = new QLabel();
    m_lastRunLabel->setObjectName("instanceLastRun");
    infoLayout->addWidget(m_lastRunLabel);

    cardLayout->addLayout(infoLayout, 1);

    m_statusBadge = new QLabel();
    m_statusBadge->setObjectName("statusBadge");
    m_statusBadge->setAlignment(Qt::AlignCenter);
    m_statusBadge->setVisible(false);
    cardLayout->addWidget(m_statusBadge);

    QHBoxLayout *actionsLayout = new QHBoxLayout();
    actionsLayout->setSpacing(4);

    m_updateBtn = new QPushButton(tr("Update"));
    m_updateBtn->setObjectName("updateBtn");
    m_updateBtn->setCursor(Qt::PointingHandCursor);
    actionsLayout->addWidget(m_updateBtn);

    m_settingsBtn = new QPushButton(tr("Settings"));
    m_settingsBtn->setObjectName("settingsBtn");
    m_settingsBtn->setCursor(Qt::PointingHandCursor);
    actionsLayout->addWidget(m_settingsBtn);

    m_deleteBtn = new QPushButton(tr("Delete"));
    m_deleteBtn->setObjectName("deleteBtn");
    m_deleteBtn->setCursor(Qt::PointingHandCursor);
    actionsLayout->addWidget(m_deleteBtn);

    m_launchBtn = new QPushButton(tr("Launch"));
    m_launchBtn->setObjectName("launchBtn");
    m_launchBtn->setCursor(Qt::PointingHandCursor);
    m_launchBtn->setMinimumWidth(100);
    actionsLayout->addWidget(m_launchBtn);

    cardLayout->addLayout(actionsLayout);

    m_rootLayout = new QVBoxLayout(this);
    m_rootLayout->setContentsMargins(8, 4, 8, 4);
    m_rootLayout->addWidget(m_card);

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

void InstanceWidget::enterEvent(QEnterEvent *event) {
    m_blurAnim->stop();
    m_blurAnim->setEndValue(25);
    m_blurAnim->start();

    m_offsetAnim->stop();
    m_offsetAnim->setEndValue(8);
    m_offsetAnim->start();
    
    QWidget::enterEvent(event);
}

void InstanceWidget::leaveEvent(QEvent *event) {
    m_blurAnim->stop();
    m_blurAnim->setEndValue(15);
    m_blurAnim->start();

    m_offsetAnim->stop();
    m_offsetAnim->setEndValue(4);
    m_offsetAnim->start();

    QWidget::leaveEvent(event);
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
        m_updateBtn->setEnabled(false);
    } else {
        m_statusBadge->setVisible(false);
        m_launchBtn->setText(tr("Launch"));
        m_settingsBtn->setEnabled(true);
        m_deleteBtn->setEnabled(true);
        m_updateBtn->setEnabled(true);
    }

    if (m_updateAvailable && !m_latestTag.isEmpty()) {
        m_updateBtn->setText(tr("\xe2\xac\x86 Update to %1").arg(m_latestTag));
        m_updateBtn->setStyleSheet("color: #4ecdc4; border-color: #4ecdc4;");
    } else {
        m_updateBtn->setText(tr("Change Version"));
        m_updateBtn->setStyleSheet("");
    }
}
