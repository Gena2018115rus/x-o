#include "clientwindow.h"
#include "ui_clientwindow.h"
#include <QLayout>
#include "eth-lib/eth-lib.hpp"
#include <thread>
#include <QMessageBox>
#include <QIntValidator>
#include <QPushButton>

extern client_t *g_connection2host;
extern std::mutex g_connection2host_mtx;
std::string nickname;
extern std::map<std::string, client_settings_t> client_settings;
extern std::map<std::string, client_ref_t> waiting_clients;
extern int N4Win;

ClientWindow::ClientWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::ClientWindow)
{
    ui->setupUi(this);

    ui->m_pLeN4Win->setValidator(new QIntValidator(3, 99, this));

//    in_buf_mtx.lock();
//    g_connection2host_mtx.lock();
//    g_connection2host->write("GetWaitingClients");
//    g_connection2host_mtx.unlock();
//    if () {

//    }
//    in_buf_mtx.unlock();

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
    if (ui->m_pLeN4Win->text() == "") {
        QMessageBox::information(this, "Упс!", "Надо ввести, сколько надо поставить одинаковых фигур в ряд для победы. Рекомендуется выбрать от 3 до 6");
        return;
    }
    std::thread([N = ui->m_pLeN4Win->text().toStdString(), F = std::to_string(ui->m_pCbOpponentIsFirst->isChecked())] {
        N4Win = std::stoi(N);
        qApp->exit(2);
        g_connection2host_mtx.lock();
        g_connection2host->write("HELLO=" + nickname + '&' + N + '&' + F);
        g_connection2host_mtx.unlock();
    }).detach();
}

std::map<QWidget *, std::string> btn2nick;

void ClientWindow::updateWaitingClients(std::string list)
{
    QGridLayout *gl = dynamic_cast<QGridLayout *>(ui->scrollAreaWidgetContents->layout());

    static std::vector<QWidget *> widgets2update;

    for (QWidget *p : widgets2update) {
        p->deleteLater();
    }
    widgets2update = {};

    int i = 2;
    static std::regex re{"(.*?)&(.*?)&(.*)\r\n"};
    std::for_each(std::regex_iterator{list.begin(), list.end(), re}, {}, [&](auto &&m) {
        std::string nick = m[1];
        std::string N4Win = m[2];
        std::string opponent_is_first = m[3];
        QWidget *tmp = new QLabel(QString::fromUtf8(nick.data(), nick.size()));
        widgets2update.push_back(tmp);
        gl->addWidget(tmp, i, 0);
        tmp = new QLabel(QString::fromUtf8(N4Win.data(), N4Win.size()));
        widgets2update.push_back(tmp);
        gl->addWidget(tmp, i, 1);
        tmp = new QLabel(opponent_is_first[0] == '0' ? "Не Вы" : "Вы");
        widgets2update.push_back(tmp);
        gl->addWidget(tmp, i, 2);
        tmp = new QPushButton("Присоединиться");
        btn2nick[tmp] = nick;
        std::cout << tmp << std::endl;
        QObject::connect(tmp, SIGNAL(clicked()), this, SLOT(connect2opponent()));
        widgets2update.push_back(tmp);
        gl->addWidget(tmp, i, 3);
        i++;
    });

    if (i == 2) {
        QWidget *tmp = new QLabel("На данный момент в сети никого нет..");
        widgets2update.push_back(tmp);
        gl->addWidget(tmp, 2, 0);
        i++;
    }

    static QSpacerItem *si = new QSpacerItem(0, 0, QSizePolicy::Fixed, QSizePolicy::Expanding);
    gl->removeItem(si);
    gl->addItem(si, i, 0);
}

void ClientWindow::connect2opponent() {
    std::thread([nick = btn2nick[dynamic_cast<QWidget *>(sender())]] {
        qApp->exit(3);
        g_connection2host_mtx.lock();
        g_connection2host->write("connectTo=" + nick);

        g_connection2host_mtx.unlock();
    }).detach();
}
