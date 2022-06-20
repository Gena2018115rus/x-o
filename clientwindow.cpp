#include "clientwindow.h"
#include "ui_clientwindow.h"
#include <QLayout>
#include "eth-lib/eth-lib.hpp"
#include <thread>
#include <QMessageBox>

extern client_t *g_connection2host;
extern std::mutex g_connection2host_mtx;
std::string nickname;

ClientWindow::ClientWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::ClientWindow)
{
    ui->setupUi(this);

    g_connection2host_mtx.lock();
    g_connection2host->write("GetWaitingClients");
    g_connection2host_mtx.unlock();

//    ui->widget->setLayout(new QVBoxLayout);
//    QHBoxLayout *hl = new QHBoxLayout;
//    ui->widget->layout()->addItem(hl);
//    hl->addWidget(new QLabel("123"));
//    hl->addWidget(new QLabel("123"));
//    hl->addWidget(new QLabel("123"));
//    hl->addWidget(new QLabel("123"));

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


void ClientWindow::on_m_pPbWait_clicked()
{
    nickname = ui->m_pLeNickname->text().toStdString();
    if (nickname.empty()) {
        QMessageBox::information(this, "Упс!", "Надо ввести псевдоним");
        return;
    }
    std::thread([] {
        qApp->exit(2);
        g_connection2host_mtx.lock();
        g_connection2host->write("HELLO=" + nickname);
        g_connection2host_mtx.unlock();
    }).detach();
}

