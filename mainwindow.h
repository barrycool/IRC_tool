#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QDialog>
#include <QMainWindow>
#include <QMessageBox>
#include <aboutirc.h>
#include <learningwave.h>
#include <upgradedialog.h>
#include <ir_learning.h>
#include <sirlistwidget.h>
//#include "upgrade.h"

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
#include <QSettings>
#include <windows.h>
#include "upgradethread.h"
#include <QTcpSocket>
#include<windows.h>
#include<winver.h>

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
struct at_panel_Button_t {
  QPushButton *buttonWidget;
  bool isEnable;
  bool canbeCustomized;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    //QList ir_slot_List;
   void sendwificmd(QString cmd);
   void startWithDebugMode(QString protocalFile,QString scriptFile,int loopcnt);

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
    void portChanged(int);
    /*-------lianlian add for Upgrade----------------*/
    //void upDownloadButton_slot();
    void upCancelButton_slot();
    //void openUrl_slot(QString);
    void getCurrentMcuVersion();
    void httpDowloadFinished(bool flag);
    /*-------lianlian add for Learningkey----------------*/
    void leIrPanel_slot();
    void leStartRecordButton_slot();
    void leAddToListButton_slot();
    void leRemoveFromListButton_slot();
    void leSaveKeymapButton_slot();
    void leSaveReWriteButton_slot();
    void leClearButton_slot();
    void leRealTimeTestButton_slot();
    void returnfromLearningWave();
    void analysisFinshed(QString);
    void leSetIRDevice(int index);
    void leLoadKeyMap();
    //void leReadyReadSlot();
    /*---------lianlian add for AgingTest---------------*/
    void atCustomizeKeyListWidgetItemClicked_slot(QListWidgetItem* );
    void atScriptlistWidgetClicked_slot(QListWidgetItem* item);
    void atIrPanel_slot();
    void atAddButton_slot();
    void atRemoveButton_slot();
    void atLoadscriptBut_slot();
    void atSaveButton_slot();
    void atClearScriptWidget();
    void atDownloadButton_slot();
    void atStartButton_slot();
    //void atLoadKeyMapButton_slot();
    void atRealTimeSendButton_slot();
    void atReadPushButton_slot();
     /*---------lianlian add for MCU upgrade---------------*/
    //void ReadMcuVersionSlot();
    void sendcmdTimeout();
    //void checkForMcuUpgrade(); //lialian add for upgrade
    void sendCmdforUpgradeSlot(uint8_t *,int);

    void on_actionIRWave_triggered();

    void on_actionUser_Manual_triggered();

    void on_actionLog_triggered();
    void logSaveAsButton_slot();
    void logSaveButton_slot();
    void logClearButton_slot();
    void textChanged_SLOT(const QString &text);
    void returnfromUpgrade(bool,uint32_t);
    void upgradedialog_reject();

    void on_atUpMove_clicked();

    void on_atDownMove_clicked();

    void on_actionDown_Binary_triggered();

    void on_actionDownload_MainTool_triggered();

    void on_actionDownload_UserManual_triggered();

    void on_actionDownload_SerialDriver_triggered();
    void on_itemClicked(QListWidgetItem * item);
    void on_itemDoubleClicked(QListWidgetItem * item);
    void on_actionWifiSetting_triggered();
    void on_wifi_setting();
    void on_tcp_connect_state(QAbstractSocket::SocketState state);
    void dropSlotforScriptlw(QString,int);
    void update_IR_items_List();
    void click_timer_timeout();
    void dragLeaveEventSlot(int);

    void on_actionTCP_mode_triggered(bool checked);

    void on_actionUSB_mode_triggered(bool checked);
    //void checkToolVersion();
    void maintoolNeedUpdate_slot(QString);

    void on_atReturnToList_clicked();

    void on_atTurnToPanel_clicked();

signals:
    //void sendsignal();
    void send2learningwave();
    void updateVersionSignal(uint32_t,uint32_t);
    void receiveAckSignal(int);
    void cmdFailSignal();
    void enableFreshVersion(bool);

private:
    Ui::MainWindow *ui;
    QSerialPort serial;
    QTcpSocket socket;
    QComboBox *portBox;
    struct portSetting_t portSetting;
    ComPort_Setting *fport;
    bool isAutotestState;
    //bool isLearingkeyState;
    void ir_button_Slot_connect();
    //QList <IR_item_t> IR_items;
    //QList <IR_map_t> IR_maps;
    void add_to_list(QString,uint32_t);
    QList <IR_map_t> learningkey_maps;
    void setToKeyListWidget(QString);
    //void saveToIrMaps(QString);
    QTimer click_timer;
    int cmd_index;
    void output_log(QString,int);
    //int currentLoopIndex;
    //int totalLoopCnt;

    void clear_cmd_list_handle();
    void set_cmd_list_handle();
    QString keyMapDirPath;

    void sendCmd2MCU(uint8_t *,uint8_t);
    void sendAck(uint8_t seq_num, uint8_t msg_id);
    QTimer sendcmd_timer;
    void resendBackupCmd();
    //QSemaphore *cmdSemaphore;

    /*---------lianlian add for MCU upgrade---------------*/
    LearningWave *lw;
    UpgradeDialog *fupdiaglog;
    UpgradeThread *upgradethread;
    uint32_t currentMcuVersion;
    uint32_t availableMcuVersion;
    void printIrItemInfo(IR_item_t ir_item);

    void set_IR_protocol();
    void loadInsetIrMapTable();
    void set_IR_device(int index);
    void set_IR_command_list();
    void atAddItem2ScriptListWidget(int ir_type,QString button_name,int delaytime);
    void atClearScriptWidgetOnly();

    QSettings *settings;
    QAction *SerialPortListQAction;
    QList <at_panel_Button_t> atbuttonList;
    void fresh_atIrPanel();
    void atButtonList_init();
};
/*
struct at_panel_Button_t at_panel_Buttons[64]= {
    {.isEnable = 1, .isCustomized =0},
};
*/
#endif // MAINWINDOW_H
