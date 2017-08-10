#ifndef UPGRADE_H
#define UPGRADE_H

#include <QDialog>

namespace Ui {
class Upgrade;
}

class Upgrade : public QDialog
{
    Q_OBJECT

public:
    explicit Upgrade(QWidget *parent = 0);
    ~Upgrade();

private:
    Ui::Upgrade *ui;
};

#endif // UPGRADE_H
