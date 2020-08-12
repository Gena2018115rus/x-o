#include "mainwindow.h"
#include "openglwidget.h"
#include <QApplication>

bool botEnabled;
int N4Win;
extern bool stepX;
MainWindow *wnd;

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    MainWindow w;
//    w.setAttribute(Qt::WA_AcceptTouchEvents);
    wnd = &w;
    w.show();

    N4Win = QInputDialog::getInt(&w, "Привет!", "Сколько в ряд для пообеды?", 4, 3);

    if (QMessageBox::question(&w, "Сейчас начнём", "Играть с компьютером?", QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes)
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
