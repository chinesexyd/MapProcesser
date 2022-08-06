#ifndef ROOT_H
#define ROOT_H
//把程序所有用到的库都导入到此处
////////////////
#include <QTableView>
#include <QFrame>
#include <QPushButton>
#include <QHeaderView>
#include <QLabel>
#include <QSplitter>
#include <QComboBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QButtonGroup>
#include <QMenu>
#include <QAction>
#include <QGraphicsView>
#include <QGraphicsItem>
#include <QGraphicsScene>
#include <QListView>
#include <QTreeView>
#include <QLineEdit>
#include <QStatusBar>
#include <QToolBar>
#include <QMessageBox>
////////////////
#include <QAbstractTableModel>
#include <QStyledItemDelegate>
#include <QSortFilterProxyModel>
////////////////
#include <QFile>
#include <QDir>
#include <QFileSystemWatcher>
////////////////
#include <QStackedLayout>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
////////////////
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDomDocument>
////////////////
#include <QDebug>
#include <QVariant>
#include <QFileInfo>
#include <QDateTime>
#include <QPainter>
#include <QWheelEvent>
#include <QThread>
#include <QtMath>
#include <QGraphicsSceneContextMenuEvent>
////////////////
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
////////////////
#include <string>
#include "yaml-cpp/yaml.h"

#include "MQTTAsync.h"
#include "QtMqtt/QtMqttDepends"
#include "QtMqtt/qmqttclient.h"
#include "QtMqtt/qmqttmessage.h"
#include "QtMqtt/qmqtttype.h"

////////////////
#define INDEX_VALUE_FILES_DIR QDir::homePath()+"/MapProcessor/MapProcessor/IndexValue"
#define MAP_FILES_DIR QDir::homePath()+"/maps"
#define MAPS_DIR QDir::homePath()+"/MapProcessor/MapProcessor/Maps"
#define SAVE_MAPS_DIR QDir::homePath()+"/MapProcessor/MapProcessor/SaveMaps"
#define CONFIG_FILE QDir::homePath()+"/MapProcessor/MapProcessor/Config/Config.json"
#define ROBOTS_FILE QDir::homePath()+"/MapProcessor/MapProcessor/Robots/Robots.json"
//结束
#endif // ROOT_H
