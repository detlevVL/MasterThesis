#ifndef PROJECTTRACEABILITY_H
#define PROJECTTRACEABILITY_H

#include <QObject>

#include "dynamicanalysis.h"

class ProjectTraceability : public QObject
{
    Q_OBJECT
public:
    explicit ProjectTraceability(QString projectPath, QObject *parent = 0);
    QList<QString> values(QString sourceFile);
    void createLinks(QList<QString> sources, QList<QString> tests,
                     QList<QString> modifiedSources, QList<QString> modifiedTests,
                     QList<QString> deletedSources, QList<QString> deletedTests);
    void loadLinks();

signals:
    void linkCreationComplete(QString projectPath);

public slots:

private slots:
    void dynamicAnalysisComplete(QMultiHash<QString, QString> dynamicLinks);

private:
    void deleteLinks(QList<QString> deletedSources, QList<QString> deletedTests);
    void saveLinks();
    void addLink(const QString &source, const QString &test);
    void addLinks(QMultiHash<QString, QString> toAdd);


    QString projectPath;
    QMultiHash<QString, QString> links;
    DynamicAnalysis *dynamicAnalysis;
};

#endif // PROJECTTRACEABILITY_H
