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
#include <QInputDialog>
#include <QMessageBox>
#include <QMainWindow>
#include <QMenu>
#include <QPlainTextEdit>

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

    // Tracked projects settings
    foreach (const QString &key, settings.allKeys()) {
        if (key.endsWith(QString::fromStdString(".pro"))) {
            trackedProjectsMap.insert(key, settings.value(key).toString());
        }
    }

    // Add actions to menus
    QAction *startTracking = new QAction(tr("Start Tracking Project"), this);
    Core::Command *startTrackingCmd = Core::ActionManager::registerAction(startTracking, Constants::STARTTRACKING_ID,
                                                             Core::Context(Core::Constants::C_GLOBAL));
    connect(startTracking, SIGNAL(triggered()), this, SLOT(triggerStartTracking()));

    QAction *startAnalysis = new QAction(tr("Do Project Analysis"), this);
    Core::Command *startAnalysisCmd = Core::ActionManager::registerAction(startAnalysis, Constants::STARTANALYSIS_ID,
                                                             Core::Context(Core::Constants::C_GLOBAL));
    connect(startAnalysis, SIGNAL(triggered()), this, SLOT(triggerStartAnalysis()));

    QAction *giveHelp = new QAction(tr("Give Maintenance Help"), this);
    Core::Command *giveHelpCmd = Core::ActionManager::registerAction(giveHelp, Constants::GIVEHELP_ID,
                                                             Core::Context(Core::Constants::C_GLOBAL));
    connect(giveHelp, SIGNAL(triggered()), this, SLOT(triggerGiveHelp()));

    Core::ActionContainer *menu = Core::ActionManager::createMenu(Constants::MENU_ID);
    menu->menu()->setTitle(tr("MaintenanceHelper"));
    menu->addAction(startTrackingCmd);
    menu->addAction(startAnalysisCmd);
    menu->addAction(giveHelpCmd);
    Core::ActionManager::actionContainer(Core::Constants::M_TOOLS)->addMenu(menu);

    return true;
}

void MaintenanceHelperPlugin::extensionsInitialized()
{
    // Retrieve objects from the plugin manager's object pool
    // In the extensionsInitialized function, a plugin can be sure that all
    // plugins that depend on it are completely initialized.

    // fill the projectChanges and projectTraceabilities maps
    foreach (const QString &project, trackedProjectsMap.keys()) {
        projectChangesMap.insert(project, new ProjectChanges(trackedProjectsMap.value(project)));
        projectTraceabilityMap.insert(project, new ProjectTraceability(trackedProjectsMap.value(project)));
    }
}

ExtensionSystem::IPlugin::ShutdownFlag MaintenanceHelperPlugin::aboutToShutdown()
{
    // Save settings
    // Disconnect from signals that are not needed during shutdown
    // Hide UI (if you add UI that is not in the main window directly)

    // for each tracked project, save the file changes and disconnect from qmljs signals
    foreach (const QString &project, trackedProjectsMap.keys()) {
        projectChangesMap.value(project)->saveFileChanges();
        disconnect(ModelManagerInterface::instance(), 0, projectChangesMap.value(project), 0);
    }

    return SynchronousShutdown;
}

void MaintenanceHelperPlugin::triggerStartTracking()
{
    // select project to track
    QString fileName = QFileDialog::getOpenFileName(Core::ICore::mainWindow(), tr("Select Project File"),
                                              QDir::homePath(),
                                              tr("Project Files (*.pro)"));

    if (!fileName.isEmpty()) {
        QString project = fileName.section(QString::fromStdString("/"), -1);
        QString projectPath = fileName.section(QString::fromStdString("/"), 0, -2);

        if (trackedProjectsMap.contains(project)) {
            // already tracking project
            QMessageBox::warning(Core::ICore::mainWindow(),
                                 tr("Tracking failed"),
                                 tr("This project is already being tracked"));
        } else {
            // add to tracked projects
            QSettings settings(QString::fromStdString("Detlev"), QString::fromStdString("MaintenanceHelper"));
            settings.setValue(project, projectPath);
            trackedProjectsMap.insert(project, projectPath);
            projectChangesMap.insert(project, new ProjectChanges(projectPath));
            projectTraceabilityMap.insert(project, new ProjectTraceability(projectPath));
            QMessageBox::information(Core::ICore::mainWindow(),
                                     tr("Tracking started"),
                                     tr("Now tracking: ").append(project));
        }
    }
}

void MaintenanceHelperPlugin::triggerStartAnalysis()
{
    // select project to analyse
    QStringList projects = trackedProjectsMap.keys();

    if (projects.isEmpty()) {
        QMessageBox::warning(Core::ICore::mainWindow(),
                             tr("Analysis failed"),
                             tr("There are no tracked projects to analyse"));
    } else {
        bool ok;
        QString project = QInputDialog::getItem(Core::ICore::mainWindow(), tr("Project Analysis"),
                                             tr("Select Project"), projects, 0, false, &ok);

        if (ok && !project.isEmpty()) {
            connect(projectTraceabilityMap.value(project), SIGNAL(linkCreationComplete(QString)), this, SLOT(analysisComplete(QString)));
            projectTraceabilityMap.value(project)->createLinks();
        }
    }
}

void MaintenanceHelperPlugin::analysisComplete(QString projectPath)
{
    QString project = trackedProjectsMap.key(projectPath);

    QMessageBox::information(Core::ICore::mainWindow(),
                             tr("Analysis complete"),
                             tr("Completed analysis: ").append(project));

    disconnect(projectTraceabilityMap.value(project), SIGNAL(linkCreationComplete(QString)), this, SLOT(analysisComplete(QString)));
}

void MaintenanceHelperPlugin::triggerGiveHelp()
{
    // select project to give help for
    QStringList projects = trackedProjectsMap.keys();

    if (projects.isEmpty()) {
        QMessageBox::warning(Core::ICore::mainWindow(),
                             tr("Giving help failed"),
                             tr("There are no tracked projects to give help for"));
    } else {
        bool ok;
        QString project = QInputDialog::getItem(Core::ICore::mainWindow(), tr("Maintenance Help"),
                                             tr("Select Project"), projects, 0, false, &ok);
        if (ok && !project.isEmpty()) {
            maintenanceHelp(project);
        }
    }
}

void MaintenanceHelperPlugin::maintenanceHelp(const QString &project)
{
    projectTraceabilityMap.value(project)->loadLinks();
    QString theHelp;

    foreach (const QString &file, projectChangesMap.value(project)->getModifiedFiles()) {
        QString baseName = file.section(QString::fromStdString("/"), -1);

        if (!baseName.startsWith(QString::fromStdString("tst_"))) {
            theHelp.append(QString::fromStdString("Modified "));
            theHelp.append(baseName);
            theHelp.append(QString::fromStdString(", check:\n"));

            foreach (const QString &test, projectTraceabilityMap.value(project)->values(file)) {
                theHelp.append(QString::fromStdString("   "));
                theHelp.append(test);
                theHelp.append(QString::fromStdString("\n"));
            }

            theHelp.append(QString::fromStdString("\n"));
        }
    }

    foreach (const QString &file, projectChangesMap.value(project)->getDeletedFiles()) {
        QString baseName = file.section(QString::fromStdString("/"), -1);

        if (!baseName.startsWith(QString::fromStdString("tst_"))) {
            theHelp.append(QString::fromStdString("Deleted "));
            theHelp.append(baseName);
            theHelp.append(QString::fromStdString(", check:\n"));

            foreach (const QString &test, projectTraceabilityMap.value(project)->values(file)) {
                theHelp.append(QString::fromStdString("   "));
                theHelp.append(test);
                theHelp.append(QString::fromStdString("\n"));
            }

            theHelp.append(QString::fromStdString("\n"));
        }
    }

    QPlainTextEdit *text = new QPlainTextEdit(theHelp);
    text->setReadOnly(true);
    text->setWindowTitle(QString::fromStdString("Maintenance Help"));
    text->show();
}

