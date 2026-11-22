#pragma once
#include <QString>
#include <QImage>
#include <QtPlugin>

class QObject;

class ICaptureForgePlugin {
public:
    virtual ~ICaptureForgePlugin() = default;
    virtual QString name() const = 0;
    virtual QString version() const = 0;
    virtual void initialize(QObject *host) = 0;
    virtual void shutdown() {}
};

#define ICaptureForgePlugin_iid "com.captureforge.ICaptureForgePlugin/1.0"
Q_DECLARE_INTERFACE(ICaptureForgePlugin, ICaptureForgePlugin_iid)

// Optional OCR capability. A plugin DLL can implement this interface only, or
// implement it together with ICaptureForgePlugin. Returning false lets the host
// fall back to the built-in Windows OCR provider.
class ICaptureForgeOcrPlugin {
public:
    virtual ~ICaptureForgeOcrPlugin() = default;
    virtual QString ocrProviderName() const = 0;
    virtual QString ocrProviderVersion() const = 0;
    virtual bool recognize(const QImage &image, QString &text, QString &error) = 0;
};

#define ICaptureForgeOcrPlugin_iid "com.captureforge.ICaptureForgeOcrPlugin/1.0"
Q_DECLARE_INTERFACE(ICaptureForgeOcrPlugin, ICaptureForgeOcrPlugin_iid)
