#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    filename = "";
    isShowMsg = 0;
    isServre = 1;
    TFTPSMainThreadStatus = 0;
    TFTPCThreadStatus = 0;
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_comboBox_server_activated(const QString &arg1)
{
    if ("server" == arg1) {
        initServer();
    }
    else {
        initClient();
    }
}

void MainWindow::initServer()
{
    isShowMsg = 0;
    isServre = 1;

    stopTFTPSMainThread();

    stopTFTPCThread();

    ui->lineEdit_localPort->setText("69");
    ui->lineEdit_remotePort->setText("6969");
    ui->listWidget->clear();
    ui->progressBar->setValue(0);

    ui->lineEdit_localPort->setEnabled(true);
    ui->lineEdit_remoteHost->setEnabled(false);
    ui->lineEdit_remotePort->setEnabled(false);
    ui->comboBox_wrrq->setEnabled(false);
    ui->pushButton_filename->setEnabled(false);
    ui->progressBar->setEnabled(false);
    ui->pushButton_start->setEnabled(false);
}

void MainWindow::initClient()
{
    isShowMsg = 0;
    isServre = 0;

    stopTFTPSMainThread();

    stopTFTPCThread();

    ui->lineEdit_localPort->setText("6969");
    ui->lineEdit_remotePort->setText("69");
    ui->listWidget->clear();
    ui->progressBar->setValue(0);

    ui->lineEdit_localPort->setEnabled(false);
    ui->lineEdit_remoteHost->setEnabled(true);
    ui->lineEdit_remotePort->setEnabled(true);
    ui->comboBox_wrrq->setEnabled(true);
    ui->progressBar->setEnabled(false);
    ui->pushButton_start->setEnabled(false);
    if ("put" == ui->comboBox_wrrq->currentText()) {
        ui->pushButton_filename->setEnabled(true);
    }
}

int MainWindow::checkInput()
{
    if (ui->lineEdit_localPort->text().isEmpty()) {
        QMessageBox::warning(this, "Error!", "Local port is empty!", QMessageBox::Yes);
        ui->lineEdit_localPort->setFocus();
        return 0;
    }
    if (ui->lineEdit_remoteHost->text().isEmpty()) {
        QMessageBox::warning(this, "Error!", "Remote host is empty!", QMessageBox::Yes);
        ui->lineEdit_remoteHost->setFocus();
        return 0;
    }
    if (ui->lineEdit_remotePort->text().isEmpty()) {
        QMessageBox::warning(this, "Error!", "Remote port is empty!", QMessageBox::Yes);
        ui->lineEdit_remotePort->setFocus();
        return 0;
    }
    return 1;
}

void MainWindow::on_comboBox_wrrq_activated(const QString &arg1)
{
    if ("put" == arg1 && 0 == isServre) {
        ui->pushButton_filename->setEnabled(true);
    }
    else {
        ui->pushButton_filename->setEnabled(false);
    }
}

void MainWindow::on_pushButton_filename_clicked()
{
    filename = QFileDialog::getOpenFileName(this);

    if ("" != filename) {
        QFileInfo info;
        info.setFile(filename);
        ui->label_filename->setText(info.fileName());
    }
    else {
        ui->label_filename->setText("filename");
    }

    if (TFTPCThreadStatus) {
        ui->pushButton_start->setEnabled(true);
    }
}

void MainWindow::getFilename(const QString &str)
{
    ui->listWidget->addItem(str);
}

void MainWindow::getMsg(const QString &str)
{
    if (isShowMsg) {
        ui->listWidget->addItem(str);
    }
}

void MainWindow::startTFTPSMainThread()
{
    if (!TFTPSMainThreadStatus) {
        connect(this, SIGNAL(postLocalPort(QString)), &tftpsMainThread, SLOT(getPort(QString)));
        emit postLocalPort(ui->lineEdit_localPort->text());

        connect(&tftpsMainThread, SIGNAL(postMsg(QString)), this, SLOT(getMsg(QString)));

        tftpsMainThread.init();

        tftpsMainThread.start();

        TFTPSMainThreadStatus = 1;
    }
    ui->pushButton_connect->setText("disconnect");
}

void MainWindow::stopTFTPSMainThread()
{
    if (TFTPSMainThreadStatus) {
        tftpsMainThread.stop();

        TFTPSMainThreadStatus = 0;
    }
    ui->pushButton_connect->setText("connect");
}

void MainWindow::initTFTPCThread()
{
    if (!TFTPCThreadStatus) {
        connect(this, SIGNAL(postRemoteHost(QString)), &tftpcThread, SLOT(getHost(QString)));
        emit postRemoteHost(ui->lineEdit_remoteHost->text());

        connect(this, SIGNAL(postRemotePort(QString)), &tftpcThread, SLOT(getPort(QString)));
        emit postRemotePort(ui->lineEdit_remotePort->text());

        connect(&tftpcThread, SIGNAL(postFilename(QString)), this, SLOT(getFilename(QString)));
        connect(&tftpcThread, SIGNAL(postMsg(QString)), this, SLOT(getMsg(QString)));

        tftpcThread.init();

        tftpcThread.Connect();

        TFTPCThreadStatus = 1;
    }
    ui->pushButton_connect->setText("disconnect");
}

void MainWindow::stopTFTPCThread()
{
    if (TFTPCThreadStatus) {
        tftpcThread.stop();

        TFTPCThreadStatus = 0;
    }
    ui->pushButton_connect->setText("connect");
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    stopTFTPSMainThread();

    stopTFTPCThread();
}

void MainWindow::on_listWidget_clicked(const QModelIndex &index)
{
    if(!isServre && "get" == ui->comboBox_wrrq->currentText()) {
        ui->pushButton_start->setEnabled(true);
    }
}

void MainWindow::on_pushButton_connect_clicked()
{
    ui->listWidget->clear();
    ui->progressBar->setValue(0);

    if (checkInput()) {
        if (isServre && !TFTPSMainThreadStatus) {
            isShowMsg = 1;
            startTFTPSMainThread();
        }
        else if (isServre && TFTPSMainThreadStatus) {
            isShowMsg = 1;
            stopTFTPSMainThread();
        }
        else if (!isServre && !TFTPCThreadStatus) {
            isShowMsg = 0;
            initTFTPCThread();
        }
        else if (!isServre && TFTPCThreadStatus) {
            isShowMsg = 0;
            stopTFTPCThread();
        }
    }
}

void MainWindow::on_pushButton_start_clicked()
{
    isShowMsg = 1;

    if (!isServre && "put" == ui->comboBox_wrrq->currentText() && "" == filename) {
        QMessageBox::warning(this, "Error!", "Filename is empty!", QMessageBox::Yes);
        ui->pushButton_filename->setFocus();
    }
    else {
        ui->progressBar->setValue(0);

        ui->progressBar->setEnabled(true);
        ui->pushButton_connect->setEnabled(false);
        ui->listWidget->setEnabled(false);

        connect(this, SIGNAL(postWRRQ(QString)), &tftpcThread, SLOT(getWRRQ(QString)));
        emit postWRRQ(ui->comboBox_wrrq->currentText());

        connect(this, SIGNAL(postFilename(QString)), &tftpcThread, SLOT(getFilename(QString)));
        if ("get" == ui->comboBox_wrrq->currentText()) {
            QString temp = ui->listWidget->currentItem()->text().split(":")[1];
            ui->progressBar->setMaximum(temp.toInt());

            filename = ui->listWidget->currentItem()->text().split(":")[0];
            emit postFilename(filename);
        }
        else if ("put" == ui->comboBox_wrrq->currentText()){
            QFileInfo info;
            info.setFile(filename);
            ui->progressBar->setMaximum(info.size());

            emit postFilename(filename);
        }

        connect(&tftpcThread, SIGNAL(postProcess(int)), this, SLOT(getProcessValue(int)));

        connect(&tftpcThread, SIGNAL(postFinish(int)), this, SLOT(getFinish(int)));

        ui->listWidget->clear();

        tftpcThread.start();
    }
}

void MainWindow::on_progressBar_valueChanged(int value)
{
    if (value == ui->progressBar->maximum()) {
        ui->listWidget->setEnabled(true);
        ui->pushButton_connect->setEnabled(true);
    }
}

void MainWindow::getProcessValue(const int i)
{
    ui->progressBar->setValue(ui->progressBar->value()+i);
}

void MainWindow::getFinish(const int i)
{
    stopTFTPCThread();

    ui->pushButton_start->setEnabled(false);

    filename = "";
    ui->label_filename->setText("filename");

    QMessageBox::warning(this, "Finish!", "Thread is Finish!", QMessageBox::Yes);
    ui->pushButton_filename->setFocus();
}
