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
    void triggerStartAnalysis();

private:
    void namingAnalysis(const QString &projectPath);
    void loadLinks(const QString &projectPath);
    void saveLinks(const QString &projectPath);
    void scanDirectory(const QFileInfoList &fileInfoList, QStringList &sources, QStringList &tests);

    QMap<QString,QString> trackedProjects;
    QMultiHash<QString,QString> projectLinks;
};

} // namespace Internal
} // namespace MaintenanceHelper

#endif // MAINTENANCEHELPER_H

