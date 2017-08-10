#ifndef ABOUTIRC_H
#define ABOUTIRC_H

#include <QDialog>


namespace Ui {
class AboutIrc;
}

class AboutIrc : public QDialog
{
    Q_OBJECT

public:
    explicit AboutIrc(QWidget *parent = 0);
    ~AboutIrc();

private:
    Ui::AboutIrc *ui;
};

#endif // ABOUTIRC_H
