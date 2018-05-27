#include "track/track.h"
#include "track/trackplaytimers.h"
#include <QObject>

class TrackTimingInfo : public QObject {
    Q_OBJECT
  public:
    TrackTimingInfo(TrackPointer pTrack);
    TrackTimingInfo(const TrackTimingInfo& other) = delete;
    void pausePlayedTime();
    void resumePlayedTime();
    void resetPlayedTime();
    void setElapsedTimer(TrackTimers::ElapsedTimer *elapsedTimer);
    void setTimer(TrackTimers::RegularTimer *timer);
    void setMsPlayed(qint64 ms);
    bool isScrobbable() const;
    void setTrackPointer(TrackPointer pTrack);
  public slots:
    void slotCheckIfScrobbable();
    void slotGuiTick(double timeSinceLastTick);
  signals:
    void readyToBeScrobbled(TrackPointer pTrack);
  private:
    std::unique_ptr<TrackTimers::ElapsedTimer> m_pElapsedTimer;
    std::unique_ptr<TrackTimers::RegularTimer> m_pTimer;
    TrackPointer m_pTrackPtr;
    qint64 m_playedMs;
    bool m_isTrackScrobbable;
};