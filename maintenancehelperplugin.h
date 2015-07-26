#ifndef MAINTENANCEHELPER_H
#define MAINTENANCEHELPER_H

#include "maintenancehelper_global.h"

#include <extensionsystem/iplugin.h>

#include <qmljs/qmljsmodelmanagerinterface.h>

#include "projecttraceability.h"
#include "projectchanges.h"

using namespace QmlJS;

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
    void triggerGiveHelp();
    void analysisComplete(QString projectPath);

private:
    void maintenanceHelp(const QString &project);

    QMap<QString,QString> trackedProjectsMap;
    QMap<QString,ProjectTraceability*> projectTraceabilityMap;
    QMap<QString,ProjectChanges*> projectChangesMap;
    QMap<QString,QDateTime> lastAnalysisMap;
};

} // namespace Internal
} // namespace MaintenanceHelper

#endif // MAINTENANCEHELPER_H

