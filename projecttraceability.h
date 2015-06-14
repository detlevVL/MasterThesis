#ifndef PROJECTLINKS_H
#define PROJECTLINKS_H

#include <QObject>
#include <QMultiHash>
#include <QFileInfoList>

class ProjectTraceability : public QObject
{
    Q_OBJECT
public:
    explicit ProjectTraceability(QString projectPath, QObject *parent = 0);
    QList<QString> values(QString sourceFile);
    void createLinks();
    void loadLinks();

signals:

public slots:

private:
    void saveLinks();
    void addLink(const QString &source, const QString &test);
    void namingAnalysis();
    void scanDirectory(const QFileInfoList &fileInfoList, QStringList &sources, QStringList &tests);
    void dynamicAnalysis();

    QString projectPath;
    QMultiHash<QString, QString> links;
};

#endif // PROJECTLINKS_H
