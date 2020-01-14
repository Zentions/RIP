#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMessageBox>
#include<string.h>

MainWindow::MainWindow(QString para,QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    this->SEND_TIME_OUT = 6000;
    this->RECEIVE_TIME_OUT = 18000;
    QStringList list = para.split(",");
    this->RouterId = list[0].toInt();
    this->port = list[1].toUShort();
    //为每个邻居节点设置超时定时器
    for(int i=2;i<list.size();i++)
    {
        QTimer* timer = new QTimer;
        this->neighbor[timer]=list[i].toUShort();
        connect(timer, SIGNAL(timeout()), this, SLOT(receiveHandleTimeout()));
        timer->start(this->RECEIVE_TIME_OUT);
    }
    server = new QUdpSocket;
    //绑定端口
    server->bind(this->port);
    connect(server,SIGNAL(readyRead()),this,SLOT(messageReceive()));
    this->ui->label_6->setText(QString::number(this->port));
    //启动发送报文定时器
    this->send_timer = new QTimer;
    connect(send_timer, SIGNAL(timeout()), this, SLOT(handleTimeout()));
    send_timer->start(this->SEND_TIME_OUT);
}

MainWindow::~MainWindow()
{
    delete ui;
}
//发送udp报文
void MainWindow::sendData(QByteArray array,quint16 port)
{
    server->writeDatagram(array,QHostAddress("127.0.0.1"),port);
}
void MainWindow::messageReceive()
{
    char *buf = new char[500];
    //等待数据接受
    while(server->hasPendingDatagrams())
    {
        memset(buf, 0, 500);
        QHostAddress addr;
        quint16 port;
        qDebug()<<"size"<<server->pendingDatagramSize();
        server->readDatagram(buf,500,&addr,&port);
        if(addr.toIPv4Address()!=0)
        {
            //第一个字符是b，说明是正常的udp报文
            if(buf[0]=='b')
            {
                short ip[4];
                memcpy(ip,buf+1,sizeof(short)*4);
                qDebug()<<ip[0]<<ip[1]<<ip[2]<<ip[3];
                int i;
                //查找路由表，寻找下一跳
                for(i=0;i<route_table.size();i++)
                {
                    rip_data r_item = route_table[i].item;
                    if(r_item.metric !=16 && (r_item.mask[0]&ip[0]) == r_item.netid[0] &&(r_item.mask[1]&ip[1]) == r_item.netid[1]
                            && (r_item.mask[2]&ip[2]) == r_item.netid[2] &&(r_item.mask[3]&ip[3]) == r_item.netid[3])
                    {
                        if(r_item.next==0) ui->textBrowser->append(QString("%1.%2.%3.%4直连，已发出").arg(ip[0]).arg(ip[1]).arg(ip[2]).arg(ip[3]));
                        else
                        {
                            char buf[15];
                            memset(buf,0,15);
                            buf[0] = 'b';
                            memcpy(buf+1,ip,sizeof(short)*4);
                            buf[1+sizeof(short)*4]='0';
                            buf[2+sizeof(short)*4]='1';
                            buf[3+sizeof(short)*4]='2';
                            this->server->writeDatagram(buf,15,QHostAddress("127.0.0.1"),r_item.next);
                            ui->textBrowser->append(QString("向%1.%2.%3.%4发出一个包，下一跳%5").arg(ip[0]).arg(ip[1]).arg(ip[2]).arg(ip[3]).arg(r_item.next));
                        }
                        break;
                    }
                }
                if(i==route_table.size())
                    ui->textBrowser->append(QString("%1.%2.%3.%4不可达！").arg(ip[0]).arg(ip[1]).arg(ip[2]).arg(ip[3]));
            }
            else
            {
                //如果第一个字符是a，标识路由信息报文
                //邻居节点active，重置定时器
                ResetTimer(port);
                //检测报文来源，决定是否拒收
                if(reject.contains(port))
                {
                    ui->textBrowser->append(QString("拒收来自%1的路由报文！").arg(port));
                }
                else
                {
                    //第二个字符标识udp报文中含有的路由条目
                    int num = (int)buf[1];
                    qDebug()<<"num:"<<num;
                    int size = sizeof(rip_data);
                    //根据距离向量算法更新路由表
                    for(int i=0;i<num;i++)
                    {
                        rip_data *data = (rip_data *)(buf+2+i*size);
                        data->next = port;
                        data->metric+=1;
                        qDebug()<<"receive log"<<data->ip()<<data->sub_mask()<<data->next<<data->metric;
                        QDateTime time = QDateTime::currentDateTime();   //获取当前时间
                        int j;
                        for(j=0;j<route_table.size();j++)
                        {
                            if(route_table[j].item.ip()==data->ip())
                            {
                                if(route_table[j].item.next==data->next)
                                {
                                    route_table[j].item.metric = data->metric;
                                    route_table[j].time = time.toTime_t();
                                    ui->textBrowser->append(QString("到达%1相同下一跳，更新metric为%2！").arg(data->ip()).arg(data->metric));
                                }
                                else
                                {
                                    if(route_table[j].item.metric> data->metric)
                                    {
                                        route_table[j].item.next = data->next;
                                        route_table[j].item.metric = data->metric;
                                        route_table[j].time = time.toTime_t();
                                        ui->textBrowser->append(QString("到达%1下一跳不同，metric更短，更新为%2！").arg(data->ip()).arg(data->metric));
                                    }
                                }
                                break;
                            }
                        }
                        if(j==route_table.size())
                        {
                            route_item item;
                            memcpy(&(item.item),data,sizeof(rip_data));
                            item.time = time.toTime_t();
                            route_table.append(item);
                            ui->textBrowser->append(QString("添加%1到路由表，下一跳为%2，metric为%3！").arg(data->ip()).arg(data->next).arg(data->metric));
                        }
                    }
                }

            }
        }
    }
    delete buf;
}
void MainWindow::on_pushButton_clicked()
{
    QString txt = ui->lineEdit->text();
    //输出邻居节点
    if(txt =="N")
    {
        QList<quint16> list = neighbor.values();
        ui->textBrowser->append("邻居节点：");
        for(int i=0;i<list.size();i++)
        {
            ui->textBrowser->append(QString::number(list[i]));
        }
    }
    //输出路由信息
    else if(txt =="T")
    {
        ui->textBrowser->append(QString("%1\t%2\t%3\t%4").arg("网络号").arg("子网掩码").arg("metric").arg("next"));
        for(int i=0;i<route_table.size();i++)
        {
            ui->textBrowser->append(QString("%1\t%2\t%3\t%4").arg(route_table[i].item.ip()).arg(route_table[i].item.sub_mask()).arg(route_table[i].item.metric).arg(route_table[i].item.next));

        }
    }
    //指定一个目的地址，发送一个udp数据包
    else if(txt.contains("D"))
    {
        QStringList list = txt.split(" ")[1].split(".");
        short ip[4];
        for(int i=0;i<4;i++)
        {
            ip[i] = list[i].toShort();
        }
        int i;
        for(i=0;i<route_table.size();i++)
        {
            rip_data r_item = route_table[i].item;
            if(r_item.metric !=16 && (r_item.mask[0]&ip[0]) == r_item.netid[0] &&(r_item.mask[1]&ip[1]) == r_item.netid[1]
                    && (r_item.mask[2]&ip[2]) == r_item.netid[2] &&(r_item.mask[3]&ip[3]) == r_item.netid[3])
            {
                //检查是否直连
                if(r_item.next==0) ui->textBrowser->append(QString("%1直连，已发出").arg(txt.split(" ")[1]));
                else
                {
                    char buf[15];
                    memset(buf,0,15);
                    buf[0] = 'b';
                    memcpy(buf+1,ip,sizeof(short)*4);
                    buf[1+sizeof(short)*4]='0';
                    buf[2+sizeof(short)*4]='1';
                    buf[3+sizeof(short)*4]='2';
                    this->server->writeDatagram(buf,15,QHostAddress("127.0.0.1"),r_item.next);
                    ui->textBrowser->append(QString("向%1发出一个包，下一跳%2").arg(txt.split(" ")[1]).arg(r_item.next));
                }
                break;
            }
        }
        if(i==route_table.size())
            ui->textBrowser->append(QString("%1不可达！").arg(txt.split(" ")[1]));
    }
    else if (txt.contains("R"))
    {
        quint16 port = txt.split(" ")[1].toUShort();
        if(!reject.contains(port)) reject.insert(port);
        ui->textBrowser->append(QString("以设置拒收来自%1的路由信息报文！").arg(port));
    }

}
void MainWindow::handleTimeout()
{
    qDebug()<<"send timeout";
    char* m_send_buf = new char[500];
    QList<quint16> list = neighbor.values();
    for(int i=0;i<list.size();i++)
    {
        int num = 0;
        memset(m_send_buf,0,500);
        m_send_buf[0] = 'a';
        //给每个邻居节点发送路由报文，使用水平分割的技术
        for(int j=0;j<route_table.size();j++)
        {
            if(route_table[j].item.next != list[i])
            {
                memcpy(m_send_buf+num*sizeof(rip_data)+2,&(route_table[j].item),sizeof(rip_data));
                num++;
            }
        }
        m_send_buf[1]=num;
        rip_data *data = (rip_data *)(m_send_buf+2);
        server->writeDatagram(m_send_buf,500,QHostAddress("127.0.0.1"),list[i]);
    }


}
void MainWindow::receiveHandleTimeout()
{
    qDebug()<<"receive——timeout";
    //检测不到邻居节点active，将其从邻居节点中删除，并把经过该节点的metric置为16，意味不可达
    QTimer *timer =  qobject_cast<QTimer *>(sender());
    quint16 un_reach = this->neighbor[timer];
    this->neighbor.remove(timer);
    timer->stop();
    delete timer;
    for (int i=0;i<route_table.size();i++)
    {
         route_item item = route_table[i];
         if(item.item.next == un_reach)
         {
             item.item.metric = 16;
         }
    }



}
void MainWindow::ResetTimer(quint16 port)
{
    QList<QTimer *> list = neighbor.keys();
    for(int i=0;i<list.size();i++)
    {
        if(neighbor[list[i]]==port)
        {
            list[i]->stop();
            list[i]->start(this->RECEIVE_TIME_OUT);
            break;
        }
    }
}
//配置直连网络
void MainWindow::on_pushButton_2_clicked()
{
    QString netId = this->ui->lineEdit_2->text();
    QString mask = this->ui->lineEdit_3->text();
    route_item item;
    QStringList list = netId.split(".");
    for(int i=0;i<4;i++)
    {
        item.item.netid[i] = list[i].toShort();
    }

    QStringList list1 = mask.split(".");
    for(int i=0;i<4;i++)
    {
        item.item.mask[i] = list1[i].toShort();
    }
    item.item.next=0;
    item.item.metric = 0;
    QDateTime time = QDateTime::currentDateTime();   //获取当前时间
    item.time = time.toTime_t();
    route_table.append(item);
    qDebug()<<item.item.ip()<<item.item.sub_mask();
}
//配置超时时间
void MainWindow::on_pushButton_3_clicked()
{
    int time1 = ui->lineEdit_4->text().toInt();
    int time2 = ui->lineEdit_5->text().toInt();
    this->SEND_TIME_OUT = time1*1000;
    this->RECEIVE_TIME_OUT = time2*1000;
    qDebug()<<SEND_TIME_OUT<<RECEIVE_TIME_OUT;
    send_timer->stop();
    send_timer->start(this->SEND_TIME_OUT);
    QList<QTimer *>list = neighbor.keys();
    for(int i=0;i<list.size();i++)
    {
        list[i]->stop();
        list[i]->start(this->RECEIVE_TIME_OUT);
    }
}
