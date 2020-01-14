#include "mainwindow.h"
#include <QApplication>
#include <QDebug>
int main(int argc, char *argv[])
{
    qDebug()<<argv[1];
    QApplication a(argc, argv);
    QByteArray array(argv[1]);
    QString para = array;
    MainWindow w(para);
    w.show();

    return a.exec();
}
