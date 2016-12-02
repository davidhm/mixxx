#include "library/crate/cratestorage.h"

#include "library/crate/crateschema.h"
#include "library/dao/trackschema.h"

#include "util/db/dbconnection.h"
#include "util/db/sqltransaction.h"
#include "util/db/fwdsqlquery.h"

#include <QtDebug>


namespace {

const QString CRATETABLE_NAME = "name";
const QString CRATETABLE_LOCKED = "locked";

const QString CRATE_SUMMARY_VIEW = "crate_summary";

const QString CRATESUMMARY_TRACK_COUNT = "track_count";
const QString CRATESUMMARY_TRACK_DURATION = "track_duration";

const QString kCrateTracksJoin = QString(
        "LEFT JOIN %3 ON %3.%4=%1.%2").arg(
                CRATE_TABLE,
                CRATETABLE_ID,
                CRATE_TRACKS_TABLE,
                CRATETRACKSTABLE_CRATEID);

const QString kLibraryTracksJoin = kCrateTracksJoin + QString(
        " LEFT JOIN %3 ON %3.%4=%1.%2").arg(
                CRATE_TRACKS_TABLE,
                CRATETRACKSTABLE_TRACKID,
                LIBRARY_TABLE,
                LIBRARYTABLE_ID);

const QString kCrateSummaryViewSelect = QString(
        "SELECT %1.*,"
            "COUNT(CASE %2.%4 WHEN 0 THEN 1 ELSE NULL END) AS %5,"
            "SUM(CASE %2.%4 WHEN 0 THEN %2.%3 ELSE 0 END) AS %6 "
            "FROM %1").arg(
                CRATE_TABLE,
                LIBRARY_TABLE,
                LIBRARYTABLE_DURATION,
                LIBRARYTABLE_MIXXXDELETED,
                CRATESUMMARY_TRACK_COUNT,
                CRATESUMMARY_TRACK_DURATION);

const QString kCrateSummaryViewQuery = QString(
            "CREATE TEMPORARY VIEW IF NOT EXISTS %1 AS %2 %3 GROUP BY %4.%5").arg(
                    CRATE_SUMMARY_VIEW,
                    kCrateSummaryViewSelect,
                    kLibraryTracksJoin,
                    CRATE_TABLE,
                    CRATETABLE_ID);


class CrateQueryBinder {
  public:
    explicit CrateQueryBinder(FwdSqlQuery& query)
        : m_query(query) {
    }
    virtual ~CrateQueryBinder() {}

    void bindId(const QString& placeholder, const Crate& crate) const {
        m_query.bindValue(placeholder, crate.getId());
    }
    void bindName(const QString& placeholder, const Crate& crate) const {
        m_query.bindValue(placeholder, crate.getName());
    }
    void bindLocked(const QString& placeholder, const Crate& crate) const {
        m_query.bindValue(placeholder, crate.isLocked());
    }
    void bindAutoDjSource(const QString& placeholder, const Crate& crate) const {
        m_query.bindValue(placeholder, crate.isAutoDjSource());
    }

  protected:
    FwdSqlQuery& m_query;
};


class CrateSummaryQueryBinder: public CrateQueryBinder {
  public:
    explicit CrateSummaryQueryBinder(FwdSqlQuery& query)
        : CrateQueryBinder(query) {
    }
    ~CrateSummaryQueryBinder() override {}

    void bindTrackCount(const QString& placeholder, const CrateSummary& crateSummary) const {
        m_query.bindValue(placeholder, crateSummary.getTrackCount());
    }
    void bindTrackDuration(const QString& placeholder, const CrateSummary& crateSummary) const {
        m_query.bindValue(placeholder, crateSummary.getTrackDuration());
    }
};

} // anonymous namespace


CrateQueryFields::CrateQueryFields(const FwdSqlQuery& query)
    : m_iId(query.fieldIndex(CRATETABLE_ID)),
      m_iName(query.fieldIndex(CRATETABLE_NAME)),
      m_iLocked(query.fieldIndex(CRATETABLE_LOCKED)),
      m_iAutoDjSource(query.fieldIndex(CRATETABLE_AUTODJ_SOURCE)) {
}


void CrateQueryFields::readValues(const FwdSqlQuery& query, Crate* pCrate) const {
    pCrate->setId(getId(query));
    pCrate->setName(getName(query));
    pCrate->setLocked(isLocked(query));
    pCrate->setAutoDjSource(isAutoDjSource(query));
}


CrateTrackQueryFields::CrateTrackQueryFields(const FwdSqlQuery& query)
    : m_iCrateId(query.fieldIndex(CRATETRACKSTABLE_CRATEID)),
      m_iTrackId(query.fieldIndex(CRATETRACKSTABLE_TRACKID)) {
}


CrateSummaryQueryFields::CrateSummaryQueryFields(const FwdSqlQuery& query)
    : CrateQueryFields(query),
      m_iTrackCount(query.fieldIndex(CRATESUMMARY_TRACK_COUNT)),
      m_iTrackDuration(query.fieldIndex(CRATESUMMARY_TRACK_DURATION)) {
}

void CrateSummaryQueryFields::readValues(
        const FwdSqlQuery& query,
        CrateSummary* pCrateSummary) const {
    CrateQueryFields::readValues(query, pCrateSummary);
    pCrateSummary->setTrackCount(getTrackCount(query));
    pCrateSummary->setTrackDuration(getTrackDuration(query));
}


CrateStorage::CrateStorage() {
}


CrateStorage::~CrateStorage() {
}


void CrateStorage::repairDatabase(QSqlDatabase database) {
    DEBUG_ASSERT(!m_database.isOpen());

    // Crates
    {
        FwdSqlQuery query(database, QString(
                "DELETE FROM %1 WHERE %2 IS NULL OR TRIM(%2)=''").arg(
                        CRATE_TABLE,
                        CRATETABLE_NAME));
        if (query.execPrepared() && (query.numRowsAffected() > 0)) {
            qWarning() << "Deleted" << query.numRowsAffected() << "crates with empty names";
        }
    }
    {
        FwdSqlQuery query(database, QString(
                "UPDATE %1 SET %2=0 WHERE %2 NOT IN (0,1)").arg(
                        CRATE_TABLE,
                        CRATETABLE_LOCKED));
        if (query.execPrepared() && (query.numRowsAffected() > 0)) {
            qWarning() << "Fixed boolean values in"
                    "table" << CRATE_TABLE
                    << "column" << CRATETABLE_LOCKED
                    << "for" << query.numRowsAffected() << "crates";
        }
    }
    {
        FwdSqlQuery query(database, QString(
                "UPDATE %1 SET %2=0 WHERE %2 NOT IN (0,1)").arg(
                        CRATE_TABLE,
                        CRATETABLE_AUTODJ_SOURCE));
        if (query.execPrepared() && (query.numRowsAffected() > 0)) {
            qWarning() << "Fixed boolean values in"
                    "table" << CRATE_TABLE
                    << "column" << CRATETABLE_AUTODJ_SOURCE
                    << "for" << query.numRowsAffected() << "crates";
        }
    }

    // Crate tracks
    {
        FwdSqlQuery query(database, QString(
                "DELETE FROM %1 WHERE %2 NOT IN (SELECT %3 FROM %4)").arg(
                        CRATE_TRACKS_TABLE,
                        CRATETRACKSTABLE_CRATEID,
                        CRATETABLE_ID,
                        CRATE_TABLE));
        if (query.execPrepared() && (query.numRowsAffected() > 0)) {
            qWarning() << "Deleted" << query.numRowsAffected() << "crate tracks of non-existent crates";
        }
    }
    {
        FwdSqlQuery query(database, QString(
                "DELETE FROM %1 WHERE %2 NOT IN (SELECT %3 FROM %4)").arg(
                        CRATE_TRACKS_TABLE,
                        CRATETRACKSTABLE_TRACKID,
                        LIBRARYTABLE_ID,
                        LIBRARY_TABLE));
        if (query.execPrepared() && (query.numRowsAffected() > 0)) {
            qWarning() << "Deleted" << query.numRowsAffected() << "crate tracks of non-existent tracks";
        }
    }
}


void CrateStorage::attachDatabase(QSqlDatabase database) {
    m_database = database;
    createViews();
    refreshCaches();
}


void CrateStorage::detachDatabase() {
    clearCaches();
}


void CrateStorage::clearCaches() {
    m_deletedCratesCache.clear();
    m_trackCratesCache.clear();
}


void CrateStorage::refreshCaches() {
    clearCaches();
    {
        // Minor optimization: Reserve space in cache before populating
        FwdSqlQuery query(m_database, QString(
                "SELECT COUNT(*) FROM %1").arg(
                        CRATE_TRACKS_TABLE));
        if (query.execPrepared() && query.next()) {
            m_trackCratesCache.reserve(query.fieldValue(0).toUInt());
            DEBUG_ASSERT(!query.next());
        }
    }
    {
        FwdSqlQuery query(m_database, QString(
                "SELECT %1,%2 FROM %3").arg(
                        CRATETRACKSTABLE_TRACKID,
                        CRATETRACKSTABLE_CRATEID,
                        CRATE_TRACKS_TABLE));
        if (query.execPrepared()) {
            CrateTrackSelectIterator crateTracks(query);
            while (crateTracks.next()) {
                m_trackCratesCache.insert(
                        crateTracks.trackId(),
                        crateTracks.crateId());
            }
        }
    }
}


void CrateStorage::createViews() {
    FwdSqlQuery(m_database, kCrateSummaryViewQuery).execPrepared();
}


uint CrateStorage::countCrates() const {
    FwdSqlQuery query(m_database, QString(
            "SELECT COUNT(*) FROM %1").arg(
                    CRATE_TABLE));
    if (query.execPrepared() && query.next()) {
        uint result = query.fieldValue(0).toUInt();
        DEBUG_ASSERT(!query.next());
        return result;
    } else {
        return 0;
    }
}


bool CrateStorage::readCrateById(CrateId id, Crate* pCrate) const {
    DEBUG_ASSERT(pCrate != nullptr);
    FwdSqlQuery query(m_database, QString(
            "SELECT * FROM %1 WHERE %2=:id").arg(
                    CRATE_TABLE,
                    CRATETABLE_ID));
    query.bindValue(":id", id);
    if (query.execPrepared()) {
        CrateSelectIterator crates(query);
        if ((pCrate != nullptr) ? crates.readNext(pCrate) : crates.next()) {
            DEBUG_ASSERT_AND_HANDLE(!crates.next()) {
                qWarning() << "Ambiguous crate id:" << id;
            }
            return true;
        } else {
            qWarning() << "Crate not found by id:" << id;
        }
    }
    return false;
}


bool CrateStorage::readCrateByName(const QString& name, Crate* pCrate) const {
    FwdSqlQuery query(m_database, QString(
            "SELECT * FROM %1 WHERE %2=:name").arg(
                    CRATE_TABLE,
                    CRATETABLE_NAME));
    query.bindValue(":name", name);
    if (query.execPrepared()) {
        CrateSelectIterator crates(query);
        if ((pCrate != nullptr) ? crates.readNext(pCrate) : crates.next()) {
            DEBUG_ASSERT_AND_HANDLE(!crates.next()) {
                qWarning() << "Ambiguous crate name:" << name;
            }
            return true;
        } else {
            qDebug() << "Crate not found by name:" << name;
        }
    }
    return false;
}


CrateSelectIterator CrateStorage::selectCrates() const {
    FwdSqlQuery query(m_database, QString(
            "SELECT * FROM %1 ORDER BY %2 COLLATE %3").arg(
            CRATE_TABLE,
            CRATETABLE_NAME,
            DbConnection::kStringCollationFunc));

    if (query.execPrepared()) {
        return CrateSelectIterator(query);
    } else {
        return CrateSelectIterator();
    }
}


CrateSelectIterator CrateStorage::selectAutoDjCrates(bool autoDjSource) const {
    FwdSqlQuery query(m_database, QString(
            "SELECT * FROM %1 WHERE %2=:autoDjSource ORDER BY %3 COLLATE %4").arg(
            CRATE_TABLE,
            CRATETABLE_AUTODJ_SOURCE,
            CRATETABLE_NAME,
            DbConnection::kStringCollationFunc));
    query.bindValue(":autoDjSource", autoDjSource);
    if (query.execPrepared()) {
        return CrateSelectIterator(query);
    } else {
        return CrateSelectIterator();
    }
}


CrateSummarySelectIterator CrateStorage::selectCrateSummaries() const {
    FwdSqlQuery query(m_database, QString(
            "SELECT * FROM %1 ORDER BY %2 COLLATE %3").arg(
            CRATE_SUMMARY_VIEW,
            CRATETABLE_NAME,
            DbConnection::kStringCollationFunc));
    if (query.execPrepared()) {
        return CrateSummarySelectIterator(query);
    } else {
        return CrateSummarySelectIterator();
    }
}


uint CrateStorage::countCrateTracks(CrateId crateId) const {
    FwdSqlQuery query(m_database, QString(
            "SELECT COUNT(*) FROM %1 WHERE %2=:crateId").arg(
                    CRATE_TRACKS_TABLE,
                    CRATETRACKSTABLE_CRATEID));
    query.bindValue(":crateId", crateId);
    if (query.execPrepared() && query.next()) {
        uint result = query.fieldValue(0).toUInt();
        DEBUG_ASSERT(!query.next());
        return result;
    } else {
        return 0;
    }
}


QString CrateStorage::formatSubselectQueryForCrateTracks(
        CrateId crateId,
        const QString& trackIdColumn) const {
    return QString("%1 IN (SELECT %2 FROM %3 WHERE %4=%5)").arg(
            trackIdColumn,
            CRATETRACKSTABLE_TRACKID,
            CRATE_TRACKS_TABLE,
            CRATETRACKSTABLE_CRATEID,
            crateId.toString());

}


bool CrateStorage::isCrateTrack(CrateId crateId, TrackId trackId) const {
    return m_trackCratesCache.contains(trackId, crateId) &&
            !m_deletedCratesCache.contains(crateId);
}


CrateTrackSelectIterator CrateStorage::selectCrateTracks(CrateId crateId) const {
    // NOTE(uklotzde): The ORDER BY clause ensures a stable ordering,
    // because we are returning the result as a list for efficiency
    // reasons. Conceptually the result is an unordered set.
    FwdSqlQuery query(m_database, QString(
            "SELECT %1 FROM %2 WHERE %3=:crateId ORDER BY %1").arg(
                    CRATETRACKSTABLE_TRACKID,
                    CRATE_TRACKS_TABLE,
                    CRATETRACKSTABLE_CRATEID));
    query.bindValue(":crateId", crateId);
    if (query.execPrepared()) {
        return CrateTrackSelectIterator(query);
    } else {
        return CrateTrackSelectIterator();
    }
}


bool CrateStorage::onInsertingCrate(
        SqlTransaction& transaction,
        const Crate& crate,
        CrateId* pCrateId) {
    DEBUG_ASSERT(transaction);
    DEBUG_ASSERT_AND_HANDLE(!crate.getId().isValid()) {
        qWarning() << "Cannot insert crate with a valid id:" << crate.getId();
        return false;
    }
    FwdSqlQuery query(m_database, QString(
            "INSERT INTO %1 (%2,%3,%4) VALUES (:name,:locked,:autoDjSource)").arg(
                    CRATE_TABLE,
                    CRATETABLE_NAME,
                    CRATETABLE_LOCKED,
                    CRATETABLE_AUTODJ_SOURCE));
    DEBUG_ASSERT_AND_HANDLE(query.isPrepared()) {
        return false;
    }
    CrateQueryBinder queryBinder(query);
    queryBinder.bindName(":name", crate);
    queryBinder.bindLocked(":locked", crate);
    queryBinder.bindAutoDjSource(":autoDjSource", crate);
    DEBUG_ASSERT_AND_HANDLE(query.execPrepared()) {
        return false;
    }
    if (query.numRowsAffected() > 0) {
        DEBUG_ASSERT(query.numRowsAffected() == 1);
        if (pCrateId != nullptr) {
            *pCrateId = CrateId(query.lastInsertId());
            DEBUG_ASSERT(pCrateId->isValid());
        }
        return true;
    } else {
        return false;
    }
}


void CrateStorage::afterInsertingCrate(
        CrateId crateId) {
    DEBUG_ASSERT(crateId.isValid());
    DEBUG_ASSERT_AND_HANDLE(!m_deletedCratesCache.contains(crateId)) {
        m_deletedCratesCache.remove(crateId);
    }
}


bool CrateStorage::onUpdatingCrate(
        SqlTransaction& transaction,
        const Crate& crate) {
    DEBUG_ASSERT(transaction);
    DEBUG_ASSERT_AND_HANDLE(crate.getId().isValid()) {
        qWarning() << "Cannot update crate without a valid id";
        return false;
    }
    FwdSqlQuery query(m_database, QString(
            "UPDATE %1 SET %2=:name,%3=:locked,%4=:autoDjSource WHERE %5=:id").arg(
                    CRATE_TABLE,
                    CRATETABLE_NAME,
                    CRATETABLE_LOCKED,
                    CRATETABLE_AUTODJ_SOURCE,
                    CRATETABLE_ID));
    DEBUG_ASSERT_AND_HANDLE(query.isPrepared()) {
        return false;
    }
    CrateQueryBinder queryBinder(query);
    queryBinder.bindId(":id", crate);
    queryBinder.bindName(":name", crate);
    queryBinder.bindLocked(":locked", crate);
    queryBinder.bindAutoDjSource(":autoDjSource", crate);
    DEBUG_ASSERT_AND_HANDLE(query.execPrepared()) {
        return false;
    }
    if (query.numRowsAffected() > 0) {
        DEBUG_ASSERT_AND_HANDLE(query.numRowsAffected() <= 1) {
            qWarning() << "Updated multiple crates with the same id" << crate.getId();
        }
        return true;
    } else {
        qWarning() << "Cannot update non-existent crate with id" << crate.getId();
        return false;
    }
}


void CrateStorage::afterUpdatingCrate(
        CrateId crateId) {
    DEBUG_ASSERT(crateId.isValid());
    DEBUG_ASSERT_AND_HANDLE(!m_deletedCratesCache.contains(crateId)) {
        m_deletedCratesCache.remove(crateId);
    }
}


bool CrateStorage::onDeletingCrate(
        SqlTransaction& transaction,
        CrateId crateId) {
    DEBUG_ASSERT_AND_HANDLE(transaction) {
        return false;
    }
    DEBUG_ASSERT_AND_HANDLE(crateId.isValid()) {
        qWarning() << "Cannot delete crate without a valid id";
        return false;
    }
    {
        FwdSqlQuery query(m_database, QString(
                "DELETE FROM %1 WHERE %2=:id").arg(
                        CRATE_TRACKS_TABLE,
                        CRATETRACKSTABLE_CRATEID));
        DEBUG_ASSERT_AND_HANDLE(query.isPrepared()) {
            return false;
        }
        query.bindValue(":id", crateId);
        DEBUG_ASSERT_AND_HANDLE(query.execPrepared()) {
            return false;
        }
        if (query.numRowsAffected() <= 0) {
            qDebug() << "Deleting empty crate with id" << crateId;
        }
    }
    {
        FwdSqlQuery query(m_database, QString(
                "DELETE FROM %1 WHERE %2=:id").arg(
                        CRATE_TABLE,
                        CRATETABLE_ID));
        DEBUG_ASSERT_AND_HANDLE(query.isPrepared()) {
            return false;
        }
        query.bindValue(":id", crateId);
        DEBUG_ASSERT_AND_HANDLE(query.execPrepared()) {
            return false;
        }
        if (query.numRowsAffected() > 0) {
            DEBUG_ASSERT_AND_HANDLE(query.numRowsAffected() <= 1) {
                qWarning() << "Deleted multiple crates with the same id" << crateId;
            }
            return true;
        } else {
            qWarning() << "Cannot delete non-existent crate with id" << crateId;
            return false;
        }
    }
}


void CrateStorage::afterDeletingCrate(
        CrateId crateId) {
    DEBUG_ASSERT(crateId.isValid());
    // NOTE (uklotzde): Updating the in-memory crates per track cache
    // would be inefficient and is not necessary. The cache is rebuilt
    // upon startup and the deleted crate id will never be reused.
    // Instead we keep track of the deleted crates and perform some
    // post-filtering if required.
    m_deletedCratesCache.insert(crateId);
}


bool CrateStorage::onAddingCrateTracks(
        SqlTransaction& transaction,
        CrateId crateId,
        const QList<TrackId>& trackIds) {
    DEBUG_ASSERT_AND_HANDLE(transaction) {
        return false;
    }
    FwdSqlQuery query(m_database, QString(
            "INSERT OR IGNORE INTO %1 (%2, %3) VALUES (:crateId,:trackId)").arg(
                    CRATE_TRACKS_TABLE,
                    CRATETRACKSTABLE_CRATEID,
                    CRATETRACKSTABLE_TRACKID));
    if (!query.isPrepared()) {
        return false;
    }
    query.bindValue(":crateId", crateId);
    for (const auto& trackId: trackIds) {
        query.bindValue(":trackId", trackId);
        if (!query.execPrepared()) {
            return false;
        }
        if (query.numRowsAffected() == 0) {
            // track is already in crate
            qDebug() << "Track" << trackId << "not added to crate" << crateId;
        } else {
            DEBUG_ASSERT(query.numRowsAffected() == 1);
        }
    }
    return true;
}


void CrateStorage::afterAddingCrateTracks(
        CrateId crateId,
        const QList<TrackId>& trackIds) {
    for (const auto& trackId: trackIds) {
        m_trackCratesCache.insert(trackId, crateId);
    }
}


bool CrateStorage::onRemovingCrateTracks(
        SqlTransaction& transaction,
        CrateId crateId,
        const QList<TrackId>& trackIds) {
    // NOTE(uklotzde): We remove tracks in a transaction and loop
    // analogously to adding tracks (see above).
    DEBUG_ASSERT_AND_HANDLE(transaction) {
        return false;
    }
    FwdSqlQuery query(m_database, QString(
            "DELETE FROM %1 WHERE %2=:crateId AND %3=:trackId").arg(
                    CRATE_TRACKS_TABLE,
                    CRATETRACKSTABLE_CRATEID,
                    CRATETRACKSTABLE_TRACKID));
    if (!query.isPrepared()) {
        return false;
    }
    query.bindValue(":crateId", crateId);
    for (const auto& trackId: trackIds) {
        query.bindValue(":trackId", trackId);
        if (!query.execPrepared()) {
            return false;
        }
        if (query.numRowsAffected() == 0) {
            // track not found in crate
            qDebug() << "Track" << trackId << "not removed from crate" << crateId;
        } else {
            DEBUG_ASSERT(query.numRowsAffected() == 1);
        }
    }
    return true;
}


void CrateStorage::afterRemovingCrateTracks(
        CrateId crateId,
        const QList<TrackId>& trackIds) {
    for (const auto& trackId: trackIds) {
        m_trackCratesCache.remove(trackId, crateId);
    }
}


QSet<CrateId> CrateStorage::afterHidingOrUnhidingTracks(const QList<TrackId>& trackIds) {
    // Collect all crates from which contains tracks
    QSet<CrateId> trackCrates;
    for (const auto& trackId: trackIds) {
        QList<CrateId> crateIds = m_trackCratesCache.values(trackId);
        for (const auto& crateId: crateIds) {
            trackCrates.insert(crateId);
        }
    }
    return trackCrates;
}


bool CrateStorage::onPurgingTracks(
        SqlTransaction& transaction,
        const QList<TrackId>& trackIds) {
    // NOTE(uklotzde): We remove tracks in a transaction and loop
    // analogously to removing tracks from crates (see above).
    DEBUG_ASSERT(transaction);

    FwdSqlQuery query(m_database, QString(
            "DELETE FROM %1 WHERE %2=:trackId").arg(
                    CRATE_TRACKS_TABLE,
                    CRATETRACKSTABLE_TRACKID));
    if (!query.isPrepared()) {
        return false;
    }
    for (const auto& trackId: trackIds) {
        query.bindValue(":trackId", trackId);
        if (!query.execPrepared()) {
            return false;
        }
    }
    return true;
}


QMap<CrateId, QList<TrackId>> CrateStorage::afterPurgingTracks(
        const QList<TrackId>& trackIds) {
    // Update the cache and collect all crates from which tracks
    // have been purged
    typedef QMap<CrateId, QList<TrackId>> CrateTracks;
    CrateTracks purgedCrateTracks;
    for (const auto& trackId: trackIds) {
        QList<CrateId> crateIds = m_trackCratesCache.values(trackId);
        for (const auto& crateId: crateIds) {
            if (!m_deletedCratesCache.contains(crateId)) {
                auto it = purgedCrateTracks.find(crateId);
                if (it == purgedCrateTracks.end()) {
                    it = purgedCrateTracks.insert(crateId, QList<TrackId>());
                }
                DEBUG_ASSERT(it != purgedCrateTracks.end());
                it.value().append(trackId);
            }
        }
        m_trackCratesCache.remove(trackId);
    }
    return purgedCrateTracks;
}
