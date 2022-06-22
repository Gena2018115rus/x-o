#ifndef WAITDIALOG_H
#define WAITDIALOG_H

#include <QDialog>
#include "mainwindow.h"

namespace Ui {
class WaitDialog;
}

class WaitDialog : public QDialog
{
    Q_OBJECT

public:
    explicit WaitDialog(QWidget *parent = nullptr);
    ~WaitDialog();
    void setText(QString s);

public slots:
    void helloErr();

private:
    Ui::WaitDialog *ui;

signals:
    bool onNewClickCoordReceived(icoord coord);
private slots:
    void on_m_pPbCloseProgram_clicked();
};

#endif // WAITDIALOG_H
