#include "upgradethread.h"
#include "protocol.h"
#include <QSettings>
#include <QProcess>
#define SIR_TOOL_NAME  "Smart_IR.exe"
extern bool check_valid_upgrade_bin_version(QString fileName,uint32_t &version, uint32_t &checkSum);
static bool inihasChecked = 0;
UpgradeThread::UpgradeThread()
{


}
void UpgradeThread::run()
{
    download_manifestini();
    checkToolVersion();

    while(!isSerialReady)
    {
        QThread::usleep(1500); // delay 2s
    }
    emit getVersionSignal();

    download_mcuBin();

}
void UpgradeThread::download_mcuBin()
{
    QString appPath = qApp->applicationDirPath();
    QString fileDir = appPath.remove("/debug").remove("/release").append("/IR_stm32f103C8.bin");

    qDebug() << fileDir;
    QFile binFile(fileDir);
    if(binFile.exists())
    {
        qDebug() << "IR_stm32f103C8.bin exsit,delete it first";
        QFile::remove(fileDir);
    }

    SHELLEXECUTEINFO  ShExecInfo;
     ShExecInfo.cbSize  =   sizeof(SHELLEXECUTEINFO);
     ShExecInfo.fMask   =  SEE_MASK_NOCLOSEPROCESS;
     ShExecInfo.hwnd  =   NULL;
     ShExecInfo.lpVerb  =    NULL;
     ShExecInfo.lpFile   =   L"wget.exe";
     ShExecInfo.lpParameters  =  L"https://github.com/barrycool/bin/raw/master/IR_MCU_upgrade_bin/IR_stm32f103C8.bin";
     ShExecInfo.lpDirectory   =  NULL;
     ShExecInfo.nShow   =   SW_HIDE;
     ShExecInfo.hInstApp  =  NULL;
     ShellExecuteEx(&ShExecInfo);

     //ShellExecute(NULL, L"open", L"wget.exe", NULL, L"D://02_wind//main", SW_SHOW);
     WaitForSingleObject(ShExecInfo.hProcess,INFINITE);

    //emit disableFreshVersion(false);
    //QThread::sleep(5);

    emit finish(true);
}

void UpgradeThread::download_manifestini()
{
    QString appPath = qApp->applicationDirPath();
    QString dstfilepath = appPath.remove("/debug").remove("/release").append("/manifest.ini");
    QFile iniFile(dstfilepath);
    if(iniFile.exists())
    {
        qDebug() << "manifest.ini exsit,delete it first";
        QFile::remove(dstfilepath);
    }
    SHELLEXECUTEINFO  ShExecInfo;
     ShExecInfo.cbSize  =   sizeof(SHELLEXECUTEINFO);
     ShExecInfo.fMask   =  SEE_MASK_NOCLOSEPROCESS;
     ShExecInfo.hwnd  =   NULL;
     ShExecInfo.lpVerb  =    NULL;
     ShExecInfo.lpFile   =   L"wget.exe";
     ShExecInfo.lpParameters  =  L"https://github.com/barrycool/bin/raw/master/MainTool/manifest.ini";
     ShExecInfo.lpDirectory   =  NULL;
     ShExecInfo.nShow   =   SW_HIDE;
     ShExecInfo.hInstApp  =  NULL;
     ShellExecuteEx(&ShExecInfo);

     //ShellExecute(NULL, L"open", L"wget.exe", NULL, L"D://02_wind//main", SW_SHOW);
     WaitForSingleObject(ShExecInfo.hProcess,INFINITE);
     //emit downloadIniFinish();
}
void UpgradeThread::download_mainTool()
{
    QString appPath = qApp->applicationDirPath();
    QString dstfilepath = appPath.remove("/debug").remove("/release").append("/Download_files").append("/Smart_IR.exe");
    QFile iniFile(dstfilepath);
    if(iniFile.exists())
    {
        qDebug() << "Smart_IR.exe exsit,delete it first";
        QFile::remove(dstfilepath);
    }

    //std::string str = dstDirectoryPath.toStdString(); //将QString转换为string
    //std::wstring wstr(str.length(), L' ');　　//初始化宽字节wstr
    //std::copy(str.begin(), str.end(), wstr.begin());　　//将str复制到wstr
    //LPCWSTR path = wstr.c_str();　　//将wstr转换为C字符串的指针,然后赋值给path
    //LPCWSTR str = TEXT("dstDirectoryPath");
    QString cmd = "-N -P Download_files  https://github.com/barrycool/bin/raw/master/MainTool/Smart_IR.exe";
    const wchar_t * wgetpath = reinterpret_cast<const wchar_t *>(cmd.utf16());//char * 转换为 wchar_t * 类型

    SHELLEXECUTEINFO  ShExecInfo;
     ShExecInfo.cbSize  =   sizeof(SHELLEXECUTEINFO);
     ShExecInfo.fMask   =  SEE_MASK_NOCLOSEPROCESS;
     ShExecInfo.hwnd  =   NULL;
     ShExecInfo.lpVerb  =    NULL;
     ShExecInfo.lpFile   =  L"wget.exe";
     ShExecInfo.lpParameters  =  wgetpath;
     ShExecInfo.lpDirectory   =    NULL;
     ShExecInfo.nShow   =   SW_HIDE;
     ShExecInfo.hInstApp  =  NULL;
     ShellExecuteEx(&ShExecInfo);

     //ShellExecute(NULL, L"open", L"wget.exe", NULL, L"D://02_wind//main", SW_SHOW);
     WaitForSingleObject(ShExecInfo.hProcess,INFINITE);
}

void UpgradeThread::checkToolVersion()
{
    //output_log("latest tool download finished!",1);
    if(inihasChecked == 1)
    {
        return;
    }
    qDebug() <<"checkToolVersion";
/* CRC信息和版本信息存储在另外的ini配置文件中，只需下载ini配置文件，判断有新版本后再下载tool*/
    uint32_t xnewVersion;
    uint32_t dnewVersion;
    uint32_t checksum;
    QString appPath = qApp->applicationDirPath();
    QString szFileName = appPath.remove("/debug").remove("/release").append("/manifest.ini");
    QFile file(szFileName);
    if(!file.exists())
    {
        qDebug()<< szFileName  << "  not exsit!";
        return;
    }

    //step1: 读ini文件里的version 信息
    QSettings *configIniRead = new QSettings(szFileName, QSettings::IniFormat);
    //将读取到的ini文件保存在QString中，先取值，然后通过toString()函数转换成QString类型
    QString version = configIniRead->value("/upgrade/version").toString();
    QString filename = configIniRead->value("filename").toString();

    //打印得到的结果
    int newVersion = version.toInt();
    qDebug() << filename << " : " << version << " : " << newVersion;
    inihasChecked = 1;

    //step2:判断版本是否比本地更新
    if(filename == SIR_TOOL_NAME && newVersion > VERSION)
    {
        //step3: 若较新，询问客户是都要升级，若是则下载相应的tool

        version.replace("0","");
        version.insert(1,".");
        //QString logstr ="New version: " + version +" is available,Do you want to Download and Upgrade?";
        //QMessageBox::StandardButton reply = QMessageBox::question(this, "New Version Available ", logstr, QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
        //if(reply == QMessageBox::Yes)
        //{
            download_mainTool();
        //}
        //else
        //{
            //return;
        //}

        appPath = qApp->applicationDirPath();
        szFileName = appPath.remove("/debug").remove("/release").append("/Download_files/").append(SIR_TOOL_NAME);

        //step4：下载tool完成后，判断tool的checksum和Version，若OK，则升级
        if(check_valid_upgrade_bin_version(szFileName,dnewVersion,checksum))
        {
            qDebug() << "dnewVersion : " << dnewVersion;
            QString tmp = QString::number(dnewVersion,16); //10进制转成16进制
            xnewVersion = tmp.toInt();
            qDebug() << "xnewVersion : " << xnewVersion;
            if(xnewVersion == newVersion && newVersion> VERSION)
            {
                emit maintoolNeedUpdate(version);
            }
            else
            {
                qDebug() << "version info is not matched!";
            }
        }
        else
        {
            qDebug() << "Downloaded Smart_IR.exe fail or file is corrupted!";
        }
    }
    else
    {
        qDebug() << "no need to upgrade tool ";
    }
    //upgradethread->exit();
    //upgradethread->quit();
    return;
/*通过QT在exe中嵌入版本信息，下载tool后读取tool的的版本，没有CRC校验
 *  DWORD dwSize = 0;
    char* lpData = NULL;
    BOOL bSuccess = FALSE;
        // #pragma comment(lib,"Version.lib")
    QString VerisonInfomation;
    DWORD dwHandle;
     qDebug() <<"checkToolVersion:" <<szFileName;
    //获得文件基础信息
    //--------------------------------------------------------
    dwSize = GetFileVersionInfoSize(szFileName.toStdWString().c_str(), &dwHandle);
    if (0 == dwSize)
    {
        qDebug()<<"Get GetFileVersionInfoSize error!";
        return;
    }
    lpData = new char[dwSize];

    bSuccess = GetFileVersionInfo(szFileName.toStdWString().c_str(), dwHandle, dwSize, lpData);
    if (!bSuccess)
    {
        qDebug()<<"Get GetFileVersionInfo error!";
        delete []lpData;
        return;
    }

    //获得语言和代码页(language and code page)  //default as 080404b0
    //VerQueryValue(pBlock,TEXT("\\VarFileInfo\\Translation"),&lpBuffer,&uLen;

    //获得文件版本信息
    //-----------------------------------------------------
    LPVOID lpBuffer = NULL;
    UINT uLen = 0;
    bSuccess = VerQueryValue(lpData,TEXT("\\StringFileInfo\\080404b0\\FileVersion"),&lpBuffer,&uLen);
    if (!bSuccess)
    {
        qDebug()<<"Get FileVersion error!";
        delete []lpData;
        return;
    }
    VerisonInfomation = QString::fromUtf16((const unsigned short int *)lpBuffer);
    qDebug()<<"FileVersion:" << VerisonInfomation;
    if(VerisonInfomation.toInt() > VERSION)
    {
        QString logstr = "Newer Version of SmartIR tool is available,Do you want to upgrade?";
        QMessageBox::StandardButton reply = QMessageBox::question(this, "New Version Available ", logstr, QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
        if(reply == QMessageBox::Yes)
        {
            QProcess process(this);
            process.startDetached("Updater.exe");
            this->close();
        }
    }
    else
    {
        output_log("no need to upgrade smartir tool",0);
    }
*/
}
void UpgradeThread::serialSetReady(bool isReady)
{
    isSerialReady = isReady;
}
