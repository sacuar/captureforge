#include "PluginManager.h"
#include "PluginInterface.h"
#include <QPluginLoader>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfoList>
#include <QLibrary>
#include <QStandardPaths>

PluginManager::PluginManager(QObject *host, QObject *parent)
    : QObject(parent), hostObject(host) {}

PluginManager::~PluginManager() {
    unloadAll();
}

QString PluginManager::pluginDirectory() const {
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/addons";
}

void PluginManager::unloadAll() {
    for (QPluginLoader *loader : loaders) {
        if (QObject *instance = loader->instance()) {
            if (auto *plugin = qobject_cast<ICaptureForgePlugin*>(instance))
                plugin->shutdown();
        }
        loader->unload();
        delete loader;
    }
    loaders.clear();
    ocrProviders.clear();
    descriptions.clear();
    errors.clear();
}

void PluginManager::reload() {
    unloadAll();

    QDir dir(pluginDirectory());
    if (!dir.exists())
        dir.mkpath(".");

    const QFileInfoList files = dir.entryInfoList(QDir::Files | QDir::Readable, QDir::Name);
    for (const QFileInfo &fi : files) {
        if (!QLibrary::isLibrary(fi.fileName()))
            continue;

        auto *loader = new QPluginLoader(fi.absoluteFilePath());
        QObject *instance = loader->instance();
        if (!instance) {
            errors << QString("%1: %2").arg(fi.fileName(), loader->errorString());
            delete loader;
            continue;
        }

        auto *general = qobject_cast<ICaptureForgePlugin*>(instance);
        auto *ocr = qobject_cast<ICaptureForgeOcrPlugin*>(instance);
        if (!general && !ocr) {
            errors << QString("%1: not a CaptureForge plugin").arg(fi.fileName());
            loader->unload();
            delete loader;
            continue;
        }

        if (general) {
            general->initialize(hostObject);
            descriptions << QString("%1 %2").arg(general->name(), general->version());
        }
        if (ocr) {
            ocrProviders << ocr;
            const QString desc = QString("OCR: %1 %2").arg(ocr->ocrProviderName(), ocr->ocrProviderVersion());
            if (!descriptions.contains(desc))
                descriptions << desc;
        }
        loaders << loader;
    }
}

QStringList PluginManager::loadedDescriptions() const {
    return descriptions;
}

QStringList PluginManager::loadErrors() const {
    return errors;
}

QList<ICaptureForgeOcrPlugin*> PluginManager::ocrPlugins() const {
    return ocrProviders;
}
