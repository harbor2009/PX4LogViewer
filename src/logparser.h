#ifndef LOGPARSER_H
#define LOGPARSER_H

#include <QObject>
#include <QUrl>
#include <QVariantMap>
#include <QLineSeries>
#include <QAbstractSeries>
#include <QDateTime>
#include "sdlog2_format.h"
#include "sdlog2_messages.h"

using namespace QtCharts;

enum DataType {
    type_none,
    type_int8_t,
    type_uint8_t,
    type_int16_t,
    type_uint16_t,
    type_int32_t,
    type_uint32_t,
    type_float,
    type_char4,
    type_char16,
    type_char64,
    type_int64_t,
    type_uint64_t
};

struct log_format_struct {
    uint8_t type;
    uint8_t length;		// full packet length including header
    QString name;
    QString format;
    QString labels;
    QStringList nameList;
    QList<int> sizeList;
    QList<DataType> typeList;
};

class LogParser : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QUrl path READ path WRITE setPath NOTIFY pathChanged)
    Q_PROPERTY(bool ready READ ready NOTIFY readyChanged)
    Q_PROPERTY(QString arch MEMBER mArch NOTIFY archChanged)
    Q_PROPERTY(QVariantMap info READ info NOTIFY infoChanged)
public:
    explicit LogParser(QObject *parent = 0);

    QUrl path() { return mPath; }
    void setPath(const QUrl &path);

    bool ready() { return isReady; }
    QVariantMap info() { return mInfo; }

    Q_INVOKABLE QVariantMap getParams() { return parameterList; }
    Q_INVOKABLE QVariantMap getItemsName();
    Q_INVOKABLE QVariantList getData(const QString name);
    Q_INVOKABLE QVector<QPointF> getDataSeries(const QString name);
    Q_INVOKABLE int getDataLength(const QString &name);
    Q_INVOKABLE void update(QAbstractSeries *series, const QString &name);
    Q_INVOKABLE QDateTime minDate();
    Q_INVOKABLE QDateTime maxDate();
    Q_INVOKABLE qreal getMaxY(const QString &name);
    Q_INVOKABLE qreal getMinY(const QString &name);
    Q_INVOKABLE quint64 getMaxX(const QString &name);
    Q_INVOKABLE quint64 getMinX(const QString &name);
    Q_INVOKABLE QDateTime ms2date(quint64 ms) {return QDateTime::fromMSecsSinceEpoch(ms); }

signals:
    void pathChanged();
    void readyChanged(bool isReady);
    void archChanged();
    void infoChanged();

public slots:

private:
    QUrl mPath;
    bool isReady;
    QVariantMap parameterList;
    QMap<uint8_t, log_format_struct> formatList;
    QMap<QString, QList<QVariant> > data;
    QMap<QString, QVector<QPointF> > lineSeries;
    QMap<DataType, QString> typeName;
    QMap<char, DataType> typeEnum;
    QMap<char, int> typeSize;
    QString mArch;
    QString fw_git;
    quint64 originTimestamp;
    quint64 currentTimestamp;
    QVariantMap mInfo;

    QString charArray2string(const char *item, int size);
    void reformat(const log_format_s &oldf, log_format_struct &newf);
    void saveData(log_format_struct &format, char *buf, int len);
};

#endif // LOGPARSER_H
