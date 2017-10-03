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

struct replay_struct{
    QString filename;
    QString name;
    QString timestamp;
    QString map;
    QString winner;
    QString loser;
} ;

QMap<QString, replay_struct> replay_map;

QPair<QString, int> parse_player_report(QFile& f);
QString parse_player_name(QFile& f);
int parse_player_outcome(QFile& f);

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

QPair<QString, int> parse_player_report(QFile &f) {
    QString player_name = parse_player_name(f);
    int player_outcome = parse_player_outcome(f);
    return qMakePair(player_name, player_outcome);
}

QString parse_player_name(QFile &f) {
    QString input;
    while (!f.atEnd())
    {
        input = f.readLine();
        if (input.mid(0, 6) == "\tName:") {
            return input.mid(7).trimmed();
        }
    }

    return QString();
}

int parse_player_outcome(QFile &f) {
    QString input;
    while (!f.atEnd())
    {
        input = f.readLine();
        if (input.mid(0, 9) == "\tOutcome:")
            return input.mid(10).trimmed() == "Won";
    }

    return -1;
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

    while (!file.atEnd()) {
        line = file.readLine();
        if (line.mid(0, 9) == "\tMapTitle")
            entry.map = line.mid(11).trimmed();

        if (line.mid(0,13) == "\tStartTimeUtc")
            entry.timestamp = line.mid(15).trimmed();

        if (line.mid(0, 6) == "Player")
        {
            QPair<QString, int> outcome = parse_player_report(file);
            if (outcome.second < 0)
            {
                file.close();
                return false;
            }

            if (!outcome.second)
                entry.loser = outcome.first;
            else
                entry.winner = outcome.first;
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

    QTableWidgetItem *newItem4 = new QTableWidgetItem(entry.winner);
    ui->tableOutput->setItem(0,3,newItem4);

    QTableWidgetItem *newItem5 = new QTableWidgetItem(entry.loser);
    ui->tableOutput->setItem(0,4,newItem5);
}

void Widget::on_buttonParseFolder_clicked()
{
/*    ui->tableOutput->clearContents();
    ui->tableOutput->setRowCount(0);*/

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

    QElapsedTimer timer;
    timer.start();

    while (it.hasNext())
    {
        if (parse_replay(it.next(), entry))
        {
            qDebug() << i << timer.restart();
            replay_map[entry.filename] = entry;

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
        ui->tableOutput->setItem(i,4,new QTableWidgetItem(rmi->loser));
        ui->tableOutput->setItem(i,3,new QTableWidgetItem(rmi->winner));
        ui->tableOutput->setItem(i,2,new QTableWidgetItem(rmi->map));
        ui->tableOutput->setItem(i,0,new QTableWidgetItem(rmi->name));
        i++;
    }

    ui->tableOutput->resizeColumnsToContents();
    ui->tableOutput->setSortingEnabled(true);
}
