#pragma once

#include <QObject>
#include <QMutex>
#include <QLinkedList>
#include <functional>

#include "broadcast/metadatabroadcast.h"
#include "mixer/basetrackplayer.h"
#include "track/track.h"
#include "track/tracktiminginfo.h"
#include "track/trackplaytimers.h"

class BaseTrackPlayer;
class PlayerManager;
class PlayerManagerInterface;

class TrackAudibleStrategy {
  public:
    virtual bool isTrackAudible(TrackPointer pTrack, 
                                BaseTrackPlayer *pPlayer) const = 0;    
};

class TotalVolumeThreshold : public TrackAudibleStrategy {
  public:
    TotalVolumeThreshold(QObject *parent, double threshold);
    bool isTrackAudible(TrackPointer pTrack, 
                        BaseTrackPlayer *pPlayer) const override;
    void setVolumeThreshold(double volume);
  private:
    ControlProxy m_CPCrossfader;
    ControlProxy m_CPXFaderCurve;
    ControlProxy m_CPXFaderCalibration;
    ControlProxy m_CPXFaderMode;
    ControlProxy m_CPXFaderReverse;

    QObject *m_pParent;

    double m_volumeThreshold;
};

class ScrobblingManager : public QObject {
    Q_OBJECT
  public:
    ScrobblingManager(PlayerManagerInterface *manager);
    void setAudibleStrategy(TrackAudibleStrategy *pStrategy);
    void setMetadataBroadcaster(MetadataBroadcasterInterface *pBroadcast);
    void setTimer(TrackTimers::RegularTimer *timer);
    void setTimersFactory(const std::function
                          <std::pair<TrackTimers::RegularTimer*,
                           TrackTimers::ElapsedTimer*>
                           (TrackPointer)> &factory);

  public slots:
    void slotTrackPaused(TrackPointer pPausedTrack);
    void slotTrackResumed(TrackPointer pResumedTrack);
    void slotLoadingTrack(TrackPointer pNewTrack, TrackPointer pOldTrack);
    void slotNewTrackLoaded(TrackPointer pNewTrack);
    void slotPlayerEmpty();
    void slotGuiTick(double timeSinceLastTick);

  private:
    struct TrackInfo {
        TrackPointer m_pTrack;
        TrackTimingInfo m_trackInfo;
        QLinkedList<QString> m_players;
        TrackInfo(TrackPointer pTrack) : 
        m_pTrack(pTrack), m_trackInfo(pTrack)
        {} 
    };
    struct TrackToBeReset {
        TrackPointer m_pTrack;
        QString m_playerGroup;
        TrackToBeReset(TrackPointer pTrack, const QString &playerGroup) :
        m_pTrack(pTrack), m_playerGroup(playerGroup) {}
    };   

    PlayerManagerInterface *m_pManager;

    std::unique_ptr<MetadataBroadcasterInterface> m_pBroadcaster;   
    
    QLinkedList<TrackInfo*> m_trackList;
    QLinkedList<TrackToBeReset> m_tracksToBeReset;

    std::unique_ptr<TrackAudibleStrategy> m_pAudibleStrategy;

    std::unique_ptr<TrackTimers::RegularTimer> m_pTimer;

    std::function<std::pair<TrackTimers::RegularTimer*,
                            TrackTimers::ElapsedTimer*>
                            (TrackPointer)> m_timerFactory;

    void resetTracks();
    bool isStrayFromEngine(TrackPointer pTrack,const QString &group) const;
    bool playerNotInTrackList(const QLinkedList<QString> &list, const QString &group) const;
    void deletePlayerFromList(const QString &player,QLinkedList<QString> &list);
    void deleteTrackInfoAndNotify(QLinkedList<TrackInfo*>::iterator &it);
  private slots:    
    void slotReadyToBeScrobbled(TrackPointer pTrack);
    void slotCheckAudibleTracks();
};