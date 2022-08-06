#include "p_mainwindow.h"

#include <QApplication>
int main(int argc, char *argv[]){
    QApplication a(argc, argv);
    a.setStyleSheet(TOOL::StyleSheet(":/Qss/default.qss"));
    TOOL::CreateDirOrFile();
    MainWindow w;
    w.resize(1024,768);
    w.show();
    return a.exec();
}
