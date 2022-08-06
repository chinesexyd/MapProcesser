#include "tool.h"

/*@@@@@@@@@@@@@@@@@@@@@@TOOL@@@@@@@@@@@@@@@@@@@@@@@@*/
QString TOOL::StyleSheet(QString path){
    QFile file(path);
    file.open(QFile::ReadOnly);
    QString styleSheet = QLatin1String(file.readAll());
    file.close();
    return styleSheet;
}
void TOOL::CreateDirOrFile(){
    QDir indexValueDir(INDEX_VALUE_FILES_DIR);
    QDir mapFilesDir(MAP_FILES_DIR);
    QDir saveMapsDir(SAVE_MAPS_DIR);
    QFile configFile(CONFIG_FILE);
    if(!indexValueDir.exists()){
        indexValueDir.mkpath(INDEX_VALUE_FILES_DIR);
    }
    if(!mapFilesDir.exists()){
        mapFilesDir.mkpath(MAP_FILES_DIR);
    }
    if(!saveMapsDir.exists()){
        saveMapsDir.mkpath(SAVE_MAPS_DIR);
    }
    if(!configFile.exists()){
        configFile.open(QIODevice::WriteOnly);
        configFile.close();
    }
}
//QStringList TOOL::GetMapFiles(){
//    QJsonDocument doc;
//    QJsonArray a;

//}
/*@@@@@@@@@@@@@@@@@@@@@@GLOBAL_@@@@@@@@@@@@@@@@@@@@@@@@*/
GLOBAL_::GLOBAL_(){
    WeekStringList<<"不限制"<<"星期一"<<"星期二"<<"星期三"<<"星期四"<<"星期五"<<"星期六"<<"星期日"<<"星期一～星期五";
    TimeStringList<<"不限制"<<"00:00"<<"01:00"<<"02:00"<<"03::00"<<"04:00"<<"05:00"<<"06:00"<<"07:00"<<"08:00"<<"09:00"<<"10:00"<<"11:00"<<"12:00"<<"13:00"<<"14:00"<<"15:00"<<"16:00"<<"17:00"<<"18:00"<<"19:00"<<"20:00"<<"21:00"<<"22:00"<<"23:00"<<"24:00";
    BaseVersionList<<"不限制";
    CustomerAddressList<<"不限制";
    MacAddressList<<"不限制";
    AreaStringList<<"不限制";
    DirectionStringList<<"不限制"<<"GO"<<"BACK";

    CloudCustomerAddressList<<"不限制";
    CloudAreaIDList<<"不限制";
    CloudWeekList<<"不限制"<<"星期一"<<"星期二"<<"星期三"<<"星期四"<<"星期五"<<"星期六"<<"星期日"<<"星期一～星期五";
    CloudTimeList<<"不限制"<<"00:00"<<"01:00"<<"02:00"<<"03::00"<<"04:00"<<"05:00"<<"06:00"<<"07:00"<<"08:00"<<"09:00"<<"10:00"<<"11:00"<<"12:00"<<"13:00"<<"14:00"<<"15:00"<<"16:00"<<"17:00"<<"18:00"<<"19:00"<<"20:00"<<"21:00"<<"22:00"<<"23:00"<<"24:00";
}
GLOBAL_* GLOBAL=new GLOBAL_();
/*@@@@@@@@@@@@@@@@@@@@@@SIGNALS_@@@@@@@@@@@@@@@@@@@@@@@@*/
SIGNALS_::SIGNALS_(QObject*parent):QObject(parent){}
SIGNALS_* SIGNALS=new SIGNALS_();
