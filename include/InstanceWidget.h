#pragma once

#include "Types.h"
#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QGraphicsDropShadowEffect>
#include <QPropertyAnimation>
#include <QFrame>
#include <QVBoxLayout>

class InstanceWidget : public QWidget {
    Q_OBJECT
public:
    explicit InstanceWidget(const Instance &instance, QWidget *parent = nullptr);

    void setInstance(const Instance &instance);
    void setRunning(bool running);
    void setUpdateAvailable(bool available, const QString &newTag = QString());

    QString instanceId() const;

signals:
    void launchRequested(QString id);
    void stopRequested(QString id);
    void settingsRequested(QString id);
    void updateRequested(QString id);
    void deleteRequested(QString id);

protected:
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    Instance m_instance;
    bool m_running;
    bool m_updateAvailable;
    QString m_latestTag;

    QLabel *m_nameLabel;
    QLabel *m_versionLabel;
    QLabel *m_lastRunLabel;
    QLabel *m_statusBadge;
    QPushButton *m_launchBtn;
    QPushButton *m_settingsBtn;
    QPushButton *m_updateBtn;
    QPushButton *m_deleteBtn;

    QGraphicsDropShadowEffect *m_shadow;
    QPropertyAnimation *m_blurAnim;
    QPropertyAnimation *m_offsetAnim;
    QFrame *m_card;
    QVBoxLayout *m_rootLayout;

    void setupUi();
    void refresh();
};
