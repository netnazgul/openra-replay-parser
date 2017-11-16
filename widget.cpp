#include "widget.h"
#include "ui_widget.h"

#include <QFile>
#include <QFileDialog>
#include <QMessageBox>
#include <QPair>
#include <QDirIterator>
#include <QDebug>
#include <QElapsedTimer>
#include <QMap>
#include <QDateTime>
#include <QTimeZone>

struct player_struct{
    QString name;
    int random_faction;
    QString faction;
    int outcome;
};

struct replay_struct{
    QString filename;
    QString name;
    QString timestamp;
    QString timestamp2;
    QString map;
    player_struct player[2];
    QString winner() {
        if (player[0].outcome == 1)
            return player[0].name;

        if (player[1].outcome == 1)
            return player[1].name;

        return "-";
    }
    QString loser() {
        if (player[0].outcome == 0)
            return player[0].name;

        if (player[1].outcome == 0)
            return player[1].name;

        return "-";
    }

    QTime duration() {
        QDateTime ts = QDateTime::fromString(timestamp,"yyyy-MM-dd HH-mm-ss");
        QDateTime ts2 = QDateTime::fromString(timestamp2,"yyyy-MM-dd HH-mm-ss");
        QTime diff = QTime::fromMSecsSinceStartOfDay(ts.msecsTo(ts2));
        return diff;
    }
} ;

player_struct parse_player_report(QFile& f);

Widget::Widget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Widget)
{
    ui->setupUi(this);
}

Widget::~Widget()
{
    delete ui;
}

player_struct parse_player_report(QFile &f) {
    player_struct p;
    p.name = "";
    p.random_faction = -1;
    p.faction = "";
    p.outcome = -1;

    // getting name, faction and outcome
    QString input;
    while (!f.atEnd())
    {
        input = f.readLine();

        if (input.mid(0, 6) == "\tName:")
            p.name = input.mid(7).trimmed();

        if (input.mid(0, 13) == "\tFactionName:")
            p.faction = input.mid(14).trimmed();

        if (input.mid(0, 17) == "\tIsRandomFaction:")
            p.random_faction = input.contains("True");

        if (input.mid(0, 9) == "\tOutcome:")
            if (input.contains("Won"))
                p.outcome = 1;
            else if (input.contains("Lost"))
                p.outcome = 0;
            else
                p.outcome = 2;

        // exit function when all params are read
        if (p.name.size() && p.faction.size() && (p.outcome != -1) && (p.random_faction != -1))
            return p;
    }

    return p;
}

bool parse_replay(QString filename, replay_struct& entry)
{
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
       return false;

    QString line;

    QFileInfo fi(filename);
    entry.filename = filename;
    entry.name = fi.fileName();
    entry.name.chop(7);
    int i = 0;

    while (!file.atEnd()) {
        line = file.readLine();
        if (line.mid(0, 10) == "\tMapTitle:")
            entry.map = line.mid(11).trimmed();

        if (line.mid(0,14) == "\tStartTimeUtc:")
            entry.timestamp = line.mid(15).trimmed();

        if (line.mid(0,12) == "\tEndTimeUtc:")
            entry.timestamp2 = line.mid(13).trimmed();

        if (line.mid(0, 7) == "Player@")
        {
            i = line[7].digitValue();

            player_struct player = parse_player_report(file);

            if (player.outcome < 0)
            {
                file.close();
                return false;
            }

            if (i == 0 || i == 1)
                entry.player[i] = player;
            else
            {
                file.close();
                return false;
            }
        }
    }

    file.close();
    return true;
}

void Widget::on_buttonParseFile_clicked()
{
    ui->tableOutput->setRowCount(0);

    QString filename = QFileDialog::getOpenFileName(this,
                                            "Select valid orarep file",
                                            "",
                                            "orarep files (*.orarep);;All files (*.*)"
                                            );
    if (filename.size() < 1)
    {
        QMessageBox::warning(this, "File operation",
                                   "Error when opening file",
                                   QMessageBox::Ok,
                                   QMessageBox::Ok);
        return;
    }

/////

    replay_struct entry;
    if (!parse_replay(filename, entry))
    {
        QMessageBox::warning(this, "File operation",
                                   "Error when opening file",
                                   QMessageBox::Ok,
                                   QMessageBox::Ok);
    }

    ui->tableOutput->setRowCount(ui->tableOutput->rowCount()+1);

    QTableWidgetItem *newItem1 = new QTableWidgetItem(entry.name);
    ui->tableOutput->setItem(0,0,newItem1);

    QTableWidgetItem *newItem2 = new QTableWidgetItem(entry.timestamp);
    ui->tableOutput->setItem(0,1,newItem2);

    QTableWidgetItem *newItem3 = new QTableWidgetItem(entry.map);
    ui->tableOutput->setItem(0,2,newItem3);

    QTableWidgetItem *newItem4 = new QTableWidgetItem(entry.duration().toString("H:mm:ss"));
    ui->tableOutput->setItem(0,3,newItem4);

    QTableWidgetItem *newItem5 = new QTableWidgetItem(entry.winner());
    ui->tableOutput->setItem(0,4,newItem5);

    QTableWidgetItem *newItem6 = new QTableWidgetItem(entry.loser());
    ui->tableOutput->setItem(0,5,newItem6);

    ui->tableOutput->resizeColumnsToContents();
}

void Widget::on_buttonParseFolder_clicked()
{

    QString dir = QFileDialog::getExistingDirectory(this, tr("Open Directory"),
                                                "",
                                                QFileDialog::ShowDirsOnly
                                                | QFileDialog::DontResolveSymlinks);

    if (dir.size() < 1)
    {
        QMessageBox::warning(this, "File operation",
                                   "Error when opening file",
                                   QMessageBox::Ok,
                                   QMessageBox::Ok);
        return;
    }

    QDirIterator ct(dir, QStringList() << "*.orarep", QDir::Files, QDirIterator::Subdirectories);

    int count = 0;
    while (ct.hasNext())
        ct.next(), count++;

    ui->progressBar->setValue(0);
    ui->progressBar->setMaximum(count);
    QApplication::processEvents();

    QDirIterator it(dir, QStringList() << "*.orarep", QDir::Files, QDirIterator::Subdirectories);

    replay_struct entry;
    int i = 0;

    QMap<QString, replay_struct> replay_map;
    QMap<QString, int> map_count;
    QMap<QString, int> faction_count;
    QMap<QString, int> faction_wincount;
    QMap<QTime, replay_struct> duration_map;

    QDateTime total_duration = QDateTime::fromMSecsSinceEpoch(0,QTimeZone(0));
    QTime average_duration = QTime::fromMSecsSinceStartOfDay(0);

    QElapsedTimer timer;
    timer.start();

    while (it.hasNext())
    {
        if (parse_replay(it.next(), entry))
        {
            qDebug() << i << timer.restart();
            replay_map[entry.filename] = entry;
            duration_map[entry.duration()] = entry;

            total_duration = total_duration.addMSecs(entry.duration().msecsSinceStartOfDay());

            i++;
            ui->progressBar->setValue(i);
            QApplication::processEvents();
        }
    }

    QMap<QString, replay_struct>::iterator rmi;

    ui->tableOutput->clearContents();
    ui->tableOutput->setRowCount(0);
    ui->tableOutput->setSortingEnabled(false);

    i = 0;
    for (rmi = replay_map.begin(); rmi != replay_map.end(); rmi++)
    {
        ui->tableOutput->insertRow(i);

        ui->tableOutput->setItem(i,1,new QTableWidgetItem(rmi->timestamp));
        ui->tableOutput->setItem(i,5,new QTableWidgetItem(rmi->loser()));
        ui->tableOutput->setItem(i,4,new QTableWidgetItem(rmi->winner()));
        ui->tableOutput->setItem(i,3,new QTableWidgetItem(rmi->duration().toString("H:mm:ss")));
        ui->tableOutput->setItem(i,2,new QTableWidgetItem(rmi->map));
        ui->tableOutput->setItem(i,0,new QTableWidgetItem(rmi->name));

        map_count[rmi->map]++;
        faction_count[rmi->player[0].faction]++;
        faction_count[rmi->player[1].faction]++;
        if (rmi->player[0].outcome == 1)
            faction_wincount[rmi->player[0].faction]++;
        if (rmi->player[1].outcome == 1)
            faction_wincount[rmi->player[1].faction]++;

        i++;
    }

    ui->tableOutput->resizeColumnsToContents();
    ui->tableOutput->setSortingEnabled(true);

    average_duration = QTime::fromMSecsSinceStartOfDay(total_duration.toMSecsSinceEpoch()/replay_map.size());

    // stats crunch
    ui->listStats->clear();
    ui->listStats->addItem(QString("Total files parsed: %1").arg(count));
    ui->listStats->addItem(QString("Correct files: %1").arg(replay_map.size()));
    ui->listStats->addItem("");
    ui->listStats->addItem(QString("Total files duration: %1").arg(total_duration.toString("HH:mm:ss")));
    ui->listStats->addItem(QString("Average game duration: %1").arg(average_duration.toString("HH:mm:ss")));
    ui->listStats->addItem(QString("Shortest game: %1 (%2)").arg(duration_map.firstKey().toString("HH:mm:ss")).arg(duration_map.first().name));
    ui->listStats->addItem(QString("Longest game: %1 (%2)").arg(duration_map.lastKey().toString("HH:mm:ss")).arg(duration_map.last().name));

    // map stats table
    ui->tableMaps->clearContents();
    ui->tableMaps->setRowCount(0);
    ui->tableMaps->setSortingEnabled(false);
    i = 0;
    for (auto it = map_count.begin(); it != map_count.end(); it++)
    {
        ui->tableMaps->insertRow(i);

        ui->tableMaps->setItem(i,0,new QTableWidgetItem(it.key()));
        ui->tableMaps->setItem(i,1,new QTableWidgetItem(QString("%1 (%2%)")
                                                        .arg(it.value())
                                                        .arg(100.0*it.value()/replay_map.size())));
        i++;
    }
    ui->tableMaps->resizeColumnsToContents();
    ui->tableMaps->setSortingEnabled(true);

    // faction stats table
    ui->tableFactions->clearContents();
    ui->tableFactions->setRowCount(0);
    ui->tableFactions->setSortingEnabled(false);
    i = 0;
    for (auto it = faction_count.begin(); it != faction_count.end(); it++)
    {
        ui->tableFactions->insertRow(i);

        ui->tableFactions->setItem(i,0,new QTableWidgetItem(it.key()));
        ui->tableFactions->setItem(i,1,new QTableWidgetItem(QString("%1").arg(it.value())));
        ui->tableFactions->setItem(i,2,new QTableWidgetItem(QString("%1 (%2%)")
                                                        .arg(faction_wincount[it.key()])
                                                        .arg(100.0*faction_wincount[it.key()]/it.value())));
        i++;
    }
    ui->tableFactions->resizeColumnsToContents();
    ui->tableFactions->setSortingEnabled(true);

}
