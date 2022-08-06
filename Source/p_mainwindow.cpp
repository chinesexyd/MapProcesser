#include "p_mainwindow.h"
/*@@@@@@@@@@@@@@@@@@@@@@AButton@@@@@@@@@@@@@@@@@@@@@@@@*/
AButton::AButton(QWidget* parent):QPushButton(parent){
    setObjectName("A");
}
/*@@@@@@@@@@@@@@@@@@@@@@MainWindow@@@@@@@@@@@@@@@@@@@@@@@@*/
MainWindow::MainWindow(QWidget *parent): QFrame(parent){
    setObjectName("MainWindow");
    _mapUpdateButton=new AButton(this);
    _mapUpdateButton->setText("地图处理");
    _mapUpdateButton->setIcon(QIcon(":/Image/MapHolder.png"));
    _mapUpdateButton->setCheckable(true);
    _routePlanButton=new AButton(this);
    _routePlanButton->setText("路径管理");
    _routePlanButton->setIcon(QIcon(":/Image/RoutePlanner.png"));
    _routePlanButton->setCheckable(true);
    _telepresenceButton=new AButton(this);
    _telepresenceButton->setText("小车监控");
    _telepresenceButton->setIcon(QIcon(":/Image/RobotMonitor.png"));
    _telepresenceButton->setCheckable(true);
    _telepresenceButton->setChecked(true);
    _pathGenerateButton=new AButton(this);
    _pathGenerateButton->setText("路径生成");
    _pathGenerateButton->setIcon(QIcon(":/Image/PathGenerator.png"));
    _pathGenerateButton->setCheckable(true);
    _buttonGroup=new QButtonGroup(this);
    _buttonGroup->addButton(_mapUpdateButton);
    _buttonGroup->addButton(_routePlanButton);
    _buttonGroup->addButton(_telepresenceButton);
    _buttonGroup->addButton(_pathGenerateButton);
    _mapUpdater=new MapUpdater(this);
    _routePlanner=new RoutePlanner(this);
    _telepresencer=new Telepresencer(this);
    _pathGenerator=new PathGenerator(this);
    _sLayout=new QStackedLayout();
    _sLayout->addWidget(_telepresencer);
    _sLayout->addWidget(_routePlanner);
    _sLayout->addWidget(_mapUpdater);
    _sLayout->addWidget(_pathGenerator);
    _gLayout=new QGridLayout(this);
    _gLayout->setHorizontalSpacing(1);
    _gLayout->setContentsMargins(20,5,20,20);
    _gLayout->addWidget(_telepresenceButton,0,0,1,1);
    _gLayout->addWidget(_routePlanButton,0,1,1,1);
    _gLayout->addWidget(_mapUpdateButton,0,2,1,1);
    _gLayout->addWidget(_pathGenerateButton,0,3,1,1);
    _gLayout->addLayout(_sLayout,1,0,1,4);
    _gLayout->setRowStretch(0,1);
    _gLayout->setRowStretch(1,19);
    _gLayout->setColumnStretch(0,1);
    _gLayout->setColumnStretch(1,1);
    _gLayout->setColumnStretch(2,1);
    _gLayout->setColumnStretch(3,1);
    setLayout(_gLayout);
    connect(_mapUpdateButton,&QPushButton::clicked,[this](){_sLayout->setCurrentWidget(_mapUpdater);contentProvider->endTimer();});
    connect(_routePlanButton,&QPushButton::clicked,[this](){_sLayout->setCurrentWidget(_routePlanner);contentProvider->endTimer();});
    connect(_pathGenerateButton,&QPushButton::clicked,[this](){_sLayout->setCurrentWidget(_pathGenerator);contentProvider->endTimer();});
    connect(_telepresenceButton,&QPushButton::clicked,[this](){_sLayout->setCurrentWidget(_telepresencer);contentProvider->beginTimer();});
    firstStart();
}
MainWindow::~MainWindow(){
    delete _sLayout;
    delete obs;
    delete dataProvider;
    delete contentProvider;
}
void MainWindow::firstStart(){
    contentProvider->beginTimer();
}
