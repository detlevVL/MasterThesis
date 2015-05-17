#ifndef MAINTENANCEHELPER_H
#define MAINTENANCEHELPER_H

#include "maintenancehelper_global.h"

#include <extensionsystem/iplugin.h>

#include <QFileDialog>

namespace MaintenanceHelper {
namespace Internal {

class MaintenanceHelperPlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "MaintenanceHelper.json")

public:
    MaintenanceHelperPlugin();
    ~MaintenanceHelperPlugin();

    bool initialize(const QStringList &arguments, QString *errorString);
    void extensionsInitialized();
    ShutdownFlag aboutToShutdown();

private slots:
    void triggerStartTracking();

private:
    void scanDirectory(const QFileInfoList &fileInfoList);
    QStringList trackedProjs;
};

} // namespace Internal
} // namespace MaintenanceHelper

#endif // MAINTENANCEHELPER_H

