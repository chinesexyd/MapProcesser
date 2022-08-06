#ifndef IMAGEVIEWER_H
#define IMAGEVIEWER_H

#include "tool.h"

/*@@@@@@@@@@@@@@@@@@@@@@MapShower@@@@@@@@@@@@@@@@@@@@@@@@*/
class MapShower:public QFrame{
    Q_OBJECT
public:
    explicit MapShower(QWidget* parent=nullptr);
    ~MapShower();
    void setSourceData(QByteArray bytes);
    void changeData(QJsonArray array);
    void clearSourceData();
    void saveDataToPGM();//保存数据为pgm图片
protected:
    void paintEvent(QPaintEvent *) override;
    void wheelEvent(QWheelEvent *) override;
    void mousePressEvent(QMouseEvent *) override;
    void mouseMoveEvent(QMouseEvent *) override;
    void resizeEvent(QResizeEvent *) override;
private:
    void _zoomIn();
    void _zoomOut();
    void _changePixmapWHXY();
    void _keepInFrame();
public slots:
    void receiveJsonArrayListSlot(QList<QJsonArray>);
    void saveMapFileSlot(QString);
private:
    struct WH{
        qint32 width;
        qint32 height;
    };
    struct XY{
        qint16 x;
        qint16 y;
    };
    QPixmap _pixmap;
    QByteArray _pixmapData;
    QByteArray _pixmapDataTmp;
    bool _isSetPixmapData;
    double _scalingRatio;//缩放比
    WH _wh,_deertaWh;
    XY _xy,_fakeXy,_mousePressPoint,_zoomCenter;
};
/*@@@@@@@@@@@@@@@@@@@@@@MapDealer@@@@@@@@@@@@@@@@@@@@@@@@*/
class MapDealer:public QFrame{
    Q_OBJECT
public:
    explicit MapDealer(QWidget* parent=nullptr);
public slots:
    void checkSourceMapButtonClickedSlot();
    void saveMapButtonClickedSlot();
private:
    QGridLayout* _gLayout;
    QPushButton* _checkSourceMapButton;
    QLabel* _sourceMapLabel;
    QPushButton* _saveMapButton;
    QFileDialog* _fileDialog;
};
/*@@@@@@@@@@@@@@@@@@@@@@MapViewer@@@@@@@@@@@@@@@@@@@@@@@@*/
class MapViewer:public QFrame{
    Q_OBJECT
public:
    explicit MapViewer(QWidget* parent=nullptr);
public slots:
    void openSourceMapFileSlot(QString filePath);
private:
    QVBoxLayout* _vLayout;
    MapShower* _mapShower;
    MapDealer* _mapDealer;
};
class LoadIndexValueThread;
/*@@@@@@@@@@@@@@@@@@@@@@MapInfoItemModel@@@@@@@@@@@@@@@@@@@@@@@@*/
class MapInfoItemModel:public QAbstractTableModel{
    Q_OBJECT
public:
    struct INFO{
        QFileInfo info;
        bool checkable;
        QJsonArray indexValue;
        qint64 baseVersion;
        qint64 updateTime;
        quint64 orderId;
        QString customerAddress;
        QString macAddress;
        QString areaId;
        bool visable;
        QString direction;
    };
public:
    explicit MapInfoItemModel(QObject* parent=nullptr);
    MapInfoItemModel::INFO fileInfo(const QModelIndex &);
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
protected:
    int rowCount(const QModelIndex &) const override;
    int columnCount(const QModelIndex &) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
private:
    void _modelLoadData();
public slots:
    void receiveData(QString,QJsonObject);
    void checkedItemsChangedSlot();
    void directoryChangedSlot(QString);
    void receiveCheckallableSlot(bool);
signals:
    void checkedItemsChangedSignal();
private:
    LoadIndexValueThread* _loadIndexValueThread;
    QMap<QString,INFO> _infoMap;
    QStringList _hearderData;
    QStringList _infoMapKeys;
    QFileSystemWatcher* _watcher;
};
/*@@@@@@@@@@@@@@@@@@@@@@MapInfoItemDelegate@@@@@@@@@@@@@@@@@@@@@@@@*/
class MapInfoItemDelegate:public QStyledItemDelegate{
    Q_OBJECT
public:
    explicit MapInfoItemDelegate(QObject* parent=nullptr);
protected:
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    bool editorEvent(QEvent *event, QAbstractItemModel *model, const QStyleOptionViewItem &option, const QModelIndex &index) override;
private:
    QPixmap _pixmapChecked,_pixmapUnchecked;
};
/*@@@@@@@@@@@@@@@@@@@@@@MapInfoItemProxyModel@@@@@@@@@@@@@@@@@@@@@@@@*/
class MapInfoItemProxyModel:public QSortFilterProxyModel{
    Q_OBJECT
public:
    explicit MapInfoItemProxyModel(QObject* parent=nullptr);
protected:
    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override;
public slots:
    void sendSearchConditionSlot(TOOL::SEARCH_TYPE,QString);
private:
    bool _checkVersion(MapInfoItemModel::INFO info)const;
    bool _checkWeek(MapInfoItemModel::INFO info)const;
    bool _checkTime(MapInfoItemModel::INFO info)const;
    bool _checkCustomerAddress(MapInfoItemModel::INFO info)const;
    bool _checkMacAddress(MapInfoItemModel::INFO info)const;
    bool _checkArea(MapInfoItemModel::INFO info)const;
    bool _checkDirection(MapInfoItemModel::INFO info)const;
private:
    struct CONDITIONS{
        quint8 VERSION=0;
        quint8 WEEK=1;
        quint8 TIME=2;
        quint8 CUSTOMER=3;
        quint8 MAC=4;
        quint8 AREA=5;
        quint8 DIRECTION=6;
    }_conditions;
    QMap<quint8,QString> _searchConditions;
    QMap<QString,qint8> _timeStringMap;
    QMap<QString,quint8> _chineseWeekMap;
    QMap<QString,quint8> _englisthWeekMap;
};
/*@@@@@@@@@@@@@@@@@@@@@@加载地图差别信息的线程@@@@@@@@@@@@@@@@@@@@@@@@*/
class LoadIndexValueThread:public QThread{
    Q_OBJECT
public:
    explicit LoadIndexValueThread(QObject* parent=nullptr);
    void loadIndexValueFile(QFileInfoList list);
protected:
    void run() override;
signals:
    void emitData(QString,QJsonObject);
private:
    QFileInfoList _infoList;
};
/*@@@@@@@@@@@@@@@@@@@@@@SearchConditionFrame@@@@@@@@@@@@@@@@@@@@@@@@*/
class SearchConditionBox:public QFrame{
    Q_OBJECT
public:
    explicit SearchConditionBox(QWidget* parent=nullptr);
    inline void setTitle(QString title){_titleLabel->setText(title);};
    void setSearchType(TOOL::SEARCH_TYPE searchType);
public slots:
    void currentIndexChangedSlot(int);
    void addSearchConditionSlot(TOOL::SEARCH_TYPE,QString);
private:
    QHBoxLayout* _hLayout;
    QLabel* _titleLabel;
    QComboBox* _comboBox;
    TOOL::SEARCH_TYPE _searchType;
    QStringList _stringList;
};
/*@@@@@@@@@@@@@@@@@@@@@@SearchConditionTimeBox@@@@@@@@@@@@@@@@@@@@@@@@*/
class SearchConditionTimeBox:public QFrame{
    Q_OBJECT
public:
    explicit SearchConditionTimeBox(QWidget* parent=nullptr);
public slots:
    void currentIndexChangedSlot(int);
private:
    QHBoxLayout* _hLayout;
    QLabel* _startLabel;
    QLabel* _endLabel;
    QComboBox* _startComboBox;
    QComboBox* _endComboBox;
};
/*@@@@@@@@@@@@@@@@@@@@@@MapSearcher@@@@@@@@@@@@@@@@@@@@@@@@*/
class MapSearcher:public QFrame{
    Q_OBJECT
public:
    explicit MapSearcher(QWidget* parent=nullptr);
private:
    QGridLayout* _gLayout;
    SearchConditionBox* _areaBox;
    SearchConditionBox* _directionBox;
    SearchConditionBox* _baseVersionBox;
    SearchConditionBox* _updateWeekBox;
    SearchConditionTimeBox* _updateTimeBox;
    SearchConditionBox* _customerAddressBox;
    SearchConditionBox* _macAddressBox;
    QPushButton* _checkAllButton;
    QPushButton* _uncheckAllButton;
};
/*@@@@@@@@@@@@@@@@@@@@@@MapInfoView@@@@@@@@@@@@@@@@@@@@@@@@*/
class MapInfoView:public QTableView{
    Q_OBJECT
public:
    explicit MapInfoView(QWidget* parent=nullptr);
public slots:
    void showMenuSlot(QPoint);
    void deleteActionTriggeredSlot();
private:
    MapInfoItemModel* _infoModel;
    MapInfoItemDelegate* _infoDelegate;
    MapInfoItemProxyModel* _infoProxyModel;
    QMenu* _menu;
    QAction* _deleteAction;
};
/*@@@@@@@@@@@@@@@@@@@@@@MapItemer@@@@@@@@@@@@@@@@@@@@@@@@*/
class MapItemer:public QFrame{
    Q_OBJECT
public:
    explicit MapItemer(QWidget* parent=nullptr);
private:
    QVBoxLayout* _vLayout;
    MapSearcher* _mapSearcher;
    MapInfoView* _infoView;
};
/*@@@@@@@@@@@@@@@@@@@@@@MapDealer@@@@@@@@@@@@@@@@@@@@@@@@*/
class MapHolder:public QSplitter{
    Q_OBJECT
public:
    explicit MapHolder(QWidget* parent=nullptr);
private:
    QSplitter* _splitter;
    MapItemer* _mapItemer;
    MapViewer* _mapViewer;
};
/*@@@@@@@@@@@@@@@@@@@@@@@华为OBS@@@@@@@@@@@@@@@@@@@@@@@*/
class OBS:public QObject{
    Q_OBJECT
public:
    explicit OBS(QObject* parent=nullptr);
    void enumerateObjectsInBucket();//列举桶内对象
    void putObject(QString objectName,QByteArray data);//放置对象
    void getObjectContent(QString objectName);//获取对象内容
    void getObjectMetadata(QString objectName);//获取对象元数据
    void deleteObject(QString objectName);//删除对象
    void batchDeleteObject(QByteArray xmlData);//批量删除对象
private:
    QByteArray _hmacSha1(QByteArray key, QByteArray baseString);
    QString _formatDate();
    QByteArray _createAuthorization(TOOL::SIGNATURE signature);
    void enumerateObjectsInBucketReply(QNetworkReply* reply);
    void putObjectReply(QNetworkReply* reply);
    void getObjectContentReply(QNetworkReply* reply);
    void getObjectMetadataReply(QNetworkReply* reply);
    void deleteObjectReply(QNetworkReply* reply);
    void batchDeleteObjectReply(QNetworkReply* reply);
public slots:
    void finishedSlot(QNetworkReply* reply);
signals:
    void sendObjectsInBucketSignal(QMap<QString,TOOL::OBJECT_INFO>);
    void sendObjectPutedSignal(QString,bool);
    void sendObjectDownloadedSignal(QString,bool);
    void sendObjectDeletedSignal(QString,bool);
    void sendObjectMetadataSignal(QString,bool);
    void sendObjectBatchDeletedSignal(QStringList,QStringList);
private:
    QNetworkAccessManager* _netManager;
    const QString _userName="hw27575955";
    const QString _accessKeyId="VMV7KZXOXCPLEB4J90DY";
    const QString _secretAccessKey="MUg18JONwN5EJHfvGXXIABwaDvN4elhiGJsWPAcA";
    const QString _endPoint="obs.cn-east-3.myhuaweicloud.com";
    const QString _accountID="0aa5cdabd280f5fe0f0fc01c85f7f7a0";//帐号
    const QString _host="ros-map.obs.cn-east-3.myhuaweicloud.com";
    const QString _accountAccessDomainName="https://ros-map.obs.cn-east-3.myhuaweicloud.com";//访问域名
    TOOL::REQUEST_SIGN _requestSign;
    QStringList _batchDeleteObjectNames;
};
extern OBS* obs;
/*@@@@@@@@@@@@@@@@@@@@@@CloudObjectItemModel@@@@@@@@@@@@@@@@@@@@@@@@*/
class CloudObjectItemModel:public QAbstractTableModel{
    Q_OBJECT
public:
    explicit CloudObjectItemModel(QObject* parent=nullptr);
    bool setData(const QModelIndex &index,const QVariant &value,int role=Qt::EditRole)override;
    TOOL::OBJECT_INFO objectInfo(const QModelIndex index);
protected:
    int rowCount(const QModelIndex &) const override;
    int columnCount(const QModelIndex &) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
private:
    QString _size(quint32 size) const;
public slots:
    void receiveObjectsInBucketSlot(QMap<QString,TOOL::OBJECT_INFO>);
    void receiveObjectDownloadedSlot(QString,bool);
    void receiveObjectDeletedSlot(QString,bool);
    void receiveObjectBatchDeletedSlot(QStringList successList,QStringList faidList);
    void batchDeleteSlot();
    void batchDownloadSlot();
    void receiveCloudCheckallableSlot(bool);
private:
    QStringList _hearderData;
    QMap<QString,TOOL::OBJECT_INFO> _objectInfoMap;
    QStringList _infoMapKeys;
};
/*@@@@@@@@@@@@@@@@@@@@@@CloudObjectItemDelegate@@@@@@@@@@@@@@@@@@@@@@@@*/
class CloudObjectItemDelegate:public QStyledItemDelegate{
    Q_OBJECT
public:
    explicit CloudObjectItemDelegate(QObject* parent=nullptr);
protected:
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    bool editorEvent(QEvent *event, QAbstractItemModel *model, const QStyleOptionViewItem &option, const QModelIndex &index) override;
private:
    QPixmap _pixmapChecked,_pixmapUnchecked,_pixmapUndownload,_pixmapDownloaded,_pixmapDownloading,_pixmapUndelete,_pixmapDeleting;
};
/*@@@@@@@@@@@@@@@@@@@@@@CloudObjectProxyModel@@@@@@@@@@@@@@@@@@@@@@@@*/
class CloudObjectProxyModel:public QSortFilterProxyModel{
    Q_OBJECT
public:
    explicit CloudObjectProxyModel(QObject* parent=nullptr);
protected:
    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override;
private:
    bool _checkWeek(TOOL::OBJECT_INFO info)const;
    bool _checkTime(TOOL::OBJECT_INFO info)const;
    bool _checkCustomerAddress(TOOL::OBJECT_INFO info)const;
    bool _checkArea(TOOL::OBJECT_INFO info)const;
public slots:
    void receiveCloudSearchConditionSlot(TOOL::CLOUD_SEARCH_TYPE type,QString str);
private:
    struct CONDITIONS{
        quint8 WEEK=1;
        quint8 TIME=2;
        quint8 CUSTOMER=3;
        quint8 AREA=5;
    }_conditions;
    QMap<quint8,QString> _searchConditions;
    QMap<QString,qint8> _timeStringMap;
    QMap<QString,quint8> _chineseWeekMap;
    QMap<QString,quint8> _englisthWeekMap;
};
/*@@@@@@@@@@@@@@@@@@@@@@CloudObjectItemView@@@@@@@@@@@@@@@@@@@@@@@@*/
class CloudObjectItemView:public QTableView{
    Q_OBJECT
public:
    explicit CloudObjectItemView(QWidget* parent=nullptr);
protected:
    void resizeEvent(QResizeEvent*);
private:
    void _resizeColoumWidth();
public slots:
    void batchDealSlot(bool);
    void deleteColumnSlot(bool);
private:
    CloudObjectItemModel* _objectModel;
    CloudObjectItemDelegate* _objectDelegate;
    CloudObjectProxyModel* _objectProxyModel;
};
/*@@@@@@@@@@@@@@@@@@@@@@CloudDealer@@@@@@@@@@@@@@@@@@@@@@@@*/
class CloudDealer:public QFrame{
    Q_OBJECT
public:
    explicit CloudDealer(QWidget* parent=nullptr);
public slots:
    void batchDealButtonClickedSlot();
    void deleteButtonClickedSlot();
private:
    QPushButton* _refreshBuketButton;
    QPushButton* _batchDealButton;
    QPushButton* _batchDownloadButton;
    QPushButton* _batchDeleteButton;
    QPushButton* _deleteButton;
    QPushButton* _checkallButton;
    QPushButton* _uncheckallButton;
    QGridLayout* _layout;
};
/*@@@@@@@@@@@@@@@@@@@@@@CloudSearchConditionBox@@@@@@@@@@@@@@@@@@@@@@@@*/
class CloudSearchConditionBox:public QFrame{
    Q_OBJECT
public:
    explicit CloudSearchConditionBox(QWidget* parent=nullptr);
    inline void setTitle(QString title){_titleLabel->setText(title);};
    void setSearchType(TOOL::CLOUD_SEARCH_TYPE searchType);
public slots:
    void currentIndexChangedSlot(int);
    void addCloudSearchConditionSlot(TOOL::CLOUD_SEARCH_TYPE,QString);
private:
    QHBoxLayout* _hLayout;
    QLabel* _titleLabel;
    QComboBox* _comboBox;
    TOOL::CLOUD_SEARCH_TYPE _searchType;
    QStringList _stringList;
};
/*@@@@@@@@@@@@@@@@@@@@@@CloudSearchConditionTimeBox@@@@@@@@@@@@@@@@@@@@@@@@*/
class CloudSearchConditionTimeBox:public QFrame{
    Q_OBJECT
public:
    explicit CloudSearchConditionTimeBox(QWidget* parent=nullptr);
public slots:
    void currentIndexChangedSlot(int);
private:
    QHBoxLayout* _hLayout;
    QLabel* _startLabel;
    QLabel* _endLabel;
    QComboBox* _startComboBox;
    QComboBox* _endComboBox;
};
/*@@@@@@@@@@@@@@@@@@@@@@CloudSearcher@@@@@@@@@@@@@@@@@@@@@@@@*/
class CloudSearcher:public QFrame{
    Q_OBJECT
public:
    explicit CloudSearcher(QWidget* parent=nullptr);
private:
    CloudSearchConditionBox* _areaIDBox;
    CloudSearchConditionTimeBox* _timeBox;
    CloudSearchConditionBox* _weekBox;
    CloudSearchConditionBox* _customerAddressBox;
    QVBoxLayout* _vLayout;
};
/*@@@@@@@@@@@@@@@@@@@@@@MapLoader@@@@@@@@@@@@@@@@@@@@@@@@*/
class MapLoader:public QFrame{
    Q_OBJECT
public:
    explicit MapLoader(QWidget* parent=nullptr);
private:
    CloudDealer* _cloudDealer;
    CloudSearcher* _cloudSearcher;
    QFrame* _spaceFrame;
    CloudObjectItemView* _objectView;
    QGridLayout* _layout;
};
/*@@@@@@@@@@@@@@@@@@@@@@MapUpdater@@@@@@@@@@@@@@@@@@@@@@@@*/
class MapUpdater:public QTabWidget{
    Q_OBJECT
public:
    explicit MapUpdater(QWidget* parent=nullptr);
private:
    MapHolder* _holder;
    MapLoader* _loader;
};

#endif // IMAGEVIEWER_H
