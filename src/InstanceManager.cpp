#include "InstanceManager.h"

#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QStandardPaths>
#include <QUuid>

InstanceManager::InstanceManager(QObject *parent)
    : QObject(parent)
{
    QString configDir = QDir::homePath() + "/.local/share/LegacyLauncher";
    QDir().mkpath(configDir);
    m_configPath = configDir + "/config.json";
    load();
}

QList<Instance> InstanceManager::instances() const {
    return m_instances;
}

Instance *InstanceManager::findById(const QString &id) {
    for (auto &inst : m_instances) {
        if (inst.id == id) return &inst;
    }
    return nullptr;
}

bool InstanceManager::addInstance(const Instance &instance) {
    m_instances.append(instance);
    saveInstance(instance);
    emit instancesChanged();
    return true;
}

bool InstanceManager::updateInstance(const Instance &instance) {
    for (auto &inst : m_instances) {
        if (inst.id == instance.id) {
            inst = instance;
            saveInstance(instance);
            emit instancesChanged();
            return true;
        }
    }
    return false;
}

bool InstanceManager::removeInstance(const QString &id) {
    for (int i = 0; i < m_instances.size(); ++i) {
        if (m_instances[i].id == id) {
            QString installPath = m_instances[i].installPath;
            m_instances.removeAt(i);
            if (m_lastRanId == id) m_lastRanId.clear();
            
            QFile::remove(installPath + "/instance.json");
            saveConfig();
            emit instancesChanged();
            return true;
        }
    }
    return false;
}

void InstanceManager::setLastRan(const QString &id) {
    m_lastRanId = id;
    if (Instance *inst = findById(id)) {
        inst->lastRun = QDateTime::currentDateTime();
        saveInstance(*inst);
    }
    saveConfig();
}

QString InstanceManager::lastRanId() const {
    return m_lastRanId;
}

void InstanceManager::load() {
    QFile f(m_configPath);
    if (f.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
        QJsonObject root = doc.object();
        m_lastRanId = root["lastRanId"].toString();
    }

    QString instancesDir = QDir::homePath() + "/.local/share/LegacyLauncher/instances";
    QDir dir(instancesDir);
    if (!dir.exists()) {
        QDir().mkpath(instancesDir);
        return;
    }

    QFileInfoList entries = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QFileInfo &entry : entries) {
        QString instanceJson = entry.absoluteFilePath() + "/instance.json";
        QFile f(instanceJson);
        if (f.open(QIODevice::ReadOnly)) {
            QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
            QJsonObject obj = doc.object();
            Instance inst;
            inst.id = obj["id"].toString();
            inst.name = obj["name"].toString();
            inst.installPath = entry.absoluteFilePath();
            inst.protonId = obj["protonId"].toString();
            inst.installedTag = obj["installedTag"].toString();
            inst.installedAt = QDateTime::fromString(obj["installedAt"].toString(), Qt::ISODate);
            inst.username = obj["username"].toString();
            inst.headlessMode = obj["headlessMode"].toBool(false);
            inst.autoIp = obj["autoIp"].toString();
            inst.autoPort = obj["autoPort"].toInt(0);
            inst.lastRun = QDateTime::fromString(obj["lastRun"].toString(), Qt::ISODate);
            inst.weaveLoaderEnabled = obj["weaveLoaderEnabled"].toBool(false);
            inst.weaveLoaderTag = obj["weaveLoaderTag"].toString();
            inst.weaveLoaderInstalledAt = QDateTime::fromString(obj["weaveLoaderInstalledAt"].toString(), Qt::ISODate);
            m_instances.append(inst);
        }
    }
}

void InstanceManager::saveInstance(const Instance &inst) {
    QString instanceJson = inst.installPath + "/instance.json";
    QJsonObject obj;
    obj["id"] = inst.id;
    obj["name"] = inst.name;
    obj["installPath"] = inst.installPath;
    obj["protonId"] = inst.protonId;
    obj["installedTag"] = inst.installedTag;
    obj["installedAt"] = inst.installedAt.toString(Qt::ISODate);
    obj["username"] = inst.username;
    obj["headlessMode"] = inst.headlessMode;
    obj["autoIp"] = inst.autoIp;
    obj["autoPort"] = inst.autoPort;
    obj["lastRun"] = inst.lastRun.toString(Qt::ISODate);
    obj["weaveLoaderEnabled"] = inst.weaveLoaderEnabled;
    obj["weaveLoaderTag"] = inst.weaveLoaderTag;
    obj["weaveLoaderInstalledAt"] = inst.weaveLoaderInstalledAt.toString(Qt::ISODate);

    QFile f(instanceJson);
    if (f.open(QIODevice::WriteOnly)) {
        f.write(QJsonDocument(obj).toJson());
    }
}

void InstanceManager::saveConfig() {
    QJsonObject root;
    root["lastRanId"] = m_lastRanId;

    QFile f(m_configPath);
    if (f.open(QIODevice::WriteOnly)) {
        f.write(QJsonDocument(root).toJson());
    }
}

void InstanceManager::save() {
    for (const Instance &inst : m_instances) {
        saveInstance(inst);
    }
    saveConfig();
}

QString InstanceManager::generateId() {
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}
