#include <memory>

#pragma once

#include <QObject>
#include <QLinkedList>
#include <QHash>
#include <QSet>
#include <functional>
#include <QString>

#include "control/controlobject.h"
#include "broadcast/metadatabroadcast.h"
#include "mixer/basetrackplayer.h"
#include "track/track.h"
#include "track/tracktiminginfo.h"
#include "track/trackplaytimers.h"

class BaseTrackPlayer;
class PlayerManager;
class PlayerManagerInterface;
class MixxxMainWindow;

class TrackAudibleStrategy {
  public:
    virtual ~TrackAudibleStrategy() = default;
    virtual bool isPlayerAudible(BaseTrackPlayer *pPlayer) const = 0;
};

class TotalVolumeThreshold : public TrackAudibleStrategy {
  public:
    TotalVolumeThreshold(QObject *parent, double threshold);
    bool isPlayerAudible(BaseTrackPlayer *pPlayer) const override;
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

typedef std::function<std::shared_ptr<TrackTimingInfo>(TrackPointer)> TrackTimingFactory;

class ScrobblingManager : public QObject {
    Q_OBJECT
  public:
    explicit ScrobblingManager(PlayerManagerInterface *manager);
    ~ScrobblingManager() = default;
    void setAudibleStrategy(TrackAudibleStrategy *pStrategy);
    void setMetadataBroadcaster(MetadataBroadcasterInterface *pBroadcast);
    void setTimer(TrackTimers::RegularTimer *timer);
    void setTrackInfoFactory(const std::function<std::shared_ptr<TrackTimingInfo>(TrackPointer)> &factory);
    bool hasScrobbledAnyTrack() const;

  public slots:
    void slotTrackPaused(TrackPointer pPausedTrack);
    void slotTrackResumed(TrackPointer pResumedTrack, const QString &playerGroup);
    void slotNewTrackLoaded(TrackPointer pNewTrack, const QString &playerGroup);

  private:

    struct TrackInfo {
        std::shared_ptr<TrackTimingInfo> m_trackInfo;
        QSet <QString> m_players;
        void init (const TrackTimingFactory& factory,
                   const TrackPointer& pTrack) {
            if (factory) {
                m_trackInfo = factory(pTrack);
            }
            else {
                m_trackInfo = std::make_shared<TrackTimingInfo>(pTrack);
            }
        }
    };

#ifdef MIXXX_BUILD_DEBUG
    friend QDebug operator<<(QDebug debug, const ScrobblingManager::TrackInfo& info);
#endif

    QHash<TrackId, TrackInfo> m_trackInfoHashDict;

    PlayerManagerInterface *m_pManager;

    std::unique_ptr<MetadataBroadcasterInterface> m_pBroadcaster;


    std::unique_ptr<TrackAudibleStrategy> m_pAudibleStrategy;

    std::unique_ptr<TrackTimers::RegularTimer> m_pTimer;

    TrackTimingFactory m_trackInfoFactory;

    bool m_scrobbledAtLeastOnce;

    ControlProxy m_GuiTickObject;

private slots:
    void slotReadyToBeScrobbled(TrackPointer pTrack);
    void slotCheckAudibleTracks();
    void slotGuiTick(double timeSinceLastTick);
};