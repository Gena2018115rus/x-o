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
std::mutex out_buf_mtx/*, in_buf_mtx*/, g_client_mtx, g_connection2host_mtx;
client_ref_t *g_client;
client_t *g_connection2host;
const std::regex re{"(-?\\d+)&(-?\\d+)"};

int main(int argc, char *argv[])
{
    if (IT_IS_SERVER) {
        std::cout << "SERVER OK" << std::endl;

        static std::vector<client_ref_t> clients;

        (std::cout << "введите порт сервера: ").flush();
        int port;
        std::cin >> port;
        listener_t(port).run([](client_ref_t client) { // сделать нормальное сообщение если порт занят
            clients.push_back(std::move(client));
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
            } else if (1) {
                std::cout << '"' << msg << '"' << std::endl;
            }
        });




        return 0;
    }

    QApplication a(argc, argv);

    std::thread([] {
        std::string server_full_addr;
        std::getline(std::ifstream("SERVER_ADDRESS.txt"), server_full_addr);
        auto index = server_full_addr.find(":");
        std::string server_addr = {server_full_addr.begin(), server_full_addr.begin() + index}, server_port = {server_full_addr.begin() + index + 1, server_full_addr.end()};
        std::cout << server_addr << ' ' << server_port << std::endl;

//        client_t client(server_addr, server_port);
//        g_connection2host_mtx.lock();
//        g_connection2host = &client;
//        g_connection2host_mtx.unlock();
//        client.listen([&](std::string chunk) {
//            std::cout << chunk << std::endl;
////                    in_buf_mtx.lock();
//            in_buf += chunk;
//            for (;;) {
//                auto index = in_buf.find("\r\n");
//                if (index != (size_t)-1) {
//                    std::string line = {in_buf.begin(), in_buf.begin() + index};
//                    in_buf = {in_buf.begin() + index + 2, in_buf.end()};
//                    // обработка line
//                    std::cout << "найдено: >>>" << line << "<<<" << std::endl;
//                    std::smatch m;
//                    if (std::regex_match(line, m, re)) {
//                        std::cout << "x = " << m[1] << "; y = " << m[2] << ';' << std::endl;
//                        emit wait_dialog->onNewClickCoordReceived({{stoi(m[1]), stoi(m[2])}}); // надо записывать результат в weHaveAWinner
//                        w.get_openGLWidget()->update();
//                    } else if (line == "ClientIsFirst") {
//                        waiting = true; // мб ещё мьютексов посоздавать для переменных которые в обоих потоках используются?
//                    } else if (std::regex_match(line, m, std::regex{"N4Win=(\\d+)"})) {
//                        std::cout << "N4Win = " << m[1] << '!' << std::endl;
//                        N4Win = stoi(m[1]);
//                    }
//                } else {
//                    break;
//                }
//            }
////                    in_buf_mtx.unlock();
//        });
    }).detach();

    auto client_window = new ClientWindow;
    client_window->show();
    if (!a.exec()) {
        return 0;
    }
    delete client_window;






    MainWindow w;
//    w.setAttribute(Qt::WA_AcceptTouchEvents);
    wnd = &w;
    w.show();

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
                listener_t(port).run([](client_ref_t client) { // сделать нормальное сообщение если порт занят
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
                client.listen([&](std::string chunk) {
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
