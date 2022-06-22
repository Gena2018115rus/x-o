#include "waitdialog.h"
#include "ui_waitdialog.h"

WaitDialog::WaitDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::WaitDialog)
{
    ui->setupUi(this);
}

WaitDialog::~WaitDialog()
{
    delete ui;
}

void WaitDialog::setText(QString s)
{
    ui->label->setText(s);
}

void WaitDialog::on_m_pPbCloseProgram_clicked()
{
    exit(0);
}

void WaitDialog::helloErr() {
    QMessageBox::information(this, "Упс!", "Ошибка, имя уже занято");
    exit(0);
}
