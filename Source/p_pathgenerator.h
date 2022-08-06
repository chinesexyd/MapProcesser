#ifndef P_PATHGENERATOR_H
#define P_PATHGENERATOR_H

#include "tool.h"
/*@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@DataDealer@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@*/
class DataDealer:public QObject{
    Q_OBJECT
public:
    enum ACTION_TYPE{
        OPEN_MAPDIR,
        DRAW_POLYGON,
        MODIFY_POLYGON,
        CREATE_PATH,
        MODIFY_PATH,
    };
    enum PATH_TYPE{
        PATH_OOO,//回字形
        PATH_SSS,//Z字形
    };
    struct MAP_INFO{
        quint16 width;
        quint16 height;
        double originX;
        double originY;
        double resolution;
    };
public:
    explicit DataDealer(QObject* parent=nullptr);
    void setMapDir(QString mapDir);
    QString mapDir();
    void setActionType(DataDealer::ACTION_TYPE type);
    DataDealer::ACTION_TYPE actionType();
    QPointF realPosFromMapPos(QPointF pos);
    QPointF mapPosFromRealPos(QPointF pos);
    void setMapInfo(quint16 width,quint16 height);
    QList<QPointF> getInnerPolygon(QList<QPointF> polygon,double distance);
    QList<QPointF> createSweepPath(QList<QPointF> polygon,double pathDistance,PATH_TYPE type=DataDealer::PATH_OOO);
private:
    bool _isPointInPolygon(QList<QPointF> polygon,QPointF pos);
    void _createSweepPath(QList<QPointF> &sweepPath,QList<QPointF> polygon,double pathDistance);
signals:
    void setMapSignal(QString);
    void loadPolygonPathSignal(QString);
    void setActionTypeSignal(DataDealer::ACTION_TYPE);
    void setRadiusDistanceSignal(double,double);
    void savePolygonPathSignal();
private:
    MAP_INFO _mapInfo;
    ACTION_TYPE _actionType;
    QString _mapDir;
};
extern DataDealer* dataDealer;
/*@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@GeneratorPathPoint@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@*/
class GeneratorPathLine:public QGraphicsObject{
    Q_OBJECT
public:
    enum{Type=UserType+1};
    explicit GeneratorPathLine(QGraphicsItem* parent=nullptr);
    void setStartPos(QPointF pos);
    void setEndPos(QPointF pos);
    QPointF _startPos=QPointF(0,0);
    QPointF _endPos=QPointF(0,0);
protected:
    int type() const override;
    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = Q_NULLPTR) override;
};
/*@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@GeneratorPathPoint@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@*/
class GeneratorPathPoint:public QGraphicsObject{
    Q_OBJECT
public:
    enum{Type=UserType+2};
    explicit GeneratorPathPoint(QGraphicsItem* parent=nullptr);
signals:
    void posChangedSignal(GeneratorPathPoint*);
protected:
    int type() const override;
    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = Q_NULLPTR) override;
};
/*@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@GeneratorPolygonLine@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@*/
class GeneratorPolygonLine:public QGraphicsObject{
public:
    enum{Type=UserType+4};
    explicit GeneratorPolygonLine(QGraphicsItem* parent=nullptr);
    void setStartPos(QPointF pos);
    void setEndPos(QPointF pos);
protected:
    int type() const override;
    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = Q_NULLPTR) override;
    void hoverEnterEvent(QGraphicsSceneHoverEvent *event)override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent *event)override;
private:
    bool _isHoverIn=false;
    QPointF _startPos=QPointF(0,0);
    QPointF _endPos=QPointF(0,0);
    double _lineWidth=10;
};
/*@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@GeneratorPolygonPoint@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@*/
class GeneratorPolygonPoint:public QGraphicsObject{
    Q_OBJECT
public:
    enum{Type=UserType+5};
    explicit GeneratorPolygonPoint(QGraphicsItem* parent=nullptr);
protected:
    int type() const override;
    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = Q_NULLPTR) override;
    void hoverEnterEvent(QGraphicsSceneHoverEvent *event)override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent *event)override;
signals:
    void posChangedSignal(GeneratorPolygonPoint*);
private:
    bool _isHoverIn=false;
};
/*@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@GeneratorMap@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@*/
class GeneratorMap:public QObject,public QGraphicsPixmapItem{
    Q_OBJECT
public:
    explicit GeneratorMap(QGraphicsItem* parent=nullptr);
    void setMapData(QByteArray bytes);
private:
    QPixmap _pixmap;
};
/*@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@GeneratorScene@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@*/
class GeneratorScene:public QGraphicsScene{
    Q_OBJECT
public:
    struct PTR{
        GeneratorPolygonPoint* _polygonPointPtr;
        GeneratorPolygonLine* _polygonLinePtr;
        GeneratorPathPoint* _pathPointPtr;
        GeneratorPathLine* _pathLinePtr;
    };
public:
    explicit GeneratorScene(QWidget* parent=nullptr);
    ~GeneratorScene();
    void setPolygonPoint(QPointF scenepos);
    void setActionType(DataDealer::ACTION_TYPE type);
protected:
    void contextMenuEvent(QGraphicsSceneContextMenuEvent *event)override;
public slots:
    void setMapSlot(QString mapPath);
    void loadPolygonPathSlot(QString pathPath);
    void polygonPointPosChangedSlot(GeneratorPolygonPoint*);
    void pathPointPosChangedSlot(GeneratorPathPoint*);
    void delPolygonPointActionSlot();
    void addPolygonPointActionSlot();
    void delPathPointActionSlot();
    void addPathPointActionSlot();
    void setRadiusDistanceSlot(double,double);
    void savePolygonPathSlot();
private:
    GeneratorMap* _mapItem=nullptr;
    QRectF _startPosRectF;
    QList<GeneratorPolygonPoint*> _polygonPointList;
    QList<GeneratorPolygonLine*> _polygonLineList;
    QList<GeneratorPathPoint*> _pathPointList;
    QList<GeneratorPathLine*> _pathLineList;
    bool _isPolygonEnd=false;
    DataDealer::ACTION_TYPE _actionType;
    QPointF _cursorPos;
    QMenu* _polygonPointMenu;
    QAction* _delPolygonPointAction;
    QMenu* _polygonLineMenu;
    QAction* _addPolygonPointAction;
    QMenu*_pathPointMenu;
    QAction* _delPathPointAction;
    QAction* _addPathPointAction;
    PTR _ptr;
    QString _polygonPathJsonPath;
    QString _mapPath;
};
/*@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@GeneratorCreatePathDialog@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@*/
class GeneratorCreatePathDialog:public QDialog{
    Q_OBJECT
public:
    explicit GeneratorCreatePathDialog(QWidget* parent=nullptr);
protected:
    void closeEvent(QCloseEvent *) override;
public slots:
    void okayButtonClickedSlot();
private:
    QGridLayout* _layout;
    QLabel* _robotRadiusLabel;
    QLineEdit* _robotRadiusLine;
    QLabel* _lineDistanceLabel;
    QLineEdit* _lineDistanceLine;
    QPushButton* _okayButton;
};
/*@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@GeneratorView@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@*/
class GeneratorView:public QGraphicsView{
    Q_OBJECT
public:
    explicit GeneratorView(QWidget* parent=nullptr);
    ~GeneratorView();
protected:
    void mousePressEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent* event)override;
public slots:
    void openDirButtonClickedSlot();
    void drawPolygonButtonClickedSlot();
    void modPolygonButtonClickedSlot();
    void createPathButtonClickedSlot();
    void modPathButtonClickedSlot();
    void savePathButtonClickedSlot();
private:
    GeneratorScene* _scene;
    QHBoxLayout* _layout01;
    QPushButton* _openDirButton;
    QPushButton* _drawPolygonButton;
    QPushButton* _modPolygonButton;
    QPushButton* _createPathButton;
    QPushButton* _modPathButton;
    QPushButton* _savePathButton;
    QButtonGroup* _buttonGroup;
    QVBoxLayout* _layout;
    GeneratorCreatePathDialog* _dialog;
};
/*@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@扫地车路径生成器@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@*/
class PathGenerator:public QFrame{
public:
    explicit PathGenerator(QWidget* parent=nullptr);
    ~PathGenerator();
private:
    GeneratorView* _view;
    QGridLayout* _layout;
};

#endif // P_PATHGENERATOR_H
