#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QVector>
#include <QUdpSocket>
#include <QTimer>
#include <QMap>
#include <QDateTime>
#include <QSet>
namespace Ui {
class MainWindow;
}

struct rip_data{
    short netid[4];
    short mask[4];
    quint16 next;
    int metric;
    QString ip()
    {
        QString ip_addr = "";
        ip_addr = QString::number(netid[0]);
        for(int i=1;i<4;i++)
        {
            ip_addr.append('.');
            ip_addr+=QString::number(netid[i]);
        }
        return ip_addr;
    }
    QString sub_mask()
    {
        QString mask_addr = "";
        mask_addr= QString::number(mask[0]);
        for(int i=1;i<4;i++)
        {
            mask_addr.append('.');
            mask_addr+=QString::number(mask[i]);
        }
        return mask_addr;
    }
};
struct route_item{
    rip_data item;
    qint64 time;

};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QString para,QWidget *parent = nullptr);
    ~MainWindow();
    void sendData(QByteArray array,quint16 port);

    void ResetTimer(quint16 port);
private:
    Ui::MainWindow *ui;
    int RouterId;
    QMap<QTimer*,quint16> neighbor;
    QUdpSocket *server;
    quint16 port=3000;
    QVector<route_item> route_table;
    QTimer* send_timer;
    int SEND_TIME_OUT;
    int RECEIVE_TIME_OUT;
    int max_route_item_len = 25;
    QSet<quint16> reject;

private slots:
    void messageReceive();
    void on_pushButton_clicked();
    void handleTimeout();
    void receiveHandleTimeout();
    void on_pushButton_2_clicked();
    void on_pushButton_3_clicked();
};

#endif // MAINWINDOW_H
