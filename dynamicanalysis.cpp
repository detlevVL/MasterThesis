#include "dynamicanalysis.h"

#include <coreplugin/icore.h>

#include <QFileDialog>
#include <QInputDialog>
#include <QTextStream>
#include <QTimer>

#include <iostream>


using namespace std;

DynamicAnalysis::DynamicAnalysis(QObject *parent) : QObject(parent)
{
}

void DynamicAnalysis::analyse(QList<QString> sourceFiles, QList<QString> modifiedTestFiles)
{
    if (modifiedTestFiles.size() == 0) {
        emit dynamicAnalysisComplete(links);
        return;
    }

    // select executable
    QString filePath = QFileDialog::getOpenFileName(Core::ICore::mainWindow(), tr("Select Executable for Dynamic Analysis"),
                                              QDir::homePath(),
                                              tr("Executable Files (*)"));

    if (!filePath.isEmpty() && QFileInfo(filePath).isExecutable()) {
        // get input arguments
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

        program = filePath;
        sources = sourceFiles;
        modifiedTests = modifiedTestFiles;
        dynamicTestCounter = 0;
        runTest(modifiedTests.first());
    }
}

void DynamicAnalysis::receiveEv(Message message, RangeType rangeType, int bindingType, qint64 startTime, qint64 length, const QString &data, const QmlEventLocation &location, qint64 ndata1, qint64 ndata2, qint64 ndata3, qint64 ndata4, qint64 ndata5)
{
    QString eventBaseFile = location.filename.section(QString::fromStdString("/"), -1);

    // check if event is from a source file
    foreach (const QString &source, sources) {
        QString baseSource = source.section(QString::fromStdString("/"), -1);
        if (eventBaseFile.compare(baseSource, Qt::CaseInsensitive) == 0) {
            eventList.append(eventInfo(startTime, length, data, source, location.line, location.column));
        }
    }

    // check if event is from a test file
    foreach (const QString &test, modifiedTests) {
        QString baseTest = test.section(QString::fromStdString("/"), -1);
        if (eventBaseFile.compare(baseTest, Qt::CaseInsensitive) == 0) {
            eventList.append(eventInfo(startTime, length, data, location.filename, location.line, location.column));
        }
    }

}

void DynamicAnalysis::logState(const QString &msg)
{
    QString state = QString::fromStdString("MaintenanceHelper Plugin: ") + msg;
    cout << state.toStdString() << endl;
}

void DynamicAnalysis::qmlDebugConnectionOpened()
{
    logState(tr("Debug connection has opened, setting recording to true"));
    qmlclientplugin->setRecording(true);
}

void DynamicAnalysis::qmlDebugConnectionClosed()
{
    logState(tr("Debug connection has closed"));

    // test file finished, create links from event list
    createDynamicLinks();

    dynamicTestCounter++;
    if (dynamicTestCounter < modifiedTests.size()) {
        runTest(modifiedTests.at(dynamicTestCounter));
    } else {
        emit dynamicAnalysisComplete(links);
    }
}

void DynamicAnalysis::connectToDebugService()
{
    // if the program is running correctly, it should be waiting for us to connect
    if (process->state() == 2) {
        connection->connectToHost(QString::fromStdString("localhost"), 3768);
    } else {
        cout << "Starting program failed" << endl;
    }
}

void DynamicAnalysis::runTest(QString file)
{
    // Copy arguments
    QList<QString> argumentsToUse;
    foreach (const QString &argument, arguments) {
        argumentsToUse << argument;
    }

    argumentsToUse << QString::fromStdString("-input") << file;
    argumentsToUse << QString::fromStdString("-qmljsdebugger=port:3768,block");

    process = new QProcess();
    process->start(program, argumentsToUse);

    if (process->waitForStarted(5000)) {
        QTimer::singleShot(1000, this, SLOT(connectToDebugService()));
    } else {
        // check if error or still starting
        cout << "Starting program failed or timed out" << endl;
    }
}

void DynamicAnalysis::prepareConnection()
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

void DynamicAnalysis::createDynamicLinks()
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
                    links.insert(source,prevTestFile);
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
        links.insert(source,testFile);
    }

    eventList.clear();
}

