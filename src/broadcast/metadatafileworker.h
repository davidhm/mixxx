#pragma once

#include <QObject>
#include <QFile>

class MetadataFileWorker : public QObject {
    Q_OBJECT
  public:
    explicit MetadataFileWorker(const QString &filePath);
  public slots:
    void slotDeleteFile();
    void slotMoveFile(QString destination);
    void slotWriteMetadataToFile(QByteArray fileContents);
    void slotClearFile();
  private:
    QFile m_file;
};

