
#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include "listenbrainzjsonfactory.h"


QByteArray ListenBrainzJSONFactory::getJSONFromTrack(TrackPointer pTrack, JsonType type) {
    QJsonObject jsonObject;
    QString stringType;
    if (type == NowListening) {
        stringType = "playing_now";
    }
    else {
        stringType = "single";
    }

    QJsonArray payloadArray;
    QJsonObject payloadObject;
    QJsonObject metadataObject;
    QString title = pTrack->getTitle();
    QString artist = pTrack->getArtist();
    metadataObject.insert("artist_name",artist);
    metadataObject.insert("track_name",title);
    payloadObject.insert("track_metadata",metadataObject);
    qint64 timeStamp = QDateTime::currentSecsSinceEpoch();

    if (type == Single) {
        payloadObject.insert("listened_at",timeStamp);
    }
    payloadArray.append(payloadObject);
    jsonObject.insert("listen_type",stringType);
    jsonObject.insert("payload",payloadArray);
    QJsonDocument doc(jsonObject);
    return doc.toJson(QJsonDocument::Compact);
}
