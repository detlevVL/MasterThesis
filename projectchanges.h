#ifndef PROJECTCHANGES_H
#define PROJECTCHANGES_H

#include <qmljs/qmljsmodelmanagerinterface.h>

#include <QObject>

using namespace QmlJS;

class ProjectChanges : public QObject
{
    Q_OBJECT
public:
    explicit ProjectChanges(QString projectPath, QObject *parent = 0);

    QList<QString> getDeletedFiles(QDateTime since) const;
    QList<QString> getDeletedSourceFiles(QDateTime since) const;
    QList<QString> getDeletedTestFiles(QDateTime since) const;
    QList<QString> getFiles() const;
    QList<QString> getSourceFiles() const;
    QList<QString> getTestFiles() const;
    QList<QString> getModifiedFiles(QDateTime since) const;
    QList<QString> getModifiedSourceFiles(QDateTime since) const;
    QList<QString> getModifiedTestFiles(QDateTime since) const;

    void saveDeletedFiles();

signals:

public slots:

private slots:
    void fileModified(QmlJS::Document::Ptr doc);
    void fileDeleted(const QStringList &files);

private:
    void loadDeletedFiles();
    bool isInProject(QString fileName);
    bool isSource(QString fileName);
    void scanProject(const QFileInfoList &fileInfoList);

    QString projectPath;
    QSet<QString> sourceFiles;
    QSet<QString> testFiles;
    QMap<QString,QDateTime> deletedSourceFiles;
    QMap<QString,QDateTime> deletedTestFiles;
};

#endif // PROJECTCHANGES_H
