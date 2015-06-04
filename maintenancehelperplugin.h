#ifndef MAINTENANCEHELPER_H
#define MAINTENANCEHELPER_H

#include "maintenancehelper_global.h"

#include <extensionsystem/iplugin.h>

#include <qmljs/qmljsmodelmanagerinterface.h>

#include <QFileDialog>

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
    void fileModified(QmlJS::Document::Ptr doc);
    void fileDeleted(const QStringList &files);

private:
    void namingAnalysis(const QString &projectPath);
    void loadLinks(const QString &projectPath);
    void saveLinks(const QString &projectPath);
    void scanDirectory(const QFileInfoList &fileInfoList, QStringList &sources, QStringList &tests);
    void loadFileChanges();
    void saveFileChanges();
    void maintenanceHelp(const QString &projectPath);

    QMap<QString,QString> trackedProjects;
    QMultiHash<QString,QString> projectLinks;
    QMultiHash<QString,QString> modifiedFiles;
    QMultiHash<QString,QString> deletedFiles;
};

} // namespace Internal
} // namespace MaintenanceHelper

#endif // MAINTENANCEHELPER_H

