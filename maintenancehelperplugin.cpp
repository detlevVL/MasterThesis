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

    Core::ActionContainer *menu = Core::ActionManager::createMenu(Constants::MENU_ID);
    menu->menu()->setTitle(tr("MaintenanceHelper"));
    menu->addAction(startTrackingCmd);
    menu->addAction(startAnalysisCmd);
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
        }
    }
}

void MaintenanceHelperPlugin::triggerStartAnalysis()
{
    // select project to analyse
    QStringList projects = trackedProjects.keys();
    bool ok;
    QString project = QInputDialog::getItem(Core::ICore::mainWindow(), tr("Project Analysis"),
                                         tr("Select Project"), projects, 0, false, &ok);
    if (ok && !project.isEmpty()) {
        QString projectPath = trackedProjects.value(project);
        // load existing links
        loadLinks(projectPath);

        // do analyses
        namingAnalysis(projectPath);
        // other analyses

        // save links
        saveLinks(projectPath);
    }
}

void MaintenanceHelperPlugin::namingAnalysis(const QString &projectPath)
{
    QStringList sources;
    QStringList tests;
    QFileInfoList fileInfoList = QDir(projectPath).entryInfoList(QStringList(QString::fromStdString("*.qml")),
                                                                 QDir::NoDotAndDotDot | QDir::Files | QDir::AllDirs,
                                                                 QDir::DirsLast);
    scanDirectory(fileInfoList, sources, tests);

    foreach (const QString &test, tests) {
        QString possibleSource = test.section(QString::fromStdString("_"), 1);

        foreach (const QString &source, sources) {
            if (QString::compare(source,possibleSource, Qt::CaseInsensitive) == 0 && !projectLinks.contains(source,test)) {
                projectLinks.insert(source,test);
            }
        }
    }
}

void MaintenanceHelperPlugin::loadLinks(const QString &projectPath)
{
    projectLinks.clear();
    QFile linksFile(projectPath + QString::fromStdString("/links.txt"));

    if (linksFile.exists()) {
        linksFile.open(QFile::ReadOnly | QFile::Text);
        QTextStream in(&linksFile);

        QString line = in.readLine();
        while (!line.isNull()) {
            QString source = line.section(QString::fromStdString(":"), 0, 0);
            QString tests = line.section(QString::fromStdString(":"), 1, 1);

            foreach (const QString &test, tests.split(QString::fromStdString(","), QString::SkipEmptyParts)) {
                projectLinks.insert(source, test);
            }

            line = in.readLine();
        }
    }
}

void MaintenanceHelperPlugin::saveLinks(const QString &projectPath)
{
    QFile linksFile(projectPath + QString::fromStdString("/links.txt"));
    linksFile.open(QFile::ReadWrite | QFile::Truncate | QFile::Text);
    QTextStream out(&linksFile);

    foreach (const QString &source, projectLinks.uniqueKeys()) {
        out << source << ":";
        foreach (const QString &test, projectLinks.values(source)) {
            out << test << ",";
        }
        out << endl;
    }

    linksFile.close();
    projectLinks.clear();
}

void MaintenanceHelperPlugin::scanDirectory(const QFileInfoList &fileInfoList, QStringList &sources, QStringList &tests)
{
    foreach (const QFileInfo &fileInfo, fileInfoList) {
        if (fileInfo.isDir()) {
            // scan subdirectory
            QDir subDir = QDir(fileInfo.absoluteFilePath());
            QFileInfoList subFileInfoList = subDir.entryInfoList(QStringList(QString::fromStdString("*.qml")),
                                                                 QDir::NoDotAndDotDot | QDir::Files | QDir::AllDirs,
                                                                 QDir::DirsLast);
            scanDirectory(subFileInfoList, sources, tests);
        } else {
            // add to sources/tests list
            QString fileName = fileInfo.baseName();

            if (fileName.startsWith(QString::fromStdString("tst_"))) {
                tests.append(fileName);
            } else {
                sources.append(fileName);
            }
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
        QFile changesFile(projectPath + QString::fromStdString("/changes.txt"));

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
        QFile changesFile(projectPath + QString::fromStdString("/changes.txt"));
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


