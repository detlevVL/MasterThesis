#ifndef DYNAMICANALYSIS_H
#define DYNAMICANALYSIS_H

#include "eventinfo.h"

#include <qmldebug/qmldebugclient.h>
#include <qmldebug/qmlprofilertraceclient.h>

#include <QObject>
#include <QProcess>

using namespace QmlDebug;

class DynamicAnalysis : public QObject
{
    Q_OBJECT
public:
    explicit DynamicAnalysis(QObject *parent = 0);
    void analyse(QList<QString> sources, QList<QString> modifiedTests);

signals:
    void dynamicAnalysisComplete(QMultiHash<QString, QString> links);

public slots:

private slots:
    void receiveEv(QmlDebug::Message message, QmlDebug::RangeType rangeType, int bindingType,
                         qint64 startTime, qint64 length, const QString &data,
                         const QmlDebug::QmlEventLocation &location,
                         qint64 ndata1, qint64 ndata2, qint64 ndata3, qint64 ndata4, qint64 ndata5);
    void logState(const QString &msg);
    void qmlDebugConnectionOpened();
    void qmlDebugConnectionClosed();
    void connectToDebugService();

private:
    void runTest(QString file);
    void prepareConnection();
    void createDynamicLinks();

    int dynamicTestCounter;
    QList<QString> arguments;
    QList<eventInfo> eventList;
    QList<QString> modifiedTests;
    QList<QString> sources;
    QmlDebugConnection *connection;
    QmlProfilerTraceClient *qmlclientplugin;
    QMultiHash<QString, QString> links;
    QProcess *process;
    QString program;
};

#endif // DYNAMICANALYSIS_H
