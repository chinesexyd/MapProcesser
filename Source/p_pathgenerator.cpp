#include "p_pathgenerator.h"

/*@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@DataDealer@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@*/
DataDealer::DataDealer(QObject* parent):QObject(parent){}
void DataDealer::setMapDir(QString mapDir){
    QDir dir(mapDir);
    if(dir.exists()){
        _mapDir=mapDir;
        QFileInfoList infoList=dir.entryInfoList(QDir::Files|QDir::NoDotAndDotDot);
        foreach (QFileInfo info, infoList) {
            if(info.suffix()=="pgm"){
                emit setMapSignal(info.filePath());
            }else if(info.suffix()=="yaml"){
                YAML::Node yaml = YAML::LoadFile(info.filePath().toStdString());
                _mapInfo.resolution = yaml["resolution"].as<double>();
                _mapInfo.originX = yaml["origin"][0].as<double>();
                _mapInfo.originY = yaml["origin"][1].as<double>();
            }else if(info.fileName()=="path.json"){
                emit loadPolygonPathSignal(info.filePath());
            }
        }
    }
}
QString DataDealer::mapDir(){
    return _mapDir;
}
void DataDealer::setActionType(DataDealer::ACTION_TYPE type){
    _actionType=type;
    emit setActionTypeSignal(type);
}
void DataDealer::setMapInfo(quint16 width,quint16 height){
    _mapInfo.width=width;
    _mapInfo.height=height;
}
DataDealer::ACTION_TYPE DataDealer::actionType(){
    return _actionType;
}
QPointF DataDealer::realPosFromMapPos(QPointF pos){
    double x=(pos.x()-_mapInfo.originX)/_mapInfo.resolution;
    double y=_mapInfo.height-((pos.y()-_mapInfo.originY)/_mapInfo.resolution);
    return QPointF(x,y);
}
QPointF DataDealer::mapPosFromRealPos(QPointF pos){
    double x=pos.x()*_mapInfo.resolution+_mapInfo.originX;
    double y=(_mapInfo.height-pos.y())*_mapInfo.resolution+_mapInfo.originY;
    return QPointF(x,y);
}
bool DataDealer::_isPointInPolygon(QList<QPointF> polygon,QPointF pos){
    quint32 count=0;//计算交点个数
    for(int i=0;i<polygon.length();i++){
        QPointF spos=polygon[i];
        QPointF epos;
        if(i==polygon.length()-1){
            epos=polygon[0];
        }else{
            epos=polygon[i+1];
        }
        double sx=spos.x();
        double sy=spos.y();
        double ex=epos.x();
        double ey=epos.y();
        double x=pos.x();
        double y=pos.y();
        if(sy==ey||y<qMin(sy,ey)||y>=qMax(sy,ey)){
            continue;
        }
        double qx=(y-sy)*(ex-sx)/(ey-sy)+sx;
        if(qx>x){
            count++;
        }
    }
    if(count%2==1){
        return true;
    }else{
        return false;
    }
}
void DataDealer::_createSweepPath(QList<QPointF> &sweepPath,QList<QPointF> polygon,double pathDistance){
    QList<QPointF> innerPolygon=getInnerPolygon(polygon,pathDistance);
    if(innerPolygon.length()>0){
        QPointF pos=innerPolygon.last();
        innerPolygon.insert(0,pos);
        innerPolygon.removeLast();
        sweepPath.append(innerPolygon);
        sweepPath.append(innerPolygon[0]);
        _createSweepPath(sweepPath,innerPolygon,pathDistance);
    }
}
QList<QPointF> DataDealer::createSweepPath(QList<QPointF> polygon,double pathDistance,PATH_TYPE type){
    Q_UNUSED(type);
    QPointF pos=polygon.last();
    polygon.insert(0,pos);
    polygon.removeLast();
    QList<QPointF> sweepPath;
    sweepPath<<polygon<<polygon[0];
    _createSweepPath(sweepPath,polygon,pathDistance);
    return sweepPath;
}
QList<QPointF> DataDealer::getInnerPolygon(QList<QPointF> polygon,double distance){
    QList<double> kList;
    QList<double> dbList;
    QList<double> tbList;
    for(int i=0;i<polygon.length();i++){
        QPointF startPos;
        QPointF endPos;
        if(i==polygon.length()-1){
            startPos=polygon[i];
            endPos=polygon[0];
        }else{
            startPos=polygon[i];
            endPos=polygon[i+1];
        }
        double sx=startPos.x();
        double sy=startPos.y();
        double ex=endPos.x();
        double ey=endPos.y();
        double dx=qMax(sx,ex)-qMin(sx,ex);
        double dy=qMax(sy,ey)-qMin(sy,ey);
        double dl=sqrt(dx*dx+dy*dy);
        double db=distance*dl/dx;
        double k=(sy-ey)/(sx-ex);
        double tb=ey-k*ex;
        kList.append(k);
        dbList.append(db);
        tbList.append(tb);
    }
    QList<QPointF> innerPolygonPointList;
    for(int j=0;j<kList.length();j++){
        double k1;
        double k2;
        double db1;
        double db2;
        double tb1;
        double tb2;
        if(j==kList.length()-1){
            k1=kList[j];
            db1=dbList[j];
            tb1=tbList[j];
            k2=kList[0];
            db2=dbList[0];
            tb2=tbList[0];
        }else{
            k1=kList[j];
            db1=dbList[j];
            tb1=tbList[j];
            k2=kList[j+1];
            db2=dbList[j+1];
            tb2=tbList[j+1];
        }
        double x1=((tb2+db2)-(tb1+db1))/(k1-k2);
        double y1=k1*x1+tb1+db1;
        QPointF p1=QPointF(x1,y1);
        if(_isPointInPolygon(polygon,p1)){
            innerPolygonPointList.append(p1);
            continue;
        }
        double x2=((tb2+db2)-(tb1-db1))/(k1-k2);
        double y2=k1*x2+tb1-db1;
        QPointF p2=QPointF(x2,y2);
        if(_isPointInPolygon(polygon,p2)){
            innerPolygonPointList.append(p2);
            continue;
        }
        double x3=((tb2-db2)-(tb1-db1))/(k1-k2);
        double y3=k1*x3+tb1-db1;
        QPointF p3=QPointF(x3,y3);
        if(_isPointInPolygon(polygon,p3)){
            innerPolygonPointList.append(p3);
            continue;
        }
        double x4=((tb2-db2)-(tb1+db1))/(k1-k2);
        double y4=k1*x4+tb1+db1;
        QPointF p4=QPointF(x4,y4);
        if(_isPointInPolygon(polygon,p4)){
            innerPolygonPointList.append(p4);
            continue;
        }
    }
    return  innerPolygonPointList;
}
DataDealer* dataDealer=new DataDealer();
/*@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@GeneratorPathLine@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@*/
GeneratorPathLine::GeneratorPathLine(QGraphicsItem* parent):QGraphicsObject(parent){
    setFlags(ItemIsFocusable);
    setAcceptedMouseButtons(Qt::LeftButton);
    setAcceptHoverEvents(true);
    setZValue(0);
}
void GeneratorPathLine::setStartPos(QPointF pos){
    _startPos=pos;
    if(_endPos.x()!=0&&_endPos.y()!=0){
        update();
    }
}
void GeneratorPathLine::setEndPos(QPointF pos){
    _endPos=pos;
    if(_endPos.x()!=0&&_endPos.y()!=0){
        update();
    }
}
int GeneratorPathLine::type() const{
    return Type;
}
QRectF GeneratorPathLine::boundingRect() const{
    double xmin=qMin(_startPos.x(),_endPos.x());
    double ymin=qMin(_startPos.y(),_endPos.y());
    double xmax=qMax(_startPos.x(),_endPos.x());
    double ymax=qMax(_startPos.y(),_endPos.y());
    return QRectF(xmin,ymin,xmax-xmin,ymax-ymin);
}
void GeneratorPathLine::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget){
    Q_UNUSED(option);
    Q_UNUSED(widget);
    painter->save();
    QPen pen;
    pen.setColor("#6639a6");
    pen.setWidth(1);
    pen.setStyle(Qt::DashLine);
    painter->setPen(pen);
    painter->setBrush(Qt::NoBrush);
    painter->drawLine(_startPos,_endPos);
    painter->restore();
}
/*@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@GeneratorPathPoint@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@*/
GeneratorPathPoint::GeneratorPathPoint(QGraphicsItem* parent):QGraphicsObject(parent){
    setFlags(ItemIsFocusable);
    setAcceptedMouseButtons(Qt::LeftButton);
    setAcceptHoverEvents(true);
    setZValue(5);
    connect(this,&QGraphicsObject::xChanged,[this](){emit posChangedSignal(this);});
    connect(this,&QGraphicsObject::zChanged,[this](){emit posChangedSignal(this);});
}
int GeneratorPathPoint::type() const{
    return Type;
}
QRectF GeneratorPathPoint::boundingRect() const{
    return QRectF(-2,-2,4,4);
}
void GeneratorPathPoint::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget){
    Q_UNUSED(option);
    Q_UNUSED(widget);
    painter->save();
    painter->setPen(Qt::NoPen);
    painter->setBrush(QColor("#e23e57"));
    painter->drawEllipse(boundingRect());
    painter->restore();
}
/*@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@GeneratorPolygonPoint@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@*/
GeneratorPolygonLine::GeneratorPolygonLine(QGraphicsItem* parent):QGraphicsObject(parent){
    setFlags(ItemIsFocusable);
    setAcceptedMouseButtons(Qt::LeftButton);
    setAcceptHoverEvents(true);
    setZValue(4);
}
void GeneratorPolygonLine::setStartPos(QPointF pos){
    _startPos=pos;
    if(_endPos.x()!=0&&_endPos.y()!=0){
        update();
    }
}
void GeneratorPolygonLine::setEndPos(QPointF pos){
    _endPos=pos;
    if(_endPos.x()!=0&&_endPos.y()!=0){
        update();
    }
}
int GeneratorPolygonLine::type() const{
    return Type;
}
QRectF GeneratorPolygonLine::boundingRect() const{
    double xmin=qMin(_startPos.x(),_endPos.x());
    double ymin=qMin(_startPos.y(),_endPos.y());
    double xmax=qMax(_startPos.x(),_endPos.x());
    double ymax=qMax(_startPos.y(),_endPos.y());
    return QRectF(xmin,ymin,xmax-xmin,ymax-ymin);
}
void GeneratorPolygonLine::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget){
    Q_UNUSED(option);
    Q_UNUSED(widget);
    painter->save();
    QPen pen;
    pen.setColor("#6639a6");
    if(_isHoverIn){
        pen.setWidth(2);
    }else{
        pen.setWidth(1);
    }
    painter->setPen(pen);
    painter->setBrush(Qt::NoBrush);
    painter->drawLine(_startPos,_endPos);
    painter->restore();
}
void GeneratorPolygonLine::hoverEnterEvent(QGraphicsSceneHoverEvent *event){
    Q_UNUSED(event);
    if(dataDealer->actionType()==DataDealer::MODIFY_POLYGON){
        _isHoverIn=false;
        update();
    }
}
void GeneratorPolygonLine::hoverLeaveEvent(QGraphicsSceneHoverEvent *event){
    Q_UNUSED(event);
    _isHoverIn=false;
    update();
}
/*@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@GeneratorPolygonPoint@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@*/
GeneratorPolygonPoint::GeneratorPolygonPoint(QGraphicsItem* parent):QGraphicsObject(parent){
    setAcceptedMouseButtons(Qt::LeftButton);
    setAcceptHoverEvents(true);
    setZValue(5);
    connect(this,&QGraphicsObject::xChanged,[this](){emit posChangedSignal(this);});
    connect(this,&QGraphicsObject::zChanged,[this](){emit posChangedSignal(this);});
}
int GeneratorPolygonPoint::type() const{
    return Type;
}
QRectF GeneratorPolygonPoint::boundingRect() const{
    return QRectF(-5,-5,10,10);
}
void GeneratorPolygonPoint::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget){
    Q_UNUSED(option);
    Q_UNUSED(widget);
    painter->save();
    QPen pen;
    pen.setColor("#6639a6");
    if(_isHoverIn){
        pen.setWidth(2);
    }else{
        pen.setWidth(1);
    }
    painter->setPen(pen);
    painter->setBrush(QColor("#e23e57"));
    painter->drawRoundedRect(boundingRect(),1,1);
    painter->restore();
}
void GeneratorPolygonPoint::hoverEnterEvent(QGraphicsSceneHoverEvent *event){
    Q_UNUSED(event);
    if(dataDealer->actionType()==DataDealer::MODIFY_POLYGON){
        _isHoverIn=true;
        update();
    }
}
void GeneratorPolygonPoint::hoverLeaveEvent(QGraphicsSceneHoverEvent *event){
    Q_UNUSED(event);
    _isHoverIn=false;
    update();
}
/*@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@GeneratorMap@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@*/
GeneratorMap::GeneratorMap(QGraphicsItem* parent):QGraphicsPixmapItem(parent){
    setZValue(-1);
    setFlags(ItemIsMovable);
}
void GeneratorMap::setMapData(QByteArray bytes){
    _pixmap.loadFromData(bytes);
    dataDealer->setMapInfo(_pixmap.width(),_pixmap.height());
    setPixmap(_pixmap);
}
/*@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@GeneratorScene@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@*/
GeneratorScene::GeneratorScene(QWidget* parent):QGraphicsScene(parent){
    setBackgroundBrush(QBrush("#393e46"));
    _polygonPointMenu=new QMenu(parent);
    _delPolygonPointAction=new QAction(_polygonPointMenu);
    _delPolygonPointAction->setText("删除描点");
    _polygonPointMenu->addAction(_delPolygonPointAction);
    _polygonLineMenu=new QMenu(parent);
    _addPolygonPointAction=new QAction(_polygonLineMenu);
    _addPolygonPointAction->setText("添加描点");
    _polygonLineMenu->addAction(_addPolygonPointAction);
    _pathPointMenu=new QMenu(parent);
    _addPathPointAction=new QAction(_pathPointMenu);
    _addPathPointAction->setText("添加描点");
    _delPathPointAction=new QAction(_pathPointMenu);
    _delPathPointAction->setText("删除描点");
    _pathPointMenu->addAction(_addPathPointAction);
    _pathPointMenu->addAction(_delPathPointAction);
    connect(dataDealer,SIGNAL(setMapSignal(QString)),this,SLOT(setMapSlot(QString)));
    connect(dataDealer,SIGNAL(loadPolygonPathSignal(QString)),this,SLOT(loadPolygonPathSlot(QString)));
    connect(dataDealer,SIGNAL(setRadiusDistanceSignal(double,double)),this,SLOT(setRadiusDistanceSlot(double,double)));
    connect(dataDealer,SIGNAL(savePolygonPathSignal()),this,SLOT(savePolygonPathSlot()));
    connect(_delPolygonPointAction,SIGNAL(triggered()),this,SLOT(delPolygonPointActionSlot()));
    connect(_addPolygonPointAction,SIGNAL(triggered()),this,SLOT(addPolygonPointActionSlot()));
    connect(_delPathPointAction,SIGNAL(triggered()),this,SLOT(delPathPointActionSlot()));
    connect(_addPathPointAction,SIGNAL(triggered()),this,SLOT(addPathPointActionSlot()));
}
GeneratorScene::~GeneratorScene(){
    if(_mapItem!=nullptr){
        _mapItem->deleteLater();
    }
}
void GeneratorScene::contextMenuEvent(QGraphicsSceneContextMenuEvent *event){
    QTransform transform;
    QGraphicsItem* item=itemAt(event->scenePos(),transform);
    _cursorPos=event->scenePos();
    if(item==NULL){
        return;
    }
    switch (item->type()) {
    case GeneratorMap::Type:
        break;
    case GeneratorPolygonPoint::Type:
        if(dataDealer->actionType()==DataDealer::MODIFY_POLYGON){
            _ptr._polygonPointPtr=static_cast<GeneratorPolygonPoint*>(item);
            _polygonPointMenu->exec(event->screenPos());
        }
        break;
    case GeneratorPolygonLine::Type:
        if(dataDealer->actionType()==DataDealer::MODIFY_POLYGON){
            _ptr._polygonLinePtr=static_cast<GeneratorPolygonLine*>(item);
            _polygonLineMenu->exec(event->screenPos());
        }
        break;
    case GeneratorPathPoint::Type:
        if(dataDealer->actionType()==DataDealer::MODIFY_PATH){
            _ptr._pathPointPtr=static_cast<GeneratorPathPoint*>(item);
            _pathPointMenu->exec(event->screenPos());
        }
        break;
    case GeneratorPathLine::Type:
        _ptr._pathLinePtr=static_cast<GeneratorPathLine*>(item);
        break;
    default:
        break;
    }
}
void GeneratorScene::setActionType(DataDealer::ACTION_TYPE type){
    _actionType=type;
    switch (type) {
    case DataDealer::OPEN_MAPDIR:

        break;
    case DataDealer::DRAW_POLYGON:
        foreach (GeneratorPolygonPoint* point, _polygonPointList) {
            point->setFlags(QGraphicsItem::ItemIsSelectable);
        }
        foreach (GeneratorPathPoint* point, _pathPointList) {
            point->setFlags(QGraphicsItem::ItemIsSelectable);
        }
        break;
    case DataDealer::MODIFY_POLYGON:
        foreach (GeneratorPolygonPoint* point, _polygonPointList) {
            point->setFlags(QGraphicsItem::ItemIsMovable);
        }
        foreach (GeneratorPathPoint* point, _pathPointList) {
            point->setFlags(QGraphicsItem::ItemIsSelectable);
        }
        break;
    case DataDealer::CREATE_PATH:
        foreach (GeneratorPathPoint* point, _pathPointList) {
            point->setFlags(QGraphicsItem::ItemIsSelectable);
        }
        foreach (GeneratorPolygonPoint* point, _polygonPointList) {
            point->setFlags(QGraphicsItem::ItemIsSelectable);
        }
        break;
    case DataDealer::MODIFY_PATH:
        foreach (GeneratorPathPoint* point, _pathPointList) {
            point->setFlags(QGraphicsItem::ItemIsMovable);
        }
        foreach (GeneratorPolygonPoint* point, _polygonPointList) {
            point->setFlags(QGraphicsItem::ItemIsSelectable);
        }
        break;
    default:
        break;
    }
}
void GeneratorScene::setRadiusDistanceSlot(double radius,double distance){
    //清理上次
    foreach(GeneratorPathPoint* point, _pathPointList){
        point->deleteLater();
    }
    foreach(GeneratorPathLine* line, _pathLineList) {
        line->deleteLater();
    }
    _pathPointList.clear();
    _pathLineList.clear();
    //获取多边形端点
    QList<QPointF> pointList;
    foreach (GeneratorPolygonPoint* point, _polygonPointList) {
        pointList<<point->pos();
    }
    QList<QPointF> list=dataDealer->getInnerPolygon(pointList,radius);
    if(list.length()>=3){
        QList<QPointF> lista=dataDealer->createSweepPath(list,distance);
        for(int i=0;i<lista.length()-1;i++){
            GeneratorPathLine* line=new GeneratorPathLine(_mapItem);
            line->setStartPos(lista[i]);
            line->setEndPos(lista[i+1]);
            _pathLineList.append(line);
            GeneratorPathPoint* point=new GeneratorPathPoint(_mapItem);
            point->setPos(lista[i]);
            connect(point,SIGNAL(posChangedSignal(GeneratorPathPoint*)),this,SLOT(pathPointPosChangedSlot(GeneratorPathPoint*)));
            _pathPointList.append(point);
        }
        GeneratorPathPoint* point=new GeneratorPathPoint(_mapItem);
        point->setPos(lista.last());
        connect(point,SIGNAL(posChangedSignal(GeneratorPathPoint*)),this,SLOT(pathPointPosChangedSlot(GeneratorPathPoint*)));
        _pathPointList.append(point);
    }
}
void GeneratorScene::setPolygonPoint(QPointF scenepos){
    if(_mapItem!=nullptr){
        QPointF pos=_mapItem->mapFromScene(scenepos);
        if(!_isPolygonEnd){
            quint16 len=_polygonPointList.length();
            if(len==0){
                GeneratorPolygonPoint* point=new GeneratorPolygonPoint(_mapItem);
                _polygonPointList.append(point);
                point->setPos(pos);
                point->setFlags(QGraphicsItem::ItemIsSelectable);
                connect(point,SIGNAL(posChangedSignal(GeneratorPolygonPoint*)),this,SLOT(polygonPointPosChangedSlot(GeneratorPolygonPoint*)));
                _startPosRectF=QRectF(pos.x()-5,pos.y()-5,10,10);
            }else{
                GeneratorPolygonLine* line=new GeneratorPolygonLine(_mapItem);
                _polygonLineList.append(line);
                if(len>2&&_startPosRectF.contains(pos)){
                    line->setStartPos(_polygonPointList[len-1]->pos());
                    line->setEndPos(_polygonPointList[0]->pos());
                    _isPolygonEnd=true;
                }else{
                    line->setStartPos(_polygonPointList[len-1]->pos());
                    line->setEndPos(pos);
                    GeneratorPolygonPoint* point=new GeneratorPolygonPoint(_mapItem);
                    _polygonPointList.append(point);
                    point->setPos(pos);
                    point->setFlags(QGraphicsItem::ItemIsSelectable);
                    connect(point,SIGNAL(posChangedSignal(GeneratorPolygonPoint*)),this,SLOT(polygonPointPosChangedSlot(GeneratorPolygonPoint*)));
                }
            }
        }
    }
}
void GeneratorScene::setMapSlot(QString mapPath){
    if(mapPath!=_mapPath){
        QFile file(mapPath);
        if(file.exists()){
            file.open(QIODevice::ReadOnly);
            QByteArray mapBytes=file.readAll();
            file.close();
            if(_mapItem!=nullptr){
                _mapItem->deleteLater();
                _mapItem=nullptr;
            }
            _mapItem=new GeneratorMap();
            _mapItem->setMapData(mapBytes);
            addItem(_mapItem);
        }
    }
}
void GeneratorScene::loadPolygonPathSlot(QString pathPath){
    foreach (GeneratorPolygonPoint* point, _polygonPointList) {
        point->deleteLater();
    }
    _polygonPointList.clear();
    foreach (GeneratorPolygonLine* line, _polygonLineList) {
        line->deleteLater();
    }
    _polygonLineList.clear();
    foreach (GeneratorPathPoint* point, _pathPointList) {
        point->deleteLater();
    }
    _pathPointList.clear();
    foreach (GeneratorPathLine* line, _pathLineList) {
        line->deleteLater();
    }
    _pathLineList.clear();
    if(_mapItem!=nullptr){
        QFile file(pathPath);
        file.open(QIODevice::ReadOnly);
        QByteArray bytes=file.readAll();
        file.close();
        QJsonDocument doc=QJsonDocument::fromJson(bytes);
        QJsonObject rootObj=doc.object();
        foreach (QString key, rootObj.keys()) {
            QJsonObject obj=rootObj.value(key).toObject();
            QJsonArray polygonArray=obj.value("Polygon").toArray();
            QJsonArray pathArray=obj.value("Path").toArray();
            for(int i=0;i<polygonArray.size();i++){
                QJsonArray posArray=polygonArray[i].toArray();
                QPointF mappos=QPointF(posArray[0].toDouble(),posArray[1].toDouble());
                QPointF realPos=dataDealer->realPosFromMapPos(mappos);
                GeneratorPolygonPoint* point=new GeneratorPolygonPoint(_mapItem);
                point->setPos(realPos);
                point->setFlags(QGraphicsItem::ItemIsSelectable);
                connect(point,SIGNAL(posChangedSignal(GeneratorPolygonPoint*)),this,SLOT(polygonPointPosChangedSlot(GeneratorPolygonPoint*)));
                _polygonPointList.append(point);
                if(i==0){
                    _startPosRectF=QRectF(realPos.x()-5,realPos.y()-5,10,10);
                }else if(i==polygonArray.size()-1){
                    GeneratorPolygonLine* line=new GeneratorPolygonLine(_mapItem);
                    line->setStartPos(_polygonPointList[i-1]->pos());
                    line->setEndPos(realPos);
                    _polygonLineList.append(line);
                    GeneratorPolygonLine* linea=new GeneratorPolygonLine(_mapItem);
                    linea->setStartPos(realPos);
                    linea->setEndPos(_polygonPointList.first()->pos());
                    _polygonLineList.append(linea);
                }else{
                    GeneratorPolygonLine* line=new GeneratorPolygonLine(_mapItem);
                    line->setStartPos(_polygonPointList[i-1]->pos());
                    line->setEndPos(realPos);
                    _polygonLineList.append(line);
                }
                _isPolygonEnd=true;
            }
            for(int i=0;i<pathArray.size();i++){
                QJsonArray posArray=pathArray[i].toArray();
                QPointF poss=QPointF(posArray[0].toDouble(),posArray[1].toDouble());
                QPointF pos=dataDealer->realPosFromMapPos(poss);
                GeneratorPathPoint* point=new GeneratorPathPoint(_mapItem);
                point->setPos(pos);
                point->setFlags(QGraphicsItem::ItemIsSelectable);
                connect(point,SIGNAL(posChangedSignal(GeneratorPathPoint*)),this,SLOT(pathPointPosChangedSlot(GeneratorPathPoint*)));
                _pathPointList.append(point);
                if(i>0){
                    GeneratorPathLine* line=new GeneratorPathLine(_mapItem);
                    line->setStartPos(_pathPointList[i-1]->pos());
                    line->setEndPos(pos);
                    _pathLineList.append(line);
                }
            }
        }
    }
}
void GeneratorScene::polygonPointPosChangedSlot(GeneratorPolygonPoint* point){
    quint16 index=_polygonPointList.indexOf(point);
    if(index!=0){
        _polygonLineList[index-1]->setEndPos(point->pos());
        _polygonLineList[index]->setStartPos(point->pos());
    }else{
        _polygonLineList[0]->setStartPos(point->pos());
        _polygonLineList.last()->setEndPos(point->pos());
    }
}
void GeneratorScene::pathPointPosChangedSlot(GeneratorPathPoint* point){
    quint16 index=_pathPointList.indexOf(point);
    if(index==0){
        _pathLineList[0]->setStartPos(point->pos());
    }else if(index==_pathPointList.length()-1){
        _pathLineList.last()->setEndPos(point->pos());
    }else{
        _pathLineList[index-1]->setEndPos(point->pos());
        _pathLineList[index]->setStartPos(point->pos());
    }
}
void GeneratorScene::delPolygonPointActionSlot(){
    if(_polygonPointList.length()>3){
        quint16 index=_polygonPointList.indexOf(_ptr._polygonPointPtr);
        _polygonPointList.removeAt(index);
        _ptr._polygonPointPtr->deleteLater();
        _polygonLineList[index]->deleteLater();
        _polygonLineList.removeAt(index);
        if(index==0){
            _polygonLineList[_polygonLineList.length()-1]->setEndPos(_polygonPointList[0]->pos());
        }else if(index==_polygonPointList.length()){
            _polygonLineList[index-1]->setEndPos(_polygonPointList[0]->pos());
        }else{
            _polygonLineList[index-1]->setEndPos(_polygonPointList[index]->pos());
        }
        update();
    }
}
void GeneratorScene::addPolygonPointActionSlot(){
    QPointF pointPos=_mapItem->mapFromScene(_cursorPos);
    qint16 index=_polygonLineList.indexOf(_ptr._polygonLinePtr);
    GeneratorPolygonPoint* point=new GeneratorPolygonPoint(_mapItem);
    point->setPos(pointPos);
    point->setFlags(QGraphicsItem::ItemIsMovable);
    connect(point,SIGNAL(posChangedSignal(GeneratorPolygonPoint*)),this,SLOT(polygonPointPosChangedSlot(GeneratorPolygonPoint*)));
    _polygonPointList.insert(index+1,point);
    _polygonLineList[index]->setEndPos(pointPos);
    GeneratorPolygonLine* line=new GeneratorPolygonLine(_mapItem);
    line->setStartPos(pointPos);
    if(index==_polygonLineList.length()-1){
        line->setEndPos(_polygonPointList[0]->pos());
    }else{
        line->setEndPos(_polygonPointList[index+2]->pos());
    }
    _polygonLineList.insert(index+1,line);
    update();
}
void GeneratorScene::delPathPointActionSlot(){
    quint16 index=_pathPointList.indexOf(_ptr._pathPointPtr);
    if(index==0){
        _pathPointList.first()->deleteLater();
        _pathPointList.removeFirst();
        _pathLineList.first()->deleteLater();
        _pathLineList.removeFirst();
    }else if(index==_pathPointList.length()-1){
        _pathPointList.last()->deleteLater();
        _pathPointList.removeLast();
        _pathLineList.last()->deleteLater();
        _pathLineList.removeLast();
    }else{
        QPointF pos=_pathPointList[index+1]->pos();
        _pathPointList[index]->deleteLater();
        _pathPointList.removeAt(index);
        _pathLineList[index]->deleteLater();
        _pathLineList.removeAt(index);
        _pathLineList[index-1]->setEndPos(pos);
    }
    update();
}
void GeneratorScene::addPathPointActionSlot(){
    quint16 index=_pathPointList.indexOf(_ptr._pathPointPtr);
    if(index==0){
        QPointF pos=_ptr._pathPointPtr->pos()+(_pathPointList[1]->pos()-_ptr._pathPointPtr->pos())/2;
        GeneratorPathPoint* point=new GeneratorPathPoint(_mapItem);
        point->setPos(pos);
        connect(point,SIGNAL(posChangedSignal(GeneratorPathPoint*)),this,SLOT(pathPointPosChangedSlot(GeneratorPathPoint*)));
        GeneratorPathLine* line=new GeneratorPathLine(_mapItem);
        line->setStartPos(pos);
        line->setEndPos(_pathPointList.first()->pos());
        _pathLineList.insert(0,line);
        _pathPointList.insert(0,point);
    }else{
        QPointF pos=_ptr._pathPointPtr->pos()-(_ptr._pathPointPtr->pos()-_pathPointList[index-1]->pos())/2;
        GeneratorPathPoint* point=new GeneratorPathPoint(_mapItem);
        point->setPos(pos);
        point->setFlags(QGraphicsItem::ItemIsMovable);
        connect(point,SIGNAL(posChangedSignal(GeneratorPathPoint*)),this,SLOT(pathPointPosChangedSlot(GeneratorPathPoint*)));
        GeneratorPathLine* line=new GeneratorPathLine(_mapItem);
        line->setStartPos(pos);
        line->setEndPos(_pathPointList[index]->pos());
        _pathLineList[index-1]->setEndPos(pos);
        _pathLineList.insert(index,line);
        _pathPointList.insert(index,point);
    }
}
void GeneratorScene::savePolygonPathSlot(){
    QString mapDir=dataDealer->mapDir();
    QString polygonPath=mapDir+"/Path.json";
    QJsonDocument doc;
    QJsonObject obj;
    QJsonArray polygonArray;
    QJsonArray pathArray;
    QJsonObject areaObj;
    foreach(GeneratorPolygonPoint* point,_polygonPointList){
        QJsonArray array;
        QPointF pos=dataDealer->mapPosFromRealPos(point->pos());
        array.append(pos.x());
        array.append(pos.y());
        polygonArray.append(array);
    }
    foreach(GeneratorPathPoint* point,_pathPointList){
        QJsonArray array;
        QPointF pos=dataDealer->mapPosFromRealPos(point->pos());
        array.append(pos.x());
        array.append(pos.y());
        pathArray.append(array);
    }
    areaObj.insert("Polygon",polygonArray);
    areaObj.insert("Path",pathArray);
    obj.insert("Area0",areaObj);
    doc.setObject(obj);
    QFile file(polygonPath);
    file.open(QIODevice::WriteOnly);
    file.write(doc.toJson());
    file.close();
}
/*@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@GeneratorCreatePathDialog@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@*/
GeneratorCreatePathDialog::GeneratorCreatePathDialog(QWidget* parent):QDialog(parent){
    _robotRadiusLabel=new QLabel(this);
    _robotRadiusLabel->setText("设置小车半径");
    _robotRadiusLine=new QLineEdit(this);
    _lineDistanceLabel=new QLabel(this);
    _lineDistanceLabel->setText("设置路线间距");
    _lineDistanceLine=new QLineEdit(this);
    _okayButton=new QPushButton(this);
    _okayButton->setText("确认");
    _layout=new QGridLayout(this);
    _layout->addWidget(_robotRadiusLabel,0,0,1,1);
    _layout->addWidget(_robotRadiusLine,0,1,1,1);
    _layout->addWidget(_lineDistanceLabel,1,0,1,1);
    _layout->addWidget(_lineDistanceLine,1,1,1,1);
    _layout->addWidget(_okayButton,2,0,1,2);
    connect(_okayButton,SIGNAL(clicked()),this,SLOT(okayButtonClickedSlot()));
}
void GeneratorCreatePathDialog::closeEvent(QCloseEvent *){
    okayButtonClickedSlot();
}
void GeneratorCreatePathDialog::okayButtonClickedSlot(){
    QString radius=_robotRadiusLine->text();
    QString distance=_lineDistanceLine->text();
    if(!radius.isEmpty()&&!distance.isEmpty()){
        double r=radius.toDouble();
        double d=distance.toDouble();
        emit dataDealer->setRadiusDistanceSignal(r,d);
        close();
    }
}
/*@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@GeneratorView@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@*/
GeneratorView::GeneratorView(QWidget* parent):QGraphicsView(parent){
    setRenderHints(QPainter::Antialiasing|QPainter::SmoothPixmapTransform);
    setAlignment(Qt::AlignCenter);
    setViewportUpdateMode(QGraphicsView::BoundingRectViewportUpdate);
    setMouseTracking(true);
    _scene=new GeneratorScene(this);
    setScene(_scene);
    _openDirButton=new QPushButton(this);
    _openDirButton->setFixedSize(30,30);
    _openDirButton->setIcon(QIcon(":/Image/OpenDir.png"));
    _openDirButton->setIconSize(QSize(20,20));
    _openDirButton->setToolTip("打开地图所在文件夹");
    _drawPolygonButton=new QPushButton(this);
    _drawPolygonButton->setFixedSize(30,30);
    _drawPolygonButton->setIcon(QIcon(":/Image/CreatePolygon.png"));
    _drawPolygonButton->setIconSize(QSize(20,20));
    _drawPolygonButton->setCheckable(true);
    _drawPolygonButton->setToolTip("绘制路径生成区域");
    _modPolygonButton=new QPushButton(this);
    _modPolygonButton->setFixedSize(30,30);
    _modPolygonButton->setIcon(QIcon(":/Image/ModPolygon.png"));
    _modPolygonButton->setIconSize(QSize(20,20));
    _modPolygonButton->setCheckable(true);
    _modPolygonButton->setToolTip("修改路径生成区域");
    _createPathButton=new QPushButton(this);
    _createPathButton->setFixedSize(30,30);
    _createPathButton->setIcon(QIcon(":/Image/CreatePath.png"));
    _createPathButton->setIconSize(QSize(20,20));
    _createPathButton->setCheckable(true);
    _createPathButton->setToolTip("修改路径生成区域");
    _modPathButton=new QPushButton(this);
    _modPathButton->setFixedSize(30,30);
    _modPathButton->setIcon(QIcon(":/Image/ModPath.png"));
    _modPathButton->setIconSize(QSize(20,20));
    _modPathButton->setCheckable(true);
    _modPathButton->setToolTip("修改路径生成区域");
    _savePathButton=new QPushButton(this);
    _savePathButton->setFixedSize(30,30);
    _savePathButton->setIcon(QIcon(":/Image/SavePath.png"));
    _savePathButton->setIconSize(QSize(20,20));
    _savePathButton->setToolTip("修改路径生成区域");
    _buttonGroup=new QButtonGroup(this);
    _buttonGroup->addButton(_drawPolygonButton);
    _buttonGroup->addButton(_modPolygonButton);
    _buttonGroup->addButton(_createPathButton);
    _buttonGroup->addButton(_modPathButton);
    _layout01=new QHBoxLayout();
    _layout01->setContentsMargins(0,0,0,0);
    _layout01->setSpacing(5);
    _layout01->addStretch(46);
    _layout01->addWidget(_openDirButton);
    _layout01->addWidget(_drawPolygonButton);
    _layout01->addWidget(_modPolygonButton);
    _layout01->addWidget(_createPathButton);
    _layout01->addWidget(_modPathButton);
    _layout01->addWidget(_savePathButton);
    _layout01->addStretch(46);
    _layout=new QVBoxLayout(this);
    _layout->setContentsMargins(0,10,0,0);
    _layout->addLayout(_layout01);
    _layout->addStretch(99);
    setLayout(_layout);
    _dialog=new GeneratorCreatePathDialog(this);
    connect(_openDirButton,SIGNAL(clicked()),this,SLOT(openDirButtonClickedSlot()));
    connect(_drawPolygonButton,SIGNAL(clicked()),this,SLOT(drawPolygonButtonClickedSlot()));
    connect(_modPolygonButton,SIGNAL(clicked()),this,SLOT(modPolygonButtonClickedSlot()));
    connect(_createPathButton,SIGNAL(clicked()),this,SLOT(createPathButtonClickedSlot()));
    connect(_modPathButton,SIGNAL(clicked()),this,SLOT(modPathButtonClickedSlot()));
    connect(_savePathButton,SIGNAL(clicked()),this,SLOT(savePathButtonClickedSlot()));
}
GeneratorView::~GeneratorView(){
    _layout01->deleteLater();
}
void GeneratorView::mousePressEvent(QMouseEvent *event){
    if(dataDealer->actionType()==DataDealer::DRAW_POLYGON){
        _scene->setPolygonPoint(mapToScene(event->pos()));
    }
    QGraphicsView::mousePressEvent(event);
}
void GeneratorView::wheelEvent(QWheelEvent *event){
    if(event->delta()>0){
        scale(1.1,1.1);
    }else{
        scale(1/1.1,1/1.1);
    }
}
void GeneratorView::openDirButtonClickedSlot(){
    QString dirPath = QFileDialog::getExistingDirectory(this, "打开地图所在文件夹",MAPS_DIR);
    dataDealer->setMapDir(dirPath);
}
void GeneratorView::drawPolygonButtonClickedSlot(){
    dataDealer->setActionType(DataDealer::DRAW_POLYGON);
    _scene->setActionType(DataDealer::DRAW_POLYGON);
}
void GeneratorView::modPolygonButtonClickedSlot(){
    dataDealer->setActionType(DataDealer::MODIFY_POLYGON);
    _scene->setActionType(DataDealer::MODIFY_POLYGON);
}
void GeneratorView::createPathButtonClickedSlot(){
    dataDealer->setActionType(DataDealer::CREATE_PATH);
    _scene->setActionType(DataDealer::CREATE_PATH);
    _dialog->show();
}
void GeneratorView::modPathButtonClickedSlot(){
    dataDealer->setActionType(DataDealer::MODIFY_PATH);
    _scene->setActionType(DataDealer::MODIFY_PATH);
}
void GeneratorView::savePathButtonClickedSlot(){
    emit dataDealer->savePolygonPathSignal();
}
/*@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@扫地车路径生成器@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@*/
PathGenerator::PathGenerator(QWidget* parent):QFrame(parent){
    _view=new GeneratorView(this);
    _layout=new QGridLayout(this);
    _layout->setContentsMargins(0,0,0,0);
    _layout->setSpacing(0);
    _layout->addWidget(_view);
    setLayout(_layout);
}
PathGenerator::~PathGenerator(){}
