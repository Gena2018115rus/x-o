#include "clientwindow.h"
#include "ui_clientwindow.h"
#include <QLayout>

ClientWindow::ClientWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::ClientWindow)
{
    ui->setupUi(this);

    ui->widget->setLayout(new QVBoxLayout);
    QHBoxLayout *hl = new QHBoxLayout;
    ui->widget->layout()->addItem(hl);
    hl->addWidget(new QLabel("123"));
    hl->addWidget(new QLabel("123"));
    hl->addWidget(new QLabel("123"));
    hl->addWidget(new QLabel("123"));

}

ClientWindow::~ClientWindow()
{
    delete ui;
}

void ClientWindow::on_m_pPbQuit_clicked()
{
    qApp->exit(0);
}


void ClientWindow::on_m_pPbStartGame_clicked()
{
    qApp->exit(1);
}

