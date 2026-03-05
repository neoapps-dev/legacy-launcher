#pragma once

#include "Types.h"
#include <QObject>
#include <QList>

class InstanceManager : public QObject {
    Q_OBJECT
public:
    explicit InstanceManager(QObject *parent = nullptr);

    QList<Instance> instances() const;
    Instance *findById(const QString &id);
    bool addInstance(const Instance &instance);
    bool updateInstance(const Instance &instance);
    bool removeInstance(const QString &id);
    void setLastRan(const QString &id);
    QString lastRanId() const;

signals:
    void instancesChanged();

private:
    QList<Instance> m_instances;
    QString m_lastRanId;
    QString m_configPath;

    void load();
    void save();
    void saveInstance(const Instance &inst);
    void saveConfig();
    QString generateId();
};
