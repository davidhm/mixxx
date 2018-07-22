
#include "broadcast/mpris/mprisplayer.h"
#include "mixer/deck.h"
#include "mixer/playermanager.h"
#include "mixer/playerinfo.h"

namespace {

    const QString kPlaybackStatusPlaying = "Playing";
    const QString kPlaybackStatusPaused = "Paused";
    const QString kPlaybackStatusStopped = "Stopped";

    // the playback will stop when there are no more tracks to play
    const QString kLoopStatusNone = "None";
    // The current track will start again from the begining once it has finished playing
    const QString kLoopStatusTrack = "Track";
    // The playback loops through a list of tracks
    const QString kLoopStatusPlaylist = "Playlist";
    const QString playerInterfaceName = "org.mpris.MediaPlayer2.Player";
}

#define AUTODJENABLED m_bComponentsInitialized && m_CPAutoDjEnabled.toBool()
#define AUTODJIDLE AUTODJENABLED && m_CPAutoDJIdle.toBool()

MprisPlayer::MprisPlayer(PlayerManager *pPlayerManager,
                         MixxxMainWindow *pWindow,
                         Mpris *pMpris)
        :  m_pPlayerManager(pPlayerManager),
           m_pWindow(pWindow),
           m_bComponentsInitialized(false),
           m_bPropertiesEnabled(false),
           m_pMpris(pMpris) {
    connect(m_pWindow,&MixxxMainWindow::componentsInitialized,
            this,&MprisPlayer::mixxxComponentsInitialized);
}

QString MprisPlayer::playbackStatus() const {
    if (!m_bComponentsInitialized)
        return kPlaybackStatusStopped;
    for (unsigned int i = 1; i <= m_pPlayerManager->numberOfDecks(); ++i) {
        if (!m_pPlayerManager->getDeck(i)->isTrackPaused()) {
            return kPlaybackStatusPlaying;
        }
    }
    return kPlaybackStatusPaused;
}

QString MprisPlayer::loopStatus() const {
    if (!m_bComponentsInitialized)
        return kLoopStatusNone;
    return QString();
}

void MprisPlayer::setLoopStatus(const QString &value) {
    double repeat = 0.0;
    if (value == kLoopStatusTrack) {
        repeat = 1.0;
    }

    TrackPointer pTrack;
    int deckIndex = PlayerInfo::instance().getCurrentPlayingDeck();
    if (deckIndex >= 0) {
        QString group = PlayerManager::groupForDeck(deckIndex);
        ConfigKey key(group, "repeat");
        ControlObject::set(key, repeat);
        // TODO: Decide when how to handle playlist repeat mode
    }
}

QVariantMap MprisPlayer::metadata() const {
    TrackPointer pTrack = PlayerInfo::instance().getCurrentPlayingTrack();
    return getMetadataFromTrack(pTrack);
}

double MprisPlayer::volume() const {
    return 0;
}

void MprisPlayer::setVolume(double value) {

}

qlonglong MprisPlayer::position() const {
    if (AUTODJIDLE) {
        for (unsigned int i = 1; i <= m_pPlayerManager->numberOfDecks(); ++i) {
            ControlProxy playing(ConfigKey(PlayerManager::groupForDeck(i),"play"));
            if (playing.toBool()) {
                DeckAttributes *pDeck = m_deckAttributes.at(i-1);
                qlonglong playPosition =
                        static_cast<qlonglong>(pDeck->playPosition() * //Fraction of duration
                                               pDeck->getLoadedTrack()->getDuration() * //Duration in seconds
                                               1e6);
                return playPosition;
            }
        }
    }
    return 0;
}

bool MprisPlayer::canGoNext() const {
    return AUTODJIDLE;
}

bool MprisPlayer::canGoPrevious() const {
    return false;
}

bool MprisPlayer::canPlay() const {
    return AUTODJENABLED;
}

bool MprisPlayer::canPause() const {
    return AUTODJIDLE;
}

bool MprisPlayer::canSeek() const {
    return AUTODJIDLE;
}

void MprisPlayer::nextTrack() {
    if (AUTODJIDLE) {
        m_CPFadeNow.set(true);
    }
}

void MprisPlayer::pause() {
    if (AUTODJIDLE) {
        DeckAttributes *playingDeck = findPlayingDeck();
        if (playingDeck != nullptr) {
            playingDeck->stop();
            m_pMpris->notifyPropertyChanged(playerInterfaceName,"Metadata",QVariantMap());
            m_pausedDeck = playingDeck->group;
        }
    }
}

void MprisPlayer::playPause() {
    if (AUTODJIDLE) {
        DeckAttributes *playingDeck = findPlayingDeck();
        if (playingDeck != nullptr) {
            playingDeck->stop();
            m_pMpris->notifyPropertyChanged(playerInterfaceName,"Metadata",QVariantMap());
            m_pausedDeck = playingDeck->group;
        }
        else {
            ControlProxy playing(ConfigKey(m_pausedDeck,"play"));
            BaseTrackPlayer *player = m_pPlayerManager->getPlayer(m_pausedDeck);
            DEBUG_ASSERT(player);
            TrackPointer pTrack = player->getLoadedTrack();
            playing.set(true);
            m_pMpris->notifyPropertyChanged(playerInterfaceName,
                                            "Metadata",
                                            getMetadataFromTrack(pTrack));
        }
    }

}

void MprisPlayer::play() {
    if (AUTODJENABLED) {
        DeckAttributes *playingDeck = findPlayingDeck();
        if (playingDeck == nullptr) {
            ControlProxy playing(ConfigKey(m_pausedDeck,"play"));
            BaseTrackPlayer *player = m_pPlayerManager->getPlayer(m_pausedDeck);
            DEBUG_ASSERT(player);
            TrackPointer pTrack = player->getLoadedTrack();
            playing.set(true);
            m_pMpris->notifyPropertyChanged(playerInterfaceName,
                                            "Metadata",
                                            getMetadataFromTrack(pTrack));
        }
    }
}

void MprisPlayer::stop() {

}

void MprisPlayer::seek(qlonglong offset) {

}

void MprisPlayer::setPosition(const QDBusObjectPath &trackId, qlonglong position) {

}

void MprisPlayer::openUri(const QString &uri) {

}

void MprisPlayer::mixxxComponentsInitialized() {
    m_bComponentsInitialized = true;

    m_CPAutoDjEnabled.initialize(ConfigKey("[AutoDJ]","enabled"));
    m_CPFadeNow.initialize(ConfigKey("[AutoDJ]","fade_now"));
    m_CPAutoDJIdle.initialize(ConfigKey("[AutoDJ]","idle"));

    for (unsigned int i = 1; i <= m_pPlayerManager->numberOfDecks(); ++i) {
        DeckAttributes *attributes = new DeckAttributes
                                 (i,
                                  m_pPlayerManager->getDeck(i),
                                  i%2==0 ? EngineChannel::RIGHT : EngineChannel::LEFT);
        m_deckAttributes.append(attributes);
        connect(attributes,&DeckAttributes::playChanged,
                this,&MprisPlayer::slotPlayChanged);
        connect(attributes,&DeckAttributes::playPositionChanged,
                this,&MprisPlayer::slotPlayPositionChanged);
    }

    m_CPAutoDjEnabled.connectValueChanged(this,SLOT(slotChangeProperties(double)));
    m_CPAutoDJIdle.connectValueChanged(this,SLOT(slotChangeProperties(double)));
}

void MprisPlayer::slotChangeProperties(double enabled) {
    if (enabled != m_bPropertiesEnabled) {
        broadcastPropertiesChange(enabled);
        m_bPropertiesEnabled = static_cast<bool>(enabled);
    }
}

void MprisPlayer::broadcastPropertiesChange(bool enabled) {
    for (const QString &property : autoDJDependentProperties) {
        m_pMpris->notifyPropertyChanged(playerInterfaceName,
                                        property,
                                        enabled);
    }
}

QVariantMap MprisPlayer::getMetadataFromTrack(TrackPointer pTrack) const {
    QVariantMap metadata;
    if (!pTrack)
        return metadata;
    metadata.insert("mpris:trackid","/org/mixxx/" + pTrack->getId().toString());
    double trackDurationSeconds = pTrack->getDuration();
    trackDurationSeconds *= 1e6;
    metadata.insert("mpris:length", static_cast<long long int>(trackDurationSeconds));
    QStringList artists;
    artists << pTrack->getArtist();
    metadata.insert("xesam:artist", artists);
    metadata.insert("xesam:title", pTrack->getTitle());
    return metadata;
}

void MprisPlayer::slotPlayChanged(DeckAttributes *pDeck, bool playing) {
    bool otherDeckPlaying = false;
    DeckAttributes *playingDeck = playing ? pDeck : nullptr;
    for (unsigned int i = 0; i < m_deckAttributes.size(); ++i) {
        if (m_deckAttributes.at(i)->group != pDeck->group &&
            m_deckAttributes.at(i)->isPlaying()) {
            otherDeckPlaying = true;
            playingDeck = m_deckAttributes.at(i);
            break;
        }
    }
    m_pMpris->notifyPropertyChanged(playerInterfaceName,"PlaybackStatus",
                                    playing || otherDeckPlaying ?
                                    kPlaybackStatusPlaying :
                                    kPlaybackStatusPaused);
    if (!playing && !otherDeckPlaying) {
        m_pMpris->notifyPropertyChanged(playerInterfaceName,"Metadata",
                                        QVariantMap());
    }
    else if (!playing || !otherDeckPlaying) {
        m_pMpris->notifyPropertyChanged(playerInterfaceName,"Metadata",
                                        getMetadataFromTrack(playingDeck->getLoadedTrack()));
    }
}

MprisPlayer::~MprisPlayer() {
    for (DeckAttributes *attrib : m_deckAttributes) {
        delete attrib;
    }
}

void MprisPlayer::slotPlayPositionChanged(DeckAttributes *pDeck, double position) {
    if (AUTODJIDLE) {
        qlonglong playPosition = static_cast<qlonglong>(position * //Fraction of duration
                                                        pDeck->getLoadedTrack()->getDuration() * //Duration in seconds
                                                        1e6);
        m_pMpris->notifyPropertyChanged(playerInterfaceName,"Position",playPosition);
    }
}

DeckAttributes *MprisPlayer::findPlayingDeck() const {
    for (int i = 0; i < m_deckAttributes.count(); ++i) {
        if (m_deckAttributes.at(i)->isPlaying()) {
            return m_deckAttributes.at(i);
        }
    }
    return nullptr;
}


