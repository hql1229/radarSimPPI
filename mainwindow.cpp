#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    RadarWidget *radar = new RadarWidget(this);
    setCentralWidget(radar);
    resize(600, 600); // 设置窗口大小
}

MainWindow::~MainWindow()
{
    delete ui;
}

