#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include <QDebug>
#include <QDir>
#include <QCheckBox>
#include <QProcess>
#include <QMessageBox>
#include <QCloseEvent>
#include <QTextCodec>
#include <QTranslator>
#include <QTimer>
#include <QHostAddress>
#include <QFile>
#include <QRadioButton>
#include <QDateTime>


#include "xmllibrary.h"
#include "csegtab.h"


#define TMP_SCR             "./tmp.bat"
#define FILE_HW             "hardware.conf"
#define FILE_EXT            ".conf"
#define LINUX_MNT           "/mnt/flash"
#define DIR_STORAGE         "/GateStorage"

#define TITLE_MAIN          "GateEditor v.1.3.4"
/*history
 *
 *1.1.1 use alternate additional config system sections
 * checking path & names
 *1.2.1 optional get & put by ftp
 * select source host
 *1.2.2 embedded ftp transport
 *1.3.1 backup for linux
 *1.3.2 signature filtering for backup
 *1.3.3 backup for windows correct info
 *1.3.4 protect for missed Id in filters
 *
 *final linux & windows release 11.10.2017
*/

namespace Ui {
class MainWindow;
}

struct SGate
{
    QString name;
    QString dirName;
    QString localDir;
    QString remoteDir;
    QHostAddress host;
    bool connected;
    QString revision;
    QString sign;
};

struct SFlash
{
    QString dev;
    QString label;
    QString uuid;
    bool mount;
};

struct SQuotes
{
    QString key;
    QString value;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0, QStringList args = QStringList());
    ~MainWindow();

private slots:
    void pbCancel_click();
    void pbSave_click();
    void tableItem_change(QTableWidgetItem * item);
    void pbSelect_click();
    void chFlash_change(bool state);
    void chFlashAdd_change(bool state);

    void init_shot();
    void timer_shot();

private:
    Ui::MainWindow *ui;

    CCfgConf baseGate;
    QTableWidgetItem tblI;
    quint8 count;
    QVector<CSegTab> segs;

    QString systemSignature;
    QVector<SGate> gate;
    QList<QRadioButton *> rbNet;
    quint8 base;


    QString path_conf;
    bool changed;
    bool loaded;
    bool started;
    bool restored;
    bool flashReady;


    QMap<QString, QString> segPath;

    QTimer *init;
    QTimer *timer;
    QProcess *script;

    void startLoading();
    void SetTabs(quint8 i);
    void FindExternalSegments(quint8 i);
    quint8 FindSegmentPath();
    void FindAbonents(quint8 i);
    void FillTables(quint8 i);
    void FillPermissions(quint8 i);
    void ClearCheckList(quint8 i);

    CIdEntry GetIdFromFile(const QString &path);
    QString GetSection(const QString &name, quint8 i);
    void DisableEditItem(QTableWidgetItem *item);
    void ViewProgressLoad();
    void ViewProgressSave();
    void AddInfo(const QString &text);
    void Defrost(quint8 n = 3);

    void GateInit();
    bool GetConfigFromGate(quint8 item);
    void PutConfigToGates();
    bool PutConfig(quint8 host, quint8 item);
    bool MoveFileFtp(const QString &cmd, const QString &dir, const QString &file, quint8 n=0);

    bool EjectWinFlash(const QString &disk);
    QList<SFlash> PluggedFlash();
    void MountFlash();
    void UnMountFlash();
    void SaveBackup(const QString &timestamp);
    void ViewStorage();
    bool LoadBackup();
    void GetRevision(quint8 item);
    QString GetSignature(const QString &path);

    QString LeftTo(const QString &text, const QString &target);
    QString RightFrom(const QString &text, const QString &target);
    QString RightFromLast(const QString &text, const QString &target);
    void ExtractLine(QMap<QString, QString> *, const QString &text, const QString &mark = "=");


protected:
    void closeEvent(QCloseEvent *ev);

};


#endif // MAINWINDOW_H
