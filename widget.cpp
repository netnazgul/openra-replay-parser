#include "widget.h"
#include "ui_widget.h"

#include <QFile>
#include <QFileDialog>
#include <QMessageBox>
#include <QPair>
#include <QDirIterator>

struct replay_struct{
   QString name;
   QString timestamp;
   QString map;
   QString winner;
   QString loser;
} ;

QPair<QString, int> parse_player_report(QFile& file);
QString parse_player_name(QFile& file);
int parse_player_outcome(QFile& file);

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

QPair<QString, int> parse_player_report(QFile& file) {
    QString name;
    QString player_name = parse_player_name(file);
    int player_outcome = parse_player_outcome(file);
    return qMakePair(player_name, player_outcome);
}

QString parse_player_name(QFile& file) {
    QString input;
    while (!file.atEnd())
    {
        input = file.readLine();
        if (input.mid(0, 6) == "\tName:") {
            return input.mid(7).trimmed();
        }
    }

    return QString();
}

int parse_player_outcome(QFile& file) {
    QString input;
    while (!file.atEnd())
    {
        input = file.readLine();
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
    entry.name = fi.fileName();
    entry.name.chop(7);

    while (!file.atEnd()) {
        line = file.readLine();
        if (line.mid(0, 9) == "\tMapTitle")
            entry.map = line.mid(11);

        if (line.mid(0,13) == "\tStartTimeUtc")
            entry.timestamp = line.mid(15);

        if (line.mid(0, 6) == "Player")
        {
            QPair<QString, int> outcome{parse_player_report(file)};
            if (outcome.second < 0)
                return false;

            if (!outcome.second)
                entry.loser = outcome.first;
            else
                entry.winner = outcome.first;
        }
    }

    return true;
}

void Widget::on_buttonParse_clicked()
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

void Widget::on_pushButton_clicked()
{
    ui->tableOutput->setRowCount(0);

    QString dir = QFileDialog::getExistingDirectory(this, tr("Open Directory"),
                                                "",
                                                QFileDialog::ShowDirsOnly
                                                | QFileDialog::DontResolveSymlinks);

    QDirIterator it(dir, QStringList() << "*.orarep", QDir::Files, QDirIterator::Subdirectories);

    replay_struct entry;
    int i = 0;
    while (it.hasNext())
    {
        if (parse_replay(it.next(), entry))
        {
            ui->tableOutput->setRowCount(i+1);

            QTableWidgetItem *newItem1 = new QTableWidgetItem(entry.name);
            ui->tableOutput->setItem(i,0,newItem1);

            QTableWidgetItem *newItem2 = new QTableWidgetItem(entry.timestamp);
            ui->tableOutput->setItem(i,1,newItem2);

            QTableWidgetItem *newItem3 = new QTableWidgetItem(entry.map);
            ui->tableOutput->setItem(i,2,newItem3);

            QTableWidgetItem *newItem4 = new QTableWidgetItem(entry.winner);
            ui->tableOutput->setItem(i,3,newItem4);

            QTableWidgetItem *newItem5 = new QTableWidgetItem(entry.loser);
            ui->tableOutput->setItem(i,4,newItem5);
            i++;
        }
    }

    ui->tableOutput->resizeColumnsToContents();

    ui->tableOutput->sortByColumn(1,Qt::AscendingOrder);
}
