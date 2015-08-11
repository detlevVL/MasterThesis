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
            QString dateKey = QString(key).append(QString::fromStdString(".lastanalysis"));
            lastAnalysisMap.insert(key, settings.value(dateKey).toDateTime());
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

    QSettings settings(QString::fromStdString("Detlev"), QString::fromStdString("MaintenanceHelper"));

    // for each tracked project, save the last analysis date and file changes, and disconnect from qmljs signals
    foreach (const QString &project, trackedProjectsMap.keys()) {
        QString dateKey = QString(project).append(QString::fromStdString(".lastanalysis"));
        settings.setValue(dateKey, lastAnalysisMap.value(project));
        projectChangesMap.value(project)->saveDeletedFiles();
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
            ProjectChanges *projectChanges = new ProjectChanges(projectPath);
            projectChangesMap.insert(project, projectChanges);
            ProjectTraceability *projectTraceability = new ProjectTraceability(projectPath);
            projectTraceabilityMap.insert(project, projectTraceability);

            QMessageBox::information(Core::ICore::mainWindow(),
                                     tr("Tracking started"),
                                     tr("Tracking started, prepare to analyse: ").append(project));

            // full project analysis here
            QList<QString> sources = projectChanges->getSourceFiles();
            QList<QString> tests = projectChanges->getTestFiles();
            QList<QString> empty;
            connect(projectTraceability, SIGNAL(linkCreationComplete(QString)), this, SLOT(analysisComplete(QString)));
            // new project => all files count as modified
            projectTraceability->createLinks(sources, tests, sources, tests, empty, empty);
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
            QDateTime lastAnalysis = lastAnalysisMap.value(project);
            QList<QString> sources = projectChangesMap.value(project)->getSourceFiles();
            QList<QString> tests = projectChangesMap.value(project)->getTestFiles();
            QList<QString> modifiedSources = projectChangesMap.value(project)->getModifiedSourceFiles(lastAnalysis);
            QList<QString> modifiedTests = projectChangesMap.value(project)->getModifiedTestFiles(lastAnalysis);
            QList<QString> deletedSources = projectChangesMap.value(project)->getDeletedSourceFiles(lastAnalysis);
            QList<QString> deletedTests = projectChangesMap.value(project)->getDeletedTestFiles(lastAnalysis);

            connect(projectTraceabilityMap.value(project), SIGNAL(linkCreationComplete(QString)), this, SLOT(analysisComplete(QString)));
            projectTraceabilityMap.value(project)->createLinks(sources, tests, modifiedSources, modifiedTests, deletedSources, deletedTests);
        }
    }
}

void MaintenanceHelperPlugin::analysisComplete(QString projectPath)
{
    QString project = trackedProjectsMap.key(projectPath);

    lastAnalysisMap.insert(project, QDateTime::currentDateTime());

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
    QSet<QString> testsToRun;

    if (projectChangesMap.value(project)->getModifiedFiles(lastAnalysisMap.value(project)).size() == 0 &&
            projectChangesMap.value(project)->getDeletedFiles(lastAnalysisMap.value(project)).size() == 0) {
        QMessageBox::information(Core::ICore::mainWindow(),
                             tr("Maintenance Help"),
                             tr("There are no modified/deleted files since last analysis"));
    } else {
        theHelp.append(QString::fromStdString("Source files:\n"));
        foreach (const QString &file, projectChangesMap.value(project)->getModifiedSourceFiles(lastAnalysisMap.value(project))) {
            QString baseName = file.section(QString::fromStdString("/"), -1);

            theHelp.append(QString::fromStdString("Modified "));
            theHelp.append(baseName);
            theHelp.append(QString::fromStdString(", to run:\n"));

            foreach (const QString &test, projectTraceabilityMap.value(project)->values(file)) {
                theHelp.append(QString::fromStdString("   "));
                theHelp.append(test);
                theHelp.append(QString::fromStdString("\n"));
                testsToRun.insert(test);
            }

            theHelp.append(QString::fromStdString("\n"));
        }

        foreach (const QString &file, projectChangesMap.value(project)->getDeletedSourceFiles(lastAnalysisMap.value(project))) {
            QString baseName = file.section(QString::fromStdString("/"), -1);

            theHelp.append(QString::fromStdString("Deleted "));
            theHelp.append(baseName);
            theHelp.append(QString::fromStdString(", to run:\n"));

            foreach (const QString &test, projectTraceabilityMap.value(project)->values(file)) {
                theHelp.append(QString::fromStdString("   "));
                theHelp.append(test);
                theHelp.append(QString::fromStdString("\n"));
                testsToRun.insert(test);
            }

            theHelp.append(QString::fromStdString("\n"));
        }

        theHelp.append(QString::fromStdString("Modified test files:\n"));
        foreach (const QString &test, projectChangesMap.value(project)->getModifiedTestFiles(lastAnalysisMap.value(project))) {
            theHelp.append(QString::fromStdString("   "));
            theHelp.append(test);
            theHelp.append(QString::fromStdString("\n"));
            testsToRun.insert(test);
        }

        QPlainTextEdit *text = new QPlainTextEdit(theHelp);
        text->setReadOnly(true);
        text->setWindowTitle(QString::fromStdString("Maintenance Help"));
        text->show();

        int ret = QMessageBox::question(text, QString::fromStdString("Run tests"),
                                        QString::fromStdString("Run these tests now? (See Maintenance Help window)"),
                                        (QMessageBox::Yes|QMessageBox::No), QMessageBox::No);
        if (ret == QMessageBox::Yes) {
            text->close();
            runTests(testsToRun);
        } else {
            text->close();
        }
    }
}

void MaintenanceHelperPlugin::runTests(QSet<QString> testsToRun)
{
    // select executable
    QString program = QFileDialog::getOpenFileName(Core::ICore::mainWindow(), tr("Select Executable for Running Tests"),
                                              QDir::homePath(),
                                              tr("Executable Files (*)"));

    if (!program.isEmpty() && QFileInfo(program).isExecutable()) {
        // get input arguments
        bool ok;
        QList<QString> arguments;
        QString text = QInputDialog::getText(Core::ICore::mainWindow(), tr("Input Arguments"),
                                             tr("Enter Input Arguments:"), QLineEdit::Normal,
                                             tr(""), &ok);
        if (ok && !text.isEmpty()) {
            foreach (const QString &argument, text.split(QString::fromStdString(" "), QString::SkipEmptyParts)) {
                arguments << argument;
            }
        }

        QString output;
        int passed = 0;
        int failed = 0;
        int skipped = 0;
        int blacklisted = 0;

        foreach (const QString &test,testsToRun) {

            // Copy arguments
            QList<QString> argumentsToUse;
            foreach (const QString &argument, arguments) {
                argumentsToUse << argument;
            }

            argumentsToUse << QString::fromStdString("-input") << test;

            QProcess process;
            process.start(program, argumentsToUse);
            process.waitForFinished();

            QString testOutput = QString::fromStdString(process.readAllStandardOutput().toStdString());
            output.append(testOutput);

            // find passed, failed, skipped, blacklisted results
            QRegExp regExp(QString::fromStdString("Totals: (\\d+) passed, (\\d+) failed, (\\d+) skipped, (\\d+) blacklisted"));

            int pos = regExp.indexIn(testOutput);
            if (pos > -1) {
                passed += regExp.cap(1).toInt();
                failed += regExp.cap(2).toInt();
                skipped += regExp.cap(3).toInt();
                blacklisted += regExp.cap(4).toInt();
            }
        }

        // total results string to prepend to output
        QString toPrepend;

        toPrepend.append(QString::fromStdString("Totals: "));
        toPrepend.append(QString::number(passed));
        toPrepend.append(QString::fromStdString(" passed, "));
        toPrepend.append(QString::number(failed));
        toPrepend.append(QString::fromStdString(" failed, "));
        toPrepend.append(QString::number(skipped));
        toPrepend.append(QString::fromStdString(" skipped, "));
        toPrepend.append(QString::number(blacklisted));
        toPrepend.append(QString::fromStdString(" blacklisted\n\n"));

        output.prepend(toPrepend);

        // show output
        QPlainTextEdit *result = new QPlainTextEdit(output);
        result->setReadOnly(true);
        result->setWindowTitle(QString::fromStdString("Test Results"));
        result->show();
    }

}

