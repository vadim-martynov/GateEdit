#ifndef CSEGTAB_H
#define CSEGTAB_H

#include <QObject>
#include <QTableWidget>

#include "cidentry.h"


class CSegTab
{
public:
    CSegTab();

    QWidget* tab;
    QTableWidget* table;
    quint8 adapter;
    quint8 network;
    quint8 ip1;
    quint8 ip2;
    quint8 ip3;
    quint8 ip4;
    QString name;                       //name for tabs and table headers
    QStringList segList;                //external names for current segments
    QList<quint8> segId;                //external segment ids for current segments
    QMap<quint16,CIdEntry> segMap;
    QString confPath;

};

enum checkStates
{
    SEG_GOOD,
    SEG_BAD_NAME,
    SEG_NAME_DUPLICATION,
    SEG_BAD_MAIN_CONF,
    SEG_BAD_ADD_CONF
};

#endif // CSEGTAB_H
