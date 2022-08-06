#ifndef ROUTEPLANNER_H
#define ROUTEPLANNER_H

#include "tool.h"
class BasementLine;
class BasementMap;
/*@@@@@@@@@@@@@@@@@@@@@@DataProvider@@@@@@@@@@@@@@@@@@@@@@@@@*/
class DataProvider{
public:
    DataProvider();
    void setAreaDir(QString path);
    inline void setPixmapRect(QRect rect){_allInfo.pixmapRect=rect;}
    inline QRect getPixmapRect(){return _allInfo.pixmapRect;}
    inline QStringList getLinesName(){return _allInfo.linesObj.keys();}
    inline QStringList getBuildsName(){return _allInfo.buildsObj.keys();}
    inline QString getMapPath(){return _allInfo.mapPath;}
    QJsonArray getLinePoints(QString lineName);
    QJsonArray getBuildLines(QString buildName);
    void addLine(QString lineName,QJsonArray linePoints);//增加线
    void removeLine(QString lineName);//删除线
    void modifyLine(QString lineName,QJsonArray linePoints);//修改线
    void addBuild(QString build,QJsonArray buildLines);//增加楼栋
    void removeBuild(QString build);//删除楼栋
    void modifyBuild(QString build,QJsonArray buildLines);//修改楼栋包含的线
    void saveLines();
    void saveBuilds();
    void saveRightDrivingPath();
    QPointF realPosFromMapPos(QPointF pos);
    QPointF mapPosFromRealPos(QPointF pos);

    inline void appendBaseLine(QString linename,BasementLine* lineInfo){_lineInfoMap.insert(linename,lineInfo);};
    inline void removeBaseLine(QString linename){_lineInfoMap.remove(linename);}
    inline void modifyBaseLine(QString linename,BasementLine* lineInfo){_lineInfoMap[linename]=lineInfo;}
    inline BasementLine* getBaseLine(QString linename){return _lineInfoMap[linename];}
private:
    TOOL::XYZW_POINT _getXYZWPOINTFromXYAPOINT(TOOL::XYA_POINT point);
    double _getAngleFromTwoPoint(QPointF spos,QPointF epos);
private:
    TOOL::PATH_INFO _allInfo;
    QMap<QString,BasementLine*> _lineInfoMap;
};
extern DataProvider* dataProvider;
/*@@@@@@@@@@@@@@@@@@@@@@BasementPoint@@@@@@@@@@@@@@@@@@@@@@@@@*/
class BasementPoint:public QObject,public QGraphicsItem{
    Q_OBJECT
public:
    struct INFO{
        QPointF mapPos;
        int index;
        QString lineName;
    };
    enum{Type=UserType+1};
    explicit BasementPoint(QGraphicsItem* parent=nullptr);
    ~BasementPoint();
    int type() const override{return Type;}
    void setInfo(QString lineName,int index,QPointF mapPos);
    void addPoint();
    void delPoint();
    void delLine();
protected:
    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = Q_NULLPTR) override;
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
private:
    void _setToolTip();
private:
    INFO _info;
    BasementLine* _parentItem;
};
/*@@@@@@@@@@@@@@@@@@@@@@BasementLine@@@@@@@@@@@@@@@@@@@@@@@@@*/
class BasementLine:public QObject,public QGraphicsItem{
    Q_OBJECT
public:
    struct INFO{
        QJsonArray points;
        QString lineName;
        QString buildName;
    };
    enum{Type=UserType+2};
    explicit BasementLine(QGraphicsItem* parent=nullptr);
    ~BasementLine();  
    int type() const override{return Type;}
    void setLineName(QString lineName);
    void addPoint(int index);
    void modPoint(int index,QPointF pos);
    void delPoint(int index);
    void delLine();
    inline QString getLineName(){return _info.lineName;}
    void setChecked();
protected:
    QRectF boundingRect() const override;
    QPainterPath shape() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = Q_NULLPTR) override;
    void timerEvent(QTimerEvent* event)override;
private:
    void _rect();
    void _reLine();
private:
    INFO _info;
    QPainterPath _painterPath;
    BasementMap* _parentItem;
    QRectF _rectF;
    QList<QPointF> _textPointList;
    QList<BasementPoint*> _pointList;
    bool _isChecked=false;
    int _timerId;
    quint8 _timerCount=0;
};
/*@@@@@@@@@@@@@@@@@@@@@@BasementMap@@@@@@@@@@@@@@@@@@@@@@@@@*/
class BasementMap:public QObject,public QGraphicsPixmapItem{
    Q_OBJECT
public:
    explicit BasementMap(QGraphicsItem* parent=nullptr);
    void setMap(QString path);//设置地图
    void addLine(QPointF pos);
protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
private:
    QPixmap _pixmap;
};
/*@@@@@@@@@@@@@@@@@@@@@@BasementScene@@@@@@@@@@@@@@@@@@@@@@@@@*/
class BasementScene:public QGraphicsScene{
    Q_OBJECT
public:
    struct ITEM_PTR{
        BasementMap* mapPtr;
        BasementLine* linePtr;
        BasementPoint* pointPtr;
    };
    explicit BasementScene(QWidget* parent=nullptr);
    void movePos(QPointF pos);
protected:
    void contextMenuEvent(QGraphicsSceneContextMenuEvent *event)override;
public slots:
    void addPointActionPointTriggeredSlot(bool);
    void delPointActionPointTriggeredSlot(bool);
    void delLineActionPointTriggeredSlot(bool);
    void addLineActionMapTriggeredSlot(bool);
    void resetMapSlot();
private:
    BasementMap* _mapItem;
    /************_mapMenu*************/
    QMenu* _mapMenu;
    QAction* _addLineAction_map;
    /************_lineMenu*************/
    QMenu* _lineMenu;
    /************_pointMenu*************/
    QMenu* _pointMenu;
    QAction* _addPointAction_point;
    QAction* _delPointAction_point;
    QAction* _delLineAction_point;
    ITEM_PTR _ptr;
    QPointF _cursorPos;
};
/*@@@@@@@@@@@@@@@@@@@@@@BasementView@@@@@@@@@@@@@@@@@@@@@@@@@*/
class BasementView:public QGraphicsView{
    Q_OBJECT
public:
    explicit BasementView(QWidget* parent=nullptr);
protected:
    void wheelEvent(QWheelEvent *event)override;
    void mouseMoveEvent(QMouseEvent* event) override;
private:
    float _scale=1.0;
    BasementScene* _scene;
};
/*@@@@@@@@@@@@@@@@@@@@@@LineModel@@@@@@@@@@@@@@@@@@@@@@@@@*/
class LineModel:public QAbstractListModel{
    Q_OBJECT
public:
    explicit LineModel(QWidget* parent=nullptr);
protected:
    int rowCount(const QModelIndex & parent = QModelIndex()) const override;
    QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const override;
public slots:
    void resetLineModelSlot();
};
/*@@@@@@@@@@@@@@@@@@@@@@LineDelegate@@@@@@@@@@@@@@@@@@@@@@@@@*/
class LineDelegate:public QStyledItemDelegate{
    Q_OBJECT
public:
    explicit LineDelegate(QWidget* parent=nullptr);
protected:
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
};
/*@@@@@@@@@@@@@@@@@@@@@@LineProxy@@@@@@@@@@@@@@@@@@@@@@@@@*/
class LineProxy:public QSortFilterProxyModel{
    Q_OBJECT
public:
    explicit LineProxy(QWidget* parent);
protected:
    bool lessThan(const QModelIndex &source_left, const QModelIndex &source_right) const override;
};
/*@@@@@@@@@@@@@@@@@@@@@@LineView@@@@@@@@@@@@@@@@@@@@@@@@@*/
class LineView:public QListView{
    Q_OBJECT
public:
    explicit LineView(QWidget* parent=nullptr);
public slots:
    void clickedSlot(QModelIndex);
    void showMenuSlot(QPoint);
    void delLineActionTriggeredSlot();
private:
    LineModel* _lineModel;
    LineDelegate* _lineDelegate;
    LineProxy* _lineProxy;
    QMenu* _menu;
    QAction* _delLineAction;
    QModelIndex _currentIndex;
};
/*@@@@@@@@@@@@@@@@@@@@@@LineFrame@@@@@@@@@@@@@@@@@@@@@@@@*/
class LineFrame:public QFrame{
    Q_OBJECT
public:
    explicit LineFrame(QWidget* parent=nullptr);
private:
    LineView* _lineView;
    QPushButton* _saveLineButton;
    QVBoxLayout* _layout;
};
/*@@@@@@@@@@@@@@@@@@@@@@BuildModel@@@@@@@@@@@@@@@@@@@@@@@@@*/
class BuildModel:public QAbstractTableModel{
    Q_OBJECT
public:
    explicit BuildModel(QWidget* parent=nullptr);
protected:
    QVariant headerData(int section, Qt::Orientation orientation,int role = Qt::DisplayRole) const override;
    QVariant data(const QModelIndex & index, int role = Qt::DisplayRole)const override;
    int rowCount(const QModelIndex & parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent=QModelIndex())const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role)override;
public slots:
    void resetBuildModelSlot();
private:
    QStringList _headerData;
};
/*@@@@@@@@@@@@@@@@@@@@@@FakeDialogFrame@@@@@@@@@@@@@@@@@@@@@@@@*/
class FakeDialog:public QDialog{
    Q_OBJECT
public:
    explicit FakeDialog(QWidget* parent=nullptr);
    inline void setLabelText(QString text){_label->setText(text);}
signals:
    void sendFakeDialogResultTextSignal(QString);
private:
    QGridLayout* _layout;
    QLabel* _label;
    QLineEdit* _edit;
    QPushButton* _okayButton;
    QPushButton* _cancelButton;
    QString _resultText;
};
/*@@@@@@@@@@@@@@@@@@@@@@BuildModel@@@@@@@@@@@@@@@@@@@@@@@@@*/
class BuildView:public QTableView{
    Q_OBJECT
public:
    explicit BuildView(QWidget* parent=nullptr);
public slots:
    void clickedSlot(QModelIndex);
    void showMenuSlot(QPoint);
    void addBuildActionTriggeredSlot();
    void delBuildActionTriggeredSlot();
    void receiveFakeDialogResultTextSlot(QString);
private:
    BuildModel* _buildModel;
    QMenu* _menu;
    QAction* _addBuildAction;
    QAction* _delBuildAction;
    QModelIndex _currentIndex;
    FakeDialog* _buildNameDialog;
};
/*@@@@@@@@@@@@@@@@@@@@@@BuildFrame@@@@@@@@@@@@@@@@@@@@@@@@@*/
class BuildFrame:public QFrame{
    Q_OBJECT
public:
    explicit BuildFrame(QWidget* parent=nullptr);
public slots:
    void checkDirSlot();
private:
    BuildView* _buildView;
    QPushButton* _checkDirButton;
    QPushButton* _saveBuildButton;
    QPushButton* _saveRoadButton;
    QVBoxLayout* _layout;
};
/*@@@@@@@@@@@@@@@@@@@@@@RoutePlanner@@@@@@@@@@@@@@@@@@@@@@@@@*/
class RoutePlanner:public QFrame{
    Q_OBJECT
public:
    explicit RoutePlanner(QWidget* parent=nullptr);
public slots:
    void receiveStatusBarStringSignal(QString);
private:
    BasementView* _mapView;
    BuildFrame* _buildFrame;
    QPushButton* _saveBuildButton;
    LineFrame* _lineFrame;
    QPushButton* _saveLineButton;
    QStatusBar* _statusBar;
    QSplitter* _splitter01;
    QSplitter* _splitter02;
    QVBoxLayout* _layout;
};
#endif // ROUTEPLANNER_H
