#pragma once

#include <QObject>
#include <QLinkedList>
#include <list>

#include "broadcast/scrobblingservice.h"
#include "track/track.h"
#include "track/trackid.h"
#include "track/trackplaytimers.h"

class MetadataBroadcasterInterface : public QObject {
    Q_OBJECT
  public slots:
    virtual void slotNowListening(TrackPointer pTrack) = 0;
    virtual void slotAttemptScrobble(TrackPointer pTrack) = 0;
    virtual void slotAllTracksPaused() = 0;
  public:
    virtual ~MetadataBroadcasterInterface() = default;
    virtual MetadataBroadcasterInterface& 
        addNewScrobblingService(const ScrobblingServicePtr &newService) = 0;
    virtual void newTrackLoaded(TrackPointer pTrack) = 0;
    virtual void trackUnloaded(TrackPointer pTrack) = 0;
};

class MetadataBroadcaster : public MetadataBroadcasterInterface {
    Q_OBJECT    
  private:
    struct GracePeriod {
        double m_msSinceEjection;
        unsigned int m_timesTrackHasBeenScrobbled = 0;
        bool m_firstTimeLoaded = true;
        bool m_hasBeenEjected = false;
        GracePeriod() :
        m_msSinceEjection(0.0) {}
    };  
  public:   

    MetadataBroadcaster();
    MetadataBroadcasterInterface&
        addNewScrobblingService(const ScrobblingServicePtr &newService) override;
    void newTrackLoaded(TrackPointer pTrack) override;
    void trackUnloaded(TrackPointer pTrack) override;
    void slotNowListening(TrackPointer pTrack) override;
    void slotAttemptScrobble(TrackPointer pTrack) override;
    void slotAllTracksPaused() override;
    void guiTick(double timeSinceLastTick);

  private:    
    unsigned int m_gracePeriodSeconds;
    QHash<TrackId,GracePeriod> m_trackedTracks;
    QLinkedList<ScrobblingServicePtr> m_scrobblingServices;
};