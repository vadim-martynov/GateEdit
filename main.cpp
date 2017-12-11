#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QTranslator t;
    QString qmPath = "gateedit_";
    t.load(qmPath + QLocale::system().name());
    a.installTranslator(&t);

#if defined(WIN32)
    //
#else
    //QTextCodec *c;
    //c->setCodecForLocale(QTextCodec::codecForName("CP1251"));
#endif

    MainWindow w(0, a.arguments());
    w.show();

    return a.exec();
}
