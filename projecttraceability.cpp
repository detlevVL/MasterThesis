#include "projecttraceability.h"

#include <coreplugin/icore.h>

#include <QFileDialog>
#include <QInputDialog>
#include <QTextStream>
#include <QTimer>


#include "naminganalysis.h"



ProjectTraceability::ProjectTraceability(QString projectPath, QObject *parent) : QObject(parent)
{
    this->projectPath = projectPath;
}

QList<QString> ProjectTraceability::values(QString sourceFile)
{
    return links.values(sourceFile);
}

void ProjectTraceability::createLinks(QList<QString> sources, QList<QString> tests,
                                      QList<QString> modifiedSources, QList<QString> modifiedTests,
                                      QList<QString> deletedSources, QList<QString> deletedTests)
{
    loadLinks();

    // delete links to/from deleted files
    deleteLinks(deletedSources, deletedTests);

    // naming analysis
    NamingAnalysis namingAnalysis = NamingAnalysis();
    addLinks(namingAnalysis.analyse(sources, tests, modifiedSources, modifiedTests));

    // dynamic analysis
    dynamicAnalysis = new DynamicAnalysis();
    connect(dynamicAnalysis, SIGNAL(dynamicAnalysisComplete(QMultiHash<QString, QString>)), this, SLOT(dynamicAnalysisComplete(QMultiHash<QString, QString>)));
    dynamicAnalysis->analyse(sources, modifiedTests);
    // continues in dynamicAnalysisComplete once signal is received
}

void ProjectTraceability::dynamicAnalysisComplete(QMultiHash<QString,QString> dynamicLinks)
{
    addLinks(dynamicLinks);

    saveLinks();

    disconnect(dynamicAnalysis, SIGNAL(dynamicAnalysisComplete(QMultiHash<QString, QString>)), this, SLOT(dynamicAnalysisComplete(QMultiHash<QString, QString>)));
    delete dynamicAnalysis;

    // signal that link creation is complete
    emit linkCreationComplete(projectPath);
}

void ProjectTraceability::deleteLinks(QList<QString> deletedSources, QList<QString> deletedTests)
{
    foreach (const QString &source, deletedSources) {
        links.remove(source);
    }
    foreach (const QString &test, deletedTests) {
        foreach (const QString &source, links.uniqueKeys()) {
            links.remove(source, test);
        }
    }
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

void ProjectTraceability::addLinks(QMultiHash<QString, QString> toAdd)
{
    foreach (const QString &source, toAdd.uniqueKeys()) {
        foreach (const QString &test, toAdd.values(source)) {
            addLink(source, test);
        }
    }
}
