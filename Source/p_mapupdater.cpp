#include "p_mapupdater.h"

/*@@@@@@@@@@@@@@@@@@@@@@MapShower@@@@@@@@@@@@@@@@@@@@@@@@*/
////////public
MapShower::MapShower(QWidget* parent):QFrame(parent){
    setObjectName("MapShower");
    setContextMenuPolicy(Qt::ActionsContextMenu);
    connect(SIGNALS,SIGNAL(sendJsonArrayListSignal(QList<QJsonArray>)),this,SLOT(receiveJsonArrayListSlot(QList<QJsonArray>)));
    connect(SIGNALS,SIGNAL(saveMapFileSignal(QString)),this,SLOT(saveMapFileSlot(QString)));
}
MapShower::~MapShower(){}
void MapShower::setSourceData(QByteArray bytes){
    _pixmapData=_pixmapDataTmp=bytes;
    _isSetPixmapData=true;
    _pixmap.loadFromData(_pixmapData);
    double wr=width()/(float)_pixmap.width();
    double hr=height()/(float)_pixmap.height();
    _scalingRatio=(wr<=hr)?wr:hr;
    _zoomCenter.x=width()/2;
    _zoomCenter.y=height()/2;
    _deertaWh.width=_pixmap.width()/2;
    _deertaWh.height=_pixmap.height()/2;
    _changePixmapWHXY();
}
void MapShower::changeData(QJsonArray array){
    for(int i=0;i<array.size();i++){
        int index=array.at(i).toArray().at(0).toInt();
        char data=array.at(i).toArray().at(1).toInt();
        _pixmapData[index]=data;
    }
}
void MapShower::clearSourceData(){
    _pixmapData.clear();
}
////////protect
void MapShower::paintEvent(QPaintEvent *){
    if(_isSetPixmapData){
        QPainter painter(this);
        painter.save();
        painter.setRenderHint(QPainter::SmoothPixmapTransform);
        painter.drawPixmap(QRect(_xy.x,_xy.y,_wh.width,_wh.height),_pixmap);
        painter.restore();
    }
}
void MapShower::wheelEvent(QWheelEvent *e){
    QPoint p=e->pos();
    if(_zoomCenter.x!=p.x()&&_zoomCenter.y!=p.y()){
        _zoomCenter.x=p.x();
        _zoomCenter.y=p.y();
        _deertaWh.width=(_zoomCenter.x-_xy.x)/_scalingRatio;
        _deertaWh.height=(_zoomCenter.y-_xy.y)/_scalingRatio;
    }
    if(e->delta()>0){
        _zoomIn();
    }else{
        _zoomOut();
    }
    _changePixmapWHXY();
}
void MapShower::mousePressEvent(QMouseEvent *e){
    _mousePressPoint.x=e->x();
    _mousePressPoint.y=e->y();
    _fakeXy=_xy;
}
void MapShower::mouseMoveEvent(QMouseEvent *e){
    _xy.x=_fakeXy.x+e->x()-_mousePressPoint.x;
    _xy.y=_fakeXy.y+e->y()-_mousePressPoint.y;
    _keepInFrame();
    update();
}
void MapShower::resizeEvent(QResizeEvent *){
    _keepInFrame();
    update();
}
/////////private
void MapShower::_zoomIn(){
    _scalingRatio+=_scalingRatio*0.25;
    if(_scalingRatio>7.7){
        _scalingRatio=7.7;
    }
}
void MapShower::_zoomOut(){
    _scalingRatio-=_scalingRatio*0.25;
    if(_scalingRatio<0.077){
        _scalingRatio=0.077;
    }
}
void MapShower::_changePixmapWHXY(){
    _wh.width=(int)_pixmap.width()*_scalingRatio;
    _wh.height=(int)_pixmap.height()*_scalingRatio;
    _xy.x=_zoomCenter.x-_deertaWh.width*_scalingRatio;
    _xy.y=_zoomCenter.y-_deertaWh.height*_scalingRatio;
    _keepInFrame();
    update();
}
void MapShower::_keepInFrame(){
    quint16 mapWidth=_pixmap.width()*_scalingRatio;
    quint16 mapHeight=_pixmap.height()*_scalingRatio;
    if(_xy.x<(-mapWidth+10)){
        _xy.x=-mapWidth+10;
    }else if(_xy.x>(width()-10)){
        _xy.x=width()-10;
    }else{}
    if(_xy.y<(-mapHeight+10)){
        _xy.y=(-mapHeight+10);
    }else if(_xy.y>(height()-10)){
        _xy.y=height()-10;
    }else{}
}
void MapShower::receiveJsonArrayListSlot(QList<QJsonArray> list){
    _pixmapData=_pixmapDataTmp;
    for(int i=0;i<list.length();i++){
        changeData(list[i]);
    }
    _pixmap.loadFromData(_pixmapData);
    update();
}
void MapShower::saveMapFileSlot(QString filepath){
    QStringList filepathList=filepath.split(".");
    if(filepathList.length()>1){
        if(filepathList[1]=="pgm"){
            QFile file(filepath);
            if(file.exists()){
                QMessageBox::information(this,"警告","该文件存在！");
            }else{
                if(_pixmapData.isEmpty()){
                    QMessageBox::information(this,"警告","请确认是否已打开文件");
                }else{

                    file.open(QIODevice::WriteOnly);
                    file.write(_pixmapData);
                    file.close();
                }
            }
        }
        else{
            QMessageBox::information(this,"警告","请确认文件名是否为后缀(.pgm)");
        }
    }else{
        QMessageBox::information(this,"警告","请添加文件后缀(.pgm)");
    }
}
/*@@@@@@@@@@@@@@@@@@@@@@MapDealer@@@@@@@@@@@@@@@@@@@@@@@@*/
MapDealer::MapDealer(QWidget* parent):QFrame(parent){
    setObjectName("MapDealer");
    _checkSourceMapButton=new QPushButton(this);
    _checkSourceMapButton->setText("选择基础地图");
    _sourceMapLabel=new QLabel(this);
    _sourceMapLabel->setObjectName("SourceMapLabel");
    _saveMapButton=new QPushButton(this);
    _saveMapButton->setText("保存地图");
    _gLayout=new QGridLayout(this);
    _gLayout->setContentsMargins(10,10,10,10);
    _gLayout->setHorizontalSpacing(50);
    _gLayout->setVerticalSpacing(10);
    _gLayout->addWidget(_checkSourceMapButton,0,0,1,1);
    _gLayout->addWidget(_sourceMapLabel,0,1,1,1);
    _gLayout->addWidget(_saveMapButton,1,0,1,1);
    _gLayout->setColumnStretch(0,1);
    _gLayout->setColumnStretch(1,99);
    setLayout(_gLayout);
    _fileDialog=new QFileDialog(this);
    _fileDialog->setWindowTitle("选择基础地图");
    _fileDialog->setDirectory(MAP_FILES_DIR);
    _fileDialog->setNameFilter("Maps(*.pgm)");
    _fileDialog->setViewMode(QFileDialog::Detail);
    connect(_checkSourceMapButton,SIGNAL(clicked()),this,SLOT(checkSourceMapButtonClickedSlot()));
    connect(_saveMapButton,SIGNAL(clicked()),this,SLOT(saveMapButtonClickedSlot()));
}
void MapDealer::checkSourceMapButtonClickedSlot(){
    _fileDialog->show();
    if(_fileDialog->exec()){
        QString filepath=_fileDialog->selectedFiles()[0];
        _sourceMapLabel->setText(filepath);
        emit SIGNALS->openSourceMapFileSignal(filepath);
    }
}
void MapDealer::saveMapButtonClickedSlot(){
    QString filepath=QFileDialog::getSaveFileName(this,"保存文件",SAVE_MAPS_DIR,"Maps(*.pgm)");
    emit SIGNALS->saveMapFileSignal(filepath);
}
/*@@@@@@@@@@@@@@@@@@@@@@MapViewer@@@@@@@@@@@@@@@@@@@@@@@@*/
MapViewer::MapViewer(QWidget* parent):QFrame(parent){
    setObjectName("MapViewer");
    _mapShower=new MapShower(this);
    _mapDealer=new MapDealer(this);
    _vLayout=new QVBoxLayout(this);
    _vLayout->setContentsMargins(0,0,0,0);
    _vLayout->setSpacing(5);
    _vLayout->addWidget(_mapShower);
    _vLayout->addWidget(_mapDealer);
    _vLayout->setStretch(0,9);
    _vLayout->setStretch(1,1);
    setLayout(_vLayout);
    connect(SIGNALS,SIGNAL(openSourceMapFileSignal(QString)),this,SLOT(openSourceMapFileSlot(QString)));
}
void MapViewer::openSourceMapFileSlot(QString filePath){
    QFile file(filePath);
    if(file.exists()){
        file.open(QIODevice::ReadOnly);
        _mapShower->setSourceData(file.readAll());
        file.close();
    }
}
/*@@@@@@@@@@@@@@@@@@@@@@MapInfoItemModel@@@@@@@@@@@@@@@@@@@@@@@@*/
MapInfoItemModel::MapInfoItemModel(QObject* parent):QAbstractTableModel(parent){
    _hearderData<<"选中"<<"文件名";
    _watcher=new QFileSystemWatcher(this);
    _watcher->addPath(INDEX_VALUE_FILES_DIR);
    _loadIndexValueThread=new LoadIndexValueThread(this);
    connect(_loadIndexValueThread,SIGNAL(emitData(QString,QJsonObject)),this,SLOT(receiveData(QString,QJsonObject)));
    connect(SIGNALS,SIGNAL(sendSearchConditionSignal(TOOL::SEARCH_TYPE,QString)),this,SLOT(checkedItemsChangedSlot()));
    connect(SIGNALS,SIGNAL(sendCheckallableSignal(bool)),this,SLOT(receiveCheckallableSlot(bool)));
    connect(_watcher,SIGNAL(directoryChanged(QString)),this,SLOT(directoryChangedSlot(QString)));
    _modelLoadData();
}
int MapInfoItemModel::rowCount(const QModelIndex &) const{
    return _infoMapKeys.size();
}
int MapInfoItemModel::columnCount(const QModelIndex &) const{
    return _hearderData.length();
}
QVariant MapInfoItemModel::headerData(int section, Qt::Orientation orientation, int role) const{
    if(orientation==Qt::Vertical){
        if(role==Qt::DisplayRole){
            return section+1;
        }else{
            return QVariant();
        }
    }else{
        if(role==Qt::DisplayRole){
            return _hearderData[section];
        }else{
            return QVariant();
        }
    }
}
QVariant MapInfoItemModel::data(const QModelIndex &index, int role) const{
    if (!index.isValid())
        return QVariant();
    INFO info=_infoMap[_infoMapKeys[index.row()]];
    if(role==Qt::DisplayRole&&index.column()==1){
        return _infoMapKeys[index.row()];
    }else if(role==Qt::UserRole&&index.column()==0){
        return info.checkable;
    }else if(role==Qt::TextAlignmentRole){
        return Qt::AlignCenter;
    }else if(role==Qt::ToolTipRole){
        quint64 orderId=info.orderId;
        QString mac=info.macAddress;
        qint64 updateTime=info.updateTime;
        QString week=QDateTime::fromMSecsSinceEpoch(updateTime).toString("dddd");
        QString time=QDateTime::fromMSecsSinceEpoch(updateTime).toString("yyyy-MM-dd hh:mm:ss");
        QString customer=info.customerAddress;
        qint64 baseVersion=info.baseVersion;
        QString version=QDateTime::fromMSecsSinceEpoch(baseVersion).toString("yyyy-MM-dd_hh:mm:ss");
        QString filepath=info.info.filePath();
        QString area=info.areaId;
        QString direction=info.direction;
        return QString("小区编号：%1\n基础版本：%2\n上传星期：%3\n上传时间：%4\n顾客楼栋：%5\n行驶方向：%6\n订单编号：%7\n车识别码：%8\n文件路径：%9")
                .arg(area).arg(version).arg(week).arg(time).arg(customer).arg(direction).arg(orderId).arg(mac).arg(filepath);
    }else{
        return QVariant();
    }
}
Qt::ItemFlags MapInfoItemModel::flags(const QModelIndex &index) const{
    if (!index.isValid())
        return 0;
    if(index.column()==0){
        return Qt::ItemIsEnabled |Qt::ItemIsSelectable | Qt::ItemIsUserCheckable;
    }else{
        return QAbstractTableModel::flags(index);
    }
}
bool MapInfoItemModel::setData(const QModelIndex &index, const QVariant &value, int role){
    if(!index.isValid())
        return false;
    if (role == Qt::UserRole && index.column()==0){
         _infoMap[_infoMapKeys[index.row()]].checkable = value.toBool();
        checkedItemsChangedSlot();
        return true;
    }else if (role == Qt::DecorationRole && index.column()==0){
        _infoMap[_infoMapKeys[index.row()]].visable = value.toBool();
        return true;
    }
    return true;
}
void MapInfoItemModel::MapInfoItemModel::_modelLoadData(){
    QDir indexValueFilesDir(INDEX_VALUE_FILES_DIR);
    if(!indexValueFilesDir.exists()){
        return;
    }else{
        beginResetModel();
        QStringList filters;
        filters<<QString("*.json");
        indexValueFilesDir.setFilter(QDir::Files | QDir::NoSymLinks); //设置类型过滤器，只为文件格式
        QFileInfoList infoList=indexValueFilesDir.entryInfoList(filters);
        for(int i=0;i<infoList.length();i++){
            INFO info;
            info.info=infoList[i];
            info.checkable=false;
            info.visable=true;
            _infoMap.insert(infoList[i].baseName(),info);
            _infoMapKeys.append(infoList[i].baseName());
        }
        _loadIndexValueThread->loadIndexValueFile(infoList);
        endResetModel();
    }
}
MapInfoItemModel::INFO MapInfoItemModel::fileInfo(const QModelIndex &index){
    return _infoMap[_infoMapKeys[index.row()]];
}
void MapInfoItemModel::receiveData(QString key,QJsonObject obj){
    _infoMap[key].indexValue=obj.value("IndexValue").toArray();
    qint64 baseVersion=obj.value("BaseVersion").toVariant().toLongLong();
    _infoMap[key].baseVersion=baseVersion;
    qint64 updateTime=obj.value("UpdateTime").toVariant().toLongLong();
    _infoMap[key].updateTime=updateTime;
    _infoMap[key].orderId=obj.value("OrderId").toVariant().toULongLong();
    QString customerAddress=obj.value("CustomerAddress").toString();
    _infoMap[key].customerAddress=customerAddress;
    QString macAddress=obj.value("MacAddress").toString();
    _infoMap[key].macAddress=macAddress;
    QString baseVesionStr=QDateTime::fromMSecsSinceEpoch(baseVersion).toString("yyyy-MM-dd_hh:mm:ss");
    QString areaId=obj.value("AreaId").toString();
    _infoMap[key].direction=obj.value("Direction").toString();
    _infoMap[key].areaId=areaId;
    if(!baseVesionStr.isEmpty()&&!GLOBAL->BaseVersionList.contains(baseVesionStr)){
        GLOBAL->BaseVersionList.append(baseVesionStr);
        emit SIGNALS->addSearchConditionSignal(TOOL::BASE_VERSION,baseVesionStr);
    }
    if(!customerAddress.isEmpty()&&!GLOBAL->CustomerAddressList.contains(customerAddress)){
        GLOBAL->CustomerAddressList.append(customerAddress);
        emit SIGNALS->addSearchConditionSignal(TOOL::CUSTOMER_ADDRESS,customerAddress);
    }
    if(!macAddress.isEmpty()&&!GLOBAL->MacAddressList.contains(macAddress)){
        GLOBAL->MacAddressList.append(macAddress);
        emit SIGNALS->addSearchConditionSignal(TOOL::MAC_ADDRESS,macAddress);
    }
    if(!areaId.isEmpty()&&!GLOBAL->AreaStringList.contains(areaId)){
        GLOBAL->AreaStringList.append(areaId);
        emit SIGNALS->addSearchConditionSignal(TOOL::AREA_ID,areaId);
    }
}
void MapInfoItemModel::checkedItemsChangedSlot(){
    QList<QJsonArray> jsonArrayList;
    for(int i=0;i<_infoMapKeys.length();i++){
        INFO info=_infoMap[_infoMapKeys[i]];
        if(info.visable&&info.checkable){
            jsonArrayList.append(info.indexValue);
        }
    }
    emit SIGNALS->sendJsonArrayListSignal(jsonArrayList);
}
void MapInfoItemModel::directoryChangedSlot(QString){
    QDir indexValueFilesDir(INDEX_VALUE_FILES_DIR);
    QStringList filters;
    filters<<QString("*.json");
    indexValueFilesDir.setFilter(QDir::Files | QDir::NoSymLinks); //设置类型过滤器，只为文件格式
    QFileInfoList infoList=indexValueFilesDir.entryInfoList(filters);
    if(infoList.length()>_infoMapKeys.length()){
        for(int i=0;i<infoList.length();i++){
            QFileInfo info=infoList[i];
            if(!_infoMapKeys.contains(info.baseName())){
                INFO _info;
                _info.info=info;
                _info.checkable=false;
                _info.visable=true;
                QFile file(info.filePath());
                file.open(QIODevice::ReadOnly);
                QByteArray bytes=file.readAll();
                file.close();
                QJsonDocument doc=QJsonDocument::fromJson(bytes);
                QJsonObject obj=doc.object();
                _info.indexValue=obj.value("IndexValue").toArray();
                _info.baseVersion=obj.value("BaseVersion").toVariant().toLongLong();;
                _info.updateTime=obj.value("UpdateTime").toVariant().toLongLong();
                _info.orderId=obj.value("OrderId").toVariant().toULongLong();
                _info.customerAddress=obj.value("CustomerAddress").toString();
                _info.macAddress=obj.value("MacAddress").toString();
                QString baseVesionStr=QDateTime::fromMSecsSinceEpoch(_info.baseVersion).toString("yyyy-MM-dd_hh:mm:ss");
                _info.direction=obj.value("Direction").toString();
                _info.areaId=obj.value("AreaId").toString();
                if(!baseVesionStr.isEmpty()&&!GLOBAL->BaseVersionList.contains(baseVesionStr)){
                    GLOBAL->BaseVersionList.append(baseVesionStr);
                    emit SIGNALS->addSearchConditionSignal(TOOL::BASE_VERSION,baseVesionStr);
                }
                if(!_info.customerAddress.isEmpty()&&!GLOBAL->CustomerAddressList.contains(_info.customerAddress)){
                    GLOBAL->CustomerAddressList.append(_info.customerAddress);
                    emit SIGNALS->addSearchConditionSignal(TOOL::CUSTOMER_ADDRESS,_info.customerAddress);
                }
                if(!_info.macAddress.isEmpty()&&!GLOBAL->MacAddressList.contains(_info.macAddress)){
                    GLOBAL->MacAddressList.append(_info.macAddress);
                    emit SIGNALS->addSearchConditionSignal(TOOL::MAC_ADDRESS,_info.macAddress);
                }
                if(!_info.areaId.isEmpty()&&!GLOBAL->AreaStringList.contains(_info.areaId)){
                    GLOBAL->AreaStringList.append(_info.areaId);
                    emit SIGNALS->addSearchConditionSignal(TOOL::AREA_ID,_info.areaId);
                }
                _infoMap.insert(info.baseName(),_info);
                _infoMapKeys.insert(i,info.baseName());
                beginInsertRows(QModelIndex(),i,i);
                insertRow(i);
                endInsertRows();
            }
        }
    }else if(infoList.length()<_infoMapKeys.length()){
        QStringList pathList=indexValueFilesDir.entryList(filters);
        for(int i=0;i<_infoMapKeys.length();i++){
            QString path=_infoMapKeys[i]+".json";
            if(!pathList.contains(path)){
                if(_infoMap[_infoMapKeys[i]].visable&&_infoMap[_infoMapKeys[i]].checkable){
                    _infoMap.remove(_infoMapKeys[i]);
                    _infoMapKeys.removeAt(i);
                    checkedItemsChangedSlot();
                }else{
                    _infoMap.remove(_infoMapKeys[i]);
                    _infoMapKeys.removeAt(i);
                }
                beginRemoveRows(QModelIndex(),i,i);
                removeRow(i);
                endRemoveRows();
            }
        }
    }
}
void MapInfoItemModel::receiveCheckallableSlot(bool isTrue){
    if(isTrue){
        for(int i=0;i<_infoMapKeys.length();i++){
            QString key=_infoMapKeys[i];
            if(_infoMap[key].visable&&!_infoMap[key].checkable){
                _infoMap[key].checkable=true;
                QModelIndex index=createIndex(i,0);
                emit dataChanged(index,index);
            }
        }
    }else{
        for(int i=0;i<_infoMapKeys.length();i++){
            QString key=_infoMapKeys[i];
            if(_infoMap[key].visable&&_infoMap[key].checkable){
                _infoMap[key].checkable=false;
                QModelIndex index=createIndex(i,0);
                emit dataChanged(index,index);
            }
        }
    }
    checkedItemsChangedSlot();
}
/*@@@@@@@@@@@@@@@@@@@@@@MapInfoItemDelegate@@@@@@@@@@@@@@@@@@@@@@@@*/
MapInfoItemDelegate::MapInfoItemDelegate(QObject* parent):QStyledItemDelegate(parent){
    _pixmapChecked.load(":/Image/Checked.png");
    _pixmapUnchecked.load(":/Image/Unchecked.png");
}
void MapInfoItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const{
    QRect wRect=option.rect;
    bool isChecked=index.model()->data(index,Qt::UserRole).toBool();
    painter->setRenderHint(QPainter::SmoothPixmapTransform);
    double unit=wRect.height()/(float)10;
    if(isChecked){
        painter->drawPixmap(QRectF((wRect.width()-unit*8)/(float)2,unit+wRect.y(),unit*8,unit*8),_pixmapChecked,_pixmapChecked.rect());
    }else{
        painter->drawPixmap(QRectF((wRect.width()-unit*8)/(float)2,unit+wRect.y(),unit*8,unit*8),_pixmapUnchecked,_pixmapUnchecked.rect());
    }
}
bool MapInfoItemDelegate::editorEvent(QEvent *event, QAbstractItemModel *model, const QStyleOptionViewItem &option, const QModelIndex &index){
    QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
    if(event->type()==QEvent::MouseButtonPress&&option.rect.contains(mouseEvent->pos())){
        bool data = model->data(index, Qt::UserRole).toBool();
        model->setData(index, !data, Qt::UserRole);
    }
    return true;
}
/*@@@@@@@@@@@@@@@@@@@@@@MapInfoItemProxyModel@@@@@@@@@@@@@@@@@@@@@@@@*/
MapInfoItemProxyModel::MapInfoItemProxyModel(QObject* parent):QSortFilterProxyModel(parent){
    _searchConditions.insert(_conditions.VERSION,"不限制");
    _searchConditions.insert(_conditions.WEEK,"不限制");
    _searchConditions.insert(_conditions.TIME,"不限制-不限制");
    _searchConditions.insert(_conditions.CUSTOMER,"不限制");
    _searchConditions.insert(_conditions.MAC,"不限制");
    _searchConditions.insert(_conditions.AREA,"不限制");
    _searchConditions.insert(_conditions.DIRECTION,"不限制");
    _chineseWeekMap.insert("星期一",1);
    _chineseWeekMap.insert("星期二",2);
    _chineseWeekMap.insert("星期三",3);
    _chineseWeekMap.insert("星期四",4);
    _chineseWeekMap.insert("星期五",5);
    _chineseWeekMap.insert("星期六",6);
    _chineseWeekMap.insert("星期日",7);
    ///////////////////
    _timeStringMap.insert("不限制",-1);
    _timeStringMap.insert("00:00",0);
    _timeStringMap.insert("01:00",1);
    _timeStringMap.insert("02:00",2);
    _timeStringMap.insert("03:00",3);
    _timeStringMap.insert("04:00",4);
    _timeStringMap.insert("05:00",5);
    _timeStringMap.insert("06:00",6);
    _timeStringMap.insert("07:00",7);
    _timeStringMap.insert("08:00",8);
    _timeStringMap.insert("09:00",9);
    _timeStringMap.insert("10:00",10);
    _timeStringMap.insert("11:00",11);
    _timeStringMap.insert("12:00",12);
    _timeStringMap.insert("13:00",13);
    _timeStringMap.insert("14:00",14);
    _timeStringMap.insert("15:00",15);
    _timeStringMap.insert("16:00",16);
    _timeStringMap.insert("17:00",17);
    _timeStringMap.insert("18:00",18);
    _timeStringMap.insert("19:00",19);
    _timeStringMap.insert("20:00",20);
    _timeStringMap.insert("21:00",21);
    _timeStringMap.insert("22:00",22);
    _timeStringMap.insert("23:00",23);
    connect(SIGNALS,SIGNAL(sendSearchConditionSignal(TOOL::SEARCH_TYPE,QString)),this,SLOT(sendSearchConditionSlot(TOOL::SEARCH_TYPE,QString)));
}
bool MapInfoItemProxyModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const{
    MapInfoItemModel* _sourceModel=static_cast<MapInfoItemModel*>(sourceModel());
    MapInfoItemModel::INFO info=_sourceModel->fileInfo(_sourceModel->index(source_row,0,source_parent));
    if(!_checkVersion(info)||!_checkWeek(info)||!_checkTime(info)||!_checkMacAddress(info)||!_checkCustomerAddress(info)||!_checkArea(info)||!_checkDirection(info)){
        _sourceModel->setData(_sourceModel->index(source_row,0,source_parent),false,Qt::DecorationRole);
        return false;
    }else{
        _sourceModel->setData(_sourceModel->index(source_row,0,source_parent),true,Qt::DecorationRole);
        return true;
    }
}
void MapInfoItemProxyModel::sendSearchConditionSlot(TOOL::SEARCH_TYPE type,QString str){
    switch (type) {
    case TOOL::BASE_VERSION:
        _searchConditions[_conditions.VERSION]=str;
        break;
    case TOOL::UPDATE_WEEK:
        _searchConditions[_conditions.WEEK]=str;
        break;
    case TOOL::UPDATE_TIME:
        _searchConditions[_conditions.TIME]=str;
        break;
    case TOOL::CUSTOMER_ADDRESS:
        _searchConditions[_conditions.CUSTOMER]=str;
        break;
    case TOOL::MAC_ADDRESS:
        _searchConditions[_conditions.MAC]=str;
        break;
    case TOOL::AREA_ID:
        _searchConditions[_conditions.AREA]=str;
        break;
    case TOOL::DIRECTION:
        _searchConditions[_conditions.DIRECTION]=str;
        break;
    default:
        break;
    }
    invalidateFilter();
}
bool MapInfoItemProxyModel::_checkVersion(MapInfoItemModel::INFO info)const{
    if(_searchConditions[_conditions.VERSION]!="不限制"){
        QString baseVesionStr=QDateTime::fromMSecsSinceEpoch(info.baseVersion).toString("yyyy-MM-dd_hh:mm:ss");
        if(baseVesionStr!=_searchConditions[_conditions.VERSION]){
            return false;
        }
    }
    return true;
}
bool MapInfoItemProxyModel::_checkWeek(MapInfoItemModel::INFO info)const{
    if(_searchConditions[_conditions.WEEK]!="不限制"){
        QString updateWeekStr=QDateTime::fromMSecsSinceEpoch(info.updateTime).toString("dddd");
        if(updateWeekStr.contains("星期")){
            quint8 now=_chineseWeekMap[updateWeekStr];
            if(_searchConditions[_conditions.WEEK].contains("～")){
                QStringList updateWeekStrList=_searchConditions[_conditions.WEEK].split("～");
                quint8 start=_chineseWeekMap[updateWeekStrList[0]];
                quint8 end=_chineseWeekMap[updateWeekStrList[1]];
                if(now<start||now>end){
                    return false;
                }
            }else{
                if(updateWeekStr!=_searchConditions[_conditions.WEEK]){
                    return false;
                }
            }
        }else{
            //英文星期
        }
    }
    return true;
}
bool MapInfoItemProxyModel::_checkTime(MapInfoItemModel::INFO info)const{
    if(!_searchConditions[_conditions.TIME].contains("不限制")){
        QStringList strList=_searchConditions[_conditions.TIME].split("-");
        QString startStr=strList[0];
        QString endStr=strList[1];
        QString nowStr=QDateTime::fromMSecsSinceEpoch(info.updateTime).toString("hh:00");
        quint8 now=_timeStringMap[nowStr];
        quint8 start=_timeStringMap[startStr];
        quint8 end=_timeStringMap[endStr];
        if(start<end){
            if(now<start||now>=end){
                return false;
            }
        }else if(start>end){
            if(now<start||now>=end){
                return false;
            }
        }
    }
    return true;
}
bool MapInfoItemProxyModel::_checkCustomerAddress(MapInfoItemModel::INFO info)const{
    if(_searchConditions[_conditions.CUSTOMER]!="不限制"){
        if(_searchConditions[_conditions.CUSTOMER]!=info.customerAddress){
            return false;
        }
    }
    return true;
}
bool MapInfoItemProxyModel::_checkMacAddress(MapInfoItemModel::INFO info)const{
    if(_searchConditions[_conditions.MAC]!="不限制"){
        if(_searchConditions[_conditions.MAC]!=info.macAddress&&!info.macAddress.isEmpty()){
            return false;
        }
    }
    return true;
}
bool MapInfoItemProxyModel::_checkArea(MapInfoItemModel::INFO info) const{
    if(_searchConditions[_conditions.AREA]!="不限制"){
        if(_searchConditions[_conditions.AREA]!=info.areaId&&!info.areaId.isEmpty()){
            return false;
        }
    }
    return true;
}
bool MapInfoItemProxyModel::_checkDirection(MapInfoItemModel::INFO info) const{
    if(_searchConditions[_conditions.DIRECTION]!="不限制"){
        if(_searchConditions[_conditions.DIRECTION]!=info.direction&&!info.direction.isEmpty()){
            return false;
        }
    }
    return true;
}
/*@@@@@@@@@@@@@@@@@@@@@@加载地图差别信息的线程@@@@@@@@@@@@@@@@@@@@@@@@*/
LoadIndexValueThread::LoadIndexValueThread(QObject* parent):QThread(parent){}
void LoadIndexValueThread::loadIndexValueFile(QFileInfoList list){
    _infoList=list;
    start();
}
void LoadIndexValueThread::run(){
    QMap<QString,QJsonObject> indexValueMap;
    for(int i=0;i<_infoList.length();i++){
        QString indexValueFilePath=_infoList[i].filePath();
        QString indexValueFileName=_infoList[i].baseName();
        QFile file(indexValueFilePath);
        if(file.exists()){
            file.open(QIODevice::ReadOnly);
            QByteArray bytes=file.readAll();
            file.close();
            QJsonDocument doc=QJsonDocument::fromJson(bytes);
            QJsonObject obj=doc.object();
            emit emitData(indexValueFileName,obj);
        }
    }
}
/*@@@@@@@@@@@@@@@@@@@@@@SearchConditionFrame@@@@@@@@@@@@@@@@@@@@@@@@*/
SearchConditionBox::SearchConditionBox(QWidget* parent):QFrame(parent){
    _titleLabel=new QLabel(this);
    _comboBox=new QComboBox(this);
    _hLayout=new QHBoxLayout(this);
    _hLayout->setContentsMargins(0,0,0,0);
    _hLayout->setSpacing(20);
    _hLayout->addWidget(_titleLabel);
    _hLayout->addWidget(_comboBox);
    _hLayout->setStretch(0,1);
    _hLayout->setStretch(1,99);
    setLayout(_hLayout);
    connect(_comboBox,SIGNAL(currentIndexChanged(int)),this,SLOT(currentIndexChangedSlot(int)));
    connect(SIGNALS,SIGNAL(addSearchConditionSignal(TOOL::SEARCH_TYPE,QString)),this,SLOT(addSearchConditionSlot(TOOL::SEARCH_TYPE,QString)));
}
void SearchConditionBox::setSearchType(TOOL::SEARCH_TYPE searchType){
    _searchType=searchType;
    switch (searchType) {
    case TOOL::BASE_VERSION:
        _stringList=GLOBAL->BaseVersionList;
        _comboBox->addItems(GLOBAL->BaseVersionList);
        break;
    case TOOL::CUSTOMER_ADDRESS:
        _stringList=GLOBAL->CustomerAddressList;
        _comboBox->addItems(GLOBAL->CustomerAddressList);
        break;
    case TOOL::MAC_ADDRESS:
        _stringList=GLOBAL->MacAddressList;
        _comboBox->addItems(GLOBAL->MacAddressList);
        break;
    case TOOL::UPDATE_WEEK:
        _stringList=GLOBAL->WeekStringList;
        _comboBox->addItems(GLOBAL->WeekStringList);
        break;
    case TOOL::AREA_ID:
        _stringList=GLOBAL->AreaStringList;
        _comboBox->addItems(GLOBAL->AreaStringList);
        break;
    case TOOL::DIRECTION:
        _stringList=GLOBAL->DirectionStringList;
        _comboBox->addItems(GLOBAL->DirectionStringList);
        break;
    default:
        break;
    }
}
void SearchConditionBox::currentIndexChangedSlot(int index){
    if(index>=0&&index<_stringList.length()){
        emit SIGNALS->sendSearchConditionSignal(_searchType,_stringList[index]);
    }
}
void SearchConditionBox::addSearchConditionSlot(TOOL::SEARCH_TYPE searchType,QString str){
    if(searchType==_searchType){
        _stringList.append(str);
        _comboBox->addItem(str);
    }
}
/*@@@@@@@@@@@@@@@@@@@@@@SearchConditionTimeBox@@@@@@@@@@@@@@@@@@@@@@@@*/
SearchConditionTimeBox::SearchConditionTimeBox(QWidget* parent):QFrame(parent){
    _startLabel=new QLabel(this);
    _startLabel->setText("开始时间");
    _endLabel=new QLabel(this);
    _endLabel->setText("结束时间");
    _startComboBox=new QComboBox(this);
    _startComboBox->addItems(GLOBAL->TimeStringList);
    _endComboBox=new QComboBox(this);
    _endComboBox->addItems(GLOBAL->TimeStringList);
    _hLayout=new QHBoxLayout(this);
    _hLayout->setContentsMargins(0,0,0,0);
    _hLayout->setSpacing(20);
    _hLayout->addWidget(_startLabel);
    _hLayout->addWidget(_startComboBox);
    _hLayout->addWidget(_endLabel);
    _hLayout->addWidget(_endComboBox);
    _hLayout->setStretch(0,1);
    _hLayout->setStretch(1,99);
    _hLayout->setStretch(2,1);
    _hLayout->setStretch(3,99);
    setLayout(_hLayout);
    connect(_startComboBox,SIGNAL(currentIndexChanged(int)),this,SLOT(currentIndexChangedSlot(int)));
    connect(_endComboBox,SIGNAL(currentIndexChanged(int)),this,SLOT(currentIndexChangedSlot(int)));
}
void SearchConditionTimeBox::currentIndexChangedSlot(int){
    QString startStr=GLOBAL->TimeStringList[_startComboBox->currentIndex()];
    QString endStr=GLOBAL->TimeStringList[_endComboBox->currentIndex()];
    emit SIGNALS->sendSearchConditionSignal(TOOL::UPDATE_TIME,QString("%1-%2").arg(startStr).arg(endStr));
};
/*@@@@@@@@@@@@@@@@@@@@@@MapSearcher@@@@@@@@@@@@@@@@@@@@@@@@*/
MapSearcher::MapSearcher(QWidget* parent):QFrame(parent){
    _areaBox=new SearchConditionBox(this);
    _areaBox->setTitle("小区编号");
    _areaBox->setSearchType(TOOL::AREA_ID);
    _baseVersionBox=new SearchConditionBox(this);
    _baseVersionBox->setTitle("基础版本");
    _baseVersionBox->setSearchType(TOOL::BASE_VERSION);
    _updateWeekBox=new SearchConditionBox(this);
    _updateWeekBox->setTitle("上传星期");
    _updateWeekBox->setSearchType(TOOL::UPDATE_WEEK);
    _updateTimeBox=new SearchConditionTimeBox(this);
    _customerAddressBox=new SearchConditionBox(this);
    _customerAddressBox->setTitle("顾客楼栋");
    _customerAddressBox->setSearchType(TOOL::CUSTOMER_ADDRESS);
    _directionBox=new SearchConditionBox(this);
    _directionBox->setTitle("路线方向");
    _directionBox->setSearchType(TOOL::DIRECTION);
    _macAddressBox=new SearchConditionBox(this);
    _macAddressBox->setTitle("车识别码");
    _macAddressBox->setSearchType(TOOL::MAC_ADDRESS);
    _checkAllButton=new QPushButton(this);
    _checkAllButton->setText("全选");
    _uncheckAllButton=new QPushButton(this);
    _uncheckAllButton->setText("全不选");
    _gLayout=new QGridLayout(this);
    _gLayout->setContentsMargins(5,5,5,5);
    _gLayout->setSpacing(5);
    _gLayout->addWidget(_areaBox,0,0,1,4);
    _gLayout->addWidget(_baseVersionBox,1,0,1,4);
    _gLayout->addWidget(_updateWeekBox,2,0,1,4);
    _gLayout->addWidget(_updateTimeBox,3,0,1,4);
    _gLayout->addWidget(_customerAddressBox,4,0,1,4);
    _gLayout->addWidget(_directionBox,5,0,1,4);
    _gLayout->addWidget(_macAddressBox,6,0,1,4);
    _gLayout->addWidget(_checkAllButton,7,0,1,1);
    _gLayout->addWidget(_uncheckAllButton,7,1,1,1);
    setLayout(_gLayout);
    connect(_checkAllButton,&QPushButton::clicked,[](){emit SIGNALS->sendCheckallableSignal(true);});
    connect(_uncheckAllButton,&QPushButton::clicked,[](){emit SIGNALS->sendCheckallableSignal(false);});
}
/*@@@@@@@@@@@@@@@@@@@@@@MapInfoView@@@@@@@@@@@@@@@@@@@@@@@@*/
MapInfoView::MapInfoView(QWidget* parent):QTableView(parent){
    _infoProxyModel=new MapInfoItemProxyModel(this);
    _infoModel=new MapInfoItemModel(this);
    _infoProxyModel->setSourceModel(_infoModel);
    _infoDelegate=new MapInfoItemDelegate(this);
    horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    horizontalHeader()->setStretchLastSection(true);
    setItemDelegateForColumn(0,_infoDelegate);
    setAlternatingRowColors(true);
    setModel(_infoProxyModel);
    setContextMenuPolicy(Qt::CustomContextMenu);
    _deleteAction=new QAction(this);
    _deleteAction->setIcon(QIcon(":/Image/Undelete.png"));
    _deleteAction->setText("删除此文件");
    _menu=new QMenu(this);
    _menu->addAction(_deleteAction);
    connect(this,SIGNAL(customContextMenuRequested(QPoint)),this,SLOT(showMenuSlot(QPoint)));
    connect(_deleteAction,SIGNAL(triggered()),this,SLOT(deleteActionTriggeredSlot()));
}
void MapInfoView::showMenuSlot(QPoint pos){
    //计算鼠标相对父窗口的位置
    QPoint pt=mapToGlobal(pos)+QPoint(15,27);
    //判断鼠标右击位置是否是空白处，空白处则取消上一个选中焦点，不弹出菜单
    QModelIndex index = indexAt(pos);
    if (!index.isValid()){
       this->clearSelection();
       return;
    }
    _menu->exec(pt);
}
void MapInfoView::deleteActionTriggeredSlot(){
    QModelIndex index=currentIndex();
    MapInfoItemModel::INFO info=_infoModel->fileInfo(index);
    QFile file(info.info.filePath());
    if(file.exists()){
        file.remove();
    }
}
/*@@@@@@@@@@@@@@@@@@@@@@MapItemer@@@@@@@@@@@@@@@@@@@@@@@@*/
MapItemer::MapItemer(QWidget* parent):QFrame(parent){
    _mapSearcher=new MapSearcher(this);
    _infoView=new MapInfoView(this);
    _vLayout=new QVBoxLayout(this);
    _vLayout->setContentsMargins(0,0,0,0);
    _vLayout->setSpacing(10);
    _vLayout->addWidget(_mapSearcher);
    _vLayout->addWidget(_infoView);
    setLayout(_vLayout);
}
/*@@@@@@@@@@@@@@@@@@@@@@MapHolder@@@@@@@@@@@@@@@@@@@@@@@@@*/
MapHolder::MapHolder(QWidget* parent):QSplitter(parent){
    setOrientation(Qt::Horizontal);
    _mapItemer=new MapItemer(this);
    _mapViewer=new MapViewer(this);
    addWidget(_mapItemer);
    addWidget(_mapViewer);
    setStretchFactor(0,1);
    setStretchFactor(1,9);
}
/*@@@@@@@@@@@@@@@@@@@@@@@OBS@@@@@@@@@@@@@@@@@@@@@@@*/
OBS::OBS(QObject* parent):QObject(parent){
    _netManager=new QNetworkAccessManager(this);
    _netManager->setNetworkAccessible(QNetworkAccessManager::Accessible);
    connect(_netManager,&QNetworkAccessManager::finished,this,&OBS::finishedSlot);
}
void OBS::finishedSlot(QNetworkReply* reply){
    QByteArray sign=reply->request().rawHeader("Sign");
    if(reply->error()==QNetworkReply::NoError){
        if(sign==_requestSign.enumerateObjectsInBucket){
            enumerateObjectsInBucketReply(reply);
        }else if(sign==_requestSign.putObject){
            putObjectReply(reply);
        }else if(sign==_requestSign.getObjectContent){
            getObjectContentReply(reply);
        }else if(sign==_requestSign.getObjectMetadata){
            getObjectMetadataReply(reply);
        }else if(sign==_requestSign.deleteObject){
            deleteObjectReply(reply);
        }else if(sign==_requestSign.batchDeleteObject){
            batchDeleteObjectReply(reply);
        }else{}
    }else{
        if(sign==_requestSign.enumerateObjectsInBucket){
            QMap<QString,TOOL::OBJECT_INFO> map;
            emit sendObjectsInBucketSignal(map);
        }else if(sign==_requestSign.putObject){
            QByteArray objectName=reply->request().rawHeader("ObjectName");
            emit sendObjectPutedSignal(objectName,false);
        }else if(sign==_requestSign.getObjectContent){
            QByteArray objectName=reply->request().rawHeader("ObjectName");
            emit sendObjectDownloadedSignal(objectName,false);
        }else if(sign==_requestSign.getObjectMetadata){
            QByteArray objectName=reply->request().rawHeader("ObjectName");
            emit sendObjectMetadataSignal(objectName,false);
        }else if(sign==_requestSign.deleteObject){
            QByteArray objectName=reply->request().rawHeader("ObjectName");
            emit sendObjectDeletedSignal(objectName,false);
        }else if(sign==_requestSign.batchDeleteObject){
            emit sendObjectBatchDeletedSignal(QStringList(),_batchDeleteObjectNames);
        }else{}
    }
    reply->deleteLater();
}
void OBS::enumerateObjectsInBucket(){
    TOOL::SIGNATURE signature;
    signature.httpVerb="GET";
    signature.date=_formatDate();
    signature.canonicalizedResource="/ros-map/";
    QByteArray authorization=_createAuthorization(signature);
    QNetworkRequest request;
    request.setHeader(QNetworkRequest::UserAgentHeader,"curl/7.29.0");
    request.setRawHeader("Host",_host.toUtf8());
    request.setRawHeader("Accept","*/*");
    request.setRawHeader("Date",signature.date.toUtf8());
    request.setRawHeader("Authorization",authorization);
    request.setRawHeader("Sign",_requestSign.enumerateObjectsInBucket);
    request.setUrl(QUrl(_accountAccessDomainName));
    _netManager->get(request);
}
void OBS::putObject(QString objectName,QByteArray data){
    TOOL::SIGNATURE signature;
    signature.httpVerb="PUT";
    signature.date=_formatDate();
    signature.contentType="application/json";
    signature.canonicalizedResource="/ros-map/"+objectName;
    QByteArray authorization=_createAuthorization(signature);
    QNetworkRequest request;
    request.setHeader(QNetworkRequest::UserAgentHeader,"curl/7.29.0");
    request.setRawHeader("Host",_host.toUtf8());
    request.setRawHeader("Date",signature.date.toUtf8());
    request.setRawHeader("Authorization",authorization);
    request.setRawHeader("Content-Type",signature.contentType.toUtf8());
    request.setRawHeader("Content-Length",QString::number(data.length()).toUtf8());
    request.setRawHeader("Accept","*/*");
    request.setRawHeader("Sign",_requestSign.putObject);
    request.setRawHeader("ObjectName",objectName.toUtf8());
    request.setUrl(QUrl(_accountAccessDomainName+"/"+objectName));
    _netManager->put(request,data);
}
void OBS::getObjectContent(QString objectName){
    TOOL::SIGNATURE signature;
    signature.httpVerb="GET";
    signature.date=_formatDate();
    signature.canonicalizedResource="/ros-map/"+objectName;
    QByteArray authorization=_createAuthorization(signature);
    QNetworkRequest request;
    request.setHeader(QNetworkRequest::UserAgentHeader,"curl/7.29.0");
    request.setRawHeader("Host",_host.toUtf8());
    request.setRawHeader("Date",signature.date.toUtf8());
    request.setRawHeader("Authorization",authorization);
    request.setRawHeader("Range","bytes");
    request.setRawHeader("Accept","*/*");
    request.setRawHeader("Sign",_requestSign.getObjectContent);
    request.setRawHeader("ObjectName",objectName.toUtf8());
    request.setUrl(QUrl(_accountAccessDomainName+"/"+objectName));
    _netManager->get(request);
}
void OBS::getObjectMetadata(QString objectName){
    TOOL::SIGNATURE signature;
    signature.httpVerb="HEAD";
    signature.date=_formatDate();
    signature.canonicalizedResource="/ros-map/"+objectName;
    QByteArray authorization=_createAuthorization(signature);
    QNetworkRequest request;
    request.setHeader(QNetworkRequest::UserAgentHeader,"curl/7.29.0");
    request.setRawHeader("Host",_host.toUtf8());
    request.setRawHeader("Date",signature.date.toUtf8());
    request.setRawHeader("Authorization",authorization);
    request.setRawHeader("Accept","*/*");
    request.setRawHeader("Sign",_requestSign.getObjectMetadata);
    request.setRawHeader("ObjectName",objectName.toUtf8());
    request.setUrl(QUrl(_accountAccessDomainName+"/"+objectName));
    _netManager->head(request);
}
void OBS::deleteObject(QString objectName){
    TOOL::SIGNATURE signature;
    signature.httpVerb="DELETE";
    signature.date=_formatDate();
    signature.canonicalizedResource="/ros-map/"+objectName;
    QByteArray authorization=_createAuthorization(signature);
    QNetworkRequest request;
    request.setHeader(QNetworkRequest::UserAgentHeader,"curl/7.29.0");
    request.setRawHeader("Host",_host.toUtf8());
    request.setRawHeader("Date",signature.date.toUtf8());
    request.setRawHeader("Authorization",authorization);
    request.setRawHeader("Accept","*/*");
    request.setRawHeader("Sign",_requestSign.deleteObject);
    request.setRawHeader("ObjectName",objectName.toUtf8());
    request.setUrl(QUrl(_accountAccessDomainName+"/"+objectName));
    _netManager->deleteResource(request);
}
void OBS::batchDeleteObject(QByteArray xmlData){
    TOOL::SIGNATURE signature;
    signature.httpVerb="POST";
    signature.date=_formatDate();
    signature.canonicalizedResource="/ros-map/?delete";
    QByteArray authorization=_createAuthorization(signature);
    QByteArray contentSHA256=QCryptographicHash::hash(xmlData,QCryptographicHash::Sha256);
    QNetworkRequest request;
    request.setHeader(QNetworkRequest::UserAgentHeader,"curl/7.29.0");
    request.setRawHeader("Host",_host.toUtf8());
    request.setRawHeader("Date",signature.date.toUtf8());
    request.setRawHeader("Authorization",authorization);
    request.setRawHeader("Accept","*/*");
    request.setRawHeader("Content-SHA256",contentSHA256);
    request.setRawHeader("Content-Length",QString::number(xmlData.length()).toUtf8());
    request.setRawHeader("Sign",_requestSign.batchDeleteObject);
    request.setUrl(QUrl(_accountAccessDomainName+"/?delete"));
    _netManager->post(request,xmlData);
}
QByteArray OBS::_createAuthorization(TOOL::SIGNATURE signature){
    ///1.构造请求字符串(StringToSign)。
    QString stringToSign=QString("%1\n%2\n%3\n%4\n%5")
            .arg(signature.httpVerb)
            .arg(signature.contentMD5)
            .arg(signature.contentType)
            .arg(signature.date)
            .arg(signature.canonicalizedHeaders+signature.canonicalizedResource);
    ///2.对第一步的结果进行UTF-8编码。
    QByteArray stringToSignUtf8=stringToSign.toUtf8();
    ///3.使用SK对第二步的结果进行HMAC-SHA1签名计算。
    QByteArray _signature=_hmacSha1(_secretAccessKey.toUtf8(),stringToSignUtf8);
    ///4.对第三步的结果进行Base64编码，得到签名。
    QByteArray __signature=_signature.toBase64();
    QByteArray authorization=QString("OBS %1").arg(_accessKeyId).toUtf8()+":"+__signature;
    return authorization;
}
QString OBS::_formatDate(){
    QString format="ddd, dd MMM yyyy HH:mm:ss";
    QLocale loc=QLocale(QLocale::English,QLocale::UnitedStates);
    QDateTime t=QDateTime::currentDateTime();
    QString date=loc.toString(t.toUTC(),format)+" GMT";
    return date;
}
QByteArray OBS::_hmacSha1(QByteArray key, QByteArray baseString){
    int blockSize = 64; // HMAC-SHA-1 block size, defined in SHA-1 standard
    if (key.length() > blockSize) { // if key is longer than block size (64), reduce key length with SHA-1 compression
        key = QCryptographicHash::hash(key, QCryptographicHash::Sha1);
    }
    QByteArray innerPadding(blockSize, char(0x36)); // initialize inner padding with char"6"
    QByteArray outerPadding(blockSize, char(0x5c)); // initialize outer padding with char"/"
    for (int i = 0; i < key.length(); i++) {
        innerPadding[i] = innerPadding[i] ^ key.at(i); // XOR operation between every byte in key and innerpadding, of key length
        outerPadding[i] = outerPadding[i] ^ key.at(i); // XOR operation between every byte in key and outerpadding, of key length
    }
    QByteArray total = outerPadding;
    QByteArray part = innerPadding;
    part.append(baseString);
    total.append(QCryptographicHash::hash(part, QCryptographicHash::Sha1));
    QByteArray hashed = QCryptographicHash::hash(total, QCryptographicHash::Sha1);
    return hashed;
}
void OBS::enumerateObjectsInBucketReply(QNetworkReply* reply){
    QByteArray xx=reply->readAll();
    QDomDocument doc;
    QString errorStr;
    int errorLine;
    int errorColumn;
    if(!doc.setContent(xx, false, &errorStr, &errorLine, &errorColumn)){
        return;
    }
    QDomElement root=doc.documentElement();
    QDomNodeList list=root.elementsByTagName("Contents");
    QMap<QString,TOOL::OBJECT_INFO> objectInfoMap;
    for(int i=0;i<list.length();i++){
        QDomElement contents=list.at(i).toElement();
        TOOL::OBJECT_INFO info;
        QDomNodeList infoList=contents.childNodes();
        for(int j=0;j<infoList.length();j++){
            QDomElement element=infoList.at(j).toElement();
            QString tagName=element.tagName();
            if(tagName=="Key"){
                info.key=element.text();
                QFile file(INDEX_VALUE_FILES_DIR+"/"+info.key);
                if(file.exists()){
                    info.downloadstatus=TOOL::DOWNLOADED;
                }else{
                    info.downloadstatus=TOOL::UNDOWNLOAD;
                }
                QString name=info.key;
                QStringList nameList=name.split("--");
                if(nameList.length()==3){
                    info.areaId=nameList[0];
                    info.updateTime=nameList[1].toULongLong();
                    info.customerAddress="#"+nameList[2].split(".")[0];
                    if(!GLOBAL->CloudAreaIDList.contains(info.areaId)){
                        GLOBAL->CloudAreaIDList.append(info.areaId);
                        emit SIGNALS->addCloudSearchConditionSignal(TOOL::CLOUD_AREA_ID,info.areaId);
                    }
                    if(!GLOBAL->CloudCustomerAddressList.contains(info.customerAddress)){
                        GLOBAL->CloudCustomerAddressList.append(info.customerAddress);
                        emit SIGNALS->addCloudSearchConditionSignal(TOOL::CLOUD_CUSTOMER_ADDRESS,info.customerAddress);
                    }
                }
            }else if(tagName=="LastModified"){
                QDateTime sourceTime=QDateTime::fromString(element.text(),"yyyy-MM-ddThh:mm:ss.zzzZ");
                sourceTime.setTimeSpec(Qt::UTC);
                info.lastModified=sourceTime.toLocalTime();
            }else if(tagName=="ETag"){
                info.etag=element.text();
            }else if(tagName=="Size"){
                info.size=element.text().toUInt();
            }else{}
        }
        info.checkable=false;
        info.visable=true;
        info.deleteStatus=TOOL::UNDELETE;
        objectInfoMap.insert(info.key,info);
    }
    emit sendObjectsInBucketSignal(objectInfoMap);
}
void OBS::putObjectReply(QNetworkReply* reply){
    QString objectName=reply->request().rawHeader("ObjectName");
    emit sendObjectPutedSignal(objectName,true);
}
void OBS::getObjectContentReply(QNetworkReply* reply){
    QByteArray data=reply->readAll();
    QString objectName=reply->request().rawHeader("ObjectName");
    QString filepath=INDEX_VALUE_FILES_DIR+"/"+objectName;
    QFile file(filepath);
    if(file.exists()){
        return;
    }
    file.open(QIODevice::WriteOnly);
    file.write(data);
    file.close();
    emit sendObjectDownloadedSignal(objectName,true);
}
void OBS::getObjectMetadataReply(QNetworkReply* reply){
    QString objectName=reply->request().rawHeader("ObjectName");
    emit sendObjectMetadataSignal(objectName,true);
}
void OBS::deleteObjectReply(QNetworkReply* reply){
    QString objectName=reply->request().rawHeader("ObjectName");
    emit sendObjectDeletedSignal(objectName,true);
}
void OBS::batchDeleteObjectReply(QNetworkReply* reply){
    QByteArray xx=reply->readAll();
    QDomDocument doc;
    QString errorStr;
    int errorLine;
    int errorColumn;
    if(!doc.setContent(xx, false, &errorStr, &errorLine, &errorColumn)){
        return;
    }
    QDomElement root=doc.documentElement();
    QDomNode deletedNode=root.elementsByTagName("Deleted").at(0);
    QDomNodeList keyNodeList=deletedNode.toElement().elementsByTagName("Key");
    QStringList keyList;
    for(int i=0;i<keyNodeList.length();i++){
        keyList.append(keyNodeList.at(i).toElement().text());
    }
    QDomNode errorNode=root.elementsByTagName("Error").at(0);
    QDomNodeList errorNodeList=deletedNode.toElement().elementsByTagName("Key");
    QStringList errorList;
    for(int j=0;j<errorList.length();j++){
        errorList.append(errorNodeList.at(j).toElement().text());
    }
    emit sendObjectBatchDeletedSignal(keyList,errorList);
}
OBS* obs=new OBS();
/*@@@@@@@@@@@@@@@@@@@@@@ClouldObjectItemModel@@@@@@@@@@@@@@@@@@@@@@@@*/
CloudObjectItemModel::CloudObjectItemModel(QObject* parent):QAbstractTableModel(parent){
    _hearderData<<"选中"<<"文件名"<<"修改时间"<<"文件大小"<<"下载"<<"删除";
    connect(obs,SIGNAL(sendObjectsInBucketSignal(QMap<QString,TOOL::OBJECT_INFO>)),this,SLOT(receiveObjectsInBucketSlot(QMap<QString,TOOL::OBJECT_INFO>)));
    connect(obs,SIGNAL(sendObjectDownloadedSignal(QString,bool)),this,SLOT(receiveObjectDownloadedSlot(QString,bool)));
    connect(obs,SIGNAL(sendObjectDeletedSignal(QString,bool)),this,SLOT(receiveObjectDeletedSlot(QString,bool)));
    connect(obs,SIGNAL(sendObjectBatchDeletedSignal(QStringList,QStringList)),this,SLOT(receiveObjectBatchDeletedSlot(QStringList,QStringList)));
    connect(SIGNALS,SIGNAL(batchDeleteSignal()),this,SLOT(batchDeleteSlot()));
    connect(SIGNALS,SIGNAL(batchDownloadSignal()),this,SLOT(batchDownloadSlot()));
    connect(SIGNALS,SIGNAL(sendCloudCheckallableSignal(bool)),this,SLOT(receiveCloudCheckallableSlot(bool)));
    obs->enumerateObjectsInBucket();
}
TOOL::OBJECT_INFO CloudObjectItemModel::objectInfo(const QModelIndex index){
    return  _objectInfoMap[_infoMapKeys[index.row()]];
}
int CloudObjectItemModel::rowCount(const QModelIndex &) const{
    return _infoMapKeys.length();
}
int CloudObjectItemModel::columnCount(const QModelIndex &) const{
    return _hearderData.length();
}
QVariant CloudObjectItemModel::headerData(int section, Qt::Orientation orientation, int role) const{
    if(orientation==Qt::Vertical){
        if(role==Qt::DisplayRole){
            return section+1;
        }else{
            return QVariant();
        }
    }else{
        if(role==Qt::DisplayRole){
            return _hearderData[section];
        }else{
            return QVariant();
        }
    }
}
QVariant CloudObjectItemModel::data(const QModelIndex &index, int role) const{
    if (!index.isValid())
        return QVariant();
    TOOL::OBJECT_INFO info=_objectInfoMap[_infoMapKeys[index.row()]];
    int column=index.column();
    if(role==Qt::DisplayRole){
        switch (column) {
        case 1:
            return info.key.split(".")[0];
        case 2:
            return info.lastModified.toString("yyyy/MM/dd hh:mm:ss.zzz");
        case 3:
            return _size(info.size);
        default:
            return QVariant();
        }
    }else if(role==Qt::UserRole){
        switch (column) {
        case 0:
            return info.checkable;
        case 4:
            return info.downloadstatus;
        case 5:
            return info.deleteStatus;
        default:
            return QVariant();
        }
    }else if(role==Qt::TextAlignmentRole){
        return Qt::AlignCenter;
    }else{
        return QVariant();
    }
}
Qt::ItemFlags CloudObjectItemModel::flags(const QModelIndex &index) const{
    if (!index.isValid())
        return 0;
    if(index.column()==0||index.column()==4||index.column()==5){
        return Qt::ItemIsEnabled |Qt::ItemIsSelectable | Qt::ItemIsUserCheckable;
    }else{
        return QAbstractTableModel::flags(index);
    }
}
bool CloudObjectItemModel::setData(const QModelIndex &index,const QVariant &value,int role){
    if(!index.isValid())
        return false;
    int column=index.column();
    if (role == Qt::UserRole){
        if(column==0){
            _objectInfoMap[_infoMapKeys[index.row()]].checkable = value.toBool();
        }else if(column==4){
            TOOL::DOWNLOAD_STATUS status=static_cast<TOOL::DOWNLOAD_STATUS>(value.toInt());
            _objectInfoMap[_infoMapKeys[index.row()]].downloadstatus=status;
            if(status==TOOL::DOWNLOADING){
                obs->getObjectContent(_objectInfoMap[_infoMapKeys[index.row()]].key);
            }
        }else if(column==5){
            TOOL::DELETE_STATUS status=static_cast<TOOL::DELETE_STATUS>(value.toInt());
            _objectInfoMap[_infoMapKeys[index.row()]].deleteStatus=status;
            if(status==TOOL::DELETING){
                obs->deleteObject(_objectInfoMap[_infoMapKeys[index.row()]].key);
            }
        }
        emit dataChanged(index,index);
    }else if (role == Qt::DecorationRole){
            _objectInfoMap[_infoMapKeys[index.row()]].visable = value.toBool();
    }
    return true;
}
QString CloudObjectItemModel::_size(quint32 size) const{
    double mb=size/(float)(1024*1024);
    return QString("%1MB").arg(QString::number(mb,'f',3));
}
void CloudObjectItemModel::receiveObjectsInBucketSlot(QMap<QString,TOOL::OBJECT_INFO> objectInfoMap){
    beginResetModel();
    _objectInfoMap=objectInfoMap;
    _infoMapKeys=_objectInfoMap.keys();
    endResetModel();
}
void CloudObjectItemModel::receiveObjectDownloadedSlot(QString key,bool isSuccess){
    int index=_infoMapKeys.indexOf(key);
    if(isSuccess){
        setData(createIndex(index,4),TOOL::DOWNLOADED,Qt::UserRole);
    }else{
        setData(createIndex(index,4),TOOL::UNDOWNLOAD,Qt::UserRole);
    }
}
void CloudObjectItemModel::receiveObjectDeletedSlot(QString key,bool isSuccess){
    int index=_infoMapKeys.indexOf(key);
    if(isSuccess){
        beginRemoveRows(QModelIndex(),index,index);
        removeRow(index);
        _infoMapKeys.removeAt(index);
        _objectInfoMap.remove(key);
        endRemoveRows();
    }else{
        setData(createIndex(index,5),TOOL::UNDELETE,Qt::UserRole);
    }
}
void CloudObjectItemModel::receiveObjectBatchDeletedSlot(QStringList successList,QStringList faidList){
    for(int i=0;i<successList.length();i++){
        QString key=successList[i];
        receiveObjectDeletedSlot(key,true);
    }
    for(int j=0;j<faidList.length();j++){
        QString key=faidList[j];
        receiveObjectDeletedSlot(key,false);
    }
}
void CloudObjectItemModel::batchDeleteSlot(){
    QDomDocument doc;
    QDomProcessingInstruction instruction; //添加处理命令
    instruction=doc.createProcessingInstruction("xml","version=\"1.0\" encoding=\"UTF-8\"");
    doc.appendChild(instruction);
    QDomElement root=doc.createElement("Delete");
    QDomElement quiet=doc.createElement("Quiet");
    QDomText text=doc.createTextNode("false");
    quiet.appendChild(text);
    root.appendChild(quiet);
    for(int i=0;i<_infoMapKeys.length();i++){
        TOOL::OBJECT_INFO info=_objectInfoMap[_infoMapKeys[i]];
        if(info.checkable){
            QDomElement object=doc.createElement("Object");
            QDomText key=doc.createTextNode(info.key);
            object.appendChild(key);
            root.appendChild(object);
        }
    }
    doc.appendChild(root);
    obs->batchDeleteObject(doc.toByteArray());
}
void CloudObjectItemModel::batchDownloadSlot(){
    for(int i=0;i<_infoMapKeys.length();i++){
        TOOL::OBJECT_INFO info=_objectInfoMap[_infoMapKeys[i]];
        if(info.checkable){
            obs->getObjectContent(info.key);
        }
    }
}
void CloudObjectItemModel::receiveCloudCheckallableSlot(bool isTrue){
    for(int i=0;i<_infoMapKeys.length();i++){
        QString key=_infoMapKeys[i];
        if(_objectInfoMap[key].visable){
            _objectInfoMap[key].checkable=isTrue;
            QModelIndex index=createIndex(i,0);
            emit dataChanged(index,index);
        }
    }
}
/*@@@@@@@@@@@@@@@@@@@@@@CloudObjectItemDelegate@@@@@@@@@@@@@@@@@@@@@@@@*/
CloudObjectItemDelegate::CloudObjectItemDelegate(QObject* parent):QStyledItemDelegate(parent){
    _pixmapChecked.load(":/Image/Checked.png");
    _pixmapUnchecked.load(":/Image/Unchecked.png");
    _pixmapUndownload.load(":/Image/Undownload.png");
    _pixmapDownloaded.load(":/Image/Downloaded.png");
    _pixmapDownloading.load(":/Image/Downloading.png");
    _pixmapUndelete.load(":/Image/Undelete.png");
}
void CloudObjectItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const{
    QRect wRect=option.rect;
    int column=index.column();
    painter->setRenderHint(QPainter::SmoothPixmapTransform);
    double unit=wRect.height()/(float)10;
    QRectF rect=QRectF(wRect.x()+(wRect.width()-unit*8)/(float)2,unit+wRect.y(),unit*8,unit*8);
    if(column==0){
        bool isChecked=index.model()->data(index,Qt::UserRole).toBool();
        if(isChecked){
            painter->drawPixmap(rect,_pixmapChecked,_pixmapChecked.rect());
        }else{
            painter->drawPixmap(rect,_pixmapUnchecked,_pixmapUnchecked.rect());
        }
    }else if(column==4){
        TOOL::DOWNLOAD_STATUS status=static_cast<TOOL::DOWNLOAD_STATUS>(index.model()->data(index,Qt::UserRole).toInt());
        switch (status) {
        case TOOL::UNDOWNLOAD:
            painter->drawPixmap(rect,_pixmapUndownload,_pixmapUndownload.rect());
            break;
        case TOOL::DOWNLOADING:
            painter->drawPixmap(rect,_pixmapDownloading,_pixmapDownloading.rect());
            break;
        case TOOL::DOWNLOADED:
            painter->drawPixmap(rect,_pixmapDownloaded,_pixmapDownloaded.rect());
            break;
        default:
            break;
        }
    }else if(column==5){
        TOOL::DELETE_STATUS status=static_cast<TOOL::DELETE_STATUS>(index.model()->data(index,Qt::UserRole).toInt());
        switch (status) {
        case TOOL::UNDELETE:
            painter->drawPixmap(rect,_pixmapUndelete,_pixmapUndelete.rect());
            break;
        case TOOL::DELETING:
            painter->drawPixmap(rect,_pixmapDeleting,_pixmapDeleting.rect());
            break;
        default:
            break;
        }
    }else{
        QStyledItemDelegate::paint(painter,option,index);
    }
}
bool CloudObjectItemDelegate::editorEvent(QEvent *event, QAbstractItemModel *model, const QStyleOptionViewItem &option, const QModelIndex &index){
    QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
    if(event->type()==QEvent::MouseButtonPress&&option.rect.contains(mouseEvent->pos())){
        int column=index.column();
        if(column==0){
            bool data = model->data(index, Qt::UserRole).toBool();
            model->setData(index, !data, Qt::UserRole);
        }else if(column==4){
            TOOL::DOWNLOAD_STATUS status=static_cast<TOOL::DOWNLOAD_STATUS>(model->data(index,Qt::UserRole).toInt());
            switch (status) {
            case TOOL::UNDOWNLOAD:
                model->setData(index,TOOL::DOWNLOADING,Qt::UserRole);
                break;
            default:
                break;
            }
        }else if(column==5){
            TOOL::DELETE_STATUS status=static_cast<TOOL::DELETE_STATUS>(model->data(index,Qt::UserRole).toInt());
            switch (status) {
            case TOOL::UNDELETE:
                model->setData(index,TOOL::DELETING,Qt::UserRole);
                break;
            default:
                break;
            }
        }
    }
    return true;
}
/*@@@@@@@@@@@@@@@@@@@@@@CloudObjectProxyModel@@@@@@@@@@@@@@@@@@@@@@@@*/
CloudObjectProxyModel::CloudObjectProxyModel(QObject* parent):QSortFilterProxyModel(parent){
    _searchConditions.insert(_conditions.WEEK,"不限制");
    _searchConditions.insert(_conditions.TIME,"不限制-不限制");
    _searchConditions.insert(_conditions.CUSTOMER,"不限制");
    _searchConditions.insert(_conditions.AREA,"不限制");
    _chineseWeekMap.insert("星期一",1);
    _chineseWeekMap.insert("星期二",2);
    _chineseWeekMap.insert("星期三",3);
    _chineseWeekMap.insert("星期四",4);
    _chineseWeekMap.insert("星期五",5);
    _chineseWeekMap.insert("星期六",6);
    _chineseWeekMap.insert("星期日",7);
    ///////////////////
    _timeStringMap.insert("不限制",-1);
    _timeStringMap.insert("00:00",0);
    _timeStringMap.insert("01:00",1);
    _timeStringMap.insert("02:00",2);
    _timeStringMap.insert("03:00",3);
    _timeStringMap.insert("04:00",4);
    _timeStringMap.insert("05:00",5);
    _timeStringMap.insert("06:00",6);
    _timeStringMap.insert("07:00",7);
    _timeStringMap.insert("08:00",8);
    _timeStringMap.insert("09:00",9);
    _timeStringMap.insert("10:00",10);
    _timeStringMap.insert("11:00",11);
    _timeStringMap.insert("12:00",12);
    _timeStringMap.insert("13:00",13);
    _timeStringMap.insert("14:00",14);
    _timeStringMap.insert("15:00",15);
    _timeStringMap.insert("16:00",16);
    _timeStringMap.insert("17:00",17);
    _timeStringMap.insert("18:00",18);
    _timeStringMap.insert("19:00",19);
    _timeStringMap.insert("20:00",20);
    _timeStringMap.insert("21:00",21);
    _timeStringMap.insert("22:00",22);
    _timeStringMap.insert("23:00",23);
    connect(SIGNALS,SIGNAL(sendCloudSearchConditionSignal(TOOL::CLOUD_SEARCH_TYPE,QString)),this,SLOT(receiveCloudSearchConditionSlot(TOOL::CLOUD_SEARCH_TYPE,QString)));
}
bool CloudObjectProxyModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const{
    CloudObjectItemModel* _sourceModel=static_cast<CloudObjectItemModel*>(sourceModel());
    QModelIndex index=_sourceModel->index(source_row,0,source_parent);
    TOOL::OBJECT_INFO info=_sourceModel->objectInfo(index);
    if(!_checkWeek(info)||!_checkTime(info)||!_checkCustomerAddress(info)||!_checkArea(info)){
        _sourceModel->setData(index,false,Qt::DecorationRole);
        return false;
    }else{
        _sourceModel->setData(index,true,Qt::DecorationRole);
        return true;
    }
    return true;
}
void CloudObjectProxyModel::receiveCloudSearchConditionSlot(TOOL::CLOUD_SEARCH_TYPE type,QString str){
    switch (type) {
    case TOOL::CLOUD_AREA_ID:
        _searchConditions[_conditions.AREA]=str;
        break;
    case TOOL::CLOUD_UPDATE_WEEK:
        _searchConditions[_conditions.WEEK]=str;
        break;
    case TOOL::CLOUD_UPDATE_TIME:
        _searchConditions[_conditions.TIME]=str;
        break;
    case TOOL::CLOUD_CUSTOMER_ADDRESS:
        _searchConditions[_conditions.CUSTOMER]=str;
        break;
    default:
        break;
    }
    invalidateFilter();
}
bool CloudObjectProxyModel::_checkWeek(TOOL::OBJECT_INFO info)const{
    if(_searchConditions[_conditions.WEEK]!="不限制"){
        QString updateWeekStr=QDateTime::fromMSecsSinceEpoch(info.updateTime).toString("dddd");
        if(updateWeekStr.contains("星期")){
            quint8 now=_chineseWeekMap[updateWeekStr];
            if(_searchConditions[_conditions.WEEK].contains("～")){
                QStringList updateWeekStrList=_searchConditions[_conditions.WEEK].split("～");
                quint8 start=_chineseWeekMap[updateWeekStrList[0]];
                quint8 end=_chineseWeekMap[updateWeekStrList[1]];
                if(now<start||now>end){
                    return false;
                }
            }else{
                if(updateWeekStr!=_searchConditions[_conditions.WEEK]){
                    return false;
                }
            }
        }else{
            //英文星期
        }
    }
    return true;
}
bool CloudObjectProxyModel::_checkTime(TOOL::OBJECT_INFO info)const{
    if(!_searchConditions[_conditions.TIME].contains("不限制")){
        QStringList strList=_searchConditions[_conditions.TIME].split("-");
        QString startStr=strList[0];
        QString endStr=strList[1];
        QString nowStr=QDateTime::fromMSecsSinceEpoch(info.updateTime).toString("hh:00");
        quint8 now=_timeStringMap[nowStr];
        quint8 start=_timeStringMap[startStr];
        quint8 end=_timeStringMap[endStr];
        if(start<end){
            if(now<start||now>=end){
                return false;
            }
        }else if(start>end){
            if(now<start||now>=end){
                return false;
            }
        }
    }
    return true;
}
bool CloudObjectProxyModel::_checkCustomerAddress(TOOL::OBJECT_INFO info)const{
    if(_searchConditions[_conditions.CUSTOMER]!="不限制"){
        if(_searchConditions[_conditions.CUSTOMER]!=info.customerAddress){
            return false;
        }
    }
    return true;
}
bool CloudObjectProxyModel::_checkArea(TOOL::OBJECT_INFO info)const{
    if(_searchConditions[_conditions.AREA]!="不限制"){
        if(_searchConditions[_conditions.AREA]!=info.areaId&&!info.areaId.isEmpty()){
            return false;
        }
    }
    return true;
}
/*@@@@@@@@@@@@@@@@@@@@@@CloudObjectItemView@@@@@@@@@@@@@@@@@@@@@@@@*/
CloudObjectItemView::CloudObjectItemView(QWidget* parent):QTableView(parent){
    _objectModel=new CloudObjectItemModel(this);
    _objectDelegate=new CloudObjectItemDelegate(this);
    _objectProxyModel=new CloudObjectProxyModel(this);
    _objectProxyModel->setSourceModel(_objectModel);
    setModel(_objectProxyModel);
    setItemDelegate(_objectDelegate);
    setAlternatingRowColors(true);
    horizontalHeader()->setStretchLastSection(true);
    setColumnWidth(0,57);
    setColumnWidth(4,57);
    setColumnWidth(5,57);
    hideColumn(0);
    hideColumn(5);
    connect(SIGNALS,SIGNAL(batchDealSignal(bool)),this,SLOT(batchDealSlot(bool)));
    connect(SIGNALS,SIGNAL(deleteColumnSignal(bool)),this,SLOT(deleteColumnSlot(bool)));
}
void CloudObjectItemView::resizeEvent(QResizeEvent *e){
    QTableView::resizeEvent(e);
    _resizeColoumWidth();
}
void CloudObjectItemView::_resizeColoumWidth(){
    int w=width()-verticalHeader()->width()-2-6;
    if(!isColumnHidden(0)){
        setColumnWidth(0,57);
        w-=57;
    }
    if(!isColumnHidden(5)){
        setColumnWidth(5,57);
        w-=57;
    }
    setColumnWidth(4,57);
    w-=57;
    setColumnWidth(1,w*0.4);
    setColumnWidth(2,w*0.4);
    setColumnWidth(3,w*0.2);
}
void CloudObjectItemView::batchDealSlot(bool isShow){
    if(isShow){
        showColumn(0);
    }else{
        hideColumn(0);
    }
    _resizeColoumWidth();
}
void CloudObjectItemView::deleteColumnSlot(bool isShow){
    if(isShow){
        showColumn(5);
    }else{
        hideColumn(5);
    }
    _resizeColoumWidth();
}
/*@@@@@@@@@@@@@@@@@@@@@@CloudSearchConditionBox@@@@@@@@@@@@@@@@@@@@@@@@*/
CloudSearchConditionBox::CloudSearchConditionBox(QWidget* parent):QFrame(parent){
    _titleLabel=new QLabel(this);
    _comboBox=new QComboBox(this);
    _hLayout=new QHBoxLayout(this);
    _hLayout->setContentsMargins(0,0,0,0);
    _hLayout->setSpacing(20);
    _hLayout->addWidget(_titleLabel);
    _hLayout->addWidget(_comboBox);
    _hLayout->setStretch(0,1);
    _hLayout->setStretch(1,99);
    setLayout(_hLayout);
    connect(_comboBox,SIGNAL(currentIndexChanged(int)),this,SLOT(currentIndexChangedSlot(int)));
    connect(SIGNALS,SIGNAL(addCloudSearchConditionSignal(TOOL::CLOUD_SEARCH_TYPE,QString)),this,SLOT(addCloudSearchConditionSlot(TOOL::CLOUD_SEARCH_TYPE,QString)));
}
void CloudSearchConditionBox::setSearchType(TOOL::CLOUD_SEARCH_TYPE searchType){
    _searchType=searchType;
    switch (searchType) {
    case TOOL::CLOUD_AREA_ID:
        _stringList=GLOBAL->CloudAreaIDList;
        _comboBox->addItems(GLOBAL->CloudAreaIDList);
        break;
    case TOOL::CLOUD_UPDATE_WEEK:
        _stringList=GLOBAL->CloudWeekList;
        _comboBox->addItems(GLOBAL->CloudWeekList);
        break;
    case TOOL::CLOUD_CUSTOMER_ADDRESS:
        _stringList=GLOBAL->CloudCustomerAddressList;
        _comboBox->addItems(GLOBAL->CloudCustomerAddressList);
        break;
    default:
        break;
    }
}
void CloudSearchConditionBox::currentIndexChangedSlot(int index){
    if(index>=0&&index<_stringList.length()){
        emit SIGNALS->sendCloudSearchConditionSignal(_searchType,_stringList[index]);
    }
}
void CloudSearchConditionBox::addCloudSearchConditionSlot(TOOL::CLOUD_SEARCH_TYPE searchType,QString str){
    if(searchType==_searchType){
        _stringList.append(str);
        _comboBox->addItem(str);
    }
}
/*@@@@@@@@@@@@@@@@@@@@@@CloudSearchConditionTimeBox@@@@@@@@@@@@@@@@@@@@@@@@*/
CloudSearchConditionTimeBox::CloudSearchConditionTimeBox(QWidget* parent):QFrame(parent){
    _startLabel=new QLabel(this);
    _startLabel->setText("开始时间");
    _endLabel=new QLabel(this);
    _endLabel->setText("结束时间");
    _startComboBox=new QComboBox(this);
    _startComboBox->addItems(GLOBAL->CloudTimeList);
    _endComboBox=new QComboBox(this);
    _endComboBox->addItems(GLOBAL->CloudTimeList);
    _hLayout=new QHBoxLayout(this);
    _hLayout->setContentsMargins(0,0,0,0);
    _hLayout->setSpacing(20);
    _hLayout->addWidget(_startLabel);
    _hLayout->addWidget(_startComboBox);
    _hLayout->addWidget(_endLabel);
    _hLayout->addWidget(_endComboBox);
    _hLayout->setStretch(0,1);
    _hLayout->setStretch(1,99);
    _hLayout->setStretch(2,1);
    _hLayout->setStretch(3,99);
    setLayout(_hLayout);
    connect(_startComboBox,SIGNAL(currentIndexChanged(int)),this,SLOT(currentIndexChangedSlot(int)));
    connect(_endComboBox,SIGNAL(currentIndexChanged(int)),this,SLOT(currentIndexChangedSlot(int)));
}
void CloudSearchConditionTimeBox::currentIndexChangedSlot(int){
    QString startStr=GLOBAL->TimeStringList[_startComboBox->currentIndex()];
    QString endStr=GLOBAL->TimeStringList[_endComboBox->currentIndex()];
    emit SIGNALS->sendCloudSearchConditionSignal(TOOL::CLOUD_UPDATE_TIME,QString("%1-%2").arg(startStr).arg(endStr));
};
/*@@@@@@@@@@@@@@@@@@@@@@CloudSearcher@@@@@@@@@@@@@@@@@@@@@@@@*/
CloudSearcher::CloudSearcher(QWidget* parent):QFrame(parent){
    _areaIDBox=new CloudSearchConditionBox(this);
    _areaIDBox->setTitle("小区编号");
    _areaIDBox->setSearchType(TOOL::CLOUD_AREA_ID);
    _weekBox=new CloudSearchConditionBox(this);
    _weekBox->setTitle("上传星期");
    _weekBox->setSearchType(TOOL::CLOUD_UPDATE_WEEK);
    _timeBox=new CloudSearchConditionTimeBox(this);
    _customerAddressBox=new CloudSearchConditionBox(this);
    _customerAddressBox->setTitle("用户楼栋");
    _customerAddressBox->setSearchType(TOOL::CLOUD_CUSTOMER_ADDRESS);
    _vLayout=new QVBoxLayout(this);
    _vLayout->setContentsMargins(5,5,5,5);
    _vLayout->setSpacing(5);
    _vLayout->addWidget(_areaIDBox);
    _vLayout->addWidget(_weekBox);
    _vLayout->addWidget(_timeBox);
    _vLayout->addWidget(_customerAddressBox);
    setLayout(_vLayout);
}
/*@@@@@@@@@@@@@@@@@@@@@@CloudDealer@@@@@@@@@@@@@@@@@@@@@@@@*/
CloudDealer::CloudDealer(QWidget* parent):QFrame(parent){
    _refreshBuketButton=new QPushButton(this);
    _refreshBuketButton->setText("刷新OBS列表");
    _batchDealButton=new QPushButton(this);
    _batchDealButton->setText("批量处理");
    _batchDeleteButton=new QPushButton(this);
    _batchDeleteButton->setText("批量删除");
    _batchDownloadButton=new QPushButton(this);
    _batchDownloadButton->setText("批量下载");
    _deleteButton=new QPushButton(this);
    _deleteButton->setText("显示删除");
    _checkallButton=new QPushButton(this);
    _checkallButton->setText("全选");
    _uncheckallButton=new QPushButton(this);
    _uncheckallButton->setText("全不选");
    _layout=new QGridLayout(this);
    _layout->setContentsMargins(5,5,5,5);
    _layout->setSpacing(5);
    _layout->addWidget(_refreshBuketButton,0,0,1,2);
    _layout->addWidget(_batchDealButton,1,0,1,1);
    _layout->addWidget(_deleteButton,1,1,1,1);
    _layout->addWidget(_checkallButton,2,0,1,1);
    _layout->addWidget(_uncheckallButton,2,1,1,1);
    _layout->addWidget(_batchDownloadButton,3,0,1,1);
    _layout->addWidget(_batchDeleteButton,3,1,1,1);
    setLayout(_layout);
    _checkallButton->hide();
    _uncheckallButton->hide();
    _batchDeleteButton->hide();
    _batchDownloadButton->hide();
    connect(_batchDealButton,SIGNAL(clicked()),this,SLOT(batchDealButtonClickedSlot()));
    connect(_deleteButton,SIGNAL(clicked()),this,SLOT(deleteButtonClickedSlot()));
    connect(_refreshBuketButton,&QPushButton::clicked,[](){obs->enumerateObjectsInBucket();});
    connect(_batchDeleteButton,&QPushButton::clicked,[](){emit SIGNALS->batchDeleteSignal();});
    connect(_batchDownloadButton,&QPushButton::clicked,[](){emit SIGNALS->batchDownloadSignal();});
    connect(_checkallButton,&QPushButton::clicked,[](){emit SIGNALS->sendCloudCheckallableSignal(true);});
    connect(_uncheckallButton,&QPushButton::clicked,[](){emit SIGNALS->sendCloudCheckallableSignal(false);});
}
void CloudDealer::batchDealButtonClickedSlot(){
    if(_batchDealButton->text()=="批量处理"){
        emit SIGNALS->batchDealSignal(true);
        _batchDeleteButton->show();
        _batchDownloadButton->show();
        _checkallButton->show();
        _uncheckallButton->show();
        _batchDealButton->setText("取消批量");
    }else{
        emit SIGNALS->batchDealSignal(false);
        _batchDeleteButton->hide();
        _batchDownloadButton->hide();
        _checkallButton->hide();
        _uncheckallButton->hide();
        _batchDealButton->setText("批量处理");
    }
}
void CloudDealer::deleteButtonClickedSlot(){
    if(_deleteButton->text()=="显示删除"){
        emit SIGNALS->deleteColumnSignal(true);
        _deleteButton->setText("隐藏删除");
    }else{
        emit SIGNALS->deleteColumnSignal(false);
        _deleteButton->setText("显示删除");
    }
}
/*@@@@@@@@@@@@@@@@@@@@@@MapLoader@@@@@@@@@@@@@@@@@@@@@@@@*/
MapLoader::MapLoader(QWidget* parent):QFrame(parent){
    _cloudDealer=new CloudDealer(this);
    _cloudSearcher=new CloudSearcher(this);
    _spaceFrame=new QFrame(this);
    _objectView=new CloudObjectItemView(this);
    _layout=new QGridLayout(this);
    _layout->setContentsMargins(0,0,0,0);
    _layout->setVerticalSpacing(20);
    _layout->setHorizontalSpacing(0);
    _layout->addWidget(_cloudSearcher,0,0,1,1);
    _layout->addWidget(_cloudDealer,1,0,1,1);
    _layout->addWidget(_spaceFrame,2,0,1,1);
    _layout->addWidget(_objectView,0,1,3,1);
    _layout->setRowStretch(0,1);
    _layout->setRowStretch(1,1);
    _layout->setRowStretch(2,98);
    _layout->setColumnStretch(0,1);
    _layout->setColumnStretch(1,99);
    setLayout(_layout);
}
/*@@@@@@@@@@@@@@@@@@@@@@MapUpdater@@@@@@@@@@@@@@@@@@@@@@@@*/
MapUpdater::MapUpdater(QWidget* parent):QTabWidget(parent){
    _holder=new MapHolder(this);
    _loader=new MapLoader(this);
    addTab(_holder,"地图处理");
    addTab(_loader,"数据下载");
}
