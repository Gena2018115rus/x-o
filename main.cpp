#include "mainwindow.h"
#include "clientwindow.h"
#include "openglwidget.h"
#include <QApplication>
#include "eth-lib/eth-lib.hpp"
#include "waitdialog.h"
#include <fstream>

bool botEnabled, onlineMode;
int N4Win;
extern bool stepX, waiting;
MainWindow *wnd;
std::string out_buf, in_buf;
std::mutex out_buf_mtx/*, in_buf_mtx*/, g_client_mtx, g_connection2host_mtx, g_connection_established_mtx;
client_ref_t *g_client;
client_t *g_connection2host;
const std::regex re{"(-?\\d+)&(-?\\d+)"};
bool g_connection_established;
extern std::string nickname;

struct client_data_t {
    client_ref_t ref;
    std::string opponent_name;
};

struct out_buf_t {
    std::string data;
    std::mutex mtx;
};

std::map<std::string, client_ref_t> unknown_clients;
std::map<std::string, client_ref_t> waiting_clients;
std::map<std::string, client_data_t> playing_clients;
std::map<std::string, out_buf_t> out_bufs;

bool flush(std::string addr) {
    if (unknown_clients.at(addr).write(out_bufs.at(addr).data)) {
        out_bufs.at(addr).data.clear();
    } else {
        std::cerr << "Проблема с отправкой данных из буфера клиенту" << std::endl;
        exit(-201);
    }
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
        std::cerr << "Надо указать адрес сервера или режим сервера. Например localhost:20181 или -server" << std::endl;
    }

    if (argv[1] == std::string("-server")) {
        std::cout << "SERVER OK" << std::endl;

        (std::cout << "введите порт сервера: ").flush();
        int port;
        std::cin >> port;
        listener_t(port).listen([](client_ref_t client) { // сделать нормальное сообщение если порт занят
            std::string client_addr = client.addr();
            std::cout << client_addr << " подключился" << std::endl;
            unknown_clients.emplace(client_addr, std::move(client));
            out_bufs.try_emplace(client_addr);
            g_client_mtx.lock();
//            g_client = &s_client;
//                connection_established = true;
//                wait_dialog->accept();
            out_buf_mtx.lock();
            if (!out_buf.empty()) {
                if (g_client->write(out_buf)) {
                    out_buf.clear();
                } else {
                    std::cerr << "Проблема с отправкой первых значений из буфера клиенту" << std::endl;
                    exit(-100);
//                            a.closeAllWindows();  .exit()    ?     +сделать защиту от одновременных подключений к серверу       а потом кто-нибудь мог бы добавить режим наблюдателя
                }
            }
            out_buf_mtx.unlock();
            g_client_mtx.unlock();
        }, [&](std::string addr, std::string msg) {
            std::cout << addr << ": " << msg << std::endl;
            std::smatch m;
            if (std::regex_match(msg, m, re)) {
                std::cout << "x = " << m[1] << "; y = " << m[2] << ';' << std::endl;
//                    emit wait_dialog->onNewClickCoordReceived({{stoi(m[1]), stoi(m[2])}}); // надо записывать результат в weHaveAWinner
//                        QMetaObject::invokeMethod();
//                    w.get_openGLWidget()->update(); // перенести update в myClickEvent?
            } else if (std::regex_match(msg, m, std::regex{"HELLO=(.*)"})) {
                std::string client_name = m[1];
                if (client_name == "") {
//                    out_bufs.at(addr).mtx.lock(); // TODO?: синхронизация доступа к out_bufs
                    out_bufs.at(addr).data += "HELLO: ERR\r\n";
                    flush(addr);
//                    out_buf_mtx.unlock();
                    return;
                }
                auto node_handle = unknown_clients.extract(addr);
                node_handle.key() = nickname;
                waiting_clients.insert(std::move(node_handle));
                std::cout << "начал ждать человек \"" << client_name << '"' << std::endl;
            } else if (msg == "GetWaitingClients") {
                for (auto &p : waiting_clients) {
                    out_bufs.at(addr).data += p.first + "\r\n";
                }
                out_bufs.at(addr).data += "\r\n";
                flush(addr);
            } else if (1) {
                std::cout << '"' << msg << '"' << std::endl;
            }
        }, [](std::string addr) {
            std::cout << addr << " отключился" << std::endl;
        });




        return 0;
    }

    QApplication a(argc, argv);

    std::thread([&] {
        std::string server_full_addr = argv[1];
        auto index = server_full_addr.find(":");
        std::string server_addr = {server_full_addr.begin(), server_full_addr.begin() + index}, server_port = {server_full_addr.begin() + index + 1, server_full_addr.end()};
        std::cout << server_addr << ' ' << server_port << std::endl;

        client_t client(server_addr, server_port);
        g_connection2host_mtx.lock();
        g_connection2host = &client;
        g_connection2host_mtx.unlock();
        client.run([&](std::string chunk) {
            std::cout << chunk << std::endl;
//                    in_buf_mtx.lock();
            in_buf += chunk;
            for (;;) {
                auto index = in_buf.find("\r\n");
                if (index != (size_t)-1) {
                    std::string line = {in_buf.begin(), in_buf.begin() + index};
                    in_buf = {in_buf.begin() + index + 2, in_buf.end()};
                    // обработка line
                    std::cout << "найдено: >>>" << line << "<<<" << std::endl;
                    std::smatch m;
                    if (std::regex_match(line, m, re)) {
                        std::cout << "x = " << m[1] << "; y = " << m[2] << ';' << std::endl;
//                        emit wait_dialog->onNewClickCoordReceived({{stoi(m[1]), stoi(m[2])}}); // надо записывать результат в weHaveAWinner
//                        w.get_openGLWidget()->update();
                    } else if (line == "ClientIsFirst") {
                        waiting = true; // мб ещё мьютексов посоздавать для переменных которые в обоих потоках используются?
                    } else if (std::regex_match(line, m, std::regex{"N4Win=(\\d+)"})) {
                        std::cout << "N4Win = " << m[1] << '!' << std::endl;
                        N4Win = stoi(m[1]);
                    }
                } else {
                    break;
                }
            }
//                    in_buf_mtx.unlock();
        });
    }).detach();

    auto client_window = new ClientWindow;
    client_window->show();
    auto ret_code = a.exec();
    if (!ret_code) {
        return 0;
    }
    delete client_window;

    MainWindow w;
//    w.setAttribute(Qt::WA_AcceptTouchEvents);
    wnd = &w;
    w.show();

    if (ret_code == 2) {
        onlineMode = true;
        static auto wait_dialog = new WaitDialog(&w);
        QObject::connect(wait_dialog, SIGNAL(onNewClickCoordReceived(icoord)), w.get_openGLWidget(), SLOT(myClickEvent(icoord)), Qt::BlockingQueuedConnection); // переделать с использованием QMetaObject::invokeMethod чтобы не добавлять костыль в wait_dialog
        wait_dialog->setText("Ждём, " + QString::fromUtf8(nickname.data(), nickname.size()) + "...");



        while (!g_connection_established)
        {
            wait_dialog->exec();
        }

        return a.exec();
    }

    if (QMessageBox::question(&w, "Привет!", "Играть по сети?", QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes)
    {
        onlineMode = true;
        static auto wait_dialog = new WaitDialog(&w);
        QObject::connect(wait_dialog, SIGNAL(onNewClickCoordReceived(icoord)), w.get_openGLWidget(), SLOT(myClickEvent(icoord)), Qt::BlockingQueuedConnection); // переделать с использованием QMetaObject::invokeMethod чтобы не добавлять костыль в wait_dialog
        if (QMessageBox::question(&w, "Ещё пара вопросов..", "Быть хостом?", QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes)
        {
            static int port = QInputDialog::getInt(&w, "Порт по умолчанию 20181", "На каком порту слушать? [0-65535]", 20181, 0, 65535);
            wait_dialog->setText("Ждём подключения к порту " + QString::number(port) + "...");
            static bool connection_established = false;

            std::thread([&] {
                listener_t(port).listen([](client_ref_t client) { // сделать нормальное сообщение если порт занят
                    static client_ref_t s_client = std::move(client);
                    g_client_mtx.lock();
                    g_client = &s_client;
                    connection_established = true;
                    wait_dialog->accept();
                    out_buf_mtx.lock();
                    if (!out_buf.empty()) {
                        if (g_client->write(out_buf)) {
                            out_buf.clear();
                        } else {
                            std::cerr << "Проблема с отправкой первых значений из буфера клиенту" << std::endl;
                            exit(-100);
//                            a.closeAllWindows();  .exit()    ?     +сделать защиту от одновременных подключений к серверу       а потом кто-нибудь мог бы добавить режим наблюдателя
                        }
                    }
                    out_buf_mtx.unlock();
                    g_client_mtx.unlock();
                }, [&](std::string addr, std::string msg) {
                    std::cout << addr << ": " << msg << std::endl;
                    std::smatch m;
                    if (std::regex_match(msg, m, re)) {
                        std::cout << "x = " << m[1] << "; y = " << m[2] << ';' << std::endl;
                        emit wait_dialog->onNewClickCoordReceived({{stoi(m[1]), stoi(m[2])}}); // надо записывать результат в weHaveAWinner
//                        QMetaObject::invokeMethod();
                        w.get_openGLWidget()->update(); // перенести update в myClickEvent?
                    }
                }, [](std::string addr) {
                    // TODO: обработать отключение
                });
            }).detach();

            N4Win = QInputDialog::getInt(&w, "Обязательно сообщи противнику!", "Сколько в ряд для пообеды?", 4, 3);
            out_buf_mtx.lock();
            out_buf += "N4Win=" + std::to_string(N4Win) + "\r\n";
            out_buf_mtx.unlock();

            waiting = false;
            if (QMessageBox::question(&w, "Игра начинается!", "Первый ход Ваш?", QMessageBox::Yes, QMessageBox::No) == QMessageBox::No)
            {
                w.get_openGLWidget()->myClickEvent({{0, 0}});
                w.get_openGLWidget()->update();
                out_buf_mtx.lock();
                out_buf += "ClientIsFirst\r\n";
                // TODO: if (g_client) if (g_client->write(out_buf)) { ...         только вынести бы это ещё в функцию  типа flush
                out_buf_mtx.unlock();
                waiting = false;
            }

            while (!connection_established)
            {
                wait_dialog->exec();
            }
        }
        else
        {
            static auto addr = QInputDialog::getText(&w, "Пожалуйста", "Введите адрес хоста (без порта):", QLineEdit::Normal, "localhost");
            static int port = QInputDialog::getInt(&w, "А теперь..", "Введите порт хоста [0-65535]", 20181, 0, 65535);
            // сделать нормальную ошибку подключения    и мб разрывы соединения восстанавливать (обе стороны должны пытаться)

            std::thread([&] {
                client_t client(addr.toStdString(), std::to_string(port));
                g_connection2host_mtx.lock();
                g_connection2host = &client;
                g_connection2host_mtx.unlock();
                client.run([&](std::string chunk) {
                    std::cout << chunk << std::endl;
//                    in_buf_mtx.lock();
                    in_buf += chunk;
                    for (;;) {
                        auto index = in_buf.find("\r\n");
                        if (index != (size_t)-1) {
                            std::string line = {in_buf.begin(), in_buf.begin() + index};
                            in_buf = {in_buf.begin() + index + 2, in_buf.end()};
                            // обработка line
                            std::cout << "найдено: >>>" << line << "<<<" << std::endl;
                            std::smatch m;
                            if (std::regex_match(line, m, re)) {
                                std::cout << "x = " << m[1] << "; y = " << m[2] << ';' << std::endl;
                                emit wait_dialog->onNewClickCoordReceived({{stoi(m[1]), stoi(m[2])}}); // надо записывать результат в weHaveAWinner
                                w.get_openGLWidget()->update();
                            } else if (line == "ClientIsFirst") {
                                waiting = true; // мб ещё мьютексов посоздавать для переменных которые в обоих потоках используются?
                            } else if (std::regex_match(line, m, std::regex{"N4Win=(\\d+)"})) {
                                std::cout << "N4Win = " << m[1] << '!' << std::endl;
                                N4Win = stoi(m[1]);
                            }
                        } else {
                            break;
                        }
                    }
//                    in_buf_mtx.unlock();
                });
            }).detach();
        }
    }
    else if (N4Win = QInputDialog::getInt(&w, "Обязательно сообщи противнику!", "Сколько в ряд для пообеды?", 4, 3),
             QMessageBox::question(&w, "Сейчас начнём", "Играть с компьютером?", QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes)
    {
        botEnabled = 1;
        if (QMessageBox::question(&w, "Игра начинается!", "Первый ход Ваш?", QMessageBox::Yes, QMessageBox::No) == QMessageBox::No)
        {
            w.get_openGLWidget()->myClickEvent({{0, 0}});
            w.get_openGLWidget()->update();
        }
        else
        {
            stepX ^= 1;
        }
    }

    return a.exec();
}
