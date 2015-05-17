#include "maintenancehelperplugin.h"
#include "maintenancehelperconstants.h"

#include <coreplugin/icore.h>
#include <coreplugin/icontext.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/coreconstants.h>

#include <QAction>
#include <QFileDialog>
#include <QMessageBox>
#include <QMainWindow>
#include <QMenu>

#include <QtPlugin>

#include <iostream>
using namespace std;

using namespace MaintenanceHelper::Internal;

MaintenanceHelperPlugin::MaintenanceHelperPlugin()
{
    // Create your members
}

MaintenanceHelperPlugin::~MaintenanceHelperPlugin()
{
    // Unregister objects from the plugin manager's object pool
    // Delete members
}

bool MaintenanceHelperPlugin::initialize(const QStringList &arguments, QString *errorString)
{
    // Register objects in the plugin manager's object pool
    // Load settings
    // Add actions to menus
    // Connect to other plugins' signals
    // In the initialize function, a plugin can be sure that the plugins it
    // depends on have initialized their members.

    Q_UNUSED(arguments)
    Q_UNUSED(errorString)

    // Load settings
    QSettings settings(QString::fromStdString("Detlev"), QString::fromStdString("MaintenanceHelper"));

    // Tracked projects
    QStringList allKeys = settings.allKeys();
    for (int i = 0; i < allKeys.size(); i++) {
        if (allKeys.at(i).endsWith(QString::fromStdString(".pro"))) {
            trackedProjs.append(settings.value(allKeys.at(i)).toString());
        }
    }

    // Add actions to menus
    QAction *startTracking = new QAction(tr("Start Tracking Project"), this);
    Core::Command *cmd = Core::ActionManager::registerAction(startTracking, Constants::STARTTRACKING_ID,
                                                             Core::Context(Core::Constants::C_GLOBAL));
    connect(startTracking, SIGNAL(triggered()), this, SLOT(triggerStartTracking()));

    Core::ActionContainer *menu = Core::ActionManager::createMenu(Constants::MENU_ID);
    menu->menu()->setTitle(tr("MaintenanceHelper"));
    menu->addAction(cmd);
    Core::ActionManager::actionContainer(Core::Constants::M_TOOLS)->addMenu(menu);

    return true;
}

void MaintenanceHelperPlugin::extensionsInitialized()
{
    // Retrieve objects from the plugin manager's object pool
    // In the extensionsInitialized function, a plugin can be sure that all
    // plugins that depend on it are completely initialized.
}

ExtensionSystem::IPlugin::ShutdownFlag MaintenanceHelperPlugin::aboutToShutdown()
{
    // Save settings
    // Disconnect from signals that are not needed during shutdown
    // Hide UI (if you add UI that is not in the main window directly)
    return SynchronousShutdown;
}

void MaintenanceHelperPlugin::triggerStartTracking()
{
    QString fileName = QFileDialog::getOpenFileName(Core::ICore::mainWindow(), tr("Open File"),
                                              tr(""),
                                              tr("Project Files (*.pro)"));

    if (fileName.isEmpty()) {
        // no file selected
        return;
    }

    QString project = fileName.section(QString::fromStdString("/"), -1);
    QString projectPath = fileName.section(QString::fromStdString("/"), 0, -2);

    if (trackedProjs.contains(projectPath)) {
        // already tracking project
        QMessageBox::warning(Core::ICore::mainWindow(),
                             tr("Tracking failed"),
                             tr("This project is already being tracked"));
        return;
    }

    QFileInfoList fileInfoList = QDir(projectPath).entryInfoList(QStringList(QString::fromStdString("*.qml")),
                                                                 QDir::NoDotAndDotDot | QDir::Files | QDir::AllDirs,
                                                                 QDir::DirsLast);
    scanDirectory(fileInfoList);

    // add to tracked projects
    QSettings settings(QString::fromStdString("Detlev"), QString::fromStdString("MaintenanceHelper"));
    settings.setValue(project, projectPath);
    trackedProjs.append(projectPath);

    // TODO
    /*
    QMessageBox::information(Core::ICore::mainWindow(),
                             tr("Tracking started"),
                             tr("Found X source files and X test files"));
    */
}

void MaintenanceHelperPlugin::scanDirectory(const QFileInfoList &fileInfoList)
{
    for (int i = 0; i < fileInfoList.size(); i++) {
        if (fileInfoList.at(i).isDir()) {
            // scan subdirectory
            QDir subDir = QDir(fileInfoList.at(i).absoluteFilePath());
            QFileInfoList subFileInfoList = subDir.entryInfoList(QStringList(QString::fromStdString("*.qml")),
                                                                 QDir::NoDotAndDotDot | QDir::Files | QDir::AllDirs,
                                                                 QDir::DirsLast);
            scanDirectory(subFileInfoList);
        } else {
            // TODO: add to source/test files list
            QString fileName = fileInfoList.at(i).baseName();

            if (fileName.startsWith(QString::fromStdString("tst_"))) {
                cout << "TEST:   ";
            } else {
                cout << "SOURCE: ";
            }

            cout << fileName.toStdString() << endl;
        }
    }
}


