#pragma once
#include <QObject>
#include <QStringList>
#include <QList>

class QPluginLoader;
class ICaptureForgeOcrPlugin;

class PluginManager : public QObject {
    Q_OBJECT
public:
    explicit PluginManager(QObject *host, QObject *parent = nullptr);
    ~PluginManager() override;

    void reload();
    QString pluginDirectory() const;
    QStringList loadedDescriptions() const;
    QStringList loadErrors() const;
    QList<ICaptureForgeOcrPlugin*> ocrPlugins() const;

private:
    void unloadAll();

    QObject *hostObject = nullptr;
    QList<QPluginLoader*> loaders;
    QList<ICaptureForgeOcrPlugin*> ocrProviders;
    QStringList descriptions;
    QStringList errors;
};
