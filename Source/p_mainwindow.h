#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "p_mapupdater.h"
#include "p_routeplanner.h"
#include "p_telepresencer.h"
#include "p_pathgenerator.h"
/*@@@@@@@@@@@@@@@@@@@@@@AButton@@@@@@@@@@@@@@@@@@@@@@@@*/
class AButton:public QPushButton{
    Q_OBJECT
public:
    explicit AButton(QWidget* parent=nullptr);
};
/*@@@@@@@@@@@@@@@@@@@@@@MainWindow@@@@@@@@@@@@@@@@@@@@@@@@*/
class MainWindow : public QFrame{
    Q_OBJECT
public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
private:
    void firstStart();
private:
    QGridLayout* _gLayout;
    QStackedLayout* _sLayout;
    MapUpdater* _mapUpdater;
    RoutePlanner* _routePlanner;
    Telepresencer* _telepresencer;
    PathGenerator* _pathGenerator;
    AButton* _mapUpdateButton;
    AButton* _routePlanButton;
    AButton* _telepresenceButton;
    AButton* _pathGenerateButton;
    QButtonGroup* _buttonGroup;
};
#endif // MAINWINDOW_H
