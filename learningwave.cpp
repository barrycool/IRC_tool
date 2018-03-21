#include "learningwave.h"
#include "ui_learningwave.h"

LearningWave::LearningWave(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::LearningWave)
{
    ui->setupUi(this);
    this->connect(parent,SIGNAL(send2learningwave()),this,SLOT(showWave()));
    connect(this,SIGNAL(analysisWaveDataDown(QString)),parent,SLOT(analysisFinshed(QString)));

}

LearningWave::~LearningWave()
{
    emit close();
    delete ui;
}

void LearningWave::setWaveData(QString butname,uint8_t wavedata[],int len)
{
    //qDebug() << "setWaveData 000";
    this->button = butname;
    this->keylen = len;

    if(wavedata != NULL)
    {

        memcpy(this->ir_keyvalue,wavedata,len);

    }

    ui->lwbutton->setText(this->button);

}
void LearningWave::analysisWaveData()
{

    QString tmpStr;

    IR_encode(this->ir_keyvalue, keylen);

    tmpStr = IRLearningItem2String(this->keylen);

    ui->lwKey->setText(tmpStr);
    output_log("after decode:");
    output_log(tmpStr);
    emit analysisWaveDataDown(tmpStr);
}

void LearningWave::showWave()
{
    //qDebug() << "analyze the date transfer to wave...";
    QString log = this->button;
    log.append(":L=").append(QString::number(keylen)).append(":");
    for(uint8_t i = 0; i< keylen + 1; i++)
    {
        log += QString(" %1").arg(ir_keyvalue[i]);
    }
    output_log(log);

    lineseries.clear();

    uint8_t direct = 1;
    uint16_t point_cnt = 0;

    lineseries.append(point_cnt, direct);
    point_cnt += 6;
    lineseries.append(point_cnt, direct);
    direct = !direct;

    for(uint8_t i = 0; i< keylen; i++)
    {
        if (ir_keyvalue[i] > 200)
            break;

        lineseries.append(point_cnt, direct);
        point_cnt += ir_keyvalue[i];
        lineseries.append(point_cnt, direct);
        direct = !direct;
    }

    lineseries.append(point_cnt, direct);
    point_cnt += 6;
    lineseries.append(point_cnt, direct);

    QChart *chart = new QChart();
    chart->legend()->hide();
    chart->removeAllSeries();
    chart->addSeries(&lineseries);
    chart->createDefaultAxes();

    ui->lwgraphicsView->setChart(chart);
    ui->lwgraphicsView->setRenderHint(QPainter::Antialiasing);

    analysisWaveData();
}

void LearningWave::output_log(const QString log)
{
    ui->lwlog->append(log);
}

void LearningWave::on_pushButton_clicked()
{
    ui->lwlog->clear();
}
