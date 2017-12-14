#include "xmllibrary.h"
#include <QFile>
#include <QDebug>
//#include
#include <QStringList>

CCfgXML::CCfgXML()
{
    vectorSize = 10;
    xmlValue.resize(vectorSize);
    initData.clear();
    endLineMode = EL_AUTO;
}

void CCfgXML::parseData(QByteArray *pData)
{
    cfgMap.clear();
    cfgFilePath = "";
    pDataXml = pData;
    QDomDocument document = getDocument();
    QDomElement element = document.documentElement();
    parserXml(element, 1);

}

void CCfgXML::parseFile(const QString &filePath)
{
    cfgMap.clear();
    cfgFilePath = filePath;
    pDataXml = &initData;
    QDomDocument document = getDocument();
    QDomElement element = document.documentElement();
    parserXml(element, 1);
}

void CCfgXML::parserXml(const QDomNode &node, quint8 numLevel)
{
        QDomNode domNode = node.firstChild();

        while(!domNode.isNull()){

            if(domNode.isElement()){

                QDomElement domElement = domNode.toElement();
                if(!domElement.isNull()){
                    xmlValue[numLevel] = domElement.attribute("n","no set");

                    if(!domElement.hasChildNodes())
                    {
                        QString s, v;
                        for(quint8 i=1; i<=numLevel; i++)
                        {
                            s=s+ "/" + xmlValue[i];
                        }
                        v=domElement.attribute("v","empty");
                        if(v == "empty")
                        {
                            s += "/\tempty"; // s += "/empty" - old incorrect
                        }
                        cfgMap.insert(s,v);
                    }
                }
            }

            parserXml(domNode, numLevel+1);
            domNode = domNode.nextSibling();
        }
}

QString CCfgXML::getValue(const QString &pathToParameter, const QString &parameter, const QString &defValue)
{
    QString s = pathToParameter;
    if(parameter.length() != 0)
    {
        s += "/\t" + parameter;
    }
    s = cfgMap[s];
    if(s.isEmpty())
    {
        s = defValue;
    }
    return s;
}

void CCfgXML::getAllToDebug()
{
        QMap<QString,QString>::iterator it = cfgMap.begin();
        for(;it != cfgMap.end(); ++it)
        {
           qDebug() << it.key() << " : " << it.value();
        }
}

void CCfgXML::writeMapToFile(const QString &filePath)
{
    if (!filePath.isEmpty())
    {
        QFile file(filePath);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text))
        {
            QTextStream out(&file);
            out.setCodec(RU_WIN);
            QMap<QString,QString>::iterator it = cfgMap.begin();
            for(;it != cfgMap.end(); ++it)
            {
                out << it.key() << " : " << it.value() << "\n";
            }
            file.close();
        }
    }
}

QMap<QString,QString> CCfgXML::GetMap()
{
    return cfgMap;
}

void CCfgXML::SetMap(const QMap<QString, QString> &srcMap)
{
    cfgMap.clear();
    cfgMap = srcMap;
}


CCfgConf::CCfgConf() : CCfgXML()
{

}

QDomDocument CCfgXML::getDocument()
{
    QDomDocument document;
    if (!cfgFilePath.isEmpty()) {
        QFile file(cfgFilePath);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            if (document.setContent(&file)) {}
            file.close();
        }
    }
    //new
    if(!pDataXml->isEmpty())
    {
        if(document.setContent(*pDataXml))  {};
    }
    return document;
}




QDomDocument CCfgConf::getDocument()
{
    QByteArray buffer;
    QDomDocument document;

    if (!cfgFilePath.isEmpty()) {
        QFile file(cfgFilePath);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            buffer = file.readAll();
            file.close();

            QByteArray content = toXml(buffer);

            if (document.setContent(content)) {}
        }
    }
    return document;
}

QByteArray CCfgConf::toXml(const QByteArray &data)
{
    QTextCodec *codecin = QTextCodec::codecForName(RU_KOI);
    QTextCodec *codecout = QTextCodec::codecForName(RU_WIN);

    QString ss = codecin->toUnicode(data);
    QStringList list = ss.split('\n');

    quint8 l=1;
    quint16 n=0;
    foreach(QString s, list)
    {
     s=s.trimmed();
     if(s.startsWith("["))
     {
         QString node;

         if(!s.startsWith("[#"))
         {
             node.fill('\t',l);
             node = node + "<n n=\"" + s.mid(1, s.size()-2) + "\">";
             l++;
         }
         else
         {
             l--;
             node.fill('\t',l);
             node = node + "</n>";
         }
         list[n] = node;
     }
     else
     {
         if(!s.isEmpty())
         {
             QString node, prefix;
             prefix = "\t";
             node.fill('\t',l);
             node = node + "<n n=\"" + prefix + cfgParam(s) + "\" v=\"" + cfgValue(s) + "\"/>";
             list[n] = node;
         }
         else
         {
             //list.removeAt(n);
         }
     }
     n++;
    }

    list.insert(0,"<?xml version=\"1.0\" encoding=\"windows-1251\"?>");
    list.insert(1,"<n ver=\"1.1\">");
    list.append("</n>");

    QString ts = list.join("\n");
    QByteArray tmp;

    //tmp.insert(0,ts);
    tmp = codecout->fromUnicode(ts);

    return tmp;
}

QString CCfgConf::cfgParam(const QString &s)
{
    return s.left(s.indexOf(" ="));
}

QString CCfgConf::cfgValue(const QString &s)
{
    QString sv = s.mid(s.indexOf("=") + 2);
    //
    sv = symbOff(sv);
    //
    if(sv.startsWith("\""))
    {
        sv = "$" + sv.mid(1,sv.size() - 2);
    }
    return sv;
}

QString CCfgXML::symbOff(const QString &s)
{
    QString st = s;
    st = st.replace("&", "&amp;");
    st = st.replace("<", "&lt;");
    st = st.replace(">", "&gt;");
    st = st.replace("'", "&apos;");
    //st = st.replace("\"", "&quot;");
    return st;
}

QString CCfgXML::symbOn(const QString &s)
{
    QString st = s;
    st = st.replace("&amp;", "&");
    st = st.replace("&lt;", "<");
    st = st.replace("&gt;", ">");
    st = st.replace("&apos;", "'");
    //st = st.replace("&quot;", "\"");
    return st;
}

void CCfgXML::SetOutputEndLine(quint8 endLine)
{
    endLineMode = endLine;
}

void CCfgXML::writeFile(const QString &filePath)
{
    xmlOutput = filePath.endsWith(EXT_XML) || filePath.endsWith(EXT_INI);
    if (!filePath.isEmpty()) {
        QFile file(filePath);
        if (file.open(QIODevice::WriteOnly | QIODevice::Truncate))
        {
            file.setTextModeEnabled(endLineMode == EL_AUTO);

            QTextStream out(&file);
            QTextCodec* RusCodec;

            if(xmlOutput)
            {
                RusCodec = QTextCodec::codecForName(RU_WIN);
                out.setCodec(RusCodec);
                out << "<?xml version=\"1.0\" encoding=\"windows-1251\"?>" << lineTerm();
                out << "<n ver=\"1.1\">" << lineTerm();
            }
            else
            {
                RusCodec = QTextCodec::codecForName(RU_KOI);
                out.setCodec(RusCodec);
            }

            QMap<QString,QString>::iterator it = cfgMap.begin();
            qint8 level = 1, levelLast = 1;
            QString sk, sv;
            QVector<QString> xmlValueLast;
            xmlValue.resize(0);
            xmlValue.resize(vectorSize);

            for(;it != cfgMap.end(); ++it)
            {
               level = 1;
               xmlValueLast = xmlValue;
               xmlValue.resize(0);
               xmlValue.resize(vectorSize);
               sk = it.key();
               sv = it.value();
               while(1)
               {
                    sk = sk.mid(1);
                    qint8 n = sk.indexOf("/");
                    if(n == -1)
                    {
                        break;
                    }
                    xmlValue[level]= sk.left(n);
                    level++;
                    sk = sk.mid(n);
               }
               quint8 cross=0;
               for(quint8 i=1; i< xmlValue.count() ;i++)
               {
                   if(xmlValueLast[i] != xmlValue[i])
                   {
                        cross = i;
                        break;
                   }
               }
               if(cross>0)
               {
                   for(quint8 i=levelLast; i> cross ;i--)
                   {
                       out << makeEndLine(i-1,xmlValueLast[i-1]);
                   }

                   for(quint8 i=cross; i< level ;i++)
                   {
                       out << makeKeyLine(i,xmlValue[i]);
                   }
               }
               if(sv != "empty")
               {
               out << makeParamLine(level, sk.trimmed(), sv);
               }
               levelLast = level;
            }
            for(quint8 i=level; i>1; i--)
            {
                out << makeEndLine(i-1,xmlValue[i-1]);
            }
            if(xmlOutput)
            {
                out << "</n>" + lineTerm();
            }
            file.close();
        }
    }
}

QString CCfgXML::makeParamLine(const qint8 &indent, const QString &param, const QString &value)
{
    QString s, v;
    for(quint8 i=0; i<indent-1; i++)
        s += "\t";

    if(xmlOutput)
        s += "\t<n n=\"" + param + "\" v=\"" + value + "\"/>" + lineTerm();
    else
    {
        v = value;
        if(v.left(1)=="$")
        {
            v = "\"" + v.mid(1) + "\"";
        }
        s += param + " = " + v + lineTerm();
        //
        s = symbOn(s);
        //
    }
    return s;
}

QString CCfgXML::makeKeyLine(const qint8 &indent, const QString &param)
{
    QString s;
    for(quint8 i=0; i<indent-1; i++)
        s += "\t";

    if(xmlOutput)
        s += "\t<n n=\"" + param + "\">" + lineTerm();
    else
        s += "[" + param + "]" + lineTerm();
    return s;
}

QString CCfgXML::makeEndLine(const qint8 &indent, const QString &param)
{
    QString s;
    for(quint8 i=0; i<indent-1; i++)
        s += "\t";

    if(xmlOutput)
        s += "\t</n>" + lineTerm();
    else
        s += "[#" + param + "]" + lineTerm();

    return s;
}

QString CCfgXML::lineTerm()
{
    if(endLineMode == EL_WIN)
        return "\r\n";
    else
        return "\n";


//#ifdef WIN32
//    return "\n";
//#else
//    return "\r\n";
//#endif
}
