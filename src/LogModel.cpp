#include "LogModel.h"

LogModel::LogModel(QObject *parent)
    : QAbstractListModel(parent)
    , m_isPaused(false)
{
    m_entries.reserve(MAX_LOG_CAPACITY);
}

int LogModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) return 0;
    return m_entries.size();
}

QVariant LogModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_entries.size()) {
        return QVariant();
    }

    const LogEntry &entry = m_entries.at(index.row());
    switch (role) {
    case TimestampRole:
        return entry.timestamp;
    case TransportRole:
        return entry.transport;
    case SourceRole:
        return entry.source;
    case EventRole:
        return entry.event;
    case MessageTypeRole:
        return entry.messageType;
    case DetailsRole:
        return entry.details;
    case StatusRole:
        return entry.status;
    case IsErrorRole:
        return entry.isError;
    case RawHexRole:
        return entry.rawHex;
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> LogModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[TimestampRole]   = "timestamp";
    roles[TransportRole]   = "transport";
    roles[SourceRole]      = "source";
    roles[EventRole]       = "event";
    roles[MessageTypeRole] = "messageType";
    roles[DetailsRole]     = "details";
    roles[StatusRole]      = "status";
    roles[IsErrorRole]     = "isError";
    roles[RawHexRole]      = "rawHex";
    return roles;
}

void LogModel::setPaused(bool paused)
{
    if (m_isPaused == paused) return;
    m_isPaused = paused;
    emit pausedChanged();

    if (!m_isPaused && !m_pendingWhilePaused.isEmpty()) {
        // Flush pending entries
        for (const auto &entry : std::as_const(m_pendingWhilePaused)) {
            if (m_entries.size() >= MAX_LOG_CAPACITY) {
                beginRemoveRows(QModelIndex(), 0, 0);
                m_entries.removeFirst();
                endRemoveRows();
            }
            int newRow = m_entries.size();
            beginInsertRows(QModelIndex(), newRow, newRow);
            m_entries.append(entry);
            endInsertRows();
        }
        m_pendingWhilePaused.clear();
        emit countChanged();
        emit entryAdded();
    }
}

void LogModel::clear()
{
    if (!m_entries.isEmpty()) {
        beginResetModel();
        m_entries.clear();
        endResetModel();
        emit countChanged();
    }
    m_pendingWhilePaused.clear();
}

QString LogModel::getHexAt(int index) const
{
    if (index >= 0 && index < m_entries.size()) {
        return m_entries.at(index).rawHex;
    }
    return QString();
}

void LogModel::appendEntry(const QString &transport,
                           const QString &source,
                           const QString &event,
                           const QString &messageType,
                           const QString &details,
                           const QString &status,
                           bool isError,
                           const QString &rawHex)
{
    LogEntry entry;
    entry.timestamp   = QDateTime::currentDateTime().toString(QStringLiteral("hh:mm:ss.zzz"));
    entry.transport   = transport;
    entry.source      = source;
    entry.event       = event;
    entry.messageType = messageType;
    entry.details     = details;
    entry.status      = status;
    entry.isError     = isError;
    entry.rawHex      = rawHex;

    if (m_isPaused) {
        if (m_pendingWhilePaused.size() < MAX_LOG_CAPACITY) {
            m_pendingWhilePaused.append(entry);
        }
        return;
    }

    if (m_entries.size() >= MAX_LOG_CAPACITY) {
        beginRemoveRows(QModelIndex(), 0, 0);
        m_entries.removeFirst();
        endRemoveRows();
    }

    int newRow = m_entries.size();
    beginInsertRows(QModelIndex(), newRow, newRow);
    m_entries.append(entry);
    endInsertRows();

    emit countChanged();
    emit entryAdded();
}
