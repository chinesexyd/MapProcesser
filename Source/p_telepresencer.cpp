#include "p_telepresencer.h"
/*…………………………………………………………………MQTT………………………………………………………………………*/
MQTT::MQTT(QObject* parent):QObject(parent){
    _parent=static_cast<ContentProvider*>(parent);
    _client=new QMqttClient(this);
    connect(_client,&QMqttClient::connected,[this](){emit _parent->mqttConnectSignal();});
    connect(_client,&QMqttClient::disconnected,[this](){emit _parent->mqttDisconnectSignal();});
    connect(_client,SIGNAL(messageReceived(QByteArray,QMqttTopicName)),this,SLOT(messageReceivedSlot(QByteArray,QMqttTopicName)));
}
MQTT::~MQTT(){
    if(_client->state()!=QMqttClient::Disconnected){
        _client->disconnectFromHost();
    }
}
void MQTT::mqttConnect(){
    if(_client->state()==QMqttClient::Disconnected){
        _client->setHostname(_hostname);
        _client->setPort(_port);
        _client->setUsername(_username);
        _client->setPassword(_password);
        _client->setAutoKeepAlive(true);
        _client->connectToHost();
    }
}
void MQTT::mqttDisconnect(){
    if(_client->state()!=QMqttClient::Disconnected){
        _client->disconnectFromHost();
    }
}
void MQTT::subscribe(QString robotname){
    QString topic=QString("device/ros/%1/out").arg(robotname);
    _client->subscribe(topic);
}
void MQTT::dissubscribe(QString robotname){
    QString topic=QString("device/ros/%1/out").arg(robotname);
    _client->unsubscribe(topic);
}
void MQTT::messageReceivedSlot(QByteArray message,QMqttTopicName topic){
    QString topicname=topic.name();
    QString robotname=topicname.split("/")[2];
    QJsonDocument doc=QJsonDocument::fromJson(message);
    QJsonObject obj=doc.object();
    _parent->mqttReceived(robotname,obj);
}
void MQTT::messageSend(QString robotname,QByteArray message){
    QString topic=QString("device/ros/%1/in").arg(robotname);
    _client->publish(topic,message);
}
/*…………………………………………………………………ContentProvider………………………………………………………………………*/
ContentProvider::ContentProvider(QObject* parent):QObject(parent){
    _mqtt=new MQTT(this);
    _colorList<<QColor(255,0,0)<<QColor(255,165,0)<<QColor(0,255,0)<<QColor(0,127,255)<<QColor(0,0,255)<<QColor(139,0,255);
    _createNameColorFromIDMap();
}
ContentProvider::~ContentProvider(){}
void ContentProvider::mqttConnect(){
    _mqtt->mqttConnect();
}
void ContentProvider::mqttDisconnect(){
    _mqtt->mqttDisconnect();
}
void ContentProvider::mqttSubscribe(QString robotID){
    _mqtt->subscribe(robotID);
    _robotsInSubscribe.append(robotID);
    _getData(robotID);
}
void ContentProvider::mqttDissubscribe(QString robotID){
    _mqtt->dissubscribe(robotID);
    _robotsInSubscribe.removeOne(robotID);
    if(_robotsInMap.contains(robotID)){
        _robotsInMap.removeOne(robotID);
        emit robotOutSignal(robotID);
    }
    if(_robotsInData.contains(robotID)){
        closeData(robotID);
    }
    if(_robotsInLaser.contains(robotID)){
        closeLaser(robotID);
    }
    if(_robotsInCamera.contains(robotID)){
        closeCamera(robotID);
    }
    if(_robotsInControl.contains(robotID)){
        closeControl(robotID);
    }
    if(_robotsInRecorder.contains(robotID)){
        closeRecorder(robotID);
    }
}
void ContentProvider::beginTimer(){
    if(!_timerIsStart){
        _timerId=startTimer(200);
        _timerIsStart=true;
    }
}
void ContentProvider::endTimer(){
    if(_timerIsStart){
        killTimer(_timerId);
        _timerIsStart=false;
    }
}
void ContentProvider::watchMap(QString path){
    _viewType=WATCH_MAP;
    if(_watchedMapPath!=path&&!path.isEmpty()){
        _watchedMapPath=path;
        foreach (QString robotID, _robotsInMap) {
            _checkOut(robotID);
        }
        emit robotsOutSignal(_robotsInMap);
        emit mapChangedSignal(path);
        _robotsInMap.clear();
        foreach(QString robotID,_robotsInSubscribe){
            if(_robotsMapsMap[robotID]==path){
                _robotsInMap.append(robotID);
            }
        }
        emit robotsInSignal(_robotsInMap);
    }
    cancelFollowRobot();
}
void ContentProvider::followRobot(QString robotID){
    _viewType=FOLLOW_ROBOT;
    if(_robotsInSubscribe.contains(robotID)){
        _followedRobotID=robotID;
        if(_robotsMapsMap[robotID]!=_watchedMapPath&&!_robotsMapsMap[robotID].isEmpty()){
            _watchedMapPath=_robotsMapsMap[robotID];
            foreach (QString robotID, _robotsInMap) {
                _checkOut(robotID);
            }
            emit robotsOutSignal(_robotsInMap);
            emit mapChangedSignal(_watchedMapPath);
            _robotsInMap.clear();
            foreach(QString robotID,_robotsInSubscribe){
                if(_robotsMapsMap[robotID]==_watchedMapPath){
                    _robotsInMap.append(robotID);
                }
            }
            emit robotsInSignal(_robotsInMap);
        }
        emit followRobotSignal(robotID);
    }else{
        cancelFollowRobot();
    }
}
void ContentProvider::cancelFollowRobot(){
    _followedRobotID.clear();
}
void ContentProvider::openLaser(QString robotID){
    if(!_robotsInLaser.contains(robotID)){
        _robotsInLaser.append(robotID);
        emit appendLaserSignal(robotID);
    }
}
void ContentProvider::closeLaser(QString robotID){
    if(_robotsInLaser.contains(robotID)){
        _robotsInLaser.removeOne(robotID);
        emit removeLaserSignal(robotID);
    }
}
void ContentProvider::openCamera(QString robotID){
    if(!_robotsInCamera.contains(robotID)){
        _robotsInCamera.append(robotID);
        emit appendCameraItemSignal(robotID);
    }
}
void ContentProvider::closeCamera(QString robotID){
    if(_robotsInCamera.contains(robotID)){
        _robotsInCamera.removeOne(robotID);
        emit removeCameraItemSignal(robotID);
    }
}
void ContentProvider::resumeTask(QString robotID){
    _robotsIsStopTask[robotID]=false;
    QJsonObject json;
    QJsonDocument document;
    json.insert("type", QString("moveResume"));
    document.setObject(json);
    QByteArray toByte = document.toJson(QJsonDocument::Compact);
    _mqtt->messageSend(robotID,toByte);
}
void ContentProvider::cancelTask(QString robotID){
    _robotsIsStopTask[robotID]=true;
    QJsonObject json;
    QJsonDocument document;
    json.insert("type", QString("moveCancel"));
    document.setObject(json);
    QByteArray toByte = document.toJson(QJsonDocument::Compact);
    _mqtt->messageSend(robotID,toByte);
}
void ContentProvider::openDoor(QString robotID){
    QJsonObject json;
    QJsonDocument document;
    json.insert("type", "door");
    json.insert("open",true);
    document.setObject(json);
    QByteArray toByte = document.toJson(QJsonDocument::Compact);
    _mqtt->messageSend(robotID,toByte);
}
void ContentProvider::closeDoor(QString robotID){
    QJsonObject json;
    QJsonDocument document;
    json.insert("type", "door");
    json.insert("open",false);
    document.setObject(json);
    QByteArray toByte = document.toJson(QJsonDocument::Compact);
    _mqtt->messageSend(robotID,toByte);
}
void ContentProvider::initPoint(QString robotID,TOOL::XYA_POINT pos){
    TOOL::XYZW_POINT xyzwPoint=XYZWPosFromXYAPos(pos);
    QJsonObject json;
    QJsonDocument document;
    json.insert("type", "initialPoseSimple");
    json.insert("x",xyzwPoint.x);
    json.insert("y", xyzwPoint.y);
    json.insert("z",xyzwPoint.z);
    json.insert("w",xyzwPoint.w);
    document.setObject(json);
    QByteArray toByte = document.toJson(QJsonDocument::Compact);
    _mqtt->messageSend(robotID,toByte);
}
void ContentProvider::moveSimpleGoal(QString robotID,TOOL::XYA_POINT pos){
    TOOL::XYZW_POINT xyzwPoint=XYZWPosFromXYAPos(pos);
    QJsonObject json;
    QJsonDocument document;
    json.insert("type", "moveSimpleGoal");
    json.insert("x",xyzwPoint.x);
    json.insert("y", xyzwPoint.y);
    json.insert("z",xyzwPoint.z);
    json.insert("w",xyzwPoint.w);
    document.setObject(json);
    QByteArray toByte = document.toJson(QJsonDocument::Compact);
    _mqtt->messageSend(robotID,toByte);
}
void ContentProvider::moveRobot(QString robotID,double v, double vth){
    QJsonObject json;
    QJsonDocument document;
    json.insert("type", "cmdVel");
    json.insert("v",v);
    json.insert("vth",vth);
    document.setObject(json);
    QByteArray toByte = document.toJson(QJsonDocument::Compact);
    _mqtt->messageSend(robotID,toByte);
}
void ContentProvider::getRecorder(QString robotID,quint8 order,int number){
    QJsonObject json;
    QJsonDocument document;
    json.insert("type", "camera");
    json.insert("action","getRecorder");
    json.insert("order",order);
    json.insert("number",number);
    document.setObject(json);
    QByteArray toByte = document.toJson(QJsonDocument::Compact);
    _mqtt->messageSend(robotID,toByte);
}
void ContentProvider::moveResume(QString robotID){
    QJsonObject json;
    QJsonDocument document;
    json.insert("type", "moveResume");
    json.insert("skip",true);
    document.setObject(json);
    QByteArray toByte = document.toJson(QJsonDocument::Compact);
    _mqtt->messageSend(robotID,toByte);
}
void ContentProvider::setStop(QString robotID){
    QJsonObject json;
    QJsonDocument document;
    json.insert("type", "setStop");
    document.setObject(json);
    QByteArray toByte = document.toJson(QJsonDocument::Compact);
    _mqtt->messageSend(robotID,toByte);
}
void ContentProvider::startTalk(QString robotID){
    QJsonObject json;
    QJsonDocument document;
    json.insert("type", "talkCall");
    json.insert("channel","");
    document.setObject(json);
    QByteArray toByte = document.toJson(QJsonDocument::Compact);
    _mqtt->messageSend(robotID,toByte);
    int result=system("ffmpeg -f alsa -i hw:0 -f flv rtmp://srs.jilida.net.cn/live/xxxx_in");
    Q_UNUSED(result);
}
void ContentProvider::endTalk(QString robotID){
    QJsonObject json;
    QJsonDocument document;
    json.insert("type", "talkCall");
    document.setObject(json);
    QByteArray toByte = document.toJson(QJsonDocument::Compact);
    _mqtt->messageSend(robotID,toByte);
    int result=system("killall ffmpeg");
    Q_UNUSED(result);
}
TOOL::XYA_POINT ContentProvider::XYAPosFromXYZWPos(TOOL::XYZW_POINT pos){
    double yaw = atan2(2 * (pos.w * pos.z), 1 - 2 * (pos.z * pos.z));
    double angle = yaw/M_PI*180;
    TOOL::XYA_POINT xyaPoint;
    xyaPoint.x = pos.x;
    xyaPoint.y = pos.y;
    xyaPoint.angle = angle;
    return xyaPoint;
}
TOOL::XYZW_POINT ContentProvider::XYZWPosFromXYAPos(TOOL::XYA_POINT pos){
    double angle = pos.angle;
    double yaw = angle/180.0*M_PI;
    double cy = cos(yaw*0.5);
    double sy = sin(yaw*0.5);
    double cp = 1;
    double sp = 0;
    double cr = 1;
    double sr = 0;
    TOOL::XYZW_POINT xyzwPoint;
    xyzwPoint.w = cy*cp*cr+sy*sp*sr;
    xyzwPoint.z = sy*cp*cr-cy*sp*sr;
    xyzwPoint.x = pos.x;
    xyzwPoint.y = pos.y;
    return xyzwPoint;
}
void ContentProvider::openData(QString robotID){
    if(!_robotsInData.contains(robotID)){
        _robotsInData.append(robotID);
        emit appendDataItemSignal(robotID);
    }
}
void ContentProvider::closeData(QString robotID){
    if(_robotsInData.contains(robotID)){
        _robotsInData.removeOne(robotID);
        emit removeDataItemSignal(robotID);
    }
}
void ContentProvider::openControl(QString robotID){
    if(!_robotsInControl.contains(robotID)){
        _robotsInControl.append(robotID);
        emit appendControlItemSignal(robotID);
    }
}
void ContentProvider::closeControl(QString robotID){
    if(_robotsInControl.contains(robotID)){
        _robotsInControl.removeOne(robotID);
        emit removeControlItemSignal(robotID);
    }
}
void ContentProvider::openRecorder(QString robotID){
    if(!_robotsInRecorder.contains(robotID)){
        _robotsInRecorder.append(robotID);
        emit appendRecorderItemSignal(robotID);
    }
}
void ContentProvider::closeRecorder(QString robotID){
    if(_robotsInRecorder.contains(robotID)){
        _robotsInRecorder.removeOne(robotID);
        emit removeRecorderItemSignal(robotID);
    }
}
void ContentProvider::timerEvent(QTimerEvent *){
    _timerCount++;
    foreach (QString robotID, _robotsInMap) {
        _getPosition(robotID);
        if(_robotsInLaser.contains(robotID)){
            _getLaser(robotID);
        }
        if(_robotsInCamera.contains(robotID)){
            _getPicture(robotID);
        }
    }
    //1s
    if(_timerCount==5){
        _timerCount=0;
        foreach (QString robotID, _robotsInSubscribe) {
            if(_robotsInData.contains(robotID)){
                _getData(robotID);
            }
        }
    }
}
void ContentProvider::_getLaser(QString robotID){
    QJsonObject json;
    QJsonDocument document;
    json.insert("type", QString("scanDataXY"));
    document.setObject(json);
    QByteArray toByte = document.toJson(QJsonDocument::Compact);
    _mqtt->messageSend(robotID,toByte);
}
void ContentProvider::_getPicture(QString robotID){
    QJsonObject json;
    QJsonDocument document;
    json.insert("type","camera");
    json.insert("action","getPicture");
    json.insert("quality",20);
    document.setObject(json);
    QByteArray toByte = document.toJson(QJsonDocument::Compact);
    _mqtt->messageSend(robotID,toByte);
}
void ContentProvider::_getPosition(QString robotID){
    QJsonObject json;
    QJsonDocument document;
    json.insert("type", "getPosition");
    document.setObject(json);
    QByteArray toByte = document.toJson(QJsonDocument::Compact);
    _mqtt->messageSend(robotID,toByte);
}
void ContentProvider::_getData(QString robotID){
    QJsonObject json;
    QJsonDocument document;
    json.insert("type", "upLoadRobot");
    document.setObject(json);
    QByteArray toByte = document.toJson(QJsonDocument::Compact);
    _mqtt->messageSend(robotID,toByte);
}
void ContentProvider::_checkOut(QString robotID){
    if(_robotsInData.contains(robotID)){
        _robotsInLaser.removeOne(robotID);
        emit removeDataItemSignal(robotID);
    }
    if(_robotsInLaser.contains(robotID)){
        _robotsInLaser.removeOne(robotID);
    }
    if(_robotsInCamera.contains(robotID)){
        _robotsInCamera.removeOne(robotID);
        emit removeCameraItemSignal(robotID);
    }
    if(_robotsInControl.contains(robotID)){
        _robotsInControl.removeOne(robotID);
        emit removeControlItemSignal(robotID);
    }
}
void ContentProvider::_createNameColorFromIDMap(){
    QFile file(ROBOTS_FILE);
    if(file.exists()){
        file.open(QIODevice::ReadOnly);
        QJsonDocument doc=QJsonDocument::fromJson(file.readAll());
        file.close();
        QJsonObject obj=doc.object();
        QStringList areaList=obj.keys();
        foreach(QString area,areaList){
            QJsonObject areaObj=obj.value(area).toObject();
            QStringList nameList=areaObj.keys();
            for(int i=0;i<nameList.length();i++){
                QString name=nameList[i];
                QString id=areaObj.value(name).toString();
                _colorMap.insert(id,_colorList[i]);
                _nameMap.insert(id,name);
            }
        }
    }
}
void ContentProvider::mqttReceived(QString robotID,QJsonObject obj){
    QString type=obj.value("type").toString();
    if(type=="robotState"){
        QString map=obj.value("mapFile").toString();
        _robotsMapsMap[robotID]=map;
        if(_viewType==FOLLOW_ROBOT&&robotID==_followedRobotID&&_watchedMapPath!=map){
            followRobot(robotID);
        }
        if(map==_watchedMapPath){
            if(!_robotsInMap.contains(robotID)){
                _robotsInMap.append(robotID);
                emit robotInSignal(robotID);
            }
        }else{
            if(_robotsInMap.contains(robotID)){
                _robotsInMap.removeOne(robotID);
                emit robotOutSignal(robotID);
            }
        }
        if(_robotsInMap.contains(robotID)){
            emit mqttReceivedPositionSignal(robotID,obj);
            emit mqttReceivedRobotStateSignal(robotID,obj);
        }
        _robotsIsOpenDoor[robotID]=obj.value("doorState").toBool();
    }else if(type=="ackScanDataXY"){
        emit mqttReceivedScanDataSignal(robotID,obj);
    }else if(type=="ackCamera"){
        QString action=obj.value("action").toString();
        if(action=="getPicture"){
            emit mqttReceivedCameraPictureSignal(robotID,obj);
        }else if(action=="getRecorder"){
            emit mqttReceivedCameraRecorderSiganl(robotID,obj);
        }
    }else if(type=="ackGetPosition"){
        QString map=obj.value("mapFile").toString();
        _robotsMapsMap[robotID]=map;
        if(_viewType==FOLLOW_ROBOT&&robotID==_followedRobotID&&_watchedMapPath!=map){
            followRobot(robotID);
        }
        if(map==_watchedMapPath){
            if(!_robotsInMap.contains(robotID)){
                _robotsInMap.append(robotID);
                emit robotInSignal(robotID);
            }
        }else{
            if(_robotsInMap.contains(robotID)){
                _robotsInMap.removeOne(robotID);
                emit robotOutSignal(robotID);
            }
        }
        if(_robotsInMap.contains(robotID)){
            emit mqttReceivedPositionSignal(robotID,obj);
        }
    }
}
ContentProvider* contentProvider=new ContentProvider();
/*…………………………………………………………………RobotNode………………………………………………………………………*/
RobotNode::RobotNode(RobotNode* parentNode){
    _isRoot=false;
    _parentNode=parentNode;
}
RobotNode::~RobotNode(){
    foreach(const RobotNode* node, _childNodes) {
        delete node;
    }
}
void RobotNode::openFile(QString jsonPath){
    _isRoot=true;
    _isRobot=false;
    QFile file(jsonPath);
    if(file.exists()){
        file.open(QIODevice::ReadOnly);
        QJsonDocument doc=QJsonDocument::fromJson(file.readAll());
        file.close();
        _obj=doc.object();
        QStringList keys=_obj.keys();
        for(int i=0;i<keys.length();i++){
            RobotNode* node=new RobotNode(this);
            node->setRow(i);
            node->setRobot(false);
            QString key=keys[i];
            node->setValue(key,_obj.value(key).toObject());
            _childNodes.append(node);
        }
    }
}
void RobotNode::setValue(QString name,QString robotID){
    _name=name;
    _robotID=robotID;
}
void RobotNode::setValue(QString name,QJsonObject obj){
    _name=name;
    _obj=obj;
    QStringList keys=_obj.keys();
    for(int i=0;i<keys.length();i++){
        RobotNode* node=new RobotNode(this);
        node->setRow(i);
        node->setRobot(true);
        QString key=keys[i];
        node->setValue(key,_obj.value(key).toString());
        _childNodes.append(node);
    }
}
void RobotNode::setRobot(bool isTrue){
    _isRobot=isTrue;
}
bool RobotNode::isRobot(){
    return _isRobot;
}
RobotNode* RobotNode::child(int row){
    return _childNodes[row];
}
RobotNode* RobotNode::parentNode(){
    return _parentNode;
}
bool RobotNode::hasChild(){
    return !_isRobot;
}
int RobotNode::childCount(){
    return _childNodes.length();
}
int RobotNode::row(){
    return _row;
}
QVariant RobotNode::data(int column){
    if(column==0){
        return _name;
    }else{
        return _robotID;
    }
}
void RobotNode::setRow(int row){
    _row=row;
}
QString RobotNode::name(){
    return _name;
}
QString RobotNode::id(){
    return _robotID;
}
QJsonObject RobotNode::object(){
    return _obj;
}
void RobotNode::subscribe(){
    if(_isRobot){
        if(!_isSubscribe){
            contentProvider->mqttSubscribe(_robotID);
            _isSubscribe=true;
            _parentNode->checkIsSubScribe();
        }
    }else{
        foreach (RobotNode* node, _childNodes) {
            node->subscribe();
            checkIsSubScribe();
        }
    }
}
void RobotNode::unsubscribe(){
    if(_isRobot){
        if(_isSubscribe){
            contentProvider->mqttDissubscribe(_robotID);
            _isSubscribe=false;
            _parentNode->checkIsSubScribe();
        }
    }else{
        foreach (RobotNode* node, _childNodes) {
            node->unsubscribe();
            checkIsSubScribe();
        }
    }
}
bool RobotNode::isSubscribe(){
    return _isSubscribe;
}
void RobotNode::checkIsSubScribe(){
    bool flag=true;
    foreach (RobotNode* node, _childNodes) {
        if(!node->isSubscribe()){
            flag=false;
            break;
        }
    }
    _isSubscribe=flag;
}
/*…………………………………………………………………RobotModel……………………………………………………………………*/
RobotModel::RobotModel(QWidget* parent):QAbstractItemModel(parent){
    _headerList<<"小车"<<"订阅状态";
    _colorList<<QColor(255,0,0)<<QColor(255,165,0)<<QColor(0,255,0)<<QColor(0,127,255)<<QColor(0,0,255)<<QColor(139,0,255);
}
RobotModel::~RobotModel(){
    delete _rootNode;
}
QVariant RobotModel::headerData(int section, Qt::Orientation orientation, int role) const{
    if(orientation==Qt::Horizontal){
        if(role==Qt::DisplayRole){
            return _headerList[section];
        }else if(role==Qt::TextAlignmentRole){
            return Qt::AlignCenter;
        }
    }
    return QVariant();
}
QVariant RobotModel::data(const QModelIndex &index, int role) const{
    if(!index.isValid()){
        return QVariant();
    }
    RobotNode* node=nodeFromIndex(index);
    if(role==Qt::DisplayRole){
        if(index.column()==0){
            return node->name();
        }
    }else if(role==Qt::DecorationRole){
        if(index.column()==0){
            if(nodeFromIndex(index)->isRobot()){
                return QIcon(":/Image/Robot.png");
            }else{
                return QIcon(":/Image/Area.png");
            }
        }
    }else if(role==Qt::UserRole){
        return nodeFromIndex(index)->isRobot();
    }else if(role==Qt::ToolTipRole){
        return nodeFromIndex(index)->id();
    }else if(role==Qt::TextColorRole){
        if(hasChildren(index)){
            return QColor(Qt::white);
        }
        return _colorList[index.row()];
    }else if(role==Qt::EditRole){
        if(index.column()==1){
           return node->isSubscribe();
        }
    }
    return QVariant();
}
Qt::ItemFlags RobotModel::flags(const QModelIndex &index)const{
    return QAbstractItemModel::flags(index);
}
QModelIndex RobotModel::index(int row, int column, const QModelIndex &parent) const{
    if(!hasIndex(row,column,parent)){
        return QModelIndex();
    }
    RobotNode* parentNode=nodeFromIndex(parent);
    RobotNode* node=parentNode->child(row);
    if(node){
        return createIndex(row,column,node);
    }else{
        return QModelIndex();
    }
}
QModelIndex RobotModel::parent(const QModelIndex &child) const{
    if(!child.isValid()){
        return QModelIndex();
    }
    RobotNode* node=nodeFromIndex(child);
    RobotNode* parentNode=node->parentNode();
    if(parentNode==_rootNode){
        return QModelIndex();
    }
    return createIndex(parentNode->row(),0,parentNode);
}
int RobotModel::rowCount(const QModelIndex &parent) const{
    if(parent.column()>0){
        return 0;
    }
    RobotNode* parentNode=nodeFromIndex(parent);
    return parentNode->childCount();
}
int RobotModel::columnCount(const QModelIndex &parent)const{
    if(parent.column()>0){
        return 0;
    }
    return 2;
}
bool RobotModel::hasChildren(const QModelIndex &parent) const{
    if(parent.column()>0){
        return 0;
    }
    RobotNode* parentNode=nodeFromIndex(parent);
    return parentNode->hasChild();
}
bool RobotModel::setData(const QModelIndex &index, const QVariant &value, int role){
    if(index.column()==1){
        if(role==Qt::EditRole){
            RobotNode* node=nodeFromIndex(index);
            if(value.toBool()){
                node->subscribe();
            }else{
                node->unsubscribe();
            }
            emit dataChanged(index,index);
            RobotNode* parentNode=node->parentNode();
            if(parentNode!=_rootNode){
                QModelIndex parent=createIndex(parentNode->row(),1,parentNode);
                emit dataChanged(parent,parent);
            }else{
                for(int i=0;i<node->childCount();i++){
                    QModelIndex child=createIndex(i,1,node->child(i));
                    emit dataChanged(child,child);
                }
            }
        }
    }
    return true;
}
RobotNode* RobotModel::nodeFromIndex(const QModelIndex &index) const{
    if(index.isValid()){
        RobotNode* node=static_cast<RobotNode*>(index.internalPointer());
        return node;
    }else{
        return _rootNode;
    }
}
void RobotModel::resetModel(){
    beginResetModel();
    _rootNode=new RobotNode();
    _rootNode->openFile(ROBOTS_FILE);
    endResetModel();
}
bool RobotModel::isRobot(const QModelIndex &index){
    return nodeFromIndex(index)->isRobot();
}
/*…………………………………………………………………RobotProxyModel……………………………………………………………………*/
RobotDelegate::RobotDelegate(QWidget* parent):QStyledItemDelegate(parent){
    _pixmapSubscribe=QPixmap(":/Image/Subscribe.png");
    _pixmapUnsubscribe=QPixmap(":/Image/Unsubscribe.png");
    _pixmapSubscribeAll=QPixmap(":/Image/SubscribeAll.png");
    _pixmapUnsubscribeAll=QPixmap(":/Image/UnsubscribeAll.png");
}
void RobotDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const{
    QStyledItemDelegate::paint(painter,option,index);
    if(index.column()==1){
        QRect wRect=option.rect;
        bool isSubscribe=index.model()->data(index,Qt::EditRole).toBool();
        bool isRobot=index.model()->data(index,Qt::UserRole).toBool();
        painter->setRenderHint(QPainter::SmoothPixmapTransform);
        double unit=wRect.height()/(float)10;
        QRectF rect=QRectF((wRect.width()-unit*8)/(float)2+wRect.x(),unit+wRect.y(),unit*8,unit*8);
        if(isRobot){
            if(isSubscribe){
                painter->drawPixmap(rect,_pixmapSubscribe,_pixmapSubscribe.rect());
            }else{
                painter->drawPixmap(rect,_pixmapUnsubscribe,_pixmapUnsubscribe.rect());
            }
        }else{
            if(isSubscribe){
                painter->drawPixmap(rect,_pixmapSubscribeAll,_pixmapSubscribeAll.rect());
            }else{
                painter->drawPixmap(rect,_pixmapUnsubscribeAll,_pixmapUnsubscribeAll.rect());
            }
        }
    }
}
bool RobotDelegate::editorEvent(QEvent *event, QAbstractItemModel *model, const QStyleOptionViewItem &option, const QModelIndex &index){
    QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
    if(event->type()==QEvent::MouseButtonPress&&option.rect.contains(mouseEvent->pos())){
        int column=index.column();
        if(column==1){
            bool data = model->data(index, Qt::EditRole).toBool();
            model->setData(index, !data, Qt::EditRole);
            return true;
        }
    }
    return QStyledItemDelegate::editorEvent(event,model,option,index);
}
/*…………………………………………………………………RobotView……………………………………………………………………*/
RobotView::RobotView(QWidget* parent):QTreeView(parent){
    _robotModel=new RobotModel(this);
    _robotModel->resetModel();
    _robotDelegate=new RobotDelegate(this);
    setItemDelegate(_robotDelegate);
    setModel(_robotModel);
    setColumnWidth(0,200);
    setContextMenuPolicy(Qt::CustomContextMenu);
    _menu=new QMenu(this);
    _followAction=new QAction(_menu);
    _followAction->setCheckable(true);
    _followAction->setText("跟随");
    _followAction->setIcon(QIcon(":/Image/Follow.png"));
    _followAction->setToolTip("用于跟随或取消跟随");
    _laserAction=new QAction(_menu);
    _laserAction->setCheckable(true);
    _laserAction->setText("激光");
    _laserAction->setIcon(QIcon(":/Image/Laser.png"));
    _laserAction->setToolTip("用于判断小车是否偏");
    _showDataAction=new QAction(_menu);
    _showDataAction->setCheckable(true);
    _showDataAction->setText("数据");
    _showDataAction->setIcon(QIcon(":/Image/Data.png"));
    _showDataAction->setToolTip("用于展示或关闭数据");
    _cameraAction=new QAction(_menu);
    _cameraAction->setCheckable(true);
    _cameraAction->setText("图像");
    _cameraAction->setIcon(QIcon(":/Image/Camera.png"));
    _cameraAction->setToolTip("用于展示或关闭图像");
    _historyRecorderAction=new QAction(_menu);
    _historyRecorderAction->setCheckable(true);
    _historyRecorderAction->setText("查看历史影像");
    _historyRecorderAction->setIcon(QIcon(":/Image/HistoryRecorder.png"));
    _historyRecorderAction->setToolTip("用于查看历史录像");
    _sensitiveMenu=new QMenu(_menu);
    _sensitiveMenu->setTitle("敏感操作");
    _sensitiveMenu->setIcon(QIcon(":/Image/Sensitive.png"));
    _controlAction=new QAction(_sensitiveMenu);
    _controlAction->setCheckable(true);
    _controlAction->setText("操控盘");
    _controlAction->setIcon(QIcon(":/Image/Follow.png"));
    _controlAction->setToolTip("用于跟随或取消跟随");
    _stopTaskAction=new QAction(_sensitiveMenu);
    _stopTaskAction->setText("中止任务");
    _stopTaskAction->setToolTip("用于暂停任务");
    _stopTaskAction->setIcon(QIcon(":/Image/Task.png"));
    _resumeTaskAction=new QAction(_sensitiveMenu);
    _resumeTaskAction->setText("继续任务");
    _resumeTaskAction->setToolTip("用于继续任务");
    _resumeTaskAction->setIcon(QIcon(":/Image/Task.png"));
    _doorAction=new QAction(_sensitiveMenu);
    _doorAction->setText("车门");
    _doorAction->setToolTip("用于打开或关闭车门");
    _doorAction->setCheckable(true);
    _doorAction->setIcon(QIcon(":/Image/Door.png"));
    _setStopAction=new QAction(_sensitiveMenu);
    _setStopAction->setText("解除急停");
    _setStopAction->setToolTip("被人按下急停按钮，可以远程解除急停");
    _setStopAction->setIcon(QIcon(":/Image/UnlockStop.png"));
    _moveResumeAction=new QAction(_sensitiveMenu);
    _moveResumeAction->setText("跳点");
    _moveResumeAction->setToolTip("放弃当前目标点并继续任务");
    _moveResumeAction->setIcon(QIcon(":/Image/JumpPoint.png"));
    _sensitiveMenu->addAction(_controlAction);
    _sensitiveMenu->addAction(_stopTaskAction);
    _sensitiveMenu->addAction(_resumeTaskAction);
    _sensitiveMenu->addAction(_doorAction);
    _sensitiveMenu->addAction(_setStopAction);
    _sensitiveMenu->addAction(_moveResumeAction);
    _menu->addAction(_followAction);
    _menu->addAction(_laserAction);
    _menu->addAction(_showDataAction);
    _menu->addAction(_cameraAction);
    _menu->addAction(_historyRecorderAction);
    _menu->addMenu(_sensitiveMenu);
    connect(this,SIGNAL(customContextMenuRequested(QPoint)),this,SLOT(showMenuSlot(QPoint)));
    connect(_followAction,SIGNAL(triggered(bool)),this,SLOT(followActionTriggered(bool)));
    connect(_laserAction,SIGNAL(triggered(bool)),this,SLOT(laserActionTriggered(bool)));
    connect(_cameraAction,SIGNAL(triggered(bool)),this,SLOT(cameraActionTriggered(bool)));
    connect(_stopTaskAction,SIGNAL(triggered()),this,SLOT(stopTaskActionTriggered()));
    connect(_resumeTaskAction,SIGNAL(triggered()),this,SLOT(resumeTaskActionTriggered()));
    connect(_doorAction,SIGNAL(triggered(bool)),this,SLOT(doorActionTriggered(bool)));
    connect(_historyRecorderAction,SIGNAL(triggered(bool)),this,SLOT(historyRecorderActionTriggered(bool)));
    connect(_showDataAction,SIGNAL(triggered(bool)),this,SLOT(showDataActionTriggered(bool)));
    connect(_setStopAction,SIGNAL(triggered()),this,SLOT(setStopActionTriggered()));
    connect(_moveResumeAction,SIGNAL(triggered()),this,SLOT(moveResumeActionTriggered()));
}
void RobotView::_checkIsChecked(){
    RobotNode* node=static_cast<RobotNode*>(_currentIndex.internalPointer());
    QString robotID=node->id();
    _followAction->setChecked(contentProvider->isFollowRobot(robotID));
    _laserAction->setChecked(contentProvider->isInLaser(robotID));
    _controlAction->setChecked(contentProvider->isInControl(robotID));
    _showDataAction->setChecked(contentProvider->isInData(robotID));
    _cameraAction->setChecked(contentProvider->isInCamera(robotID));
    _doorAction->setChecked(contentProvider->isOpenDoor(robotID));
    _historyRecorderAction->setChecked(contentProvider->isInRecorder(robotID));
}
void RobotView::showMenuSlot(QPoint pos){
    //计算鼠标相对父窗口的位置
    QPoint pt=mapToGlobal(pos);
    //判断鼠标右击位置是否是空白处，空白处则取消上一个选中焦点，不弹出菜单
    _currentIndex = indexAt(pos);
    if(_currentIndex.isValid()){
        RobotNode* node=static_cast<RobotNode*>(_currentIndex.internalPointer());
        if(node->isRobot()){
            QPoint point=QPoint(pt.x(),pt.y()+30);
            _menu->exec(point);
        }
    }
}
void RobotView::followActionTriggered(bool isChecked){
    RobotNode* node=static_cast<RobotNode*>(_currentIndex.internalPointer());
    if(isChecked){
        contentProvider->cancelFollowRobot();
    }else{
        contentProvider->followRobot(node->id());
    }
}
void RobotView::laserActionTriggered(bool isChecked){
    RobotNode* node=static_cast<RobotNode*>(_currentIndex.internalPointer());
    QString robotID=node->id();
    if(isChecked){
        contentProvider->openLaser(robotID);
    }else{
        contentProvider->closeLaser(robotID);
    }
}
void RobotView::cameraActionTriggered(bool isChecked){
    RobotNode* node=static_cast<RobotNode*>(_currentIndex.internalPointer());
    QString robotID=node->id();
    if(isChecked){
        contentProvider->openCamera(robotID);
    }else{
        contentProvider->closeCamera(robotID);
    }
}
void RobotView::stopTaskActionTriggered(){
    RobotNode* node=static_cast<RobotNode*>(_currentIndex.internalPointer());
    QString robotID=node->id();
    contentProvider->cancelTask(robotID);
}
void RobotView::resumeTaskActionTriggered(){
    RobotNode* node=static_cast<RobotNode*>(_currentIndex.internalPointer());
    QString robotID=node->id();
    contentProvider->resumeTask(robotID);
}
void RobotView::doorActionTriggered(bool isChecked){
    RobotNode* node=static_cast<RobotNode*>(_currentIndex.internalPointer());
    QString robotID=node->id();
    if(isChecked){
        contentProvider->openDoor(robotID);
    }else{
        contentProvider->closeDoor(robotID);
    }
}
void RobotView::historyRecorderActionTriggered(bool isChecked){
    RobotNode* node=static_cast<RobotNode*>(_currentIndex.internalPointer());
    QString robotID=node->id();
    if(isChecked){
        contentProvider->openRecorder(robotID);
    }else{
        contentProvider->closeRecorder(robotID);
    }
}
void RobotView::showDataActionTriggered(bool isChecked){
    RobotNode* node=static_cast<RobotNode*>(_currentIndex.internalPointer());
    QString robotID=node->id();
    if(isChecked){
        contentProvider->openData(robotID);
    }else{
        contentProvider->closeData(robotID);
    }
}
void RobotView::setStopActionTriggered(){
    RobotNode* node=static_cast<RobotNode*>(_currentIndex.internalPointer());
    contentProvider->setStop(node->id());
}
void RobotView::moveResumeActionTriggered(){
    RobotNode* node=static_cast<RobotNode*>(_currentIndex.internalPointer());
    QMessageBox::StandardButton xx=QMessageBox::warning(this,"警告","是否要跳过当前目标点？",QMessageBox::Ok|QMessageBox::Cancel,QMessageBox::Ok);
    if(xx==QMessageBox::Ok){
        contentProvider->moveResume(node->id());
    }
}
/*…………………………………………………………………RobotFrame……………………………………………………………………*/
RobotFrame::RobotFrame(QWidget* parent):QFrame(parent){
    _robotView=new RobotView(this);
    _connectButton=new QPushButton(this);
    _connectButton->setText("连接");
    _layout=new QVBoxLayout(this);
    _layout->setContentsMargins(0,0,0,0);
    _layout->addWidget(_robotView);
    _layout->addWidget(_connectButton);
    connect(_connectButton,SIGNAL(clicked()),this,SLOT(connectButtonClickedSlot()));
    connect(contentProvider,SIGNAL(mqttConnectSignal()),this,SLOT(mqttConnectSlot()));
    connect(contentProvider,SIGNAL(mqttDisconnectSignal()),this,SLOT(mqttDisconnectSlot()));
}
void RobotFrame::connectButtonClickedSlot(){
    if(_connectButton->text()=="连接"){
        contentProvider->mqttConnect();
    }else{
        contentProvider->mqttDisconnect();
    }
}
void RobotFrame::mqttConnectSlot(){
    _connectButton->setText("断开连接");
}
void RobotFrame::mqttDisconnectSlot(){
    _connectButton->setText("连接");
}
/*…………………………………………………………………MapNode………………………………………………………………………*/
MapNode::MapNode(MapNode* parentNode){
    _parentNode=parentNode;
}
MapNode::~MapNode(){
    foreach (MapNode* node, _childNodes) {
        delete node;
    }
}
void MapNode::openDir(QString dirpath){
    QDir dir(dirpath);
    if(dir.exists()){
        QStringList childpaths=dir.entryList(QDir::Dirs|QDir::NoDotAndDotDot);
        for(int i=0;i<childpaths.length();i++){
            QString childpath=dirpath+"/"+childpaths[i];
            MapNode* node=new MapNode(this);
            node->setName(childpaths[i]);
            node->setType(AREA);
            node->setPath(childpath);
            node->setRow(i);
            _childNodes.append(node);
        }
    }
}
bool MapNode::hasChild(){
    return !(_type==UNIT||_type==GARBAGE);
}
void MapNode::setPath(QString dirpath){
    QDir dir(dirpath);
    if(dir.exists()){
        if(_type!=GARBAGE&&_type!=UNIT){
            QStringList dirpaths=dir.entryList(QDir::Dirs|QDir::NoDotAndDotDot);
            for(int k=0;k<dirpaths.length();k++){
                QString _dirpath=dirpath+"/"+dirpaths[k];
                MapNode* node=new MapNode(this);
                if(dirpaths[k].contains("building")){
                    node->setName(dirpaths[k].replace("building_","楼栋"));
                    node->setType(BUILDING);
                }else if(dirpaths[k].contains("unit")){
                    node->setName(dirpaths[k].replace("unit_","单元"));
                    node->setType(UNIT);
                }else if(dirpaths[k].contains("garbage")){
                    node->setName(dirpaths[k].replace("garbage","地下室"));
                    node->setType(GARBAGE);
                }
                node->setPath(_dirpath);
                node->setRow(k);
                _childNodes.append(node);
            }
        }else{
            QStringList filepaths=dir.entryList(QDir::Files);
            for(int m=0;m<filepaths.length();m++){
                if(filepaths[m].contains("pgm")){
                    _mapInfo.pgm=dirpath+"/"+filepaths[m];
                }
            }
        }
    }
}
void MapNode::setType(MAP_TYPE type){
    _type=type;
}
MapNode::MAP_TYPE MapNode::type(){
    return _type;
}
TOOL::PGM_YAML MapNode::info(){
    return _mapInfo;
}
void MapNode::setName(QString name){
    _name=name;
}
MapNode* MapNode::child(int row){
    return _childNodes[row];
}
MapNode* MapNode::parent(){
    return _parentNode;
}
int MapNode::childCount(){
    return _childNodes.length();
}
int MapNode::row(){
    return _row;
}
void MapNode::setRow(int row){
    _row=row;
}
QVariant MapNode::data(int column){
    if(column==0){
        return _name;
    }else{
        return QVariant();
    }
}
/*…………………………………………………………………MapModel……………………………………………………………………*/
MapModel::MapModel(QWidget* parent):QAbstractItemModel(parent){}
MapModel::~MapModel(){
    delete _rootNode;
}
void MapModel::resetModel(){
    beginResetModel();
    _rootNode=new MapNode();
    _rootNode->openDir(MAPS_DIR);
    endResetModel();
}
QVariant MapModel::headerData(int section, Qt::Orientation orientation, int role) const{
    Q_UNUSED(section);
    if(orientation==Qt::Horizontal){
        if(role==Qt::DisplayRole){
            return "地图";
        }else if(role==Qt::TextAlignmentRole){
            return Qt::AlignCenter;
        }
    }
    return QVariant();
}
QVariant MapModel::data(const QModelIndex &index, int role) const{
    if(role==Qt::DisplayRole){
        if(index.column()==0){
            return nodeFromIndex(index)->data(index.column());
        }
    }else if(role==Qt::DecorationRole){
        MapNode::MAP_TYPE type=nodeFromIndex(index)->type();
        switch (type) {
        case MapNode::AREA:
            return QIcon(":/Image/Area.png");
            break;
        case MapNode::BUILDING:
            return QIcon(":/Image/Building.png");
            break;
        case MapNode::UNIT:
            return QIcon(":/Image/Unit.png");
            break;
        case MapNode::GARBAGE:
            return QIcon(":/Image/Garbage.png");
            break;
        default:
            break;
        }
    }
    return QVariant();
}
Qt::ItemFlags MapModel::flags(const QModelIndex &index)const{
    return QAbstractItemModel::flags(index);
}
QModelIndex MapModel::index(int row, int column, const QModelIndex &parent) const{
    if(!hasIndex(row,column,parent)){
        return QModelIndex();
    }
    MapNode* parentNode=nodeFromIndex(parent);
    MapNode* node=parentNode->child(row);
    if(node){
        return createIndex(row,column,node);
    }else{
        return QModelIndex();
    }
}
QModelIndex MapModel::parent(const QModelIndex &child) const{
    if(!child.isValid()){
        return QModelIndex();
    }
    MapNode* node=nodeFromIndex(child);
    MapNode* parentNode=node->parent();
    if(parentNode==_rootNode){
        return QModelIndex();
    }
    return createIndex(parentNode->row(),0,parentNode);
}
int MapModel::rowCount(const QModelIndex &parent) const{
    if(parent.column()>0){
        return 0;
    }
    MapNode* parentNode=nodeFromIndex(parent);
    return parentNode->childCount();
}
int MapModel::columnCount(const QModelIndex &parent) const{
    Q_UNUSED(parent);
    return 1;
}
bool MapModel::hasChildren(const QModelIndex &parent) const{
    return nodeFromIndex(parent)->hasChild();
}
MapNode* MapModel::nodeFromIndex(const QModelIndex &index)const{
    if(index.isValid()){
        MapNode* node=static_cast<MapNode*>(index.internalPointer());
        return node;
    }else{
        return _rootNode;
    }
}
/*…………………………………………………………………MapProxy…………………………………………………………………*/
MapProxy::MapProxy(QWidget* parent):QSortFilterProxyModel(parent){}
bool MapProxy::lessThan(const QModelIndex &source_left, const QModelIndex &source_right) const{
    if (!source_left.isValid() || !source_right.isValid())
        return false;
    QString left=sourceModel()->data(source_left).toString();
    QString right=sourceModel()->data(source_right).toString();
    if(left.contains("楼栋")&&right.contains("楼栋")){
        int leftInt=left.remove(0,2).toInt();
        int rightInt=right.remove(0,2).toInt();
        return leftInt<rightInt;
    }
    return left<right;
}
/*…………………………………………………………………MapView……………………………………………………………………*/
MapView::MapView(QWidget* parent):QTreeView(parent){
    _mapModel=new MapModel(this);
    _mapModel->resetModel();
    setContextMenuPolicy(Qt::CustomContextMenu);
    _mapProxy=new MapProxy(this);;
    _mapProxy->setSortRole(Qt::DisplayRole);
    _mapProxy->sort(0);
    _mapProxy->setSourceModel(_mapModel);
    setModel(_mapProxy);
    _menu=new QMenu(this);
    _watchMapAction=new QAction(this);
    _watchMapAction->setText("查看地图");
    _watchMapAction->setIcon(QIcon(":/Image/Watch.png"));
    _menu->addAction(_watchMapAction);
    connect(this,SIGNAL(customContextMenuRequested(QPoint)),this,SLOT(showMenuSlot(QPoint)));
    connect(_watchMapAction,SIGNAL(triggered()),this,SLOT(watchMapActionTriggeredSlot()));
}
void MapView::showMenuSlot(QPoint pos){
    //计算鼠标相对父窗口的位置
    QPoint pt=mapToGlobal(pos);
    //判断鼠标右击位置是否是空白处，空白处则取消上一个选中焦点，不弹出菜单
    _currentIndex = _mapProxy->mapToSource(indexAt(pos));
    if(_currentIndex.isValid()){
        MapNode* node=static_cast<MapNode*>(_currentIndex.internalPointer());
        if(node->type()==MapNode::GARBAGE||node->type()==MapNode::UNIT){
            QPoint point=QPoint(pt.x(),pt.y()+30);
            _menu->exec(point);
        }
    }
}
void MapView::watchMapActionTriggeredSlot(){
    MapNode* node=static_cast<MapNode*>(_currentIndex.internalPointer());
    QString mapPath=node->info().pgm.remove(0,QString(MAPS_DIR+"/").length()).remove(".pgm");
    contentProvider->watchMap(mapPath);
}
/*…………………………………………………………………MonitorLaser……………………………………………………………………*/
MonitorLaser::MonitorLaser(QGraphicsItem* parent):QGraphicsItem(parent){
    _mapItem=static_cast<MonitorMap*>(parent);
    setZValue(1);
}
void MonitorLaser::setrobotID(QString id){
    _robotID=id;
    _brush=QBrush(QColor(255,0,0,150));
}
QString MonitorLaser::robotID(){
    return _robotID;
}
void MonitorLaser::openLaser(){
    _isOpenLaser=true;
}
void MonitorLaser::closeLaser(){
    _isOpenLaser=false;
    update();
}
int MonitorLaser::type() const{
    return Type;
}
void MonitorLaser::setLaser(QJsonObject obj){
    _bytes=QByteArray::fromBase64(obj.value("scan").toString().toStdString().c_str());
    update();
}
void MonitorLaser::setPoint(QPointF pos){
    _robotPos=pos;
}
QRectF MonitorLaser::boundingRect() const{
    return _mapItem->boundingRect();
}
void MonitorLaser::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget){
    Q_UNUSED(option);
    Q_UNUSED(widget);
    painter->save();
    if(_isOpenLaser){
        painter->setPen(Qt::NoPen);
        painter->setBrush(_brush);
        QPainterPath path(_robotPos);
        int len=_bytes.length();
        for(int i=0;i<len;i+=4){
            QByteArray byte=_bytes.mid(i,4);
            unsigned short bx=0;
            bx|=(unsigned char)byte[0];
            bx<<=8;
            bx|=(unsigned char)byte[1];
            double x=(short)bx/100.0;
            unsigned short by=0;
            by|=(unsigned char)byte[2];
            by<<=8;
            by|=(unsigned char)byte[3];
            double y=(short)by/100.0;
            QPointF pos=_mapItem->realPosFromMapPos(QPointF(x,y));
            if(i==len-1){
                path.lineTo(pos);
                path.lineTo(_robotPos);
            }else{
                path.lineTo(pos);
            }
            painter->drawEllipse(pos,0.5,0.5);
        }
        painter->drawPath(path);
    }
    painter->restore();
}
/*…………………………………………………………………MonitorLine……………………………………………………………………*/
MonitorCross::MonitorCross(QGraphicsItem* parent):QGraphicsItem(parent){
    setZValue(4);
    _font.setPointSize(15);
    _fontColor=QColor("#eeeeee");
}
void MonitorCross::setrobotID(QString id){
    _robotID=id;
    _robotName=contentProvider->name(id);
    QColor color=contentProvider->color(id);
    color.setAlpha(25);
    _brush=QBrush(color);
}
QString MonitorCross::robotID(){
    return _robotID;
}
void MonitorCross::setChecked(bool isTrue){
    _isChecked=isTrue;
    update();
}
int MonitorCross::type() const{
    return Type;
}
QRectF MonitorCross::boundingRect() const{
    return QRectF(-100,-100,200,200);
}
void MonitorCross::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget){
    Q_UNUSED(option);
    Q_UNUSED(widget);
    painter->save(); 
    painter->setPen(Qt::NoPen);
    painter->setBrush(_brush);
    if(_isChecked){
        QPainterPath circle01;
        circle01.addEllipse(QPoint(0,0),100,100);
        QPainterPath circle02;
        circle02.addEllipse(QPoint(0,0),80,80);
        QPainterPath circle03;
        circle03.addEllipse(QPoint(0,0),60,60);
        QPainterPath circle04;
        circle04.addEllipse(QPoint(0,0),40,40);
        QPainterPath circle05;
        circle05.addEllipse(QPoint(0,0),20,20);
        QPainterPath path01=circle01-circle02+circle03-circle04+circle05;
        QPainterPath path02=circle02-circle03+circle04-circle05;
        painter->drawPath(path01);
        painter->setBrush(QColor(255,255,0,25));
        painter->drawPath(path02);
    }else{
        painter->drawEllipse(QPoint(0,0),100,100);
    }
    QColor color=contentProvider->color(_robotID);
    color.setAlpha(100);
    painter->setPen(QPen(color));
    painter->drawLine(QPointF(0,-100),QPointF(0,100));
    painter->drawLine(QPointF(-100,0),QPointF(100,0));
    painter->setFont(_font);
    painter->drawText(QRect(-100,-50,200,30),Qt::AlignCenter,_robotName);
    painter->restore();
}
/*…………………………………………………………………MonitorRobot………………………………………………………………………*/
MonitorRobot::MonitorRobot(QGraphicsItem* parent):QGraphicsItem(parent){
    setFlags(ItemIsSelectable|ItemIsFocusable);
    setCursor(Qt::PointingHandCursor);
    setZValue(5);
    _robotPixmap=QPixmap(":/Image/Robot.png");
    _checkedRobotPixmap=QPixmap(":/Image/RobotChecked.png");
}
void MonitorRobot::setrobotID(QString id){
    _robotID=id;
}
QString MonitorRobot::robotID(){
    return _robotID;
}
void MonitorRobot::setAngle(double angle){
    _angle=angle;
    update();
}
void MonitorRobot::setChecked(bool isTrue){
    _isChecked=isTrue;
    update();
}
int MonitorRobot::type() const{
    return Type;
}
QRectF MonitorRobot::boundingRect() const{
    return QRectF(-5,-5,10,10);
}
void MonitorRobot::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget){
    Q_UNUSED(option);
    Q_UNUSED(widget);
    painter->save();
    painter->rotate(180-_angle);
    if(_isChecked){
        painter->drawPixmap(boundingRect(),_checkedRobotPixmap,_checkedRobotPixmap.rect());
    }else{
        painter->drawPixmap(boundingRect(),_robotPixmap,_robotPixmap.rect());
    }
    painter->restore();
}
/*…………………………………………………………………MonitorInit………………………………………………………………………*/
MonitorInit::MonitorInit(QGraphicsItem* parent):QGraphicsItem(parent){
    setZValue(10);
    _mapItem=static_cast<MonitorMap*>(parent);
}
TOOL::XYA_POINT MonitorInit::xyaPoint(){
    QPointF realpos=_mapItem->mapPosFromRealPos(_spos);
    TOOL::XYA_POINT pos;
    pos.x=realpos.x();
    pos.y=realpos.y();
    pos.angle=-_yaw;
    return pos;
}
int MonitorInit::type() const{
    return Type;
}
void MonitorInit::mousePressEvent(QGraphicsSceneMouseEvent *event){
    if(event->button()==Qt::LeftButton){
        _spos=mapToParent(event->pos());
        update();
    }
}
void MonitorInit::mouseMoveEvent(QGraphicsSceneMouseEvent *event){
    _epos=mapToParent(event->pos());
    _yaw=_yawFromTwoPoint(_spos,_epos);
    _arrowPath=_pathFromTwoPoint(_spos,_epos);
    update();
}
QRectF MonitorInit::boundingRect() const{
    return _mapItem->boundingRect();
}
void MonitorInit::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget){
    Q_UNUSED(option);
    Q_UNUSED(widget);
    painter->save();
    painter->setPen(Qt::NoPen);
    painter->setBrush(QColor("#000000"));
    painter->drawEllipse(_spos,5,5);
    painter->translate(_spos);
    painter->rotate(_yaw);
    painter->drawPath(_arrowPath);
    painter->restore();
}
double MonitorInit::_yawFromTwoPoint(QPointF spos,QPointF epos){
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
QPainterPath MonitorInit::_pathFromTwoPoint(QPointF spos, QPointF epos){
    QPointF distance=epos-spos;
    double distances=sqrt(distance.x()*distance.x()+distance.y()*distance.y());
    QPainterPath path(QPointF(0,0));
    if(distances<10){
        path.lineTo(distances,0);
    }else{
        double y=10*sin(0.5);
        double x=distances-10*cos(0.5);
        path.lineTo(x,y/2.0);
        path.lineTo(x,y);
        path.lineTo(distances,0);
        path.lineTo(x,-y);
        path.lineTo(x,-y/2.0);
        path.lineTo(0,0);
    }
    return path;
}
/*…………………………………………………………………MonitorMap………………………………………………………………………*/
MonitorMap::MonitorMap(QGraphicsItem* parent):QGraphicsPixmapItem(parent){
    setZValue(2);
    setFlags(ItemIsSelectable|ItemIsFocusable|ItemIsMovable);
    setCursor(Qt::OpenHandCursor);
    connect(contentProvider,SIGNAL(robotOutSignal(QString)),this,SLOT(robotOutSlot(QString)));
    connect(contentProvider,SIGNAL(robotInSignal(QString)),this,SLOT(robotInSlot(QString)));
    connect(contentProvider,SIGNAL(robotsOutSignal(QStringList)),this,SLOT(robotsOutSlot(QStringList)));
    connect(contentProvider,SIGNAL(robotsInSignal(QStringList)),this,SLOT(robotsInSlot(QStringList)));
    connect(contentProvider,SIGNAL(followRobotSignal(QString)),this,SLOT(followRobotSlot(QString)));
    connect(contentProvider,SIGNAL(mqttReceivedScanDataSignal(QString,QJsonObject)),this,SLOT(mqttReceivedScanDataSlot(QString,QJsonObject)));
    connect(contentProvider,SIGNAL(mqttReceivedPositionSignal(QString,QJsonObject)),this,SLOT(mqttReceivedPositionSlot(QString,QJsonObject)));
}
MonitorMap::~MonitorMap(){}
void MonitorMap::setPixmapPath(QString path){
    QString mappath=MAPS_DIR+"/"+path+".pgm";
    if(QFile(mappath).exists()){
        QPixmap pixmap(mappath);
        _mapInfo.rect=pixmap.rect();
        setPixmap(pixmap);
        QString yamlpath=MAPS_DIR+"/"+path+".yaml";
        YAML::Node yaml = YAML::LoadFile(yamlpath.toStdString());
        _mapInfo.resolution = yaml["resolution"].as<double>();
        _mapInfo.originX = yaml["origin"][0].as<double>();
        _mapInfo.originY = yaml["origin"][1].as<double>();
    }
}
void MonitorMap::openLaser(QString robotID){
    _laserMap[robotID]->openLaser();
}
void MonitorMap::closeLaser(QString robotID){
    _laserMap[robotID]->closeLaser();
}
void MonitorMap::init(){
    if(_init==nullptr){
        setFlags(ItemIsSelectable|ItemIsFocusable);
        _init=new MonitorInit(this);
    }else{
        setFlags(ItemIsSelectable|ItemIsFocusable|ItemIsMovable);
        _init->deleteLater();
        _init=nullptr;
    }
}
void MonitorMap::robotOutSlot(QString robotID){
    _crossMap[robotID]->deleteLater();
    _crossMap.remove(robotID);
    _robotMap[robotID]->deleteLater();
    _robotMap.remove(robotID);
    _laserMap[robotID]->deleteLater();
    _laserMap.remove(robotID);
}
void MonitorMap::robotInSlot(QString robotID){
    MonitorCross* cross=new MonitorCross(this);
    cross->setrobotID(robotID);
    _crossMap.insert(robotID,cross);
    MonitorRobot* robot=new MonitorRobot(this);
    robot->setrobotID(robotID);
    _robotMap.insert(robotID,robot);
    MonitorLaser* laser=new MonitorLaser(this);
    _laserMap.insert(robotID,laser);
    laser->setrobotID(robotID);
}
void MonitorMap::robotsInSlot(QStringList robotsId){
    quint8 colorIndex=0;
    foreach (QString robotID, robotsId) {
        MonitorRobot* robot=new MonitorRobot(this);
        robot->setrobotID(robotID);
        _robotMap.insert(robotID,robot);
        MonitorCross* cross=new MonitorCross(this);
        cross->setrobotID(robotID);
        _crossMap.insert(robotID,cross);
        MonitorLaser* laser=new MonitorLaser(this);
        laser->setrobotID(robotID);
        _laserMap.insert(robotID,laser);
        if(contentProvider->isFollowRobot(robotID)){
            cross->setChecked(true);
            robot->setChecked(true);
        }else{
            cross->setChecked(false);
            robot->setChecked(false);
        }
        colorIndex++;
    }
}
void MonitorMap::robotsOutSlot(QStringList robotsId){
    foreach (QString robotID, robotsId) {
        _crossMap[robotID]->deleteLater();
        _robotMap[robotID]->deleteLater();
        _laserMap[robotID]->deleteLater();
    }
    _crossMap.clear();
    _robotMap.clear();
    _laserMap.clear();
}
void MonitorMap::followRobotSlot(QString robotID){
    QStringList robotsId=_robotMap.keys();
    foreach (QString _robotID, robotsId) {
        if(robotID==_robotID){
            _robotMap[_robotID]->setChecked(true);
            _crossMap[_robotID]->setChecked(true);
        }else{
            _robotMap[_robotID]->setChecked(false);
            _crossMap[_robotID]->setChecked(false);
        }
    }
}
void MonitorMap::mqttReceivedScanDataSlot(QString robotID,QJsonObject obj){
    if(_laserMap[robotID]!=NULL){
        _laserMap[robotID]->setLaser(obj);
    }
}
void MonitorMap::mqttReceivedPositionSlot(QString robotID,QJsonObject obj){
    if(_robotMap.contains(robotID)){
        if(contentProvider->isFollowRobot(robotID)){
            _robotMap[robotID]->setChecked(true);
            _crossMap[robotID]->setChecked(true);
        }else{
            _robotMap[robotID]->setChecked(false);
            _crossMap[robotID]->setChecked(false);
        }
        QPointF mappos(obj.value("x").toDouble(),obj.value("y").toDouble());
        QPointF realpos=realPosFromMapPos(mappos);
        _robotMap[robotID]->setPos(realpos);
        if(obj.value("yaw")==QJsonValue::Undefined){
            _robotMap[robotID]->setAngle(yawFromZW(obj.value("z").toDouble(),obj.value("w").toDouble()));
        }else{
            _robotMap[robotID]->setAngle(obj.value("yaw").toDouble());
        }
        _crossMap[robotID]->setPos(realpos);
        if(contentProvider->isInLaser(robotID)){
            _laserMap[robotID]->setPoint(realpos);
        }
    }
}
QPointF MonitorMap::realPosFromMapPos(QPointF pos){
    double x=(pos.x()-_mapInfo.originX)/_mapInfo.resolution;
    double y=_mapInfo.rect.height()-(pos.y()-_mapInfo.originY)/_mapInfo.resolution;
    return QPointF(x,y);
}
QPointF MonitorMap::mapPosFromRealPos(QPointF pos){
    double x=pos.x()*_mapInfo.resolution+_mapInfo.originX;
    double y=(_mapInfo.rect.height()-pos.y())*_mapInfo.resolution+_mapInfo.originY;
    return QPointF(x,y);
}
double MonitorMap::yawFromZW(double z,double w){
    double yaw = atan2(2*(w*z),1-2*(z*z));
    return yaw/M_PI*180;
}
void MonitorMap::mousePressEvent(QGraphicsSceneMouseEvent *event){
    QGraphicsPixmapItem::mousePressEvent(event);
    if(event->button()==Qt::LeftButton){
        setCursor(Qt::ClosedHandCursor);
    }
}
void MonitorMap::mouseReleaseEvent(QGraphicsSceneMouseEvent *event){
    QGraphicsPixmapItem::mouseReleaseEvent(event);
    setCursor(Qt::OpenHandCursor);
}
void MonitorMap::keyPressEvent(QKeyEvent *event){
    if(event->key()==Qt::Key_Control){
        init();
    }
}
/*…………………………………………………………………MonitorAction………………………………………………………………………*/
MonitorAction::MonitorAction(QWidget* parent):QAction(parent){
    connect(this,SIGNAL(triggered()),this,SLOT(triggeredSlot()));
}
void MonitorAction::setrobotID(QString robotID){
    _robotID=robotID;
}
void MonitorAction::triggeredSlot(){
    emit triggeredSignal(_robotID);
}
/*…………………………………………………………………MonitorScene………………………………………………………………………*/
MonitorScene::MonitorScene(QWidget* parent):QGraphicsScene(parent){
    setBackgroundBrush(QBrush("#393e46"));
    _mapMenu=new QMenu(parent);
    _initRobotAction_map=new QAction(_mapMenu);
    _initRobotAction_map->setText("选择点");
    _initRobotAction_map->setToolTip("选择一个点");
    _initRobotAction_map->setIcon(QIcon(":/Image/Init.png"));
    _mapMenu->addAction(_initRobotAction_map);
    _pointMenu=new QMenu(parent);
    _cancelAction_point=new QAction(_pointMenu);
    _cancelAction_point->setText("取消选择点");
    _cancelAction_point->setIcon(QIcon(":/Image/Init.png"));
    _pointMenu->addAction(_cancelAction_point);
    _robotMenu=new QMenu(parent);
    _followAction_robot=new QAction(_robotMenu);
    _followAction_robot->setText("跟随");
    _followAction_robot->setCheckable(true);
    _followAction_robot->setIcon(QIcon(":/Image/Follow.png"));
    _followAction_robot->setToolTip("用于跟随或取消跟随");
    _laserAction_robot=new QAction(_robotMenu);
    _laserAction_robot->setText("激光");
    _laserAction_robot->setToolTip("用于判断小车是否偏");
    _laserAction_robot->setCheckable(true);
    _laserAction_robot->setIcon(QIcon(":/Image/Laser.png"));
    _showDataAction_robot=new QAction(_robotMenu);
    _showDataAction_robot->setText("数据");
    _showDataAction_robot->setToolTip("用于展示或关闭数据");
    _showDataAction_robot->setCheckable(true);
    _showDataAction_robot->setIcon(QIcon(":/Image/Data.png"));
    _cameraAction_robot=new QAction(_robotMenu);
    _cameraAction_robot->setText("图像");
    _cameraAction_robot->setToolTip("用于展示或关闭图像");
    _cameraAction_robot->setCheckable(true);
    _cameraAction_robot->setIcon(QIcon(":/Image/Camera.png"));
    _historyRecorderAction_robot=new QAction(this);
    _historyRecorderAction_robot->setText("查看历史影像");
    _historyRecorderAction_robot->setToolTip("用于查看历史录像");
    _historyRecorderAction_robot->setCheckable(true);
    _historyRecorderAction_robot->setIcon(QIcon(":/Image/HistoryRecorder.png"));
    _sensitiveMenu_robot=new QMenu(_robotMenu);
    _sensitiveMenu_robot->setTitle("敏感操作");
    _sensitiveMenu_robot->setIcon(QIcon(":/Image/Sensitive.png"));
    _controlAction_robot=new QAction(_sensitiveMenu_robot);
    _controlAction_robot->setText("操控盘");
    _controlAction_robot->setToolTip("用于操控小车移动");
    _controlAction_robot->setCheckable(true);
    _controlAction_robot->setIcon(QIcon(":/Image/Control.png"));
    _stopTaskAction_robot=new QAction(_sensitiveMenu_robot);
    _stopTaskAction_robot->setText("中止任务");
    _stopTaskAction_robot->setToolTip("用于暂停任务");
    _stopTaskAction_robot->setIcon(QIcon(":/Image/Task.png"));
    _resumeTaskAction_robot=new QAction(_sensitiveMenu_robot);
    _resumeTaskAction_robot->setText("继续任务");
    _resumeTaskAction_robot->setToolTip("用于继续任务");
    _resumeTaskAction_robot->setIcon(QIcon(":/Image/Task.png"));
    _doorAction_robot=new QAction(_sensitiveMenu_robot);
    _doorAction_robot->setText("车门");
    _doorAction_robot->setToolTip("用于打开或关闭车门");
    _doorAction_robot->setCheckable(true);
    _doorAction_robot->setIcon(QIcon(":/Image/Door.png"));
    _setStopAction_robot=new QAction(_sensitiveMenu_robot);
    _setStopAction_robot->setText("解除急停");
    _setStopAction_robot->setToolTip("被人按下急停按钮，可以远程解除急停");
    _setStopAction_robot->setIcon(QIcon(":/Image/UnlockStop.png"));
    _moveResumeAction_robot=new QAction(_sensitiveMenu_robot);
    _moveResumeAction_robot->setText("跳点");
    _moveResumeAction_robot->setToolTip("放弃当前目标点并继续任务");
    _moveResumeAction_robot->setIcon(QIcon(":/Image/JumpPoint.png"));
    _sensitiveMenu_robot->addAction(_controlAction_robot);
    _sensitiveMenu_robot->addAction(_stopTaskAction_robot);
    _sensitiveMenu_robot->addAction(_resumeTaskAction_robot);
    _sensitiveMenu_robot->addAction(_doorAction_robot);
    _sensitiveMenu_robot->addAction(_setStopAction_robot);
    _sensitiveMenu_robot->addAction(_moveResumeAction_robot);
    _robotMenu->addAction(_followAction_robot);
    _robotMenu->addAction(_laserAction_robot);
    _robotMenu->addAction(_showDataAction_robot);
    _robotMenu->addAction(_cameraAction_robot);
    _robotMenu->addAction(_historyRecorderAction_robot);
    _robotMenu->addMenu(_sensitiveMenu_robot);
    _crossMenu=new QMenu(parent);
    _laserMenu=new QMenu(parent);
    connect(contentProvider,SIGNAL(mapChangedSignal(QString)),this,SLOT(mapChangedSlot(QString)));
    connect(_controlAction_robot,SIGNAL(triggered(bool)),this,SLOT(controlAction_robot_triggered(bool)));
    connect(_followAction_robot,SIGNAL(triggered(bool)),this,SLOT(followAction_robot_triggered(bool)));
    connect(_laserAction_robot,SIGNAL(triggered(bool)),this,SLOT(laserAction_robot_triggered(bool)));
    connect(_cameraAction_robot,SIGNAL(triggered(bool)),this,SLOT(cameraAction_robot_triggered(bool)));
    connect(_stopTaskAction_robot,SIGNAL(triggered()),this,SLOT(stopTaskAction_robot_triggered()));
    connect(_resumeTaskAction_robot,SIGNAL(triggered()),this,SLOT(resumeTaskAction_robot_triggered()));
    connect(_doorAction_robot,SIGNAL(triggered(bool)),this,SLOT(doorAction_robot_triggered(bool)));
    connect(_historyRecorderAction_robot,SIGNAL(triggered(bool)),this,SLOT(historyRecorderAction_robot_triggered(bool)));
    connect(_showDataAction_robot,SIGNAL(triggered(bool)),this,SLOT(showDataAction_robot_triggered(bool)));
    connect(_setStopAction_robot,SIGNAL(triggered()),this,SLOT(setStopAction_robot_triggered()));
    connect(_moveResumeAction_robot,SIGNAL(triggered()),this,SLOT(moveResumeAction_robot_triggered()));
    connect(_initRobotAction_map,SIGNAL(triggered()),this,SLOT(initRobotAction_map_triggered()));
    connect(_cancelAction_point,SIGNAL(triggered()),this,SLOT(cancelAction_point_triggered()));
}
MonitorScene::~MonitorScene(){
    _map->deleteLater();
}
void MonitorScene::movePos(QPointF pos){
    if(_map!=NULL){
        QPointF realPos=_map->mapFromScene(pos);
        QPointF mapPos=_map->mapPosFromRealPos(realPos);
        QString mapX=QString::number(mapPos.x(),'f',6);
        QString mapY=QString::number(mapPos.y(),'f',6);
        emit contentProvider->sendStatusBarStringSignal(QString("X坐标：%1 | Y坐标：%2").arg(mapX).arg(mapY));
    }
}
void MonitorScene::mapChangedSlot(QString mapPath){
    if(_map!=nullptr){
        removeItem(_map);
        _map->deleteLater();
        _map=nullptr;
    }
    _map=new MonitorMap();
    _map->setPixmapPath(mapPath);
    connect(contentProvider,&ContentProvider::appendLaserSignal,[this](QString robotID){_map->openLaser(robotID);});
    connect(contentProvider,&ContentProvider::removeLaserSignal,[this](QString robotID){_map->closeLaser(robotID);});
    addItem(_map);
}
void MonitorScene::contextMenuEvent(QGraphicsSceneContextMenuEvent *event){
    QTransform transform;
    QGraphicsItem* item=itemAt(event->scenePos(),transform);
    _cursorPos=event->scenePos();
    if(item==NULL){
        return;
    }
    switch (item->type()) {
    case MonitorCross::Type:
        _createPointAction();
        _mapMenu->exec(event->screenPos());
        break;
    case MonitorMap::Type:
        _mapMenu->exec(event->screenPos());
        break;
    case MonitorLaser::Type:
        _mapMenu->exec(event->screenPos());
        break;
    case MonitorRobot::Type:
        _ptr.robotPtr=static_cast<MonitorRobot*>(item);
        _checkIsChecked();
        _robotMenu->exec(event->screenPos());
        break;
    case MonitorInit::Type:
        _ptr.initPtr=static_cast<MonitorInit*>(item);
        _createPointAction();
        _pointMenu->exec(event->screenPos());
        break;
    default:
        break;
    }
}
void MonitorScene::initRobotAction_map_triggered(){
    if(_map!=nullptr){
        _map->init();
    }
}
void MonitorScene::controlAction_robot_triggered(bool isChecked){
    QString robotID=_ptr.robotPtr->robotID();
    if(isChecked){       
        contentProvider->openControl(robotID);
    }else{
        contentProvider->closeControl(robotID);    
    }
}
void MonitorScene::showDataAction_robot_triggered(bool isChecked){
    QString robotID=_ptr.robotPtr->robotID();
    if(isChecked){
        contentProvider->openData(robotID);
    }else{
        contentProvider->closeData(robotID);
    }
}
void MonitorScene::followAction_robot_triggered(bool isChecked){
    QString robotID=_ptr.robotPtr->robotID();
    if(isChecked){
        contentProvider->followRobot(robotID);
    }else{
        contentProvider->cancelFollowRobot();
    }
}
void MonitorScene::laserAction_robot_triggered(bool isChecked){
    QString robotID=_ptr.robotPtr->robotID();
    if(isChecked){
        contentProvider->openLaser(robotID);
    }else{
        contentProvider->closeLaser(robotID);
    }
}
void MonitorScene::stopTaskAction_robot_triggered(){
    QString robotID=_ptr.robotPtr->robotID();
    contentProvider->cancelTask(robotID);
}
void MonitorScene::resumeTaskAction_robot_triggered(){
    QString robotID=_ptr.robotPtr->robotID();
    contentProvider->resumeTask(robotID);
}
void MonitorScene::doorAction_robot_triggered(bool isChecked){
    QString robotID=_ptr.robotPtr->robotID();
    if(isChecked){
        contentProvider->openDoor(robotID);
    }else{
        contentProvider->closeDoor(robotID);
    }
}
void MonitorScene::cameraAction_robot_triggered(bool isChecked){
    QString robotID=_ptr.robotPtr->robotID();
    if(isChecked){
        contentProvider->openCamera(robotID);
    }else{
        contentProvider->closeCamera(robotID);
    }
}
void MonitorScene::historyRecorderAction_robot_triggered(bool isChecked){
    QString robotID=_ptr.robotPtr->robotID();
    if(isChecked){
        contentProvider->openRecorder(robotID);
    }else{
        contentProvider->closeRecorder(robotID);
    }
}
void MonitorScene::setStopAction_robot_triggered(){
    QString robotID=_ptr.robotPtr->robotID();
    contentProvider->setStop(robotID);
}
void MonitorScene::moveResumeAction_robot_triggered(){
    QString robotID=_ptr.robotPtr->robotID();
    MonitorView* view=static_cast<MonitorView*>(parent());
    QMessageBox::StandardButton yy=QMessageBox::warning(view,"警告","是否要跳过当前目标点？",QMessageBox::Ok|QMessageBox::Cancel,QMessageBox::Ok);
    if(yy==QMessageBox::Ok){
        contentProvider->moveResume(robotID);
    }
}
void MonitorScene::cancelAction_point_triggered(){
    if(_map!=nullptr){
        _map->init();
    }
}
void MonitorScene::pointAction_init_triggered(QString robotID){
    TOOL::XYA_POINT xyaPos=_ptr.initPtr->xyaPoint();
    contentProvider->initPoint(robotID,xyaPos);
    _map->init();
}
void MonitorScene::pointAction_goto_triggered(QString robotID){
    TOOL::XYA_POINT xyaPos=_ptr.initPtr->xyaPoint();
    contentProvider->moveSimpleGoal(robotID,xyaPos);
    _map->init();
}
void MonitorScene::_createPointAction(){
    foreach(MonitorAction* action,_initActionList){
        _pointMenu->removeAction(action);
        action->deleteLater();
    }
    _initActionList.clear();
    foreach(MonitorAction* action,_gotoActionList){
        _pointMenu->removeAction(action);
        action->deleteLater();
    }
    _gotoActionList.clear();
    QStringList robotsId=contentProvider->robotsInMap();
    foreach (QString robotID, robotsId) {
        MonitorAction* action=new MonitorAction(_pointMenu);
        action->setrobotID(robotID);
        action->setText("（初始化）"+contentProvider->name(robotID));
        action->setIcon(QIcon(":/Image/Robot.png"));
        _pointMenu->addAction(action);
        connect(action,SIGNAL(triggeredSignal(QString)),this,SLOT(pointAction_init_triggered(QString)));
        _initActionList.append(action);
    }
    _pointMenu->addSeparator();
    foreach (QString robotID, robotsId) {
        MonitorAction* action=new MonitorAction(_pointMenu);
        action->setrobotID(robotID);
        action->setText("（前往点）"+contentProvider->name(robotID));
        action->setIcon(QIcon(":/Image/Robot.png"));
        _pointMenu->addAction(action);
        connect(action,SIGNAL(triggeredSignal(QString)),this,SLOT(pointAction_goto_triggered(QString)));
        _gotoActionList.append(action);
    }
}
void MonitorScene::_checkIsChecked(){
    QString robotID=_ptr.robotPtr->robotID();
    _followAction_robot->setChecked(contentProvider->isFollowRobot(robotID));
    _laserAction_robot->setChecked(contentProvider->isInLaser(robotID));
    _controlAction_robot->setChecked(contentProvider->isInControl(robotID));
    _showDataAction_robot->setChecked(contentProvider->isInData(robotID));
    _cameraAction_robot->setChecked(contentProvider->isInCamera(robotID));
    _doorAction_robot->setChecked(contentProvider->isOpenDoor(robotID));
    _historyRecorderAction_robot->setChecked(contentProvider->isInRecorder(robotID));
}
/*…………………………………………………………………MonitorDataItem……………………………………………………………………*/
MonitorDataItem::MonitorDataItem(QWidget* parent):QFrame(parent){
    _titleLabel=new QLabel(this);
    _dataLabel=new QLabel(this);
    _titleLabel->setStyleSheet("color:#00FFFF;");
    _dataLabel->setStyleSheet("color:#F4EA2A;");
    _layout=new QHBoxLayout(this);
    _layout->setContentsMargins(0,0,0,0);
    _layout->addWidget(_titleLabel);
    _layout->addWidget(_dataLabel);
    _layout->setStretch(0,4);
    _layout->setStretch(0,6);
    setLayout(_layout);
}
void MonitorDataItem::setTitle(QString title){
    _titleLabel->setText(title);
}
void MonitorDataItem::setData(QString data){
    _dataLabel->setText(data);
}
/*…………………………………………………………………MonitorDataDialog………………………………………………………………………*/
MonitorDataDialog::MonitorDataDialog(QWidget* parent):QDialog(parent){
    setFixedSize(250,450);
    _chargeItem=new MonitorDataItem(this);
    _chargeItem->setTitle("充电：");
    _collisionItem=new MonitorDataItem(this);
    _collisionItem->setTitle("碰撞：");
    _curItem=new MonitorDataItem(this);
    _curItem->setTitle("电流：");
    _doorStateItem=new MonitorDataItem(this);
    _doorStateItem->setTitle("车门：");
    _emergencyItem=new MonitorDataItem(this);
    _emergencyItem->setTitle("急停：");
    _imuItem=new MonitorDataItem(this);
    _imuItem->setTitle("陀螺仪：");
    _mahItem=new MonitorDataItem(this);
    _mahItem->setTitle("电量：");
    _mapFileItem=new MonitorDataItem(this);
    _mapFileItem->setTitle("地图：");
    _mcuHwVersionItem=new MonitorDataItem(this);
    _mcuHwVersionItem->setTitle("下位机版本：");
    _mcuSoftVersionItem=new MonitorDataItem(this);
    _mcuSoftVersionItem->setTitle("下位机软件版本：");
    _milsItem=new MonitorDataItem(this);
    _milsItem->setTitle("里程：");
    _rosHwVersionItem=new MonitorDataItem(this);
    _rosHwVersionItem->setTitle("ROS下位机版本：");
    _rosSoftVersionItem=new MonitorDataItem(this);
    _rosSoftVersionItem->setTitle("ROS软件版本：");
    _skipItem=new MonitorDataItem(this);
    _skipItem->setTitle("环境匹配度：");
    _sonarItem=new MonitorDataItem(this);
    _sonarItem->setTitle("超声波：");
    _volItem=new MonitorDataItem(this);
    _volItem->setTitle("电压：");
    _wheelItem=new MonitorDataItem(this);
    _wheelItem->setTitle("车轮：");
    _pointItem=new MonitorDataItem(this);
    _pointItem->setTitle("位置：");
    _layout=new QVBoxLayout(this);
    _layout->setContentsMargins(5,5,5,5);
    _layout->setSpacing(0);
    _layout->addWidget(_chargeItem);
    _layout->addWidget(_collisionItem);
    _layout->addWidget(_curItem);
    _layout->addWidget(_doorStateItem);
    _layout->addWidget(_emergencyItem);
    _layout->addWidget(_imuItem);
    _layout->addWidget(_mahItem);
    _layout->addWidget(_mapFileItem);
    _layout->addWidget(_mcuHwVersionItem);
    _layout->addWidget(_mcuSoftVersionItem);
    _layout->addWidget(_milsItem);
    _layout->addWidget(_rosHwVersionItem);
    _layout->addWidget(_rosSoftVersionItem);
    _layout->addWidget(_skipItem);
    _layout->addWidget(_sonarItem);
    _layout->addWidget(_volItem);
    _layout->addWidget(_wheelItem);
    _layout->addWidget(_pointItem);
    setLayout(_layout);
}
MonitorDataDialog::~MonitorDataDialog(){}
void MonitorDataDialog::setrobotID(QString robotID){
    _robotID=robotID;
    setWindowTitle(contentProvider->name(robotID));
}
QString MonitorDataDialog::robotID(){
    return _robotID;
}
void MonitorDataDialog::setData(QJsonObject obj){
    _chargeItem->setData(bool(obj.value("charge").toInt())?"是":"否");
    _collisionItem->setData(obj.value("collision").toBool()?"是":"否");
    _curItem->setData(QString::number(obj.value("cur").toInt()/10.0,'f',3)+"安培");
    _doorStateItem->setData(obj.value("doorState").toBool()?"开":"闭");
    _emergencyItem->setData(obj.value("emergency").toBool()?"开":"闭");
    _imuItem->setData(obj.value("imu").toBool()?"异常":"正常");
    _mahItem->setData(QString("%1%").arg(obj.value("mah").toInt()));
    _mapFileItem->setData(obj.value("mapFile").toString().replace("building_","楼栋").replace("unit_","单元"));
    _mcuHwVersionItem->setData(obj.value("mcu_hw_version").toString());
    _mcuSoftVersionItem->setData(obj.value("mcu_soft_version").toString());
    _milsItem->setData(QString("%1米").arg(obj.value("mils").toInt()));
    _rosHwVersionItem->setData(obj.value("ros_hw_version").toString());
    _rosSoftVersionItem->setData(obj.value("ros_soft_version").toString());
    _skipItem->setData(QString::number(obj.value("skip").toInt()));
    _sonarItem->setData(obj.value("sonar").toBool()?"异常":"正常");
    _volItem->setData(QString::number(obj.value("vol").toInt()/100.0,'f',2)+"伏特");
    _wheelItem->setData(obj.value("wheel").toBool()?"异常":"正常");
    _pointItem->setData(QString("[%1][%2][%3]")
                        .arg(QString::number(obj.value("x").toDouble(),'f',3))
                        .arg(QString::number(obj.value("y").toDouble(),'f',3))
                        .arg(QString::number(obj.value("yaw").toDouble(),'f',3)));
}
void MonitorDataDialog::closeEvent(QCloseEvent *){
    contentProvider->closeData(_robotID);
}
/*…………………………………………………………………MonitorControlFrame………………………………………………………………………*/
MonitorControlFrame::MonitorControlFrame(QWidget* parent):QFrame(parent){
    setFixedSize(160,135);
    _upButton=new QPushButton(this);
    _upButton->setFixedSize(50,50);
    _upButton->setIconSize(QSize(30,30));
    _upButton->setIcon(QIcon(":/Image/Up.png"));
    _downButton=new QPushButton(this);
    _downButton->setFixedSize(50,50);
    _downButton->setIconSize(QSize(30,30));
    _downButton->setIcon(QIcon(":/Image/Down.png"));
    _leftButton=new QPushButton(this);
    _leftButton->setFixedSize(50,50);
    _leftButton->setIconSize(QSize(30,30));
    _leftButton->setIcon(QIcon(":/Image/Left.png"));
    _rightButton=new QPushButton(this);
    _rightButton->setFixedSize(50,50);
    _rightButton->setIconSize(QSize(30,30));
    _rightButton->setIcon(QIcon(":/Image/Right.png"));
    _idLabel=new QLabel(this);
    _idLabel->setAlignment(Qt::AlignCenter);
    _idLabel->setStyleSheet("background-color:#424c50;border:1px solid #f2be45;color:#eacd76;");
    _layout=new QGridLayout(this);
    _layout->setContentsMargins(0,0,0,0);
    _layout->setSpacing(5);
    _layout->addWidget(_upButton,0,1,1,1);
    _layout->addWidget(_leftButton,1,0,1,1);
    _layout->addWidget(_downButton,1,1,1,1);
    _layout->addWidget(_rightButton,1,2,1,1);
    _layout->addWidget(_idLabel,2,0,1,3);
    setLayout(_layout);
    connect(_upButton,&QPushButton::clicked,[this](){contentProvider->moveRobot(_robotID,0.3,0.0);});
    connect(_downButton,&QPushButton::clicked,[this](){contentProvider->moveRobot(_robotID,-0.3,0.0);});
    connect(_leftButton,&QPushButton::clicked,[this](){contentProvider->moveRobot(_robotID,0.0,0.3);});
    connect(_rightButton,&QPushButton::clicked,[this](){contentProvider->moveRobot(_robotID,0.0,-0.3);});
}
void MonitorControlFrame::setrobotID(QString robotID){
    _robotID=robotID;
    _idLabel->setText(contentProvider->name(robotID));
}
QString MonitorControlFrame::robotID(){
    return _robotID;
}
/*…………………………………………………………………MonitorControlFrame………………………………………………………………………*/
MonitorCameraDialog::MonitorCameraDialog(QWidget* parent):QDialog(parent){
    setFixedSize(640,480);
}
void MonitorCameraDialog::setrobotID(QString robotID){
    setWindowTitle(contentProvider->name(robotID));
    _robotID=robotID;
}
QString MonitorCameraDialog::robotID(){
    return _robotID;
}
void MonitorCameraDialog::setData(QJsonObject obj){
    if(obj.value("data")!=QJsonValue::Undefined){
        QByteArray data=QByteArray::fromBase64(obj.value("data").toString().toStdString().c_str());
        _pixmap.loadFromData(data);
        update();
    }
}
void MonitorCameraDialog::paintEvent(QPaintEvent *){
    QPainter painter(this);
    painter.save();
    painter.drawPixmap(rect(),_pixmap,_pixmap.rect());
    painter.restore();
}
void MonitorCameraDialog::closeEvent(QCloseEvent *){
    contentProvider->closeCamera(_robotID);
}
/*…………………………………………………………………MovieFrame……………………………………………………………………*/
MovieFrame::MovieFrame(QWidget* parent):QFrame(parent){
    setFixedSize(640,480);
}
void MovieFrame::setData(QJsonObject obj){
    if(obj.value("data")!=QJsonValue::Undefined){
        QByteArray data=QByteArray::fromBase64(obj.value("data").toString().toStdString().c_str());
        _pixmap.loadFromData(data);
        update();
    }
}
void MovieFrame::paintEvent(QPaintEvent *){
    QPainter painter(this);
    painter.save();
    painter.drawPixmap(rect(),_pixmap,_pixmap.rect());
    painter.restore();
}
/*…………………………………………………………………ProcessFrame……………………………………………………………………*/
ProcessFrame::ProcessFrame(QWidget* parent):QFrame(parent){
    setFixedHeight(30);
    setMaxnum(500);
    setCurrentNum(250);
}
void ProcessFrame::setMaxnum(int max){
    _max=max;
    int seconds=_max/3;
    int hour=seconds/3600;
    int minute=seconds%3600/60;
    int second=seconds%3600%60;
    _allTime=QString("%1:%2:%3").arg(hour,2,10,QLatin1Char('0')).arg(minute,2,10,QLatin1Char('0')).arg(second,2,10,QLatin1Char('0'));
}
void ProcessFrame::setCurrentNum(int current){
    _currentNum=current;
    _pointRectF=QRectF(_currentNum/(float)_max*(width()-15),0,15,15);
    int seconds=current/3;
    int hour=seconds/3600;
    int minute=seconds%3600/60;
    int second=seconds%3600%60;
    _nowTime=QString("%1:%2:%3").arg(hour,2,10,QLatin1Char('0')).arg(minute,2,10,QLatin1Char('0')).arg(second,2,10,QLatin1Char('0'));
    update();
}
void ProcessFrame::paintEvent(QPaintEvent *){
    QPainter painter(this);
    painter.save();
    painter.setRenderHint(QPainter::Antialiasing);
    QPen pen;
    pen.setColor(QColor("#62d2a2"));
    pen.setWidth(5);
    painter.setPen(pen);
    painter.drawLine(QPointF(7.5,7.5),QPointF(7.5+_currentNum/(float)_max*(width()-15),7.5));
    pen.setColor(QColor("#d3d4d8"));
    painter.setPen(pen);
    painter.drawLine(QPointF(7.5+_currentNum/(float)_max*(width()-15),7.5),QPointF(width()-7.5,7.5));
    painter.setPen(Qt::NoPen);
    if(_isPress){
        painter.setBrush(QColor("#fc5c9c"));
    }else{
        painter.setBrush(QColor("#62d2a2"));
    }
    painter.drawEllipse(_pointRectF);
    painter.setPen(QColor("#FFFFFF"));
    painter.drawText(QRect(0,15,width(),height()-15),Qt::AlignLeft|Qt::AlignVCenter,QString("%1/%2").arg(_nowTime).arg(_allTime));
    painter.restore();
}
void ProcessFrame::mousePressEvent(QMouseEvent* event){
    QPoint pos=event->pos();
    if(pos.x()>_pointRectF.x()&&pos.x()<_pointRectF.x()+_pointRectF.width()&&pos.y()>0&&pos.y()<_pointRectF.height()){
        _isPress=true;
        update();
    }else if(pos.x()>7.5&&pos.x()<width()-7.5){
        int current=(pos.x()-7.5)/(float)(width()-15)*_max;
        emit pictureIDChangedSignal(current);
        setCurrentNum(current);
    }
}
void ProcessFrame::mouseMoveEvent(QMouseEvent* event){
    QPoint pos=event->pos();
    if(_isPress){
        if(pos.x()>7.5&&pos.x()<width()-7.5){
            _pointRectF=QRectF(pos.x()-7.5,0,15,15);
            update();
        }
    }
}
void ProcessFrame::mouseReleaseEvent(QMouseEvent* event){
    QPoint pos=event->pos();
    if(_isPress){
        _isPress=false;
        if(pos.x()>7.5&&pos.x()<width()-7.5){
            int current=(pos.x()-7.5)/(float)(width()-15)*_max;
            emit pictureIDChangedSignal(current);
            setCurrentNum(current);
        }
    }
}
/*…………………………………………………………………MonitorRecorderDialog……………………………………………………………………*/
MonitorRecorderDialog::MonitorRecorderDialog(QWidget* parent):QDialog(parent){
    setFixedSize(660,580);
    _justOrderButton=new QPushButton(this);
    _justOrderButton->setText("刚刚订单");
    _justOrderButton->setIcon(QIcon(":/Image/Order.png"));
    _justOrderButton->setCheckable(true);
    _justOrderButton->setChecked(true);
    _preOrderButton=new QPushButton(this);
    _preOrderButton->setText("上一订单");
    _preOrderButton->setIcon(QIcon(":/Image/Order.png"));
    _preOrderButton->setCheckable(true);
    _prePreOrderButton=new QPushButton(this);
    _prePreOrderButton->setText("上上订单");
    _prePreOrderButton->setIcon(QIcon(":/Image/Order.png"));
    _prePreOrderButton->setCheckable(true);
    _orderButtonGroup=new QButtonGroup(this);
    _orderButtonGroup->addButton(_justOrderButton);
    _orderButtonGroup->addButton(_preOrderButton);
    _orderButtonGroup->addButton(_prePreOrderButton);
    _x03Button=new QPushButton(this);
    _x03Button->setText("x0.3");
    _x03Button->setCheckable(true);
    _x05Button=new QPushButton(this);
    _x05Button->setText("x0.5");
    _x05Button->setCheckable(true);
    _x1Button=new QPushButton(this);
    _x1Button->setText("x1");
    _x1Button->setCheckable(true);
    _x1Button->setChecked(true);
    _x2Button=new QPushButton(this);
    _x2Button->setText("x2");
    _x2Button->setCheckable(true);
    _x25Button=new QPushButton(this);
    _x25Button->setText("x2.5");
    _x25Button->setCheckable(true);
    _x3Button=new QPushButton(this);
    _x3Button->setText("x3");
    _x3Button->setCheckable(true);
    _speedButtonGroup=new QButtonGroup(this);
    _speedButtonGroup->addButton(_x03Button);
    _speedButtonGroup->addButton(_x05Button);
    _speedButtonGroup->addButton(_x1Button);
    _speedButtonGroup->addButton(_x2Button);
    _speedButtonGroup->addButton(_x25Button);
    _speedButtonGroup->addButton(_x3Button);
    _movieFrame=new MovieFrame(this);
    _processFrame=new ProcessFrame(this);
    _stopButton=new QPushButton(this);
    _stopButton->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
    _stopButton->setIconSize(QSize(30,30));
    _stopButton->setIcon(QIcon(":/Image/Play.png"));
    _layout=new QGridLayout(this);
    _layout->addWidget(_justOrderButton,0,0,1,4);
    _layout->addWidget(_preOrderButton,0,4,1,4);
    _layout->addWidget(_prePreOrderButton,0,8,1,4);
    _layout->addWidget(_movieFrame,1,0,1,12);
    _layout->addWidget(_processFrame,2,0,1,12);
    _layout->addWidget(_stopButton,3,0,2,3);
    _layout->addWidget(_x03Button,3,3,1,3);
    _layout->addWidget(_x05Button,3,6,1,3);
    _layout->addWidget(_x1Button,3,9,1,3);
    _layout->addWidget(_x2Button,4,3,1,3);
    _layout->addWidget(_x25Button,4,6,1,3);
    _layout->addWidget(_x3Button,4,9,1,3);
    _layout->setRowStretch(0,1);
    _layout->setRowStretch(1,6);
    _layout->setRowStretch(2,1);
    _layout->setRowStretch(3,1);
    _layout->setRowStretch(4,1);
    setLayout(_layout);
    connect(_justOrderButton,&QPushButton::clicked,[this](){_orderID=1;_pictureID=0;contentProvider->getRecorder(_robotID,_orderID,_pictureID);});
    connect(_preOrderButton,&QPushButton::clicked,[this](){_orderID=2;_pictureID=0;contentProvider->getRecorder(_robotID,_orderID,_pictureID);});
    connect(_prePreOrderButton,&QPushButton::clicked,[this](){_orderID=3;_pictureID=0;contentProvider->getRecorder(_robotID,_orderID,_pictureID);});
    connect(_x03Button,&QPushButton::clicked,[this](){_timerNode=100;});
    connect(_x05Button,&QPushButton::clicked,[this](){_timerNode=67;});
    connect(_x1Button,&QPushButton::clicked,[this](){_timerNode=33;});
    connect(_x2Button,&QPushButton::clicked,[this](){_timerNode=17;});
    connect(_x25Button,&QPushButton::clicked,[this](){_timerNode=13;});
    connect(_x3Button,&QPushButton::clicked,[this](){_timerNode=11;});
    connect(_stopButton,SIGNAL(clicked()),this,SLOT(stopButtonClickedSlot()));
    connect(_processFrame,SIGNAL(pictureIDChangedSignal(int)),this,SLOT(pictureIDChangedSlot(int)));
    _timerID=startTimer(10);
}
MonitorRecorderDialog::~MonitorRecorderDialog(){
    killTimer(_timerID);
}
void MonitorRecorderDialog::setRobotID(QString robotID){
    _robotID=robotID;
    setWindowTitle(contentProvider->name(robotID));
    contentProvider->getRecorder(robotID,_orderID);
}
QString MonitorRecorderDialog::robotID(){
    return _robotID;
}
void MonitorRecorderDialog::setData(QJsonObject obj){
    _pictureNumMax=obj.value("number").toInt();
    _processFrame->setMaxnum(_pictureNumMax);
    _processFrame->setCurrentNum(_pictureID);
    _movieFrame->setData(obj);
}
void MonitorRecorderDialog::timerEvent(QTimerEvent *event){
    Q_UNUSED(event);
    if(!_isStop){
        if(_timerCount%_timerNode==0){
            _timerCount=0;
            _pictureID++;
            if(_pictureID<_pictureNumMax){
                contentProvider->getRecorder(_robotID,_orderID,_pictureID);
            }else{
                _pictureID=0;
                stopButtonClickedSlot();
            }
        }
        _timerCount++;
    }
}
void MonitorRecorderDialog::closeEvent(QCloseEvent *){
    contentProvider->closeRecorder(_robotID);
}
void MonitorRecorderDialog::stopButtonClickedSlot(){
    if(_isStop){
        _isStop=false;
        _stopButton->setIcon(QIcon(":/Image/Stop.png"));
    }else{
        _isStop=true;
        _stopButton->setIcon(QIcon(":/Image/Play.png"));
    }
}
void MonitorRecorderDialog::pictureIDChangedSlot(int pictureID){
    _pictureID=pictureID;
    contentProvider->getRecorder(_robotID,_orderID,_pictureID);
}
/*…………………………………………………………………MonitorView………………………………………………………………………*/
MonitorView::MonitorView(QWidget* parent):QGraphicsView(parent){
    setRenderHints(QPainter::Antialiasing|QPainter::SmoothPixmapTransform);
    setAlignment(Qt::AlignCenter);
    setViewportUpdateMode(QGraphicsView::BoundingRectViewportUpdate);
    setMouseTracking(true);
    _scene=new MonitorScene(this);
    setScene(_scene);
    fitInView(_scene->sceneRect(),Qt::KeepAspectRatio);
    _controlLayout=new QHBoxLayout();
    _controlLayout->setContentsMargins(0,0,0,0);
    _controlLayout->setSpacing(0);
    _spacer=new QSpacerItem(10,10,QSizePolicy::Expanding,QSizePolicy::Expanding);
    _layout=new QVBoxLayout(this);
    _layout->setContentsMargins(0,0,0,10);
    _layout->setSpacing(0);
    _layout->addSpacerItem(_spacer);
    _layout->addLayout(_controlLayout);
    setLayout(_layout);
    connect(contentProvider,SIGNAL(appendControlItemSignal(QString)),this,SLOT(appendControlItemSlot(QString)));
    connect(contentProvider,SIGNAL(removeControlItemSignal(QString)),this,SLOT(removeControlItemSlot(QString)));
    connect(contentProvider,SIGNAL(appendDataItemSignal(QString)),this,SLOT(appendDataItemSlot(QString)));
    connect(contentProvider,SIGNAL(removeDataItemSignal(QString)),this,SLOT(removeDataItemSlot(QString)));
    connect(contentProvider,SIGNAL(appendCameraItemSignal(QString)),this,SLOT(appendCameraItemSlot(QString)));
    connect(contentProvider,SIGNAL(removeCameraItemSignal(QString)),this,SLOT(removeCameraItemSlot(QString)));
    connect(contentProvider,SIGNAL(appendRecorderItemSignal(QString)),this,SLOT(appendRecorderItemSlot(QString)));
    connect(contentProvider,SIGNAL(removeRecorderItemSignal(QString)),this,SLOT(removeRecorderItemSlot(QString)));
    connect(contentProvider,SIGNAL(mqttReceivedCameraPictureSignal(QString,QJsonObject)),this,SLOT(mqttReceivedCameraPictureSlot(QString,QJsonObject)));
    connect(contentProvider,SIGNAL(mqttReceivedRobotStateSignal(QString,QJsonObject)),this,SLOT(mqttReceivedRobotStateSlot(QString,QJsonObject)));
    connect(contentProvider,SIGNAL(mqttReceivedCameraRecorderSiganl(QString,QJsonObject)),this,SLOT(mqttReceivedCameraRecorderSlot(QString,QJsonObject)));
    _timerID=startTimer(50);
}
MonitorView::~MonitorView(){
    _dataLayout->deleteLater();
    _controlLayout->deleteLater();
}
void MonitorView::appendControlItemSlot(QString robotID){
    MonitorControlFrame* _frame=new MonitorControlFrame(this);
    _frame->setrobotID(robotID);
    _controlLayout->addWidget(_frame);
    _controlFrameMap.insert(robotID,_frame);
}
void MonitorView::removeControlItemSlot(QString robotID){
    _controlLayout->removeWidget(_controlFrameMap[robotID]);
    _controlFrameMap[robotID]->deleteLater();
    _controlFrameMap.remove(robotID);
}
void MonitorView::appendDataItemSlot(QString robotID){
    MonitorDataDialog* dialog=new MonitorDataDialog(this);
    dialog->setrobotID(robotID);
    _dataDialogMap.insert(robotID,dialog);
    dialog->show();
}
void MonitorView::removeDataItemSlot(QString robotID){
    _dataDialogMap[robotID]->deleteLater();
    _dataDialogMap.remove(robotID);
}
void MonitorView::appendCameraItemSlot(QString robotID){
    MonitorCameraDialog* dialog=new MonitorCameraDialog(this);
    dialog->setrobotID(robotID);
    _cameraDialogMap.insert(robotID,dialog);
    dialog->show();
}
void MonitorView::removeCameraItemSlot(QString robotID){
    _cameraDialogMap[robotID]->deleteLater();
    _cameraDialogMap.remove(robotID);
}
void MonitorView::appendRecorderItemSlot(QString robotID){
    MonitorRecorderDialog* dialog=new MonitorRecorderDialog(this);
    dialog->setRobotID(robotID);
    _recorderDialogMap.insert(robotID,dialog);
    dialog->show();
}
void MonitorView::removeRecorderItemSlot(QString robotID){
    _recorderDialogMap[robotID]->deleteLater();
    _recorderDialogMap.remove(robotID);
}
void MonitorView::mqttReceivedCameraPictureSlot(QString robotID,QJsonObject obj){
    if(_cameraDialogMap.contains(robotID)){
        _cameraDialogMap[robotID]->setData(obj);
    }
}
void MonitorView::mqttReceivedRobotStateSlot(QString robotID,QJsonObject obj){
    if(_dataDialogMap.contains(robotID)){
        _dataDialogMap[robotID]->setData(obj);
    }
}
void MonitorView::mqttReceivedCameraRecorderSlot(QString robotID,QJsonObject obj){
    if(_recorderDialogMap.contains(robotID)&&obj.value("data")!=QJsonValue::Undefined){
        _recorderDialogMap[robotID]->setData(obj);
    }
}
void MonitorView::wheelEvent(QWheelEvent *event){
    if(event->delta()>0){
        scale(1.1,1.1);
    }else{
        scale(1/1.1,1/1.1);
    }
}
void MonitorView::mouseMoveEvent(QMouseEvent *event){
    QGraphicsView::mouseMoveEvent(event);
    _currentPosFast=event->pos();
    QPointF p0=mapToScene(event->pos());
    _scene->movePos(p0);
}
void MonitorView::timerEvent(QTimerEvent *){
    if(_currentPosFast!=_currentPosSlow){
        _currentPosSlow=_currentPosFast;
        contentProvider->beginTimer();
        _timerCount=0;
    }
    _timerCount++;
    //如果五分钟都没有移动6000
    if(_timerCount==6000){
        contentProvider->endTimer();
        _timerCount=0;
    }
}
/*…………………………………………………………………Telepresencer………………………………………………………………………*/
Telepresencer::Telepresencer(QWidget* parent):QFrame(parent){
    _robotFrame=new RobotFrame(this);
    _mapView=new MapView(this);
    _robotFrame->setMaximumWidth(300);
    _splitter01=new QSplitter(this);
    _splitter01->setOrientation(Qt::Vertical);
    _splitter01->addWidget(_mapView);
    _splitter01->addWidget(_robotFrame);
    _monitorView=new MonitorView(this);
    _splitter=new QSplitter(this);
    _splitter->setOrientation(Qt::Horizontal);
    _splitter->addWidget(_splitter01);
    _splitter->addWidget(_monitorView);
    _statusBar=new QStatusBar(this);
    _statusBar->setFixedHeight(25);
    _layout=new QVBoxLayout(this);
    _layout->setContentsMargins(0,0,0,0);
    _layout->setSpacing(0);
    _layout->addWidget(_splitter);
    _layout->addWidget(_statusBar);
    _layout->setStretch(0,99);
    _layout->setStretch(1,1);
    setLayout(_layout);
    connect(contentProvider,SIGNAL(sendStatusBarStringSignal(QString)),this,SLOT(receiveStatusBarStringSignal(QString)));
}
Telepresencer::~Telepresencer(){}
void Telepresencer::receiveStatusBarStringSignal(QString str){
    _statusBar->showMessage(str);
}
