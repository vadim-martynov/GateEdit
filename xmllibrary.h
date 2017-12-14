#ifndef XMLLIBRARY_H
#define XMLLIBRARY_H

#include <QObject>
#include <QDomDocument>
#include <QDomNode>
#include <QVector>
#include <QMap>
#include <QTextCodec>


#define RU_WIN          "CP1251"
#define RU_KOI          "KOI8-R"
#define EXT_XML         "xml"
#define EXT_INI         "ini"

enum EndLine
{
    EL_AUTO = 0,
    EL_WIN,
    EL_UNIX
};

// ////////

/*
 *
 * edit:        16.06.2016
 * add:         QByteArray initData;
 * edit:   11.08.2016
 * add:         EXT_INI
 * correct:     toXml
 * edit:   29.08.2016
 * add:         cfgMap.clear();
 * edit:   09.11.2017
 * add:         symbOff(), symbOn();
 * edit:   16.11.2017
 * correct:     add tab (\tempty);
 * edit:   06.12.2017
 * correct:     force Win CrLf for conf output in Linux
 * last edit: 13.12.2017
 * add:         SetOutputEndLine() (auto & manual endLine management);
*/

class CCfgXML
{
protected:
    void parserXml(const QDomNode &node, quint8 numLevel);
    virtual QDomDocument getDocument();
    QString cfgFilePath;
    QByteArray* pDataXml;
    QByteArray initData;
    QVector<QString> xmlValue;
    quint8 vectorSize;
    bool xmlOutput;
    QMap<QString, QString> cfgMap;
    quint8 endLineMode;

    QString makeParamLine(const qint8 &indent, const QString &param, const QString &value);
    QString makeKeyLine(const qint8 &indent, const QString &param);
    QString makeEndLine(const qint8 &indent, const QString &param);
    QString symbOn(const QString &s);
    QString symbOff(const QString &s);
    QString lineTerm();

public:
    CCfgXML();
    void parseFile(const QString &filePath);
    void parseData(QByteArray *pData);
    QString getValue(const QString &pathToParameter, const QString &parameter = "", const QString &defValue = "");
    void getAllToDebug();
    void writeMapToFile(const QString &filePath);
    void writeFile(const QString &filePath);
    QMap<QString, QString> GetMap();
    void SetMap(const QMap<QString, QString> &srcMap);
    void SetOutputEndLine(quint8 endLine);
};

class CCfgConf : public CCfgXML
{
protected:
    virtual QDomDocument getDocument();
    QByteArray toXml(const QByteArray &data);
    QString cfgParam(const QString &s);
    QString cfgValue(const QString &s);

public:
    CCfgConf();
//    {}

};



#endif // XMLLIBRARY_H
