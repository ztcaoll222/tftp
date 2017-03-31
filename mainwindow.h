#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "tftpsmainthread.h"
#include "tftpcthread.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void on_comboBox_server_activated(const QString &arg1);

    void on_comboBox_wrrq_activated(const QString &arg1);

    void on_pushButton_filename_clicked();

    void on_pushButton_connect_clicked();

    void getFilename(const QString &);

    void getMsg(const QString &);

    void on_listWidget_clicked(const QModelIndex &index);

    void on_pushButton_start_clicked();

    void on_progressBar_valueChanged(int value);

    void getProcessValue(const int i);

    void getFinish(const int i);

protected:
    void closeEvent(QCloseEvent *event);

private:
    Ui::MainWindow *ui;

    QString filename;

    int isShowMsg;

    int isServre;

    int TFTPSMainThreadStatus;

    int TFTPCThreadStatus;

    TFTPSMainThread tftpsMainThread;

    void startTFTPSMainThread();

    void stopTFTPSMainThread();

    TFTPCThread tftpcThread;

    void initTFTPCThread();

    void stopTFTPCThread();

    void initServer();

    void initClient();

    int checkInput();

signals:
    void postLocalPort(const QString &);

    void postRemoteHost(const QString &);

    void postRemotePort(const QString &);

    void postWRRQ(const QString &);

    void postFilename(const QString &);

};

#endif // MAINWINDOW_H
