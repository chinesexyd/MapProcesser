#ifndef TOOL_H
#define TOOL_H

#include "root.h"
/*@@@@@@@@@@@@@@@@@@@@@@TOOL@@@@@@@@@@@@@@@@@@@@@@@@*/
class TOOL{
public:
    static QString StyleSheet(QString path);
    static void CreateDirOrFile();
//    static QStringList GetMapFiles();
public:
    enum SEARCH_TYPE{
        BASE_VERSION=0,     //基础版本
        UPDATE_WEEK=1,      //周
        UPDATE_TIME=2,      //时刻
        CUSTOMER_ADDRESS=3, //顾客楼栋
        MAC_ADDRESS=4,      //小车MAC地址
        AREA_ID=5,        //小区货仓名
        DIRECTION=6         //方向
    };
    struct SIGNATURE{
        QString httpVerb="";
        QString contentType="";
        QString contentMD5="";
        QString date="";
        QString canonicalizedHeaders="";
        QString canonicalizedResource="";
    };//用于制造密钥
    struct REQUEST_SIGN{
        const QByteArray putObject="A";
        const QByteArray getObjectContent="B";
        const QByteArray getObjectMetadata="C";
        const QByteArray deleteObject="D";
        const QByteArray batchDeleteObject="E";;
        const QByteArray enumerateObjectsInBucket="F";
    };//用于判断返回数据是哪个请求请求到的
    enum DOWNLOAD_STATUS{
        UNDOWNLOAD,
        DOWNLOADING,
        DOWNLOADED,
    };//下载状态
    enum DELETE_STATUS{
        UNDELETE,
        DELETING,
    };//删除状态
    struct OBJECT_INFO{
        bool checkable=false;
        QString key;
        QDateTime lastModified;
        QString etag;
        quint32 size;
        DOWNLOAD_STATUS downloadstatus=UNDOWNLOAD;
        DELETE_STATUS deleteStatus=UNDELETE;
        qint64 updateTime;
        QString customerAddress;
        QString areaId;
        bool visable;
    };//华为云桶内数据包含内容
    enum CLOUD_SEARCH_TYPE{
        CLOUD_UPDATE_WEEK=1,      //周
        CLOUD_UPDATE_TIME=2,      //时刻
        CLOUD_CUSTOMER_ADDRESS=3, //顾客楼栋
        CLOUD_AREA_ID=5         //小区货仓名
    };
    ///////////////////////////////
    struct XYA_POINT{
        double x;
        double y;
        double angle;
    };
    struct XYZW_POINT{
        double x;
        double y;
        double z;
        double w;
    };
    //////////////////////////////
    struct ROBOT_INFO{
        QString areaName;//小区名
        QString robotName;//机器人名
        QString robotID;
        quint8 charge;//是否在充电
        bool collision;//碰撞是否异常
        double cur;//电流
        bool doorState;//门的状态
        bool emergency;//急停按钮是否触发
        bool imu;//陀螺仪是否异常
        quint8 mah;//电池电量
        QString mapFile;//地图路径名
        QString mcuHwVersion;//下位机版本
        QString mcuSoftVersion;//下位机软件版本
        quint64 mils;//里程数
        int posBusy;
        int posD;
        int poseC;
        quint8 relay;//继电器状态 0关机 1正常 2充电
        QString rosHwVersion;//ros下位机版本
        QString rosSoftVersion;//ros软件版本
        quint8 skip;//环境匹配度 0～100 值越大越不匹配
        bool sonar;//超声波是否异常
        int ticks;
        double vol;//电压
        bool wheel;//轮子是否异常
        XYA_POINT point;//位置
        XYZW_POINT xyzwPoint;
        double yaw;//角度
    };
    struct PGM_YAML{
        QString pgm;
        QString yaml;
    };
    struct MAP_INFO{
        double originX;
        double originY;
        double resolution;
        QRectF rect;
    };
    struct PATH_INFO{
        QRect pixmapRect;
        QJsonObject linesObj;
        QJsonObject buildsObj;
        QString mapPath;
        double originX;
        double originY;
        double resolution;
        QString rootDirPath;
        QString linesPath;
        QString buildsPath;
        QString yamlPath;
    };
};
/*@@@@@@@@@@@@@@@@@@@@@@GLOBAL_@@@@@@@@@@@@@@@@@@@@@@@@*/
class GLOBAL_{
public:
    GLOBAL_();
public:
    QStringList WeekStringList;
    QStringList TimeStringList;
    QStringList BaseVersionList;
    QStringList CustomerAddressList;
    QStringList MacAddressList;
    QStringList AreaStringList;
    QStringList DirectionStringList;

    QStringList CloudCustomerAddressList;
    QStringList CloudAreaIDList;
    QStringList CloudWeekList;
    QStringList CloudTimeList;
};
extern GLOBAL_* GLOBAL;
/*@@@@@@@@@@@@@@@@@@@@@@SIGNALS_@@@@@@@@@@@@@@@@@@@@@@@@*/
class SIGNALS_:public QObject{
    Q_OBJECT
public:
    explicit SIGNALS_(QObject* parent=nullptr);
signals:
    void sendSearchConditionSignal(TOOL::SEARCH_TYPE,QString);//发送查找条件
    void addSearchConditionSignal(TOOL::SEARCH_TYPE,QString);
    void openSourceMapFileSignal(QString);
    void sendJsonArrayListSignal(QList<QJsonArray>);
    void saveMapFileSignal(QString);

    void sendCloudSearchConditionSignal(TOOL::CLOUD_SEARCH_TYPE,QString);
    void addCloudSearchConditionSignal(TOOL::CLOUD_SEARCH_TYPE,QString);

    void batchDealSignal(bool);
    void deleteColumnSignal(bool);
    void batchDeleteSignal();
    void batchDownloadSignal();

    void sendCheckallableSignal(bool);
    void sendCloudCheckallableSignal(bool);

    void resetLineModelSignal();

    void sendStatusBarStringSignal(QString);
    void resetMapSignal();
    void resetBuildModelSignal();

    void sendPGMYAMLAndCurrentrobotIDSignal(TOOL::PGM_YAML,QString);
};
extern SIGNALS_* SIGNALS;
#endif // TOOL_H
