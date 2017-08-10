#include "aboutirc.h"
#include "ui_aboutirc.h"

AboutIrc::AboutIrc(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AboutIrc)
{
    ui->setupUi(this);
}

AboutIrc::~AboutIrc()
{
    delete ui;
}
