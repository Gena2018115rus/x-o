#ifndef CLIENTWINDOW_H
#define CLIENTWINDOW_H

#include <QMainWindow>

namespace Ui {
class ClientWindow;
}

class ClientWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit ClientWindow(QWidget *parent = nullptr);
    ~ClientWindow();

public slots:
    void updateWaitingClients(std::string list);
    void connect2opponent();

private slots:
    void on_m_pPbQuit_clicked();

    void on_m_pPbStartGame_clicked();

    void on_m_pPbWait_clicked();

private:
    Ui::ClientWindow *ui;
};

struct client_settings_t {
    int N4Win;
    bool opponent_is_first;
};

#endif // CLIENTWINDOW_H
