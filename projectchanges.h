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

    void saveFileChanges();

    QSet<QString> getModifiedFiles() const;
    QSet<QString> getDeletedFiles() const;

signals:

public slots:

private slots:
    void loadFileChanges();
    void fileModified(QmlJS::Document::Ptr doc);
    void fileDeleted(const QStringList &files);

private:
    QString projectPath;
    QSet<QString> modifiedFiles;
    QSet<QString> deletedFiles;
};

#endif // PROJECTCHANGES_H
