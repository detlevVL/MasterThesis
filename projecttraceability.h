#ifndef PROJECTTRACEABILITY_H
#define PROJECTTRACEABILITY_H

#include "eventinfo.h"

#include <qmldebug/qmldebugclient.h>
#include <qmldebug/qmlprofilertraceclient.h>

#include <QObject>
#include <QFileInfoList>
#include <QProcess>

using namespace QmlDebug;

class ProjectTraceability : public QObject
{
    Q_OBJECT
public:
    explicit ProjectTraceability(QString projectPath, QObject *parent = 0);
    QList<QString> values(QString sourceFile);
    void createLinks();
    void loadLinks();

signals:
    void linkCreationComplete(QString projectPath);

public slots:

private slots:
    void receiveEv(QmlDebug::Message message, QmlDebug::RangeType rangeType, int bindingType,
                         qint64 startTime, qint64 length, const QString &data,
                         const QmlDebug::QmlEventLocation &location,
                         qint64 ndata1, qint64 ndata2, qint64 ndata3, qint64 ndata4, qint64 ndata5);
    void logState(const QString &);
    void qmlDebugConnectionOpened();
    void qmlDebugConnectionClosed();
    void connectToDebugService();

private:
    void dynamicAnalysisComplete();
    void saveLinks();
    void addLink(const QString &source, const QString &test);
    void namingAnalysis();
    void scanDirectory(const QFileInfoList &fileInfoList, QStringList &sources, QStringList &tests);
    void dynamicAnalysis();
    void prepareConnection();
    void createDynamicLinks();

    QString projectPath;
    QMultiHash<QString, QString> links;
    QProcess *process;
    QmlDebugConnection *connection;
    QmlProfilerTraceClient *qmlclientplugin;
    QList<eventInfo> eventList;
};

#endif // PROJECTTRACEABILITY_H
