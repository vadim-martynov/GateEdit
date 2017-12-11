    #include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent, QStringList args) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    this->setWindowTitle(TITLE_MAIN);
    ui->tabSegments->setVisible(false);
    ui->frLoading->setVisible(true);
    ui->frDialog->setVisible(false);
    ui->lbSelectTimeStamp->setVisible(false);
    ui->lwStorage->setVisible(false);
    ui->chFlashAdd->setVisible(false);

    connect(ui->pbCancel, SIGNAL (clicked()), this, SLOT (pbCancel_click()));
    connect(ui->pbSave, SIGNAL (clicked()), this, SLOT (pbSave_click()));
    connect(ui->pbSelect, SIGNAL (clicked()), this, SLOT (pbSelect_click()));
    gate.resize(2);

#if defined(WIN32)
    path_conf = "c:/VoiceComm/conf";                    //by default
    gate[0].localDir = "c:/VoiceComm/gate/conf/gate_net_1";  //by default
    gate[1].localDir = "c:/VoiceComm/gate/conf/gate_net_2";  //by default
#else
    path_conf = "/data/etc/Mega/VoiceComm/conf";                    //by default
    gate[0].localDir = "/data/etc/Mega/gate/conf/gate_net_1";  //by default
    gate[1].localDir = "/data/etc/Mega/gate/conf/gate_net_2";  //by default
#endif


    started = false;
    changed = false;
    restored = false;
    base = 0;

    for(quint8 i=1; i<args.count(); i++)
    {
        switch (i)
        {
        case 1: path_conf = args[i];    break;
        case 2: gate[0].localDir = args[i];  break;
        case 3: gate[1].localDir = args[i];  break;
        default: break;
        }
    }
    //qDebug() << "args" << path_conf << gate[0].localDir << gate[1].localDir;
    path_conf = path_conf.replace('\\', '/');

    gate[0].name = tr("Lan A");
    gate[1].name = tr("Lan B");

    for(quint8 net=0; net<2; net++)
    {
        gate[net].dirName = gate[net].localDir.replace('\\', '/').split('/').last();
        QString text = QString("%1 (%2)").arg(gate[net].name, gate[net].dirName);
        rbNet.append(new QRadioButton(text));
        ui->groupBox->layout()->addWidget(rbNet.last());
        //gate[net].sign = "";
    }
    rbNet[0]->setChecked(true);

    connect(ui->chFlash, SIGNAL(toggled(bool)), SLOT(chFlash_change(bool)));
    connect(ui->chFlashAdd, SIGNAL(toggled(bool)), SLOT(chFlashAdd_change(bool)));

    init = new QTimer(this);
    connect(init, SIGNAL(timeout()), SLOT(init_shot()));
    init->start(200);
    //qDebug() << "init start";

#if defined(WIN32)
#else
#endif

    script = new QProcess(this);
    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), SLOT(timer_shot()));


    //Test
    //EjectWinFlash("O");
    //ExtractLine("/dev/sdc1: LABEL = \"NITA-VADIM\" UUID = \"FCE6-716D\" TYPE = \"vfat\"", " = ");

}

MainWindow::~MainWindow()
{
    delete ui;
    if(flashReady)
        UnMountFlash();

}
// ////////////////////////////////////////////////////////////////////////////////

void MainWindow::init_shot()
{
    init->stop();
    GateInit();
    timer->start(500);
}


void MainWindow::pbSelect_click()
{
    base = int(rbNet[1]->isChecked());
    //qDebug() << "base gate " << gate[base].dirName;

    if(ui->chFlash->isChecked())
    {
        if(LoadBackup())
        {
            restored = true;
            base = 0;
            if(flashReady)
                UnMountFlash();
        }
        else
            return;
    }

    ui->frDialog->setVisible(false);
    ui->frLoading->setVisible(true);
    startLoading();
}

void MainWindow::startLoading()
{
    count = 0;
    systemSignature = gate[base].sign;
    QString signShow = systemSignature;
    if(signShow.isEmpty())
        signShow = tr("Unknown");
    ui->statusBar->showMessage(tr("System: %1 - %2").arg(signShow, gate[base].revision));
    QString path = gate[base].localDir.replace('\\', '/') + "/" + FILE_HW;
    AddInfo(tr("Use gate config buffer: ") + path);
    baseGate.parseFile(path);
    //baseGate.getAllToDebug();

    bool run;
    do //calculate segments count
    {
        count++;
        QString segment = GetSection( "Segment_", count);
        run = (baseGate.getValue(segment, "ip_part1").toInt() != 0);
    } while(run);
    count--;

    //systemSignature = baseGate.getValue("", "system_signature").mid(1);
    AddInfo(tr("Get gate signature: ") + signShow);
    AddInfo(tr("Get segments count: %1").arg(count));
    segs.resize(count);
    ViewProgressLoad();

    //get segments headers
    //set tab and table for segment
    for(quint8 i=0; i<count; i++)
    {
        SetTabs(i);
    }
    ViewProgressLoad();

    for(quint8 i=0; i<count; i++)
    {
        FindExternalSegments(i);
    }

    switch (FindSegmentPath())
    {
    case SEG_GOOD:
    {
        for(quint8 i=0; i<count; i++)
        {
            FindAbonents(i);        ViewProgressLoad();
            AddInfo(tr("Get segment #%1 (%2) config from: %3").arg(QString::number(i+1), segs[i].name, segs[i].confPath));
            FillPermissions(i);     ViewProgressLoad();
            FillTables(i);          ViewProgressLoad();
        }
        if(count > 1)
        {
            QEventLoop loop;
            QTimer::singleShot(1000, &loop, SLOT(quit()));
            loop.exec();
            ui->tabSegments->setVisible(true);
            ui->chFlashAdd->setVisible(true);
            ui->frLoading->setVisible(false);
            started = true;
        }
        else
        {
            ui->statusBar->showMessage(tr("No segments in gate config"));
        }
        break;
    }
    case SEG_BAD_NAME:
    {
        AddInfo(tr("Incorrect additional segment designation"));
        ui->statusBar->showMessage(tr("Check additional segment designation in tcms.conf"));
        break;
    }
    case SEG_NAME_DUPLICATION:
    {
        AddInfo(tr("Additional segment name duplication"));
        ui->statusBar->showMessage(tr("Check additional segment designation in tcms.conf"));
        break;
    }
    case SEG_BAD_MAIN_CONF:
    {
        AddInfo(tr("Incorrect main config path: %1").arg(path_conf));
        ui->statusBar->showMessage(tr("Check main config path"));
        break;
    }
    case SEG_BAD_ADD_CONF:
    {
        ui->statusBar->showMessage(tr("Check segment config path in tcms.conf"));
        break;
    }
    default:
        break;
    }
}

void MainWindow::ViewProgressLoad()
{
    static quint8 vl = 0;
    static quint8 inc = 0;
    if(vl == 0)
    {
        inc = 100 /(3*count + 2);
    }
    vl += inc;
    ui->progressBar->setValue(vl);
    Defrost();
}

void MainWindow::ViewProgressSave()
{
    static quint8 vl = 0;
    static quint8 inc = 0;
    if(vl == 0)
    {
        inc = 25;
    }
    vl += inc;
    ui->progressBar->setValue(vl);
    Defrost();
}

void MainWindow::FillPermissions(quint8 i)
{
    QString prefix = GetSection( "Segment_", i+1) + "/Filter_phone";
    bool run;
    quint16 j=0;
    do
    {
        j++;
        quint16 id = baseGate.getValue(prefix, GetSection("filter_", j).mid(1)).toInt();
        //qDebug() << prefix << GetSection("filter_", j) << "Id:" << id;
        run = (id != 0);
        if(run)
        {
            if(segs[i].segMap[id].type > 0)
            {
                QString prefix_f = prefix + GetSection( "Filter_Dst_", j);
                bool pass = true;
                for(quint8 dst=1; dst<=count; dst++)
                {
                    quint8 seg = baseGate.getValue(prefix_f, GetSection("filter_dst_", dst).mid(1)).toInt();
                    if(seg == 0)
                    {
                        break;
                    }
                    else
                    {
                        pass = false;
                        if(seg <= count)
                            segs[i].segMap[id].checkList[seg-1] = true;
                    }
                }
                if(pass)
                {
                    //set permissions for all segments by default
                    for(quint8 seg=0; seg<count; seg++)
                        if(i != seg)
                            segs[i].segMap[id].checkList[seg] = true;
                }
//                else
//                {
//                    //add permissions for own segment
//                    segs[i].segMap[id].checkList[i] = true;
//                }
            }
            else
            {
                qDebug() << "Missed Id:" << id << segs[i].segMap[id].type;
            }
        }
    }while(run);
}



void MainWindow::FindExternalSegments(quint8 i)
{
    for(quint8 j=0; j<count; j++)
    {
        if(i != j)
        {
            segs[i].segList.append(segs[j].name);
            segs[i].segId.append(j);
        }
    }

    //create col names
    segs[i].table->setColumnCount(count + 1);
    for(quint8 j=0; j<count+1; j++)
    {
        QString name;
        switch (j)
        {
        case 0:     name = tr("ID");                break;
        case 1:     name = tr("Name");              break;
        default:    name = segs[i].segList[j-2];    break;
        }
        segs[i].table->setHorizontalHeaderItem(j, new QTableWidgetItem(name));
    }
}


void MainWindow::SetTabs(quint8 i)
{
    QString segment = GetSection( "Segment_", i+1);
    segs[i].adapter = baseGate.getValue(segment, "adapter_number").toInt();
    segs[i].network = baseGate.getValue(segment, "network_number").toInt();
    segs[i].ip1 = baseGate.getValue(segment, "ip_part1").toInt();
    segs[i].ip2 = baseGate.getValue(segment, "ip_part2").toInt();
    segs[i].ip3 = baseGate.getValue(segment, "ip_part3").toInt();
    segs[i].ip4 = baseGate.getValue(segment, "ip_part4").toInt();
    segs[i].name = baseGate.getValue(segment , "name").mid(1);

    //create tabs
    segs[i].tab = new QWidget(ui->tabSegments);
    ui->tabSegments->addTab(segs[i].tab, segs[i].name);
    segs[i].tab->setLayout(new QGridLayout());
    segs[i].table = new QTableWidget(segs[i].tab);
    segs[i].tab->layout()->addWidget(segs[i].table);
    segs[i].table->setToolTip(segs[i].name);
    connect(segs[i].table, SIGNAL(itemChanged(QTableWidgetItem*)),this,SLOT(tableItem_change(QTableWidgetItem*)));
}

quint8 MainWindow::FindSegmentPath()
{
#if defined(WIN32)
    QString sysSuf = "/AdditionalConfigs";
    QString sysSufAlt = "/AdditionalConfigsUnix";
#else
    QString sysSuf = "/AdditionalConfigsUnix";
    QString sysSufAlt = "/AdditionalConfigs";
#endif

    CCfgConf tcms;
    tcms.parseFile(path_conf + "/tcms.conf");
    if(tcms.getValue("", "net_mask_length").isEmpty())
        return SEG_BAD_MAIN_CONF;

    for(quint8 j=1; j<count; j++)
    {
        QString addconfname = GetSection("additional_config_name_", j).mid(1);
        QString name = tcms.getValue(sysSuf + "/AdditionalConfigNames", addconfname).mid(1);
        if(name.isEmpty())
            name = tcms.getValue(sysSufAlt + "/AdditionalConfigNames", addconfname).mid(1);
        if(name.isEmpty())
            return SEG_BAD_NAME;

        //qDebug() << "[]" << name.size() << name.isEmpty() << name;

        QString addconf = GetSection("additional_config_", j).mid(1);
        segPath[name] = tcms.getValue(sysSuf, addconf).mid(1);
    }

    //qDebug() << "[size]" << segPath.size();
    if(segPath.size() < count-1)
        return SEG_NAME_DUPLICATION; //because of name duplication

    //tcms.getAllToDebug();
    quint8 counter = 0;
    for(quint8 i=0; i<segs.size(); i++)
    {
        segs[i].confPath = segPath[segs[i].name].replace('\\', '/');
        if(segs[i].confPath.isEmpty())
        {
            segs[i].confPath = path_conf;
            counter++;
            if(counter > 1)
                return SEG_BAD_NAME;
        }
        else
        {
            AddInfo(tr("Defined path to additional config: %1 : %2").arg(segs[i].name, segs[i].confPath));
        }
        //qDebug() << "[*]" << segs[i].name << segs[i].confPath;
        QDir conf(segs[i].confPath);
        QStringList lstDirs = conf.entryList(QDir::Dirs |
                                             QDir::AllDirs |
                                             QDir::NoDotAndDotDot);
        if(lstDirs.size() == 0)
        {
            AddInfo(tr("Incorrect segment config path: %1 : %2").arg(segs[i].name, segs[i].confPath));
            return SEG_BAD_ADD_CONF;
        }
    }
    return SEG_GOOD;
}


void MainWindow::FindAbonents(quint8 i)
{
    QString screensSuf = "screens";
    QString confPath = segs[i].confPath;
    QDir conf(confPath);
    QStringList lstDirs = conf.entryList(QDir::Dirs |
                                         QDir::AllDirs |
                                         QDir::NoDotAndDotDot);

    //qDebug() << "dir size " << lstDirs.size();
    foreach(QString entry, lstDirs)
    {
        if(entry != screensSuf)
        {
            quint8 j = 0;
            bool run;
            CCfgConf hardware;
            hardware.parseFile(confPath + "/" + entry + "/hardware.conf");
            do
            {
                j++;
                QString interface = GetSection( "Interface", j);
                CIdEntry tmp;
                tmp.type = hardware.getValue(interface, "type").toInt();
                tmp.id = hardware.getValue(interface, "internal_address");
                tmp.name = hardware.getValue(interface, "name").mid(1);
                for(quint8 k=0; k<count; k++)
                {
                    tmp.checkList.append(false);
                }
                //qDebug() << "tmp" << tmp.checkList.size();

                run = (tmp.type > 0);

                switch(tmp.type)
                {
                case 0: //after last interface
                case 201: //radio dc
                case 202: //radio ac
                case 203:
                case 204: //radio t+
                case 350: //recorder
                case 403: //radio man ladoga
                case 404: //radio ladoga
                case 421: //radio ed-137
                case 491: //radio cas
                case 492: //radio ac
                case 503: //radio man qsig
                case 504: //radio qsig

                    //All unused types
                    break;
                default:
                    //qDebug() << "add int" << tmp.type << tmp.id << segs[curSegment].segMap.size();
                    if(tmp.id.toInt() > 0) segs[i].segMap[tmp.id.toInt()] = tmp;
                    if(tmp.type == 121)//ISDN
                    {
                        for(quint8 slot = 1; slot < 31; slot++)
                        {
                            tmp.id = hardware.getValue(interface + GetSection( "slot_", slot), "internal_address");
                            if(tmp.id.toInt() > 0) segs[i].segMap[tmp.id.toInt()] = tmp;
                        }
                    }
                    break;
                }

            } while(run);//interface on host
        }//no screen
    }//conf dirs

    //Get all ID from screens
    QDir screens(confPath + "/" + screensSuf);
    QStringList lstScreens = screens.entryList(QDir::Files |
                                               QDir::NoDotAndDotDot);
    foreach (QString entry, lstScreens)
    {
        CCfgConf conf;
        CIdEntry tmp;
        conf.parseFile(confPath + "/" + screensSuf + "/" + entry);
        tmp.id = conf.getValue("", "internal_address");
        tmp.type = 1;
        tmp.name = conf.getValue("", "name").mid(1);

        for(quint8 k=0; k<count; k++)
        {
            tmp.checkList.append(false);
        }
        //qDebug() << "tmp" << tmp.checkList.size();
        if(tmp.id.toInt() > 0)
            segs[i].segMap[tmp.id.toInt()] = tmp;
    }
}

void MainWindow::ClearCheckList(quint8 i)
{
    foreach(CIdEntry entry, segs[i].segMap)
    {
        for(quint8 k=0; k<count; k++)
        {
            segs[i].segMap[entry.id.toInt()].checkList[k] = false;
        }
    }
}

void MainWindow::FillTables(quint8 i)
{
    //View all segment ID
    quint16 row = 0;
    segs[i].table->clearContents();
    loaded = false;

    segs[i].table->setRowCount(segs[i].segMap.count());
    segs[i].table->setColumnWidth(0, 50);
    segs[i].table->setColumnWidth(1, 150);

    foreach(CIdEntry entry, segs[i].segMap)
    {
        //qDebug() << entry.type << entry.id << entry.name.toLocal8Bit();
        for(quint8 j = 0; j < count+1; j++)
        {
            switch (j)
            {
            case 0:
                segs[i].table->setItem(row, j, new QTableWidgetItem(entry.id));
                break;
            case 1:
                segs[i].table->setItem(row, j, new QTableWidgetItem(QString(entry.name)));
                break;
            default:
            {
                segs[i].table->setItem(row, j, new QTableWidgetItem(""));
                if(!entry.checkList.isEmpty())
                {
                    if(entry.checkList[segs[i].segId[j-2]])
                    {
                        segs[i].table->item(row, j)->setCheckState(Qt::Checked);
                        //qDebug() << entry.id << "Checked";
                    }
                    else
                    {
                        segs[i].table->item(row, j)->setCheckState(Qt::Unchecked);
                        //qDebug() << entry.id << "Unchecked";
                    }
                }
                break;
            }
            }
            DisableEditItem(segs[i].table->item(row, j));
        }//col
        row++;
    }//row
    loaded = true;
    changed = false;
    ui->pbCancel->setEnabled(changed);
    ui->pbSave->setEnabled(changed);
}


CIdEntry MainWindow::GetIdFromFile(const QString &path)
{
    CCfgConf conf;
    CIdEntry tmp;
    conf.parseFile(path);
    tmp.id = conf.getValue("","internal_address");
    tmp.type = 1;
    tmp.name = conf.getValue("" ,"name").mid(1);
    return tmp;
}

QString MainWindow::GetSection(const QString &name, quint8 i)
{
    QString s;
    s = QString::number(i);
    if(i<100)
    {
        s = "00" + s;
        s = s.right(2);
    }
    s = "/" + name + s;
    return s;
}

void MainWindow::DisableEditItem(QTableWidgetItem *item)
{
    item->setFlags(item->flags()^ Qt::ItemIsEditable);
    return;
}

void MainWindow::closeEvent(QCloseEvent *ev)
{
    if(!changed) return;
    QString msg = tr("Are you ready to exit \nwithout data saving?");
    QMessageBox dlg(QMessageBox::Question, TITLE_MAIN, msg, QMessageBox::Yes|QMessageBox::No);
    dlg.setWindowFlags(dlg.windowFlags() | Qt::WindowStaysOnTopHint);
    if(dlg.exec() == QMessageBox::No )
    {
        //qDebug() << "closeEvent - Ignore";
        ev->ignore();
    }
    else
    {
        //qDebug() << "closeEvent - Accept";
        ev->accept();
    }
}

void MainWindow::pbCancel_click()
{
    //qDebug() << "Cancel";
    for(quint8 i=0; i<count; i++)
    {
        ClearCheckList(i);
        FillPermissions(i);
        FillTables(i);
        ui->statusBar->showMessage(tr("No changes"));
    }
}

void MainWindow::pbSave_click()
{
    QString timestamp = QString::number(QDateTime::currentMSecsSinceEpoch());
    QString tab = "/\t";
    QString prefix = tab;
    QMap<QString, QString> tmpMap;
    tmpMap[prefix + "hardware_type"] = "5";
    tmpMap[prefix + "net_mask_length"] = "16";
    tmpMap[prefix + "time_stamp"] = timestamp;
    tmpMap[prefix + "system_signature"] = "$" + systemSignature;
    for(quint8 i=0; i<count; i++)
    {
        prefix = GetSection("Segment_", i+1);
        tmpMap[prefix + tab + "adapter_number"] = QString::number(segs[i].adapter);
        tmpMap[prefix + tab + "network_number"] = QString::number(segs[i].network);
        tmpMap[prefix + tab + "ip_part1"] = QString::number(segs[i].ip1);
        tmpMap[prefix + tab + "ip_part2"] = QString::number(segs[i].ip2);
        tmpMap[prefix + tab + "ip_part3"] = QString::number(segs[i].ip3);
        tmpMap[prefix + tab + "ip_part4"] = QString::number(segs[i].ip4);
        tmpMap[prefix + tab + "name"] = "$" + segs[i].name;
        prefix = GetSection("Segment_", i+1) + "/Filter_phone";
        quint16 filter = 0;
        foreach (CIdEntry entry, segs[i].segMap)
        {
            QStringList seguse;
            //seguse.clear();
            for(quint8 seg=0; seg<count; seg++)
            {//add used segments for this id entry
                if(entry.checkList[seg])
                {
                    if(seg != i)    seguse.append(QString::number(seg+1));
                }
            }
            if(seguse.size() > 0)
            {
                filter++;
                tmpMap[prefix + tab + GetSection("filter_", filter).mid(1)] = entry.id;
                if(seguse.size() < count-1)
                {
                    QString prefixdst = prefix + GetSection("Filter_Dst_", filter);
                    for(quint8 segfilter=1; segfilter<=seguse.size(); segfilter++)
                    {
                        tmpMap[prefixdst + tab + "filter_dst" + GetSection("_", segfilter).mid(1)] = seguse[segfilter-1];
                    }
                }
            }
        }//id entry
    }//seg /Interface01/Alias/\talias01


    CCfgConf out;
    //store gate1
    out.SetMap(tmpMap);
    out.writeFile(gate[base].localDir + "/" + FILE_HW);
    //out.getAllToDebug();

    //store gate alternate
    quint8 alt = int(!bool(base));
    qint8 inc = 1 - base*2;
    //qDebug() << "bai"<< base << alt << inc;
    for(quint8 i=0; i<count; i++)
    {
        prefix = GetSection("Segment_", i+1);
        tmpMap[prefix + tab + "network_number"] = QString::number(segs[i].network);
        tmpMap[prefix + tab + "ip_part2"] = QString::number(segs[i].ip2 + inc);
    }
    out.SetMap(tmpMap);
    out.writeFile(gate[alt].localDir + "/" + FILE_HW);

    ui->statusBar->showMessage(tr("Changes are applied"), 10000);
    changed = false;
    restored = false;
    ui->pbCancel->setEnabled(changed);
    ui->pbSave->setEnabled(changed);

    //save backup to storage
    if(ui->chFlashAdd->isChecked())
        SaveBackup(timestamp);

    //net store by ftp
    PutConfigToGates();
}


void MainWindow::tableItem_change(QTableWidgetItem *item)
{
    if(!loaded) return;
    if(!changed)
    {
        changed = true;
        ui->pbCancel->setEnabled(changed);
        ui->pbSave->setEnabled(changed);
    }
    QString segName = item->tableWidget()->toolTip();
    quint8 curSeg = 0;
    quint8 curCheck = 0;
    for(quint8 i=0; i<count; i++)
    {
        if(segs[i].name == segName)
        {
            curSeg = i;
            break;
        }
    }
    quint16 row = item->row();
    quint16 col = item->column();
    QString chSegName = item->tableWidget()->horizontalHeaderItem(col)->text();
    for(quint8 i=0; i<count-1; i++)
    {
        if(segs[curSeg].segList[i] == chSegName)
        {
            curCheck = segs[curSeg].segId[i];
            break;
        }
    }
    QString id = item->tableWidget()->item(row, 0)->text();
    QString name = item->tableWidget()->item(row, 1)->text();
    if(item->isSelected())
    {
        for(quint8 ic=2; ic<count+1; ic++)
        {
            for(quint16 ir=0; ir<item->tableWidget()->rowCount(); ir++)
            {
                if(item->tableWidget()->item(ir,ic)->isSelected())
                {
                    item->tableWidget()->item(ir,ic)->setSelected(false);
                    item->tableWidget()->item(ir,ic)->setCheckState(item->checkState());
                }
            }
        }
    }
    bool state = item->checkState();
    segs[curSeg].segMap[id.toInt()].checkList[curCheck] = state;
    QString status;
    state? status = tr("checked"): status = tr("unchecked");
//    qDebug() << tr("Last change: ") << segName << id << name << chSegName << state << status;
    QString msg = tr("In segment %1 for id %2 (%3) permission flag for segment %4 set %5").arg(segName, id, name, chSegName, status);
    ui->statusBar->showMessage(msg, 10000);
}

void MainWindow::AddInfo(const QString &text)
{
    ui->lwInfoStart->addItem(text);
    Defrost();
}

void MainWindow::Defrost(quint8 n)
{
    while(n-- > 0)
    {
        QApplication::processEvents();
    }
}

bool MainWindow::EjectWinFlash(const QString &disk)
{
    QString body = "";
    body += "@if (0 == 1) @end /*\n";
    body += "@cscript //E:JScript //Nologo %~f0\n";
    body += "@exit /B %ERRORLEVEL% */\n";
    body += "var shell = new ActiveXObject(\"Shell.Application\");\n";
    body += QString("shell.NameSpace(17).ParseName(\"%1\").InvokeVerb(\"Eject\");\n").arg(disk);
    body += "WSH.Sleep(500);\n";
    QProcess proc;
    QFile tmp(TMP_SCR);
    if (tmp.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate))
    {
        QTextStream out(&tmp);
        out << body;
        tmp.close();
        QString execStr = TMP_SCR;
        proc.start(execStr);
        proc.waitForFinished();
//        qDebug() << "body" << body;
//        qDebug() << "exec" << execStr;
//        qDebug() << "ret text:" << proc.readAll();
//        qDebug() << "ret code:" << proc.exitCode();
        tmp.remove();
        return (proc.exitCode() == 0);
    }
    else
        return false;
}

bool MainWindow::MoveFileFtp(const QString &cmd, const QString &dir, const QString &file, quint8 n)
{
    QString body = "";
    body += QString("open %1\n").arg(gate[n].host.toString());
    body += QString("user %1 %2\n").arg("root", "eprst");
    body += "bin\n";
    body += QString("cd %1\n").arg(dir);
    body += QString("%1 ./%2 ./%3\n").arg(cmd, file, file);
    body += "quit\n";

    QProcess move;
    QFile tmp(TMP_SCR);
    if (tmp.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate))
    {
        QTextStream out(&tmp);

#if defined(WIN32)
        out << body;
        tmp.close();
        QString execStr = QString("ftp -n -s:%1").arg(TMP_SCR);
#else
        out << "ftp -in <<EOF\n";
        out << body;
        out << "EOF\n";
        tmp.close();
        move.start(QString("chmod 777 %1").arg(TMP_SCR));
        move.waitForFinished();
        QString execStr = TMP_SCR;
#endif

        move.start(execStr);
        move.waitForFinished();
//            qDebug() << execStr;
//            qDebug() << "ret text:" << move.readAll();
//            qDebug() << "ret code:" << move.exitCode();
        tmp.remove();
        return (move.exitCode() == 0);
    }
    else
        return false;
}

void MainWindow::GateInit()
{
    ui->label->setText(tr("Gate configuration net download..."));
    ui->progressBar->setValue(0);

    QString path = path_conf.replace('\\', '/') + "/tcms.conf";
    CCfgConf tcms;
    tcms.parseFile(path);
    //tcms.getAllToDebug();
    for(quint8 net=0; net<2; net++)
    {
        QStringList host;
        for(quint8 i=1; i<=4; i++)
            host.append(tcms.getValue("/NetRouting" + GetSection("Route_", net + 1) + "/Gate", QString("ip_part%1").arg(i)));
        gate[net].host = (QHostAddress(host.join(".")));
        gate[net].remoteDir = "/vcss/conf/" + gate[net].localDir.replace('\\', '/').split('/').last();
        gate[net].connected = GetConfigFromGate(net);
        GetRevision(net);
        ui->progressBar->setValue((net + 1) * 50);
        if(!gate[net].connected)
        {
            gate[net].revision += tr("\n!(Not connected - use local data)!");
            //delay if error
            QEventLoop loop;
            QTimer::singleShot(3000, &loop, SLOT(quit()));
            loop.exec();
        }
        rbNet[net]->setText(QString("%1 (%2)  %3").arg(gate[net].name, gate[net].dirName, gate[net].revision));
    }

    ui->label->setText(tr("Segment data loading..."));
    ui->lwInfoStart->clear();
    ui->frLoading->setVisible(false);
    ui->frDialog->setVisible(true);

//    foreach (SGate entry, gate)
//    {
//        qDebug() << "SGate" << entry.host << entry.localDir << entry.remoteDir ;
//    }

}

bool MainWindow::GetConfigFromGate(quint8 item)
{
    AddInfo(tr("Loading gate configuration from host: %1 (%2)").arg(gate[item].dirName, gate[item].host.toString()));
    QFile fileHw(FILE_HW);
    if(fileHw.exists())
        fileHw.remove();
    bool ret;
    QString target = gate[item].localDir + "/" + FILE_HW;
    ret = MoveFileFtp("get", gate[item].remoteDir, FILE_HW, item);
    if(ret && fileHw.exists())
    {
        //qDebug() << "get by ftp";
        if(fileHw.exists(target))
            fileHw.remove(target);
        if(fileHw.copy(target))
        {
            //qDebug() << "put to target dir";
        }
        return true;
    }
    else
    {
        AddInfo(tr("Check out error (FTP) for host: %1 (%2)").arg(gate[item].dirName, gate[item].host.toString()));
        return false;
    }
}

void MainWindow::PutConfigToGates()
{
    ui->label->setText(tr("Gate configuration net synchronization"));
    ui->progressBar->setValue(0);
    ui->lwInfoStart->clear();
    ui->tabSegments->setVisible(false);
    ui->chFlashAdd->setVisible(false);
    ui->frLoading->setVisible(true);
    AddInfo(tr("Start synchronization process"));

    for(quint8 host=0; host<gate.size(); host++)
        for(quint8 item=0; item<gate.size(); item++)
        {
            if(PutConfig(host, item))
                AddInfo(tr("Config file: %1 was copied to host: %2").arg(gate[item].remoteDir + "/" + FILE_HW, gate[host].host.toString()));
            else
                AddInfo(tr("Error! Config file: %1 not was copied to host: %2").arg(gate[item].remoteDir + "/" + FILE_HW, gate[host].host.toString()));
            ViewProgressSave();
        }
    AddInfo(tr("Done"));
    ui->statusBar->showMessage(tr("Synchronization completed"), 10000);

}

bool MainWindow::PutConfig(quint8 host, quint8 item)
{
    QFile fileHw(FILE_HW);
    if(fileHw.exists())
        fileHw.remove();
    QString target = gate[item].localDir + "/" + FILE_HW;
    QByteArray fLocal, fRemote;
    fileHw.copy(target, FILE_HW);
    if(fileHw.open(QIODevice::ReadOnly))
    {
        fLocal = fileHw.readAll();
        fileHw.close();
    }

    bool ret;
    ret = MoveFileFtp("put", gate[item].remoteDir, FILE_HW, host);
    if(!ret)
        return false;
    if(fileHw.exists())
        fileHw.remove();
    ret = MoveFileFtp("get", gate[item].remoteDir, FILE_HW, host);
    if(ret && fileHw.exists())
    {
        if(fileHw.open(QIODevice::ReadOnly))
        {
            fRemote = fileHw.readAll();
            fileHw.close();
            fileHw.remove();
        }
        if(fRemote.contains(fLocal) && !fLocal.isEmpty())
        {
            //qDebug() << "compare ok";
            return true;
        }
        else
            return false;
    }
    else
        return false;
}


void MainWindow::SaveBackup(const QString &timestamp)
{
#if defined(WIN32)
    QString device(ui->lwStorage->whatsThis());
#else
    QString device(LINUX_MNT);
#endif
    QString path = device + QString(DIR_STORAGE);
    QDir storage;
    if(!storage.exists(path))
        if(!storage.mkdir(path))
        {
            ui->statusBar->showMessage(tr("Storage create error!"), 10000);
            //qDebug() << "storage create error!";
            return;
        }
    QString source = gate[0].localDir + "/" + FILE_HW;
    QString target = path + "/" + timestamp + FILE_EXT;
    QFile fileHw(source);
    if(fileHw.copy(source, target))
    {
        ui->statusBar->showMessage(tr("Backup file created"), 10000);
        //qDebug() << "create backup file - ok";
        ui->chFlashAdd->setChecked(false);
    }
    else
    {
        ui->statusBar->showMessage(tr("Backup file create error!"), 10000);
        //qDebug() << "error create file";
        return;
    }
}

bool MainWindow::LoadBackup()
{
    //quint16 backupIndex = ui->lwStorage->row(ui->lwStorage->currentItem);
    QString source = ui->lwStorage->currentItem()->whatsThis();
    QString target = gate[0].localDir + "/" + FILE_HW;
    //qDebug() << "+++++++++++" << source;
    QFile fileHw(target);
    if(fileHw.exists())
        fileHw.remove();
    if(fileHw.copy(source, target))
    {
        AddInfo(tr("Restore from backup file: ") + source);
        ui->statusBar->showMessage(tr("Backup file loaded"), 10000);
        GetRevision(0);
        //qDebug() << "loaded backup file - ok";
        return true;
    }
    else
    {
        ui->statusBar->showMessage(tr("Backup file load error!"), 10000);
        //qDebug() << "error load file";
        return false;
    }

}

void MainWindow::ViewStorage()
{
    //qDebug() << "---------------------------------------------";
    ui->lwStorage->clear();
    ui->pbSelect->setEnabled(false);
#if defined(WIN32)
    QString device(ui->lwStorage->whatsThis());
#else
    QString device(LINUX_MNT);
#endif
    QString path = device + QString(DIR_STORAGE);
    QDir storage(path);
    if(storage.exists())
    {
        //qDebug() << "explore" << path;
        QFileInfoList content = storage.entryInfoList(QStringList("*" + QString(FILE_EXT)), QDir::Files | QDir::NoDotAndDotDot);
        //qDebug() << "content" << content.size();
        QListWidgetItem *last = 0;
        foreach (QFileInfo entry, content)
        {
            if((GetSignature(entry.absoluteFilePath()) == gate[base].sign) || gate[base].sign.isEmpty())
            {
                QDateTime dt;
                dt.setMSecsSinceEpoch(0);
                dt = dt.addMSecs(entry.baseName().toLongLong());
                //ui->lwStorage->addItem(dt.toString(" yyyy-MM-dd hh:mm:ss") + entry.lastModified().toString(" yyyy-MM-dd hh:mm:ss"));
                ui->lwStorage->addItem(dt.toString("yyyy-MM-dd   hh:mm:ss  ") + QString("  (%1/%2)").arg(DIR_STORAGE, entry.fileName()));
                last = ui->lwStorage->item(ui->lwStorage->count() - 1);
                last->setWhatsThis(entry.absoluteFilePath());
            }
        }
        ui->lwStorage->setCurrentItem(last);
        if(ui->lwStorage->count() > 0)
            ui->pbSelect->setEnabled(true);
    }
}

void MainWindow::timer_shot()
{
    //qDebug() << "shot";
    flashReady = false;
    QList<SFlash> flash = PluggedFlash();
    if(flash.count() == 0)
    {
        ui->chFlash->setText(ui->chFlash->whatsThis());
        ui->chFlash->setEnabled(false);
        ui->chFlashAdd->setText(ui->chFlashAdd->whatsThis());
        ui->chFlashAdd->setEnabled(false);
    }
    else
    {
        QString info;
        quint8 i, index = 0;
        for(i=0; i<flash.count(); i++)
            if(flash[i].mount)
            {
                flashReady = true;
                index = i;
            }

        ui->chFlash->setEnabled(true);
        ui->chFlashAdd->setEnabled(true);
        if(flashReady)
        {
            //qDebug() << "ready dev" << index << flash[index].dev << flash[index].label << ui->lwStorage->whatsThis();
            ui->lwStorage->setWhatsThis(flash[index].dev);
            info = tr(" (flash disk mounted: %1)").arg(flash[index].label);
            ui->chFlash->setText(ui->chFlash->whatsThis() + info);
            ui->chFlashAdd->setText(ui->chFlashAdd->whatsThis() + info);
        }
        else
        {
            if(flash.count() == 1)
                info = tr(" (flash disk connected: %1)").arg(flash[0].label);
            else
                info = tr(" (flash disk connected: %1 pieces)").arg(flash.count());
            ui->chFlash->setText(ui->chFlash->whatsThis() + info);
            ui->chFlashAdd->setText(ui->chFlashAdd->whatsThis() + info);
#if defined(WIN32)
            //flash ejected & still connected
            ui->chFlash->setEnabled(false);
            ui->chFlashAdd->setEnabled(false);
#endif
        }
    }

    //select controls by flash mounted
    static bool showRealOldState = true;
    bool showReal = !(ui->chFlash->isChecked() && flashReady);
    ui->lbSelectGate->setVisible(showReal);
    ui->groupBox->setVisible(showReal);
    if(showReal != showRealOldState)
    {
        if(!showReal)
            ViewStorage();
        else
            ui->pbSelect->setEnabled(true);
    }
    ui->lbSelectTimeStamp->setVisible(!showReal);
    ui->lwStorage->setVisible(!showReal);

    showRealOldState = showReal;

    //save button manegement
    bool writeReady = ui->chFlashAdd->isChecked() && flashReady;
    ui->pbSave->setEnabled(started && (writeReady || changed || restored));

//    foreach (SFlash entry, flash)
//    {
//        qDebug() << entry.mount << entry.label << entry.uuid;
//    }
}

void MainWindow::chFlash_change(bool state)
{
    //qDebug() << "%%%" << state << flashReady;
    if(state)
    {
        if(!flashReady)
            MountFlash();
    }
    else
    {
        if(flashReady)
            UnMountFlash();
    }
}

void MainWindow::chFlashAdd_change(bool state)
{
    chFlash_change(state);
}

void MainWindow::MountFlash()
{
#if defined(WIN32)

#else
    //qDebug() << "% flashka";
    QString cmd = "/soft/scripts/flashka";
    script->start(cmd);
    script->waitForFinished();
#endif
}

void MainWindow::UnMountFlash()
{
#if defined(WIN32)
    EjectWinFlash(ui->lwStorage->whatsThis());
#else
    //qDebug() << "% uflashka";
    QString cmd = "/soft/scripts/uflashka";
    script->start(cmd);
    script->waitForFinished();
#endif
}

QList<SFlash> MainWindow::PluggedFlash()
{
    QList<SFlash> flashList;
#if defined(WIN32)
    quint8 bloks = 5;
    QString cmd = "wmic LOGICALDISK GET caption,drivetype,volumename,volumeserialnumber /FORMAT:csv";
    script->start(cmd);
    script->waitForFinished();
    QString all = script->readAllStandardOutput();
    QStringList list = all.split('\n');
    foreach (QString entry, list)
    {
        QStringList line = entry.split(',');
        if(line.size() ==  bloks)
        {
            SFlash tmp;
            tmp.dev = line.at(1);
            tmp.label = line.at(3);
            if(tmp.label.isEmpty())
                tmp.label = "NO NAME";
            tmp.uuid = line.at(4).simplified();
            if(tmp.uuid.isEmpty())
                tmp.label = tr("drive letter ") + tmp.dev;
            tmp.mount = !tmp.uuid.isEmpty();
            if(line.at(2) == "2")
                flashList.append(tmp);
        }
    }
//    foreach (SFlash entry, flashList)
//    {
//        qDebug() << "***" << entry.dev << entry.uuid << entry.mount << entry.label;
//    }
#else
    QString cmd = "/sbin/blkid";
    script->start(cmd);
    script->waitForFinished();
    QString all = script->readAllStandardOutput();
    QStringList list = all.split('\n');
    foreach (QString entry, list)
    {
        if(entry.contains("vfat"))
        {
            SFlash tmp;
            tmp.dev = LeftTo(entry, ":");
            QMap<QString, QString> elements;
            ExtractLine(&elements, entry);
            tmp.label = elements["LABEL"];
            if(tmp.label.isEmpty())
                tmp.label = "NO NAME";
            tmp.uuid = elements["UUID"];
            tmp.mount = false;
            flashList.append(tmp);
        }
    }
    cmd = "/bin/df";
    script->start(cmd);
    script->waitForFinished();
    all = script->readAllStandardOutput();
    list = all.split('\n');
    foreach (QString entry, list)
    {
        if(entry.simplified().contains("/mnt/flash"))
        {
            QStringList item = entry.split(' ');
            foreach (QString field, item)
            {
                if(field.startsWith("/dev/sd"))
                    for(quint8 i=0 ; i<flashList.count(); i++)
                        if(field == flashList[i].dev)
                            flashList[i].mount = true;
            }
        }
    }

#endif
    return flashList;
}

void MainWindow::GetRevision(quint8 item)
{
    CCfgConf hw;
    hw.parseFile(gate[item].localDir + "/" + FILE_HW);
    QString timestamp = hw.getValue("", "time_stamp");
    QDateTime dt;
    dt.setMSecsSinceEpoch(0);
    dt = dt.addMSecs(timestamp.toLongLong());
    gate[item].revision = tr("revision: %1").arg(dt.toString("yyyy-MM-dd  hh:mm:ss"));
    gate[item].sign = hw.getValue("", "system_signature").mid(1);
    return;
}

QString MainWindow::GetSignature(const QString &path)
{
    CCfgConf hw;
    hw.parseFile(path);
    return hw.getValue("", "system_signature").mid(1);
}

QString MainWindow::LeftTo(const QString &text, const QString &target)
{
    return text.left(text.indexOf(target));
}

QString MainWindow::RightFrom(const QString &text, const QString &target)
{
    return text.mid(text.indexOf(target) + target.size());
}

QString MainWindow::RightFromLast(const QString &text, const QString &target)
{
    QString ret;
    QString rem = text;
    while(true)
    {
        ret = rem;
        rem = RightFrom(rem, target);
        if(rem == ret)
            return ret;
    }
}

void MainWindow::ExtractLine(QMap<QString, QString> *line, const QString &text, const QString &mark)
{
    QString k, v, rem;
    qint16 p = 0;
    rem = text;
    while(true)
    {
        p = rem.indexOf(mark);
        if(p < 0)
            break;
        k = rem.left(p);
        k = RightFromLast(k, " ");
        rem = rem.mid(p + mark.size());
        v = LeftTo(rem.mid(1), "\"");
        line->insert(k, v);
    }
    return;
}
