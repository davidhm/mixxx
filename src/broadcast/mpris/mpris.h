#ifndef MPRIS_H
#define MPRIS_H

#include <QObject>
#include <QDBusConnection>
#include "track/track.h"

class MediaPlayer2Player;
class MixxxMainWindow;
class PlayerManager;

class Mpris : public QObject
{
    Q_OBJECT
  public:
    explicit Mpris(MixxxMainWindow *mixxx,
                   PlayerManager *pPlayerManager,
                   UserSettingsPointer pSettings);
    ~Mpris();
    void broadcastCurrentTrack();
    void notifyPropertyChanged(const QString &interface,
                               const QString &propertyName,
                               const QVariant &propertyValue);
  private:

    QDBusConnection m_busConnection;
    MediaPlayer2Player *m_pPlayer;
};

#endif // MPRIS_H
