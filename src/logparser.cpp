#include "logparser.h"
#include <QDebug>
#include <QFile>

struct log_header_s {
    uint8_t head1;
    uint8_t head2;
    uint8_t msg_type;
};

LogParser::LogParser(QObject *parent) : QObject(parent)
  , isReady(false)
{
    char lable[] = {'b','B','h','H','i','I','f','n','N',
                    'Z','c','C','e','E','L','M','q','Q'};
    QString type[] = {"int8_t","uint8_t","int16_t","uint16_t",
                      "int32_t","uint32_t","float","char[4]",
                      "char[16]","char[64]","int16_t*100",
                      "uint16_t*100","int32_t*100","uint32_t*100",
                      "int32_t latitude/longitude","uint8_t flightmode",
                      "int64_t","uint64_t"};
    DataType dtype[] = {type_int8_t,type_uint8_t,type_int16_t,type_uint16_t,
                        type_int32_t,type_uint32_t,type_float,type_char4,
                        type_char16,type_char64,type_int16_t,type_uint16_t,
                        type_int32_t,type_uint32_t,type_int32_t,
                        type_uint8_t,type_int64_t,type_uint64_t};
    int size[] = {sizeof(int8_t),sizeof(uint8_t),sizeof(int16_t),sizeof(uint16_t),
                  sizeof(int32_t),sizeof(uint32_t),sizeof(float),sizeof(char[4]),
                  sizeof(char[16]),sizeof(char[64]),sizeof(int16_t),sizeof(uint16_t),
                  sizeof(int32_t),sizeof(uint32_t),sizeof(int32_t),sizeof(uint8_t),
                  sizeof(int64_t),sizeof(uint64_t)};
    for(unsigned int i=0; i<sizeof(lable); i++) {
        typeName.insert(dtype[i], type[i]);
        typeEnum.insert(lable[i], dtype[i]);
        typeSize.insert(lable[i], size[i]);
    }
}

void LogParser::setPath(const QUrl &path)
{
    if(mPath != path) {
        QFile logFile(path.toLocalFile());
        if (!logFile.open(QIODevice::ReadOnly))
            return;

        mPath = path;
        qDebug() << "path:" << mPath;

        isReady = false;
        originTimestamp = 0;
        parameterList.clear();
        formatList.clear();
        data.clear();
        lineSeries.clear();
        mInfo.clear();
        mArch = "Unknown";
        fw_git = "";

        log_header_s header;
        qint64 bytes;
        int errorNum = 0;
        while(!logFile.atEnd()) {
            bytes = logFile.read((char *)(&header), sizeof(log_header_s));
            if(bytes != sizeof(log_header_s)
                    || header.head1 != HEAD_BYTE1
                    || header.head2 != HEAD_BYTE2) {
                qDebug() << "header error" << header.head1 << header.head2 << header.msg_type;
                errorNum ++;
                break;
            }

            if(header.msg_type == LOG_FORMAT_MSG) {
                log_format_s lf;
                bytes = logFile.read((char *)(&lf), sizeof(log_format_s));
                if(bytes != sizeof(log_format_s)) {
                    qDebug() << "format message error";
                    errorNum ++;
                    break;
                }
                if(lf.type == LOG_VER_MSG) {
                    continue;
                }
//                qDebug() << "== format:" << log_msg_format.name
//                         << log_msg_format.labels << log_msg_format.format
//                         << log_msg_format.type << log_msg_format.length;
                log_format_struct lf_s;
                reformat(lf, lf_s);
                formatList.insert(lf_s.type, lf_s);
                foreach (QString name, lf_s.nameList) {
                    QList<QVariant> list;
                    data.insert(name, list);

                    QVector<QPointF> ls;
                    lineSeries.insert(name, ls);
                }

            } else if (header.msg_type == LOG_VER_MSG) {
                log_VER_s ver;
                bytes = logFile.read((char *)(&ver), sizeof(log_VER_s));
                if(bytes != sizeof(log_VER_s)) {
                    qDebug() << "ver error";
                    errorNum ++;
                    break;
                }
                mArch = charArray2string(ver.arch, sizeof(ver.arch));
                emit archChanged();
                mInfo.insert("Hardware Version", mArch);
                fw_git = charArray2string(ver.fw_git, sizeof(ver.fw_git));
                mInfo.insert("Firmware Version", fw_git);
                qDebug() << "=== ver:" << mArch << fw_git;

            } else if (header.msg_type == LOG_PARM_MSG) {
                log_PARM_s param;
                bytes = logFile.read((char *)(&param), sizeof(log_PARM_s));
                if(bytes != sizeof(log_PARM_s)) {
                    qDebug() << "param error";
                    errorNum ++;
                    break;
                }
                int size = param.name[sizeof(param.name) - 1] != 0 ? sizeof(param.name) : -1;
                QString name = QString::fromLocal8Bit(param.name, size);
                parameterList.insert(name, QString::number(param.value));
            } else if(header.msg_type == LOG_TIME_MSG) {
                log_TIME_s time;
                bytes = logFile.read((char *)(&time), sizeof(log_TIME_s));
                if(bytes != sizeof(log_TIME_s)) {
                    qDebug() << "time error";
                    errorNum ++;
                    break;
                }

                log_format_struct format = formatList.value(LOG_TIME_MSG);
                QString name = format.nameList.first();
                QVector<QPointF> &ser = lineSeries[name];

                if(ser.isEmpty()) {
                    originTimestamp = time.t/1000;
                    qDebug() << "originTimestamp:" << originTimestamp;
                }
                currentTimestamp = time.t/1000;// - originTimestamp; //time.t/1000
                qreal x = ser.length();
                qreal y = currentTimestamp;
                ser.append(QPointF(x, y));
                //qDebug() << time.t << QDateTime::fromMSecsSinceEpoch(time.t);

            } else {
                //qDebug() << "..";
                if(formatList.contains(header.msg_type)) {
                    log_format_struct format = formatList.value(header.msg_type);
                    int len = format.length - sizeof(log_header_s);
                    char *buf = new char[len];
                    bytes = logFile.read(buf, len);
                    if(bytes != len) {
                        qDebug() << "read data error" << format.type << format.name;
                        errorNum ++;
                        break;
                    }
                    saveData(format, buf, len);
                    delete buf;
                    buf = 0;
                }
            }
        }

        if(originTimestamp != 0) {
            mInfo.insert("Start Time", QDateTime::fromMSecsSinceEpoch(originTimestamp).toString("yyyy-MM-dd hh:mm''ss"));
            mInfo.insert("Length, s", QString::number((currentTimestamp - originTimestamp)/1000));
        }
        mInfo.insert("Errors", QString::number(errorNum));
        //qDebug() << "---" << data.firstKey() << data.value("GPS.Alt");
        emit pathChanged();
        emit archChanged();
        emit infoChanged();
        isReady = true;
        emit readyChanged(isReady);
        //getItemsName();
    }
}

QString LogParser::charArray2string(const char *item, int size)
{
    return QString::fromLocal8Bit(item, item[size - 1] != 0 ? size : -1);
}

void LogParser::reformat(const log_format_s &oldf, log_format_struct &newf)
{
    newf.type = oldf.type;
    newf.length = oldf.length;
    newf.name = charArray2string(oldf.name, sizeof(oldf.name));
    newf.format = charArray2string(oldf.format, sizeof(oldf.format));
    newf.labels = charArray2string(oldf.labels, sizeof(oldf.labels));
    QStringList lableList = newf.labels.split(',');
    if(newf.format.length() != lableList.length()) {
        qDebug() << "reformat error:" << newf.type << newf.name
                 << newf.format << newf.labels
                 << newf.format.length() << lableList.length();
        return;
    }
    for(int i=0; i<lableList.length(); i++) {
        QString lable = lableList.at(i);
        newf.nameList.append(newf.name + "." + lable);
        char f = newf.format.at(i).toLatin1();
        newf.sizeList.append(typeSize.value(f, 0));
        newf.typeList.append(typeEnum.value(f, type_none));
    }
}

void LogParser::saveData(log_format_struct &format, char *buf, int len)
{
    int pos = 0;
    qreal y = 0;

    for(int i=0; i<format.nameList.length(); i++) {
        QString name = format.nameList.at(i);
        int size = format.sizeList.at(i);
        DataType type = format.typeList.at(i);
        QList<QVariant> &dl = data[name];
        QVector<QPointF> &ser = lineSeries[name];
        switch (type) {
        case type_char4: {
            QString v = charArray2string(buf+pos, size);
            dl.append(v);
            break;
        }
        case type_char16: {
            QString v = charArray2string(buf+pos, size);
            dl.append(v);
            break;
        }
        case type_char64: {
            QString v = charArray2string(buf+pos, size);
            dl.append(v);
            break;
        }
        case type_float: {
            //float v;// = float(*(buf+pos));
            //memcpy(&v, buf+pos, sizeof(float));
            float *v = (float *)(buf+pos);
            dl.append(*v);
            y = *v;
            break;
        }
        case type_int8_t: {
            int8_t *v = (int8_t*)(buf+pos);
            dl.append(*v);
            y = *v;
            break;
        }
        case type_int16_t: {
            int16_t *v = (int16_t*)(buf+pos);
            dl.append(*v);
            y = *v;
            break;
        }
        case type_int32_t: {
            int32_t *v = (int32_t*)(buf+pos);
            dl.append(*v);
            y = *v;
            break;
        }
        case type_int64_t: {
            qint64 *v = (qint64*)(buf+pos);
            dl.append(*v);
            y = *v;
            break;
        }
        case type_uint8_t: {
            uint8_t *v = (uint8_t*)(buf+pos);
            dl.append(*v);
            y = *v;
            break;
        }
        case type_uint16_t: {
            uint16_t *v = (uint16_t*)(buf+pos);
            dl.append(*v);
            y = *v;
            break;
        }
        case type_uint32_t: {
            uint32_t *v = (uint32_t*)(buf+pos);
            dl.append(*v);
            y = *v;
            break;
        }
        case type_uint64_t: {
            quint64 *v = (quint64*)(buf+pos);
            dl.append(*v);
            y = *v;
            break;
        }
        default:
            break;
        }
        pos += size;
        //        QByteArray arr(buf, len);
        //        qDebug() << name << QPointF(x, y) << pos << size << arr;
        qreal x;
        if(originTimestamp == 0) {
            x = ser.length();
        } else {
            x = currentTimestamp;
        }
        ser.append(QPointF(x, y));
    }
    if(pos != len) {
        qCritical() << format.name << "save error len:" << len;
    }
}

QVariantMap LogParser::getItemsName()
{
    QVariantMap names;
    foreach (log_format_struct item, formatList.values()) {
        for(int i=0; i<item.nameList.length(); i++) {
            if(lineSeries.value(item.nameList.at(i)).length() != 0) {
                names.insert(item.nameList.at(i),
                             typeName.value(item.typeList.at(i), "unknown"));
            }
        }
    }
    //qDebug() << names;
    return names;
}

QVector<QPointF> LogParser::getDataSeries(const QString name)
{
    return lineSeries.value(name);
}

int LogParser::getDataLength(const QString &name)
{
    return lineSeries.value(name).length();
}

QVariantList LogParser::getData(const QString name)
{
    return data.value(name);
}

void LogParser::update(QAbstractSeries *series, const QString &name)
{
    if (series) {
        //qDebug() << name << lineSeries.value(name);
        QLineSeries *xySeries = static_cast<QLineSeries *>(series);
        xySeries->replace(lineSeries.value(name));
    }
}

QDateTime LogParser::minDate()
{
    if(!formatList.contains(LOG_TIME_MSG)) {
        return QDateTime::fromMSecsSinceEpoch(0);
    }
    log_format_struct format = formatList.value(LOG_TIME_MSG);
    QString name = format.nameList.first();
    QVector<QPointF> &ser = lineSeries[name];
    return QDateTime::fromMSecsSinceEpoch(ser.first().y());
}

QDateTime LogParser::maxDate()
{
    if(!formatList.contains(LOG_TIME_MSG)) {
        return QDateTime::fromMSecsSinceEpoch(3600*1000);
    }
    log_format_struct format = formatList.value(LOG_TIME_MSG);
    QString name = format.nameList.first();
    QVector<QPointF> &ser = lineSeries[name];
    return QDateTime::fromMSecsSinceEpoch(ser.last().y());
}

qreal LogParser::getMaxY(const QString &name)
{
    QVector<QPointF> &ser = lineSeries[name];
    if(ser.isEmpty()) {
        return 1;
    }
    qreal max = ser.first().y();
    foreach (QPointF p, ser) {
        if(p.y() > max) {
            max = p.y();
        }
    }
    return max;
}

qreal LogParser::getMinY(const QString &name)
{
    QVector<QPointF> &ser = lineSeries[name];
    if(ser.isEmpty()) {
        return -1;
    }
    qreal min = ser.first().y();
    foreach (QPointF p, ser) {
        if(p.y() < min) {
            min = p.y();
        }
    }
    return min;
}

quint64 LogParser::getMaxX(const QString &name)
{
    QVector<QPointF> &ser = lineSeries[name];
    if(ser.isEmpty()) {
        return 3600*1000;
        qDebug() << "series emputy:" << name << ser.length();
    }
    return ser.last().x();
}

quint64 LogParser::getMinX(const QString &name)
{
    QVector<QPointF> &ser = lineSeries[name];
    if(ser.isEmpty()) {
        return 0;
        qDebug() << "series emputy:" << name << ser.length();
    }
    return ser.first().x();
}

//QColor string2color(const QString str)
//{
//    uint hash = qHash(str);
//    int r = (hash & 0xFF0000) >> 16;
//    int g = (hash & 0x00FF00) >> 8;
//    int b = hash & 0x0000FF;
//    return QColor(r, g, b);
//}
