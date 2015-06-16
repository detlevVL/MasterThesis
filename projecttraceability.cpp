#include "projecttraceability.h"

#include <coreplugin/icore.h>

#include <QFileDialog>
#include <QInputDialog>
#include <QTimer>

#include <iostream>

using namespace std;


ProjectTraceability::ProjectTraceability(QString projectPath, QObject *parent) : QObject(parent)
{
    this->projectPath = projectPath;
}

QList<QString> ProjectTraceability::values(QString sourceFile)
{
    return links.values(sourceFile);
}

void ProjectTraceability::createLinks()
{
    loadLinks();

    namingAnalysis();
    dynamicAnalysis();
}

void ProjectTraceability::dynamicAnalysisComplete()
{
    saveLinks();

    // signal that link creation is complete
    emit linkCreationComplete(projectPath);
}

void ProjectTraceability::loadLinks()
{
    links.clear();
    QFile linksFile(projectPath + QString::fromStdString("/MHlinks.txt"));

    if (linksFile.exists()) {
        linksFile.open(QFile::ReadOnly | QFile::Text);
        QTextStream in(&linksFile);

        QString line = in.readLine();
        while (!line.isNull()) {
            QString source = line.section(QString::fromStdString(":"), 0, 0);
            QString tests = line.section(QString::fromStdString(":"), 1, 1);

            foreach (const QString &test, tests.split(QString::fromStdString(","), QString::SkipEmptyParts)) {
                addLink(source,test);
            }

            line = in.readLine();
        }
    }
}

void ProjectTraceability::saveLinks()
{
    QFile linksFile(projectPath + QString::fromStdString("/MHlinks.txt"));
    linksFile.open(QFile::ReadWrite | QFile::Truncate | QFile::Text);
    QTextStream out(&linksFile);

    foreach (const QString &source, links.uniqueKeys()) {
        out << source << ":";
        foreach (const QString &test, links.values(source)) {
            out << test << ",";
        }
        out << endl;
    }

    linksFile.close();
}

void ProjectTraceability::addLink(const QString &source, const QString &test)
{
    if (!links.contains(source,test)) {
        links.insert(source, test);
        cout << "Link: " << source.toStdString() << " - " << test.toStdString() << endl;
    }
}

void ProjectTraceability::namingAnalysis()
{
    QStringList sources;
    QStringList tests;
    QFileInfoList fileInfoList = QDir(projectPath).entryInfoList(QStringList(QString::fromStdString("*.qml")),
                                                                 QDir::NoDotAndDotDot | QDir::Files | QDir::AllDirs,
                                                                 QDir::DirsLast);
    // fill lists with all qml source/test filepaths found in the project path
    scanDirectory(fileInfoList, sources, tests);

    // for each test file basename (i.e. "/home/somewhere/tst_something.qml" > "something.qml")
    foreach (const QString &test, tests) {
        QString possibleSource = test.section(QString::fromStdString("_"), 1);

        // check if there is a "something.qml" source file
        foreach (const QString &source, sources) {
            if (source.section(QString::fromStdString("/"),-1).compare(possibleSource, Qt::CaseInsensitive) == 0) {
                addLink(source,test);
            }
        }
    }
}

void ProjectTraceability::scanDirectory(const QFileInfoList &fileInfoList, QStringList &sources, QStringList &tests)
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
            if (fileInfo.baseName().startsWith(QString::fromStdString("tst_"))) {
                tests.append(fileInfo.filePath());
            } else {
                sources.append(fileInfo.filePath());
            }
        }
    }
}

void ProjectTraceability::dynamicAnalysis()
{
    // select executable
    QString filePath = QFileDialog::getOpenFileName(Core::ICore::mainWindow(), tr("Select Executable for Dynamic Analysis"),
                                              QDir::homePath(),
                                              tr("Executable Files (*)"));

    if (!filePath.isEmpty() && QFileInfo(filePath).isExecutable()) {
        // get input arguments
        QStringList arguments;
        bool ok;
        QString text = QInputDialog::getText(Core::ICore::mainWindow(), tr("Input Arguments"),
                                             tr("Enter Input Arguments:"), QLineEdit::Normal,
                                             tr(""), &ok);
        if (ok && !text.isEmpty()) {
            foreach (const QString &argument, text.split(QString::fromStdString(" "), QString::SkipEmptyParts)) {
                arguments << argument;
            }
        }

        prepareConnection();

        // add debugger argument and start process
        arguments << QString::fromStdString("-qmljsdebugger=port:3768,block");
        process = new QProcess();
        process->start(filePath, arguments);

        // check if the program starts
        if (process->waitForStarted(5000)) {
            // program has started, try to connect to its debug service after waiting 1 second
            QTimer::singleShot(1000, this, SLOT(connectToDebugService()));
        } else {
            // TODO check if error or still starting after 5 seconds
            cout << "Starting program failed or timed out" << endl;
        }
    }
}

void ProjectTraceability::prepareConnection()
{
    connection = new QmlDebugConnection;
    qmlclientplugin = new QmlProfilerTraceClient(connection, 1);

    connect(qmlclientplugin,
            SIGNAL(rangedEvent(QmlDebug::Message,QmlDebug::RangeType,int,qint64,qint64,
                                       QString,QmlDebug::QmlEventLocation,qint64,qint64,qint64,
                                       qint64,qint64)),
            this,
            SLOT(receiveEv(QmlDebug::Message,QmlDebug::RangeType,int,qint64,qint64,
                                     QString,QmlDebug::QmlEventLocation,qint64,qint64,qint64,qint64,
                                     qint64)));

    connect(connection, SIGNAL(stateMessage(QString)), this, SLOT(logState(QString)));
    connect(connection, SIGNAL(errorMessage(QString)), this, SLOT(logState(QString)));
    connect(connection, SIGNAL(opened()), this, SLOT(qmlDebugConnectionOpened()));
    connect(connection, SIGNAL(closed()), this, SLOT(qmlDebugConnectionClosed()));
}

void ProjectTraceability::receiveEv(QmlDebug::Message message,
                                     QmlDebug::RangeType rangeType,
                                     int detailType,
                                     qint64 startTime,
                                     qint64 length,
                                     const QString &data,
                                     const QmlDebug::QmlEventLocation &location,
                                     qint64 ndata1,
                                     qint64 ndata2,
                                     qint64 ndata3,
                                     qint64 ndata4,
                                     qint64 ndata5)
{
    // if the event is from a file in the project
    if (location.filename.contains(projectPath)) {
        eventList.append(eventInfo(startTime, length, data, location.filename, location.line, location.column));
    }
}

void ProjectTraceability::logState(const QString &msg)
{
    QString state = QString::fromStdString("MaintenanceHelper Plugin: ") + msg;
    cout << state.toStdString() << endl;
}

void ProjectTraceability::qmlDebugConnectionOpened()
{
    logState(tr("Debug connection has opened, setting recording to true"));
    qmlclientplugin->setRecording(true);
}

void ProjectTraceability::qmlDebugConnectionClosed()
{
    logState(tr("Debug connection has closed"));
    createDynamicLinks();
}

void ProjectTraceability::connectToDebugService()
{
    // if the program is running correctly, it should be waiting for us to connect
    if (process->state() == 2) {
        connection->connectToHost(QString::fromStdString("localhost"), 3768);
    } else {
        // TODO check error
        cout << "Starting program failed" << endl;
    }
}

void ProjectTraceability::createDynamicLinks()
{
    // sort on startTimes
    qSort(eventList);

    int inTestRange = -1;
    QString testFile;
    QString prevTestFile;
    QSet<QString> sourceFiles;
    QStack<qint64> parentStack = QStack<qint64>();
    QListIterator<eventInfo> i(eventList);

    while (i.hasNext()) {
        eventInfo ev = i.next();
        qint64 range = ev.startTime + ev.length;

        // while the event is not in range of the previous parent -> pop parent
        while ((!parentStack.isEmpty()) && (range > parentStack.last())) {
            parentStack.pop();
        }

        // if the event is the start of a test case/initialization
        if (ev.data.startsWith(QString::fromStdString("test_")) || ev.data.compare(QString::fromStdString("initTestCase")) == 0) {
            prevTestFile = testFile;
            testFile = ev.filename.section(QString::fromStdString("//"),-1);
            inTestRange = range;

            // if the test case/initialization is from a new file && it is not the first file
            if (testFile.compare(prevTestFile) != 0 && prevTestFile.compare(QString::fromStdString("")) != 0) {
                // add set of sourcefiles from the previous file to the project links
                foreach (const QString &source, sourceFiles) {
                    addLink(source,prevTestFile);
                }

                // clear set of sourcefiles to prepare for the new test file
                sourceFiles.clear();
            }

        // if the event happens within a test case/initialization
        } else if (range < inTestRange) {
            sourceFiles.insert(ev.filename.section(QString::fromStdString("//"),-1));
        }

        parentStack.push(range);
    }

    // add set of sourcefiles from the last file to the project links
    // because the last test file is not handled within the above loop
    foreach (const QString &source, sourceFiles) {
        addLink(source,testFile);
    }

    // back to main analysis
    dynamicAnalysisComplete();
}

