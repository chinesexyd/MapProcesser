#include "p_routeplanner.h"
/*@@@@@@@@@@@@@@@@@@@@@@DataProvider@@@@@@@@@@@@@@@@@@@@@@@@@*/
DataProvider::DataProvider(){}
void DataProvider::setAreaDir(QString path){
    QDir dir(path);
    if(dir.exists()){
        _allInfo.rootDirPath=path;
        QString linesPath=path+"/Lines.json";
        QString buildsPath=path+"/Builds.json";
        QFile linesfile(linesPath);
        QFile buildsfile(buildsPath);
        if(!linesfile.exists()){
            linesfile.open(QIODevice::WriteOnly);
            linesfile.close();
        }
        if(!buildsfile.exists()){
            buildsfile.open(QIODevice::WriteOnly);
            buildsfile.close();
        }
        QFileInfoList fileinfoList=dir.entryInfoList();
        for(int i=0;i<fileinfoList.length();i++){
            if(fileinfoList[i].suffix()=="pgm"){
                _allInfo.mapPath=fileinfoList[i].filePath();
            }else if(fileinfoList[i].suffix()=="yaml"){
                _allInfo.yamlPath=fileinfoList[i].filePath();
                YAML::Node yaml = YAML::LoadFile(_allInfo.yamlPath.toStdString());
                _allInfo.resolution = yaml["resolution"].as<double>();
                _allInfo.originX = yaml["origin"][0].as<double>();
                _allInfo.originY = yaml["origin"][1].as<double>();
            }else if(fileinfoList[i]==linesPath){
                _allInfo.linesPath=linesPath;
                QFile file(_allInfo.linesPath);
                file.open(QIODevice::ReadOnly);
                _allInfo.linesObj=QJsonDocument::fromJson(file.readAll()).object();
                file.close();
            }else if(fileinfoList[i]==buildsPath){
                _allInfo.buildsPath=buildsPath;
                QFile file(_allInfo.buildsPath);
                file.open(QIODevice::ReadOnly);
                _allInfo.buildsObj=QJsonDocument::fromJson(file.readAll()).object();
                file.close();
            }else{}
        }
    }
}
QJsonArray DataProvider::getLinePoints(QString lineName){
    if(_allInfo.linesObj.keys().contains(lineName)){
        return _allInfo.linesObj.value(lineName).toArray();
    }else{
        return QJsonArray();
    }
}
QJsonArray DataProvider:: getBuildLines(QString buildName){
    if(_allInfo.buildsObj.keys().contains(buildName)){
        return _allInfo.buildsObj.value(buildName).toArray();
    }else{
        return QJsonArray();
    }
}
void DataProvider::addLine(QString lineName,QJsonArray linePoints){
    _allInfo.linesObj.insert(lineName,linePoints);
}
void DataProvider::removeLine(QString lineName){
    if(_allInfo.linesObj.keys().contains(lineName)){
        _allInfo.linesObj.remove(lineName);
    }else{}
}
void DataProvider::modifyLine(QString lineName, QJsonArray linePoints){
    _allInfo.linesObj[lineName]=linePoints;
}
void DataProvider::addBuild(QString build,QJsonArray buildLines){
    _allInfo.buildsObj.insert(build,buildLines);
}
void DataProvider::removeBuild(QString build){
    if(_allInfo.buildsObj.keys().contains(build)){
        _allInfo.buildsObj.remove(build);
    }
}
void DataProvider::modifyBuild(QString build,QJsonArray buildLines){
    _allInfo.buildsObj.insert(build,buildLines);
}
void DataProvider::saveLines(){
    if(_allInfo.linesPath.isEmpty()){
        return;
    }
    QFile file(_allInfo.linesPath);
    file.open(QIODevice::WriteOnly);
    QJsonDocument doc;
    doc.setObject(_allInfo.linesObj);
    file.write(doc.toJson());
    file.close();
}
void DataProvider::saveBuilds(){
    if(_allInfo.linesPath.isEmpty()){
        return;
    }
    QFile file(_allInfo.buildsPath);
    file.open(QIODevice::WriteOnly);
    QJsonDocument doc;
    doc.setObject(_allInfo.buildsObj);
    file.write(doc.toJson());
    file.close();
}
void DataProvider::saveRightDrivingPath(){
    if(_allInfo.rootDirPath.isEmpty()){
        return;
    }
    QString saveDirPath=_allInfo.rootDirPath+"/GoBack";
    QDir saveDir(saveDirPath);
    if(!saveDir.exists()){
        saveDir.mkdir(saveDirPath);
    }
    QString goFilePath=saveDirPath+"/go";
    QString backFilePath=saveDirPath+"/back";
    QStringList buildnames=_allInfo.buildsObj.keys();
    QFile gofile(goFilePath);
    gofile.open(QIODevice::WriteOnly);
    QFile backfile(backFilePath);
    backfile.open(QIODevice::WriteOnly);
    for(int i=0;i<buildnames.size();i++){
        QJsonArray linenameArray=_allInfo.buildsObj.value(buildnames[i]).toArray();
        //送货顺序
        QJsonArray goArray;
        for(int j=0;j<linenameArray.size();j++){
            QString linename=linenameArray[j].toString();
            QJsonArray lineArray=_allInfo.linesObj.value(linename).toArray();
            for(int m=0;m<lineArray.size();m++){
                goArray+=lineArray[m];
            }
        }
        gofile.write(buildnames[i].toUtf8()+"\n");
        for(int k=0;k<goArray.size();k++){
            if(k<goArray.size()-1){
                QJsonArray sPosArray=goArray[k].toArray();
                double sPosX=sPosArray[0].toDouble();
                double sPosY=sPosArray[1].toDouble();
                QJsonArray ePosArray=goArray[k+1].toArray();
                double ePosX=ePosArray[0].toDouble();
                double ePosY=ePosArray[1].toDouble();
                TOOL::XYA_POINT xyaPos;
                xyaPos.x=sPosX;
                xyaPos.y=sPosY;
                xyaPos.angle=_getAngleFromTwoPoint(QPointF(sPosX,sPosY),QPointF(ePosX,ePosY));
                TOOL::XYZW_POINT xyzwPos=_getXYZWPOINTFromXYAPOINT(xyaPos);
                QString xyzwStr=QString("%1,%2,%3,%4\n").arg(QString::number(xyzwPos.x,'f',3)).arg(QString::number(xyzwPos.y,'f',3)).arg(QString::number(xyzwPos.z,'f',3)).arg(QString::number(xyzwPos.w,'f',3));
                gofile.write(xyzwStr.toUtf8());
            }else{
                QJsonArray sPosArray=goArray[k-1].toArray();
                double sPosX=sPosArray[0].toDouble();
                double sPosY=sPosArray[1].toDouble();
                QJsonArray ePosArray=goArray[k].toArray();
                double ePosX=ePosArray[0].toDouble();
                double ePosY=ePosArray[1].toDouble();
                TOOL::XYA_POINT xyaPos;
                xyaPos.x=ePosX;
                xyaPos.y=ePosY;
                xyaPos.angle=_getAngleFromTwoPoint(QPointF(sPosX,sPosY),QPointF(ePosX,ePosY));
                TOOL::XYZW_POINT xyzwPos=_getXYZWPOINTFromXYAPOINT(xyaPos);
                QString xyzwStr=QString("%1,%2,%3,%4\n").arg(QString::number(xyzwPos.x,'f',3)).arg(QString::number(xyzwPos.y,'f',3)).arg(QString::number(xyzwPos.z,'f',3)).arg(QString::number(xyzwPos.w,'f',3));
                gofile.write(xyzwStr.toUtf8());
            }
        }
        backfile.write(buildnames[i].toUtf8()+"\n");
        for(int k=goArray.size()-1;k>=0;k--){
            if(k>0){
                QJsonArray sPosArray=goArray[k].toArray();
                double sPosX=sPosArray[0].toDouble();
                double sPosY=sPosArray[1].toDouble();
                QJsonArray ePosArray=goArray[k-1].toArray();
                double ePosX=ePosArray[0].toDouble();
                double ePosY=ePosArray[1].toDouble();
                TOOL::XYA_POINT xyaPos;
                xyaPos.x=sPosX;
                xyaPos.y=sPosY;
                xyaPos.angle=_getAngleFromTwoPoint(QPointF(sPosX,sPosY),QPointF(ePosX,ePosY));
                TOOL::XYZW_POINT xyzwPos=_getXYZWPOINTFromXYAPOINT(xyaPos);
                QString xyzwStr=QString("%1,%2,%3,%4\n").arg(QString::number(xyzwPos.x,'f',3)).arg(QString::number(xyzwPos.y,'f',3)).arg(QString::number(xyzwPos.z,'f',3)).arg(QString::number(xyzwPos.w,'f',3));
                backfile.write(xyzwStr.toUtf8());
            }else{
                QJsonArray sPosArray=goArray[k+1].toArray();
                double sPosX=sPosArray[0].toDouble();
                double sPosY=sPosArray[1].toDouble();
                QJsonArray ePosArray=goArray[k].toArray();
                double ePosX=ePosArray[0].toDouble();
                double ePosY=ePosArray[1].toDouble();
                TOOL::XYA_POINT xyaPos;
                xyaPos.x=ePosX;
                xyaPos.y=ePosY;
                xyaPos.angle=_getAngleFromTwoPoint(QPointF(sPosX,sPosY),QPointF(ePosX,ePosY));
                TOOL::XYZW_POINT xyzwPos=_getXYZWPOINTFromXYAPOINT(xyaPos);
                QString xyzwStr=QString("%1,%2,%3,%4\n").arg(QString::number(xyzwPos.x,'f',3)).arg(QString::number(xyzwPos.y,'f',3)).arg(QString::number(xyzwPos.z,'f',3)).arg(QString::number(xyzwPos.w,'f',3));
                backfile.write(xyzwStr.toUtf8());
            }
        }
    }
    gofile.close();
    backfile.close();
}
QPointF DataProvider::realPosFromMapPos(QPointF pos){
    double x=(pos.x()-_allInfo.originX)/_allInfo.resolution;
    double y=_allInfo.pixmapRect.height()-(pos.y()-_allInfo.originY)/_allInfo.resolution;
    return QPointF(x,y);
}
QPointF DataProvider::mapPosFromRealPos(QPointF pos){
    double x=pos.x()*_allInfo.resolution+_allInfo.originX;
    double y=(_allInfo.pixmapRect.height()-pos.y())*_allInfo.resolution+_allInfo.originY;
    return QPointF(x,y);
}
TOOL::XYZW_POINT DataProvider::_getXYZWPOINTFromXYAPOINT(TOOL::XYA_POINT point){
    double angle = point.angle;
    double yaw = angle/180.0*M_PI;
    double cy = cos(yaw*0.5);
    double sy = sin(yaw*0.5);
    double cp = 1;
    double sp = 0;
    double cr = 1;
    double sr = 0;
    TOOL::XYZW_POINT xyzwPoint;
    xyzwPoint.x = point.x;
    xyzwPoint.y = point.y;
    xyzwPoint.w = cy*cp*cr+sy*sp*sr;
    xyzwPoint.z = sy*cp*cr-cy*sp*sr;
    return xyzwPoint;
}
double DataProvider::_getAngleFromTwoPoint(QPointF spos,QPointF epos){
    if(spos.x()==epos.x()){
        if(spos.y()==epos.y()){
            return 0;
        }
        if(spos.y()>epos.y()){
            return -90;
        }else{
            return 90;
        }
    }
    double ret=atan((epos.y()-spos.y())/(epos.x()-spos.x()));
    ret=ret*180/M_PI;
    if(epos.x()<spos.x()){
        if(epos.y()>spos.y()){
            ret+=180;
        }else{
            ret-=180;
        }
    }
    return ret;
}
DataProvider* dataProvider=new DataProvider();
/*@@@@@@@@@@@@@@@@@@@@@@BasementPoint@@@@@@@@@@@@@@@@@@@@@@@@@*/
BasementPoint::BasementPoint(QGraphicsItem* parent):QGraphicsItem(parent){
    _parentItem=static_cast<BasementLine*>(parent);
    setFlags(ItemIsSelectable|ItemIsFocusable);
    setCursor(Qt::PointingHandCursor);
}
BasementPoint::~BasementPoint(){}
void BasementPoint::setInfo(QString lineName,int index,QPointF mapPos){
    _info.lineName=lineName;
    _info.index=index;
    _info.mapPos=mapPos;
    _setToolTip();
}
void BasementPoint::addPoint(){
    _parentItem->addPoint(_info.index);
}
void BasementPoint::delPoint(){
    _parentItem->delPoint(_info.index);
}
void BasementPoint::delLine(){
    _parentItem->delLine();
}
QRectF BasementPoint::boundingRect() const{
    return QRectF(-5,-5,10,10);
}
void BasementPoint::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *){
    QPen pen;
    if(_info.index==0){
        pen.setColor("#177cb0");
        painter->setBrush(QColor(0,128,128,200));
    }else{
        pen.setColor("#f2be45");
        painter->setBrush(QColor(217,217,25,200));
    }
    pen.setWidth(2);
    painter->setPen(pen);
    painter->save();
    painter->drawEllipse(-5,-5,10,10);
    painter->drawPoint(0,0);
    painter->restore();
}
void BasementPoint::mousePressEvent(QGraphicsSceneMouseEvent *event){
    QGraphicsItem::mousePressEvent(event);
    if(event->button()==Qt::LeftButton){
        setFlags(ItemIsSelectable|ItemIsFocusable|ItemIsMovable);
    }
}
void BasementPoint::mouseMoveEvent(QGraphicsSceneMouseEvent *event){
    QGraphicsItem::mouseMoveEvent(event);
    QPointF mapPos=dataProvider->mapPosFromRealPos(pos());
    _parentItem->modPoint(_info.index,mapPos);
}
void BasementPoint::mouseReleaseEvent(QGraphicsSceneMouseEvent *event){
    QGraphicsItem::mouseReleaseEvent(event);
    setFlags(ItemIsSelectable|ItemIsFocusable);
}
void BasementPoint::_setToolTip(){
    QString location=QString("(%1,%2)").arg(_info.mapPos.x()).arg(_info.mapPos.y());
    setToolTip(QString("归属于线：%1\n点在线中脚标：%2\n位置：%3").arg(_info.lineName).arg(_info.index).arg(location));
}
/*@@@@@@@@@@@@@@@@@@@@@@BasementLine@@@@@@@@@@@@@@@@@@@@@@@@@*/
BasementLine::BasementLine(QGraphicsItem* parent):QGraphicsItem(parent){
    _parentItem=static_cast<BasementMap*>(parent);
}
BasementLine::~BasementLine(){_parentItem->update();}
void BasementLine::setLineName(QString lineName){
    dataProvider->appendBaseLine(lineName,this);
    _info.lineName=lineName;
    _info.points=dataProvider->getLinePoints(lineName);
    for(int i=0;i<_info.points.size();i++){
        QJsonArray mapPosArray=_info.points[i].toArray();
        QPointF mapPos=QPointF(mapPosArray[0].toDouble(),mapPosArray[1].toDouble());
        QPointF realPos=dataProvider->realPosFromMapPos(mapPos);
        BasementPoint* point=new BasementPoint(this);
        point->setInfo(lineName,i,mapPos);
        point->setPos(realPos);
        _pointList.append(point);
        if(i==0){
            _painterPath=QPainterPath(realPos);
        }else{
            _painterPath.lineTo(realPos);
        }
    }
    _rect();
}
void BasementLine::addPoint(int index){
    if(index==0){
        QJsonArray startArray=_info.points[0].toArray();
        QJsonArray endArray=_info.points[1].toArray();
        QPointF startPos=QPointF(startArray[0].toDouble(),startArray[1].toDouble());
        QPointF endPos=QPointF(endArray[0].toDouble(),endArray[1].toDouble());
        QPointF mapPos=startPos-(endPos-startPos)/2.0;
        QPointF realPos=dataProvider->realPosFromMapPos(mapPos);
        BasementPoint* point=new BasementPoint(this);
        point->setInfo(_info.lineName,0,mapPos);
        point->setPos(realPos);
        QJsonArray posArray{mapPos.x(),mapPos.y()};
        for(int i=0;i<_pointList.length();i++){
            QJsonArray array=_info.points[i].toArray();
            QPointF pos=QPointF(array[0].toDouble(),array[1].toDouble());
            _pointList[i]->setInfo(_info.lineName,i+1,pos);
        }
        _info.points.insert(0,posArray);
        _pointList.insert(0,point);
        _reLine();
    }else if(index>0&&index<_pointList.length()){
        QJsonArray startArray=_info.points[index].toArray();
        QJsonArray endArray=_info.points[index-1].toArray();
        QPointF startPos=QPointF(startArray[0].toDouble(),startArray[1].toDouble());
        QPointF endPos=QPointF(endArray[0].toDouble(),endArray[1].toDouble());
        QPointF mapPos=startPos+(endPos-startPos)/2.0;
        QPointF realPos=dataProvider->realPosFromMapPos(mapPos);
        BasementPoint* point=new BasementPoint(this);
        point->setInfo(_info.lineName,index,mapPos);
        point->setPos(realPos);
        QJsonArray posArray{mapPos.x(),mapPos.y()};
        for(int i=index;i<_pointList.length();i++){
            QJsonArray array=_info.points[i].toArray();
            QPointF pos=QPointF(array[0].toDouble(),array[1].toDouble());
            _pointList[i]->setInfo(_info.lineName,i+1,pos);
        }
        _info.points.insert(index,posArray);
        _pointList.insert(index,point);
        _reLine();
    }else{}
}
void BasementLine::modPoint(int index, QPointF pos){
    _info.points[index]=QJsonArray{pos.x(),pos.y()};
    dataProvider->modifyLine(_info.lineName,_info.points);
    _reLine();
}
void BasementLine::delPoint(int index){
    if(_pointList.length()>2){
        for(int i=index+1;i<_pointList.length();i++){
            QJsonArray array=_info.points[i].toArray();
            QPointF pos=QPointF(array[0].toDouble(),array[1].toDouble());
            _pointList[i]->setInfo(_info.lineName,i-1,pos);
        }
        BasementPoint* point=_pointList[index];
        point->deleteLater();
        _pointList.removeAt(index);
        _info.points.removeAt(index);
        dataProvider->modifyLine(_info.lineName,_info.points);
        _reLine();
    }
}
void BasementLine::delLine(){
    dataProvider->removeLine(_info.lineName);
    dataProvider->removeBaseLine(_info.lineName);
    emit SIGNALS->resetLineModelSignal();
    deleteLater();
}
QRectF BasementLine::boundingRect() const{
    return dataProvider->getPixmapRect();
}
QPainterPath BasementLine::shape() const{
    return _painterPath;
}
void BasementLine::setChecked(){
    if(_timerCount==0){
        _isChecked=true;
        update();
        _timerId=startTimer(100);
    }
}
void BasementLine::timerEvent(QTimerEvent *){
    _timerCount++;
    _isChecked=!_isChecked;
    if(_timerCount==10){
        _timerCount=0;
        _isChecked=false;
        killTimer(_timerId);
    }
    update();
}
void BasementLine::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *){
    QPen pen;
    pen.setColor("#177cb0");
    pen.setJoinStyle(Qt::RoundJoin);
    if(_isChecked){
        pen.setWidth(20);
        pen.setStyle(Qt::SolidLine);
    }else{
        pen.setWidth(2);
        pen.setStyle(Qt::DashDotLine);
    }
    QFont font;
    font.setPointSize(12);
    painter->setFont(font);
    painter->setPen(pen);
    painter->drawPath(_painterPath);
    pen.setColor("#17b978");
    pen.setStyle(Qt::SolidLine);
    painter->setBrush(QColor(0,128,128,200));
    for(int i=0;i<_textPointList.length();i++){
        QPointF textPoint=_textPointList[i];
        painter->setPen(Qt::NoPen);
        QRectF rectRect(textPoint.x()-20,textPoint.y()-10,40,20);
        painter->drawRoundedRect(rectRect,10,10);
        painter->setPen(Qt::white);
        QRectF textRect(textPoint.x()-10,textPoint.y()-10,20,20);
        painter->drawText(textRect,Qt::AlignCenter,_info.lineName);
    }
}
void BasementLine::_reLine(){
    for(int i=0;i<_info.points.size();i++){
        QJsonArray mapPosArray=_info.points[i].toArray();
        QPointF realPos=dataProvider->realPosFromMapPos(QPointF(mapPosArray[0].toDouble(),mapPosArray[1].toDouble()));
        if(i==0){
            _painterPath=QPainterPath(realPos);
        }else{
            _painterPath.lineTo(realPos);
        }
    }
    _rect();
    _parentItem->update();
}
void BasementLine::_rect(){
    _textPointList.clear();
    for(int i=0;i<_info.points.size()-1;i++){
        QJsonArray sArray=_info.points[i].toArray();
        QJsonArray eArray=_info.points[i+1].toArray();
        QPointF sMapPos=QPointF(sArray[0].toDouble(),sArray[1].toDouble());
        QPointF eMapPos=QPointF(eArray[0].toDouble(),eArray[1].toDouble());
        QPointF mMapPos=(eMapPos-sMapPos)/2.0+sMapPos;
        _textPointList.append(dataProvider->realPosFromMapPos(mMapPos));
    }
}
/*@@@@@@@@@@@@@@@@@@@@@@BasementMap@@@@@@@@@@@@@@@@@@@@@@@@@*/
BasementMap::BasementMap(QGraphicsItem* parent):QGraphicsPixmapItem(parent){
    setFlag(ItemIsMovable);
    setCursor(Qt::OpenHandCursor);
}
void BasementMap::setMap(QString path){
    _pixmap=QPixmap(path);
    dataProvider->setPixmapRect(_pixmap.rect());
    setPixmap(_pixmap);
    QStringList linesName=dataProvider->getLinesName();
    QString linename;
    foreach(linename, linesName) {
        BasementLine* line=new BasementLine(this);
        line->setLineName(linename);
    }
}
void BasementMap::addLine(QPointF pos){
    QPointF mapPos=dataProvider->mapPosFromRealPos(mapFromScene(pos));
    QJsonArray pos0{mapPos.x(),mapPos.y()};
    QJsonArray pos1{mapPos.x(),mapPos.y()-2};
    QJsonArray linePoints;
    linePoints.append(pos0);
    linePoints.append(pos1);
    BasementLine* line=new BasementLine(this);
    for(int i=0;i<dataProvider->getLinesName().length()+1;i++){
        QString linename=QString::number(i);
        if(!dataProvider->getLinesName().contains(linename)){
            dataProvider->addLine(linename,linePoints);
            line->setLineName(linename);
            break;
        }
    }
    emit SIGNALS->resetLineModelSignal();
    update();
}
void BasementMap::mousePressEvent(QGraphicsSceneMouseEvent *event){
    QGraphicsPixmapItem::mousePressEvent(event);
    if(event->button()==Qt::LeftButton){
        setCursor(Qt::ClosedHandCursor);
    }
}
void BasementMap::mouseReleaseEvent(QGraphicsSceneMouseEvent *event){
    QGraphicsPixmapItem::mouseReleaseEvent(event);
    setCursor(Qt::OpenHandCursor);
}
/*@@@@@@@@@@@@@@@@@@@@@@BasementScene@@@@@@@@@@@@@@@@@@@@@@@@@*/
BasementScene::BasementScene(QWidget* parent):QGraphicsScene(parent){
    setBackgroundBrush(QBrush("#393e46"));
    /************_mapMenu*************/
    _mapMenu=new QMenu(parent);
    _addLineAction_map=new QAction(_mapMenu);
    _addLineAction_map->setText("添加线");
    _addLineAction_map->setIcon(QIcon(":/Image/AddLine.png"));
    _mapMenu->addAction(_addLineAction_map);
    connect(_addLineAction_map,SIGNAL(triggered(bool)),this,SLOT(addLineActionMapTriggeredSlot(bool)));
    /************_lineMenu*************/
    _lineMenu=new QMenu(parent);
    /************_pointMenu*************/
    _pointMenu=new QMenu(parent);
    _addPointAction_point=new QAction(_lineMenu);
    _addPointAction_point->setText("添加点");
    _addPointAction_point->setIcon(QIcon(":/Image/AddPoint.png"));
    _delPointAction_point=new QAction(_lineMenu);
    _delPointAction_point->setText("删除点");
    _delPointAction_point->setIcon(QIcon(":/Image/DelPoint.png"));
    _delLineAction_point=new QAction(_lineMenu);
    _delLineAction_point->setText("删除线");
    _delLineAction_point->setIcon(QIcon(":/Image/DelLine.png"));
    _pointMenu->addAction(_addPointAction_point);
    _pointMenu->addAction(_delPointAction_point);
    _pointMenu->addAction(_delLineAction_point);
    connect(_addPointAction_point,SIGNAL(triggered(bool)),this,SLOT(addPointActionPointTriggeredSlot(bool)));
    connect(_delPointAction_point,SIGNAL(triggered(bool)),this,SLOT(delPointActionPointTriggeredSlot(bool)));
    connect(_delLineAction_point,SIGNAL(triggered(bool)),this,SLOT(delLineActionPointTriggeredSlot(bool)));
    connect(SIGNALS,SIGNAL(resetMapSignal()),this,SLOT(resetMapSlot()));
}
void BasementScene::resetMapSlot(){
    if(_mapItem!=NULL){
        removeItem(_mapItem);
        _mapItem->deleteLater();
        _mapItem=nullptr;
    }
    _mapItem=new BasementMap();
    _mapItem->setMap(dataProvider->getMapPath());
    addItem(_mapItem);
}
void BasementScene::movePos(QPointF pos){
    if(_mapItem!=NULL){
        QPointF realPos=_mapItem->mapFromScene(pos);
        QPointF mapPos=dataProvider->mapPosFromRealPos(realPos);
        QString mapX=QString::number(mapPos.x(),'f',6);
        QString mapY=QString::number(mapPos.y(),'f',6);
        emit SIGNALS->sendStatusBarStringSignal(QString("X坐标：%1 | Y坐标：%2").arg(mapX).arg(mapY));
    }
}
void BasementScene::contextMenuEvent(QGraphicsSceneContextMenuEvent *event){
    QTransform transform;
    QGraphicsItem* item=itemAt(event->scenePos(),transform);
    _cursorPos=event->scenePos();
    if(item==NULL){
        return;
    }
    switch (item->type()) {
    case BasementLine::Type:
        _ptr.linePtr=qgraphicsitem_cast<BasementLine*>(item);
        _lineMenu->exec(event->screenPos());
        break;
    case BasementPoint::Type:
        _ptr.pointPtr=qgraphicsitem_cast<BasementPoint*>(item);
        _pointMenu->exec(event->screenPos());
        break;
    case BasementMap::Type:
        _ptr.mapPtr=qgraphicsitem_cast<BasementMap*>(item);
        _mapMenu->exec(event->screenPos());
        break;
    default:
        break;
    }
}
void BasementScene::addPointActionPointTriggeredSlot(bool){
    _ptr.pointPtr->addPoint();
}
void BasementScene::delPointActionPointTriggeredSlot(bool){
    _ptr.pointPtr->delPoint();
}
void BasementScene::delLineActionPointTriggeredSlot(bool){
    _ptr.pointPtr->delLine();
}
void BasementScene::addLineActionMapTriggeredSlot(bool){
    _ptr.mapPtr->addLine(_cursorPos);
}
/*@@@@@@@@@@@@@@@@@@@@@@BasementView@@@@@@@@@@@@@@@@@@@@@@@@@*/
BasementView::BasementView(QWidget* parent):QGraphicsView(parent){
    setRenderHints(QPainter::Antialiasing|QPainter::SmoothPixmapTransform);
    setViewportUpdateMode(QGraphicsView::BoundingRectViewportUpdate);
    setMouseTracking(true);
    _scene=new BasementScene(this);
    setScene(_scene);
}
void BasementView::wheelEvent(QWheelEvent *event){
    if(event->delta()>0){
        scale(1.1,1.1);
    }else{
        scale(1/1.1,1/1.1);
    }
}
void BasementView::mouseMoveEvent(QMouseEvent *event){
    QGraphicsView::mouseMoveEvent(event);
    QPointF p0=mapToScene(event->pos());
    _scene->movePos(p0);
}
/*@@@@@@@@@@@@@@@@@@@@@@LineModel@@@@@@@@@@@@@@@@@@@@@@@@@*/
LineModel::LineModel(QWidget* parent):QAbstractListModel(parent){
    connect(SIGNALS,SIGNAL(resetLineModelSignal()),this,SLOT(resetLineModelSlot()));
}
int LineModel::rowCount(const QModelIndex &) const{
    return dataProvider->getLinesName().length();
}
QVariant LineModel::data(const QModelIndex &index, int role) const{
    if(!index.isValid()){
        return QVariant();
    }
    if(role==Qt::DisplayRole){
        return dataProvider->getLinesName()[index.row()];
    }else if(role==Qt::FontRole){
        QFont font;
        font.setPointSize(15);
        return font;
    }else if(role==Qt::ToolTipRole){
        return "请注意线的开头点位置！！！已用与线条同样颜色标注！！！";
    }else if(role==Qt::BackgroundRole){
        return QColor("#177cb0");
    }else{
        return QVariant();
    }
}
void LineModel::resetLineModelSlot(){
    beginResetModel();
    endResetModel();
}
/*@@@@@@@@@@@@@@@@@@@@@@LineDelegate@@@@@@@@@@@@@@@@@@@@@@@@@*/
LineDelegate::LineDelegate(QWidget* parent):QStyledItemDelegate(parent){}
void LineDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const{
    QRect _rect=option.rect;
    painter->setRenderHint(QPainter::Antialiasing);
    QString data=index.model()->data(index,Qt::DisplayRole).toString();
    QBrush brush;
    brush.setStyle(Qt::SolidPattern);
    brush.setColor(QColor(0,128,128,200));
    painter->setBrush(brush);
    QPen pen;
    pen.setColor(QColor("#fffbf0"));
    painter->setPen(pen);
    QFont font;
    font.setPointSize(15);
    painter->setFont(font);
    painter->drawRoundedRect(QRectF(_rect.x()+2,_rect.y()+2,_rect.width()-4,_rect.height()-4),5,5);
    painter->drawText(_rect,Qt::AlignCenter,data);
}
/*@@@@@@@@@@@@@@@@@@@@@@LineProxy@@@@@@@@@@@@@@@@@@@@@@@@@*/
LineProxy::LineProxy(QWidget* parent):QSortFilterProxyModel(parent){}
bool LineProxy::lessThan(const QModelIndex &source_left, const QModelIndex &source_right) const{
    if (!source_left.isValid() || !source_right.isValid())
        return false;
    int left=sourceModel()->data(source_left).toInt();
    int right=sourceModel()->data(source_right).toInt();
    return left<right;
}
/*@@@@@@@@@@@@@@@@@@@@@@LineView@@@@@@@@@@@@@@@@@@@@@@@@@*/
LineView::LineView(QWidget* parent):QListView(parent){
    _lineModel=new LineModel(this);
    _lineDelegate=new LineDelegate(this);
    _lineProxy=new LineProxy(this);
    _lineProxy->setSortRole(Qt::DisplayRole);
    _lineProxy->sort(0);
    _lineProxy->setSourceModel(_lineModel);
    setModel(_lineProxy);
    setItemDelegate(_lineDelegate);
    setContextMenuPolicy(Qt::CustomContextMenu);
    _menu=new QMenu(this);
    _delLineAction=new QAction(this);
    _delLineAction->setText("删除路径");
    _delLineAction->setIcon(QIcon(":/Image/DelLine.png"));
    _menu->addAction(_delLineAction);
    connect(this,SIGNAL(clicked(QModelIndex)),this,SLOT(clickedSlot(QModelIndex)));
    connect(this,SIGNAL(customContextMenuRequested(QPoint)),this,SLOT(showMenuSlot(QPoint)));
    connect(_delLineAction,SIGNAL(triggered()),this,SLOT(delLineActionTriggeredSlot()));
}
void LineView::clickedSlot(QModelIndex index){
    QString data=index.model()->data(index,Qt::DisplayRole).toString();
    if(!data.isEmpty()){
        if(dataProvider->getLinesName().contains(data)){
                dataProvider->getBaseLine(data)->setChecked();
        }
    }
}
void LineView::showMenuSlot(QPoint pos){
    //计算鼠标相对父窗口的位置
    QPoint pt=mapToGlobal(pos);
    //判断鼠标右击位置是否是空白处，空白处则取消上一个选中焦点，不弹出菜单
    _currentIndex = indexAt(pos);
    _menu->exec(pt);
}
void LineView::delLineActionTriggeredSlot(){
    if(!_currentIndex.isValid()){
        return;
    }
    QString linename=_currentIndex.model()->data(_currentIndex,Qt::DisplayRole).toString();
    BasementLine* line=dataProvider->getBaseLine(linename);
    line->delLine();
}
/*@@@@@@@@@@@@@@@@@@@@@@LineFrame@@@@@@@@@@@@@@@@@@@@@@@@@*/
LineFrame::LineFrame(QWidget* parent):QFrame(parent){
    _lineView=new LineView(this);
    _saveLineButton=new QPushButton(this);
    _saveLineButton->setText("保存路线");
    _layout=new QVBoxLayout(this);
    _layout->setContentsMargins(0,0,0,0);
    _layout->setSpacing(5);
    _layout->addWidget(_lineView);
    _layout->addWidget(_saveLineButton);
    setLayout(_layout);
    connect(_saveLineButton,&QPushButton::clicked,[](){dataProvider->saveLines();emit SIGNALS->sendStatusBarStringSignal("保存路线成功");});
}
/*@@@@@@@@@@@@@@@@@@@@@@BuildModel@@@@@@@@@@@@@@@@@@@@@@@@@*/
BuildModel::BuildModel(QWidget* parent):QAbstractTableModel(parent){
    _headerData<<"楼栋"<<"包含路线";
    connect(SIGNALS,SIGNAL(resetBuildModelSignal()),this,SLOT(resetBuildModelSlot()));
}
int BuildModel::columnCount(const QModelIndex &) const{
    return _headerData.length();
}
int BuildModel::rowCount(const QModelIndex &) const{
    return dataProvider->getBuildsName().length();
}
QVariant BuildModel::headerData(int section, Qt::Orientation orientation,int role) const{
    if(orientation==Qt::Horizontal&&role==Qt::DisplayRole){
        return _headerData[section];
    }else if(orientation==Qt::Vertical&&role==Qt::DisplayRole){
        return section+1;
    }else{
        return QVariant();
    }
}
QVariant BuildModel::data(const QModelIndex & index, int role)const{
    if(role==Qt::DisplayRole||role==Qt::EditRole){
        if(index.column()==0){
            return dataProvider->getBuildsName()[index.row()];
        }else{
            QString buildname=dataProvider->getBuildsName()[index.row()];
            QJsonArray array=dataProvider->getBuildLines(buildname);
            QString returnStr;
            for(int i=0;i<array.size();i++){
                if(i==0){
                    returnStr+=array[i].toString();
                }else{
                    returnStr+=","+array[i].toString();
                }
            }
            return returnStr;
        }
    }else if(role==Qt::TextAlignmentRole){
        if(index.column()==0){
            return Qt::AlignCenter;
        }
    }else if(role==Qt::ToolTipRole){
        return "[请注意]：按照送货顺序排列线！！！用逗号隔开！！！";
    }else if(role==Qt::TextColorRole){
        if(index.column()==1){
            return QColor("#599E8E");
        }
    }else{}
    return QVariant();
}
Qt::ItemFlags BuildModel::flags(const QModelIndex &index) const{
    if (!index.isValid())
        return 0;
    if(index.column()==1){
        return Qt::ItemIsEnabled |Qt::ItemIsEditable;
    }else{
        return QAbstractTableModel::flags(index);
    }
}
bool BuildModel::setData(const QModelIndex &index, const QVariant &value, int role){
    if(role==Qt::EditRole&&index.column()==1){
        QString val=value.toString();
        QStringList valList=val.split(",");
        QJsonArray array;
        for(int i=0;i<valList.length();i++){
            QStringList valList0=valList[i].split("，");
            for(int j=0;j<valList0.length();j++){
                array.append(valList0[j]);
            }
        }
        dataProvider->modifyBuild(dataProvider->getBuildsName()[index.row()],array);
        QVector<int> roles;
        roles.append(Qt::DisplayRole);
        roles.append(Qt::EditRole);
        emit dataChanged(index,index,roles);
    }
    return true;
}
void BuildModel::resetBuildModelSlot(){
    beginResetModel();
    endResetModel();
}
/*@@@@@@@@@@@@@@@@@@@@@@FakeDialogFrame@@@@@@@@@@@@@@@@@@@@@@@@*/
FakeDialog::FakeDialog(QWidget* parent):QDialog(parent){
    _label=new QLabel(this);
    _edit=new QLineEdit(this);
    _okayButton=new QPushButton(this);
    _okayButton->setText("确定");
    _cancelButton=new QPushButton(this);
    _cancelButton->setText("取消");
    _layout=new QGridLayout(this);
    _layout->setContentsMargins(10,10,10,10);
    _layout->setSpacing(5);
    _layout->addWidget(_label,0,0,1,1);
    _layout->addWidget(_edit,0,1,1,1);
    _layout->addWidget(_cancelButton,1,0,1,1);
    _layout->addWidget(_okayButton,1,1,1,1);
    _layout->setColumnStretch(0,1);
    _layout->setColumnStretch(1,1);
    _layout->setRowStretch(0,1);
    _layout->setRowStretch(1,1);
    setLayout(_layout);
    connect(_okayButton,&QPushButton::clicked,[this](){emit sendFakeDialogResultTextSignal(_edit->text());_edit->setText("");hide();});
    connect(_cancelButton,&QPushButton::clicked,[this](){emit sendFakeDialogResultTextSignal("");_edit->setText("");hide();});
    hide();
    setMaximumWidth(300);
}
/*@@@@@@@@@@@@@@@@@@@@@@BuildView@@@@@@@@@@@@@@@@@@@@@@@@@*/
BuildView::BuildView(QWidget* parent):QTableView(parent){
    _buildModel=new BuildModel(this);
    setModel(_buildModel);
    setAlternatingRowColors(true);
    horizontalHeader()->setStretchLastSection(true);
    setContextMenuPolicy(Qt::CustomContextMenu);
    setColumnWidth(0,50);
    setMinimumWidth(200);
    _menu=new QMenu(this);
    _addBuildAction=new QAction(this);
    _addBuildAction->setText("添加楼栋");
    _addBuildAction->setIcon(QIcon(":/Image/AddBuild.png"));
    _delBuildAction=new QAction(this);
    _delBuildAction->setText("删除楼栋");
    _delBuildAction->setIcon(QIcon(":/Image/DelBuild.png"));
    _menu->addAction(_addBuildAction);
    _menu->addAction(_delBuildAction);
    _buildNameDialog=new FakeDialog(this);
    _buildNameDialog->setLabelText("请输入楼栋：");
    connect(this,SIGNAL(customContextMenuRequested(QPoint)),this,SLOT(showMenuSlot(QPoint)));
    connect(_addBuildAction,SIGNAL(triggered()),this,SLOT(addBuildActionTriggeredSlot()));
    connect(_delBuildAction,SIGNAL(triggered()),this,SLOT(delBuildActionTriggeredSlot()));
    connect(this,SIGNAL(clicked(QModelIndex)),this,SLOT(clickedSlot(QModelIndex)));
    connect(_buildNameDialog,SIGNAL(sendFakeDialogResultTextSignal(QString)),this,SLOT(receiveFakeDialogResultTextSlot(QString)));
}
void BuildView::clickedSlot(QModelIndex index){
    if(!index.isValid()){
        return;
    }
    QString data=dataProvider->getBuildsName()[index.row()];
    QJsonArray lines=dataProvider->getBuildLines(data);
    QStringList linenames=dataProvider->getLinesName();
    for(int i=0;i<lines.size();i++){
        QString linename=lines[i].toString();
        if(linenames.contains(linename)){
            dataProvider->getBaseLine(linename)->setChecked();
        }
    }
}
void BuildView::showMenuSlot(QPoint pos){
    //计算鼠标相对父窗口的位置
    QPoint pt=mapToGlobal(pos)+QPoint(15,27);
    //判断鼠标右击位置是否是空白处，空白处则取消上一个选中焦点，不弹出菜单
    _currentIndex = indexAt(pos);
    _menu->exec(pt);
}
void BuildView::addBuildActionTriggeredSlot(){
    _buildNameDialog->show();
}
void BuildView::receiveFakeDialogResultTextSlot(QString buildname){
    if(!buildname.isEmpty()){
        dataProvider->addBuild(buildname,QJsonArray());
        _buildModel->resetBuildModelSlot();
    }
}
void BuildView::delBuildActionTriggeredSlot(){
    if(!_currentIndex.isValid()){
        return;
    }
    QString buildname=dataProvider->getBuildsName()[_currentIndex.row()];
    dataProvider->removeBuild(buildname);
    _buildModel->resetBuildModelSlot();
}
/*@@@@@@@@@@@@@@@@@@@@@@BuildFrame@@@@@@@@@@@@@@@@@@@@@@@@@*/
BuildFrame::BuildFrame(QWidget* parent):QFrame(parent){
    _buildView=new BuildView(this);
    _checkDirButton=new QPushButton(this);
    _checkDirButton->setText("打开地图所在文件夹");
    _saveBuildButton=new QPushButton(this);
    _saveBuildButton->setText("保存楼栋路线");
    _saveRoadButton=new QPushButton(this);
    _saveRoadButton->setText("导出为GO/BACK文件");
    _layout=new QVBoxLayout(this);
    _layout->setContentsMargins(0,0,0,0);
    _layout->setSpacing(5);
    _layout->addWidget(_buildView);
    _layout->addWidget(_checkDirButton);
    _layout->addWidget(_saveBuildButton);
    _layout->addWidget(_saveRoadButton);
    setLayout(_layout);
    connect(_checkDirButton,SIGNAL(clicked()),this,SLOT(checkDirSlot()));
    connect(_saveBuildButton,&QPushButton::clicked,[](){dataProvider->saveBuilds();emit SIGNALS->sendStatusBarStringSignal("保存楼栋路线成功");});
    connect(_saveRoadButton,&QPushButton::clicked,[](){dataProvider->saveLines();dataProvider->saveBuilds();dataProvider->saveRightDrivingPath();emit SIGNALS->sendStatusBarStringSignal("导出为GO/BACK文件成功");});
}
void BuildFrame::checkDirSlot(){
    QString srcDirPath = QFileDialog::getExistingDirectory(this, "打开地图所在文件夹",MAPS_DIR);
    if (srcDirPath.length() > 0){
        dataProvider->setAreaDir(srcDirPath);
        emit SIGNALS->resetMapSignal();
        emit SIGNALS->resetLineModelSignal();
        emit SIGNALS->resetBuildModelSignal();
    }
}
/*@@@@@@@@@@@@@@@@@@@@@@RoutePlanner@@@@@@@@@@@@@@@@@@@@@@@@@*/
RoutePlanner::RoutePlanner(QWidget* parent):QFrame(parent){
    _mapView=new BasementView(this);
    _buildFrame=new BuildFrame(this);
    _buildFrame->setMaximumWidth(300);
    _lineFrame=new LineFrame(this);
    _saveLineButton=new QPushButton(this);
    _saveLineButton->setText("保存路线");
    _statusBar=new QStatusBar(this);
    _splitter01=new QSplitter(this);
    _splitter01->setOrientation(Qt::Vertical);
    _splitter01->addWidget(_lineFrame);
    _splitter01->addWidget(_buildFrame);
    _splitter01->setStretchFactor(0,10);
    _splitter01->setStretchFactor(1,10);
    _splitter02=new QSplitter(this);
    _splitter02->setOrientation(Qt::Horizontal);
    _splitter02->addWidget(_splitter01);
    _splitter02->addWidget(_mapView);
    _splitter02->setStretchFactor(0,30);
    _splitter02->setStretchFactor(1,70);
    _layout=new QVBoxLayout(this);
    _layout->setSpacing(10);
    _layout->setContentsMargins(0,0,0,0);
    _layout->addWidget(_splitter02);
    _layout->addWidget(_statusBar);
    _layout->setStretch(0,99);
    _layout->setStretch(1,1);
    setLayout(_layout);
    connect(SIGNALS,SIGNAL(sendStatusBarStringSignal(QString)),this,SLOT(receiveStatusBarStringSignal(QString)));
}
void RoutePlanner::receiveStatusBarStringSignal(QString str){
    _statusBar->showMessage(str);
}
