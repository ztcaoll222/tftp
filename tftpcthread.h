#ifndef TFTPCTHREAD_H
#define TFTPCTHREAD_H

#include <base.h>

class TFTPCThread : public QThread
{
    Q_OBJECT
public:
    TFTPCThread();

    void stop();

    void init();

    void Connect();

protected:
    void run();

private:
    quint16 port;

    QString host;

    volatile bool started = false;

    SOCKET sockfd;

    struct sockaddr_in server_addr;

    struct sockaddr_in client_addr;

    QString wrrq;

    QString filename;

    my_size_t myioctl();

    int postCONNECT();

    int getDirList();

    int postRRQ(char *filename, size_t filenamelen);

    int get(char *filename);

    size_t getData(short *block, FILE *fd);

    int postACK(short block);

    int postWRQ(char *filename, size_t filenamelen);

    int put(char *filename);

    int getACK(short *block);

    int postData(short block, void *buffer, int bufferlen);

signals:
    void postFilename(const QString &);

    void postMsg(const QString &);

    void postProcess(const int &);

    void postFinish(const int &);

private slots:
    void getPort(const QString &);

    void getHost(const QString &);

    void getWRRQ(const QString &);

    void getFilename(const QString &);
};

#endif // TFTPCTHREAD_H
