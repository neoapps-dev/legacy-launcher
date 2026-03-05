#pragma once

#include "Types.h"
#include <QWidget>
#include <QLabel>
#include <QPushButton>

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

    void setupUi();
    void refresh();
};
