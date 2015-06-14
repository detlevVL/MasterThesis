#include "maintenancehelperplugin.h"
#include "maintenancehelperconstants.h"

#include <coreplugin/icore.h>
#include <coreplugin/icontext.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/coreconstants.h>

#include <QAction>
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
            trackedProjects.insert(key, settings.value(key).toString());
            projectTraceabilities.insert(key, new ProjectTraceability(settings.value(key).toString()));
        }
    }

    // Load file changes
    loadFileChanges();

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

    // connect to qmljs library signals
    connect(ModelManagerInterface::instance(), SIGNAL(documentChangedOnDisk(QmlJS::Document::Ptr)), this, SLOT(fileModified(QmlJS::Document::Ptr)));
    connect(ModelManagerInterface::instance(), SIGNAL(aboutToRemoveFiles(QStringList)), this, SLOT(fileDeleted(QStringList)));
}

ExtensionSystem::IPlugin::ShutdownFlag MaintenanceHelperPlugin::aboutToShutdown()
{
    // Save settings
    // Disconnect from signals that are not needed during shutdown
    // Hide UI (if you add UI that is not in the main window directly)

    // Save file changes
    saveFileChanges();

    // Disconnect from qmljs library signals
    disconnect(ModelManagerInterface::instance(), 0, this, 0);

    return SynchronousShutdown;
}

void MaintenanceHelperPlugin::triggerStartTracking()
{
    // select project to track
    QString fileName = QFileDialog::getOpenFileName(Core::ICore::mainWindow(), tr("Open File"),
                                              tr(""),
                                              tr("Project Files (*.pro)"));

    if (!fileName.isEmpty()) {
        QString project = fileName.section(QString::fromStdString("/"), -1);
        QString projectPath = fileName.section(QString::fromStdString("/"), 0, -2);

        if (trackedProjects.contains(project)) {
            // already tracking project
            QMessageBox::warning(Core::ICore::mainWindow(),
                                 tr("Tracking failed"),
                                 tr("This project is already being tracked"));
        } else {
            // add to tracked projects
            QSettings settings(QString::fromStdString("Detlev"), QString::fromStdString("MaintenanceHelper"));
            settings.setValue(project, projectPath);
            trackedProjects.insert(project, projectPath);
            projectTraceabilities.insert(project, new ProjectTraceability(projectPath));
            QMessageBox::information(Core::ICore::mainWindow(),
                                     tr("Tracking started"),
                                     tr("Now tracking: ").append(project));
        }
    }
}

void MaintenanceHelperPlugin::triggerStartAnalysis()
{
    // select project to analyse
    QStringList projects = trackedProjects.keys();

    if (projects.isEmpty()) {
        QMessageBox::warning(Core::ICore::mainWindow(),
                             tr("Analysis failed"),
                             tr("There are no tracked projects to analyse"));
    } else {
        bool ok;
        QString project = QInputDialog::getItem(Core::ICore::mainWindow(), tr("Project Analysis"),
                                             tr("Select Project"), projects, 0, false, &ok);

        if (ok && !project.isEmpty()) {
            projectTraceabilities.value(project)->createLinks();

            QMessageBox::information(Core::ICore::mainWindow(),
                                     tr("Analysis complete"),
                                     tr("Completed analysis: ").append(project));
        }
    }
}

void MaintenanceHelperPlugin::triggerGiveHelp()
{
    // select project to give help for
    QStringList projects = trackedProjects.keys();

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

void MaintenanceHelperPlugin::fileModified(QmlJS::Document::Ptr doc)
{
    foreach (const QString &projectPath, trackedProjects.values()) {
        if (doc->path().contains(projectPath) && !modifiedFiles.contains(projectPath, doc->fileName())) {
            modifiedFiles.insert(projectPath, doc->fileName());
        }
    }
}

void MaintenanceHelperPlugin::fileDeleted(const QStringList &files)
{
    foreach (const QString &file, files) {
        foreach (const QString &projectPath, trackedProjects.values()) {
            if (file.contains(projectPath) && !deletedFiles.contains(projectPath, file)) {
                modifiedFiles.remove(projectPath, file);
                deletedFiles.insert(projectPath, file);
            }
        }
    }
}

void MaintenanceHelperPlugin::loadFileChanges()
{
    modifiedFiles.clear();
    deletedFiles.clear();

    foreach (const QString &projectPath, trackedProjects.values()) {
        QFile changesFile(projectPath + QString::fromStdString("/MHchanges.txt"));

        if (changesFile.exists()) {
            changesFile.open(QFile::ReadOnly | QFile::Text);
            QTextStream in(&changesFile);

            QString line = in.readLine();
            while (!line.isNull()) {
                QString change = line.section(QString::fromStdString(":"), 0, 0);
                QString files = line.section(QString::fromStdString(":"), 1, 1);

                foreach (const QString &file, files.split(QString::fromStdString(","), QString::SkipEmptyParts)) {
                    if (change == QString::fromStdString("modified")) {
                        modifiedFiles.insert(projectPath, file);
                    } else if (change == QString::fromStdString("deleted")) {
                        deletedFiles.insert(projectPath, file);
                    }
                }

                line = in.readLine();
            }
        }
    }
}

void MaintenanceHelperPlugin::saveFileChanges()
{
    foreach (const QString &projectPath, trackedProjects.values()) {
        QFile changesFile(projectPath + QString::fromStdString("/MHchanges.txt"));
        changesFile.open(QFile::ReadWrite | QFile::Truncate | QFile::Text);
        QTextStream out(&changesFile);

        out << "modified" << ":";
        foreach (const QString &file, modifiedFiles.values(projectPath)) {
            out << file << ",";
        }
        out << endl;

        out << "deleted" << ":";
        foreach (const QString &file, deletedFiles.values(projectPath)) {
            out << file << ",";
        }
        out << endl;

        changesFile.close();
    }

    modifiedFiles.clear();
    deletedFiles.clear();
}

void MaintenanceHelperPlugin::maintenanceHelp(const QString &project)
{
    projectTraceabilities.value(project)->loadLinks();
    QString projectPath = trackedProjects.value(project);
    QString theHelp;

    foreach (const QString &file, modifiedFiles.values(projectPath)) {
        QString baseName = file.section(QString::fromStdString("/"), -1);

        if (!baseName.startsWith(QString::fromStdString("tst_"))) {
            theHelp.append(QString::fromStdString("Modified "));
            theHelp.append(baseName);
            theHelp.append(QString::fromStdString(", check:\n"));

            foreach (const QString &test, projectTraceabilities.value(project)->values(file)) {
                theHelp.append(QString::fromStdString("   "));
                theHelp.append(test);
                theHelp.append(QString::fromStdString("\n"));
            }

            theHelp.append(QString::fromStdString("\n"));
        }
    }

    foreach (const QString &file, deletedFiles.values(projectPath)) {
        QString baseName = file.section(QString::fromStdString("/"), -1);

        if (!baseName.startsWith(QString::fromStdString("tst_"))) {
            theHelp.append(QString::fromStdString("Deleted "));
            theHelp.append(baseName);
            theHelp.append(QString::fromStdString(", check:\n"));

            foreach (const QString &test, projectTraceabilities.value(project)->values(file)) {
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

