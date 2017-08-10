#ifndef LEARNINGWAVE_H
#define LEARNINGWAVE_H

#include <QDialog>
#include <protocol.h>
#include <ir_learning.h>
#include <QtCharts/QLineSeries>
#include <QtCharts/QChart>
#include <QtGlobal>
#include <QTime>
#include <QDebug>
#include <QtCharts>

QT_CHARTS_USE_NAMESPACE

namespace Ui {
class LearningWave;
}

class LearningWave : public QDialog
{
    Q_OBJECT

public:
    explicit LearningWave(QWidget *parent = 0);
    ~LearningWave();
    void setWaveData(QString,uint8_t *,int);
    //void setWaveLog(QString);
signals:
    void analysisWaveDataDown(QString);

private slots:
    void showWave();
    void on_pushButton_clicked();

private:
    Ui::LearningWave *ui;
    QLineSeries lineseries;
    QString button;
    uint8_t ir_keyvalue[255];
    int keylen;
    void analysisWaveData();
    void output_log(const QString);
};

#endif // LEARNINGWAVE_H
