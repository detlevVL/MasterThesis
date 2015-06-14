#include "projecttraceability.h"

#include <QFile>
#include <QTextStream>
#include <QDir>

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

    saveLinks();
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

}

