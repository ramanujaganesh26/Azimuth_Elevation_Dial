#ifndef LOGMODEL_H
#define LOGMODEL_H

#include <QAbstractListModel>
#include <QDateTime>
#include <QVector>

struct LogEntry {
    QString timestamp;
    QString transport;
    QString source;
    QString event;
    QString messageType;
    QString details;
    QString status;
    bool isError;
    QString rawHex;
};

class LogModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)
    Q_PROPERTY(bool paused READ isPaused WRITE setPaused NOTIFY pausedChanged)

public:
    enum LogRoles {
        TimestampRole = Qt::UserRole + 1,
        TransportRole,
        SourceRole,
        EventRole,
        MessageTypeRole,
        DetailsRole,
        StatusRole,
        IsErrorRole,
        RawHexRole
    };

    explicit LogModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    int count() const { return m_entries.size(); }
    bool isPaused() const { return m_isPaused; }
    void setPaused(bool paused);

    Q_INVOKABLE void clear();
    Q_INVOKABLE QString getHexAt(int index) const;

    void appendEntry(const QString &transport,
                     const QString &source,
                     const QString &event,
                     const QString &messageType,
                     const QString &details,
                     const QString &status,
                     bool isError,
                     const QString &rawHex);

signals:
    void countChanged();
    void pausedChanged();
    void entryAdded();

private:
    static constexpr int MAX_LOG_CAPACITY = 1000;
    QVector<LogEntry> m_entries;
    QVector<LogEntry> m_pendingWhilePaused;
    bool m_isPaused;
};

#endif // LOGMODEL_H
