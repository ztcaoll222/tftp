#ifndef TFTPSMAINTHREAD_H
#define TFTPSMAINTHREAD_H

#include "base.h"

class TFTPSMainThread : public QThread
{
    Q_OBJECT
public:
    TFTPSMainThread();

    void stop();

    void init();

protected:
    void run();

private:
    quint16 port;

    volatile bool started = false;

    SOCKET sockfd;

    struct sockaddr_in server_addr;

    struct sockaddr_in client_addr;

    my_size_t myioctl();

    int getCONNECT();

    int postDirList();

    int getWRRQ(struct TFTPWRRQ *wrrq);

    int put(char *filename);

    int postData(short block, void *buffer, int bufferlen);

    int getACK(short block);

    int get(char *filename);

    int postACK(short block);

    size_t getData(short block, FILE *fd);

signals:
    void postMsg(const QString &);

private slots:
    void getPort(const QString &);
};

#endif // TFTPSMAINTHREAD_H
