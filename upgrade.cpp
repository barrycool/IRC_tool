#include "upgrade.h"
#include "ui_upgrade.h"

Upgrade::Upgrade(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Upgrade)
{
    ui->setupUi(this);
}

Upgrade::~Upgrade()
{
    delete ui;
}
