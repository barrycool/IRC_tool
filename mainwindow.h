#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QDialog>
#include <QMainWindow>
#include <QMessageBox>
#include <aboutirc.h>
#include <learningwave.h>
#include <upgradedialog.h>
#include <ir_learning.h>

#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>
#include <QToolBar>
#include <QWidget>
#include <QComboBox>
#include "qtextstream.h"
#include "Qtextstream"
#include <comport_setting.h>
#include <protocol.h>
#include <crc32.h>
#include <QHBoxLayout>
#include <QCheckBox>
#include <QSize>
#include <QTimer>
#include <QTime>
#include <QListWidgetItem>
#include <QListWidget>
#include <QDesktopServices>
//#include <QSemaphore>
#include <QThread>

namespace Ui {
class MainWindow;
}

//typedef void (MainWindow::*slot_cb_fct)();
/*
enum panel_button_e {
    BUTTON_POWER,
    BUTTON_EJECT,
    BUTTON_ENTER,
    BUTTON_MAX,
};


struct ir_button_slot_t {
  enum panel_button_e butIndex;
  QWidget *buttonWidget;
  QString buttonName;
  slot_cb_fct buttonSlot;
};
*/

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    //QList ir_slot_List;

private slots:
    void on_actionAbout_IRC_triggered();

    void on_actionOpenUart_triggered();

    void on_actionFresh_triggered();

    void on_actionUpgrade_triggered();

    void on_actionLearningKey_triggered();

    //void on_actionLoadKeyMap_triggered();  // no use anymore

    void on_actionPort_Setting_triggered();

    void returnPortSetting();

    void on_actionAgingTest_triggered();
    void serial_receive_data();
    /*-------lianlian add for Upgrade----------------*/
    //void upDownloadButton_slot();
    void upCancelButton_slot();
    //void openUrl_slot(QString);
    void getCurrentMcuVersion();
    /*-------lianlian add for Learningkey----------------*/
    void leCustomizeButton_slot();
    void leReturnButton_slot();
    void leIrPanel_slot();
    void leStartRecordButton_slot();
    void leAddToListButton_slot();
    void leRemoveFromListButton_slot();
    void leSaveKeymapButton_slot();
    void leClearButton_slot();
    void leRealTimeTestButton_slot();
    void returnfromLearningWave();
    void analysisFinshed(QString);
    //void leReadyReadSlot();
    /*---------lianlian add for AgingTest---------------*/
    void atCustomizeKeyListWidgetItemClicked_slot(QListWidgetItem* );
    void atIrPanel_slot();
    void atCustomizeButton_slot();
    void atReturnButton_slot();
    void atAddButton_slot();
    void atRemoveButton_slot();
    void atLoadscriptBut_slot();
    void atSaveButton_slot();
    void atClear_slot();
    void atRunButton_slot();
    void atStop_slot();
    void atLoadKeyMapButton_slot();
    void atRealTimeSendButton_slot();
     /*---------lianlian add for MCU upgrade---------------*/
    //void ReadMcuVersionSlot();
    void sendcmdTimeout();
    //void checkForMcuUpgrade(); //lialian add for upgrade
    void sendCmdforUpgradeSlot(char *,int);

    void on_actionIRWave_triggered();

signals:
    //void sendsignal();
    void send2learningwave();
    void updateVersionSignal(IR_MCU_Version_t *);
    void receiveAckSignal();
    void receiveFinshSignal();
    void cmdFailSignal();

private:
    Ui::MainWindow *ui;
    QSerialPort serial;
    QComboBox *portBox;
    struct portSetting_t portSetting;
    ComPort_Setting *fport;
    bool isAutotestState;
    //bool isLearingkeyState;
    void ir_button_List_init();
    void ir_button_Slot_connect();
    QList <IR_item_t> IR_items;
    QList <IR_map_t> IR_maps;
    void add_to_list(QString,uint32_t);
    QList <IR_map_t> learningkey_maps;
    void setToKeyListWidget(QString);
    void saveToIrMaps(QString);
    QTimer set_cmd_list_timer;
    int cmd_index;
    void output_log(QString);
    //int currentLoopIndex;
    //int totalLoopCnt;

    void clear_cmd_list_handle();
    void set_cmd_list_handle();
    QString keyMapDirPath;

    void sendCmd2MCU(char *,uint8_t);
    void sendAck(void *);
    QTimer sendcmd_timer;
    void resendBackupCmd();
    //QSemaphore *cmdSemaphore;

    /*---------lianlian add for MCU upgrade---------------*/

    //void getCurrentMcuVersion();
    //void startUpgrade();
    //void sendUpgradePacket();
    //int currentMcuVersionYear;
    //int currentMcuVersionMonth;
    //int currentMcuVersionDay;
    //QList<char *> upgradePacketList;
    //QLabel *upWebLinklable;
    //void startUpgradeThread();
    //void handleResults(const QString);

    LearningWave *lw;
    UpgradeDialog *fupdiaglog;
    void printIrItemInfo(IR_item_t ir_item);

};

#endif // MAINWINDOW_H
