#pragma once

#include <QObject>
#include <QTimer>
#include <QElapsedTimer>

namespace TrackTimers {
  class ElapsedTimer {
    public:
      ElapsedTimer() = default;
      virtual ~ElapsedTimer() = default;
      virtual void invalidate() = 0;
      virtual bool isValid() = 0;
      virtual void start() = 0;
      virtual qint64 elapsed() = 0;
  };

  class TrackTimer : public QObject {
      Q_OBJECT
    public:
      TrackTimer() = default;
      virtual ~TrackTimer() = default;
      virtual void start(int msec) = 0;
      virtual bool isActive() = 0;
    public slots:
      virtual void stop() = 0;
    signals:
      void timeout();
  };

  class TimerQt : public TrackTimer {
      Q_OBJECT
    public:
      TimerQt();
      ~TimerQt() override = default;
      void start(int msec) override;
      bool isActive() override;
      void stop() override;
    private:
      QTimer m_Timer;    
  };

  class ElapsedTimerQt : public ElapsedTimer {
    public:
      ElapsedTimerQt() = default;
      ~ElapsedTimerQt() override = default;
      void invalidate() override;
      bool isValid() override;
      void start() override;
      qint64 elapsed() override;
    private:
      QElapsedTimer m_elapsedTimer;
  };
}
