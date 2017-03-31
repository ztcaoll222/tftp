#include "tftpcthread.h"

TFTPCThread::TFTPCThread()
{

}

void TFTPCThread::stop()
{
    started = 0;
    shutdown(sockfd, 2);
    this->terminate();
    quit();
    wait();
}

my_size_t TFTPCThread::myioctl()
{
#ifdef Q_OS_WIN
    my_size_t bufferlen = 0;
    while (0 == bufferlen) {
        if (0 == started) {
            return 0;
        }
        ioctlsocket(sockfd, FIONREAD, &bufferlen);
    }
#else
    my_size_t bufferlen = 0;
    while (0 == bufferlen) {
        if (0 == started) {
            return 0;
        }
        ioctl(sockfd, FIONREAD, &bufferlen);
    }
#endif
    return bufferlen;
}

void TFTPCThread::init()
{
#ifdef Q_OS_WIN
    WSADATA wsaData;
    WSAStartup((MAKEWORD(2,0)), &wsaData);
#endif

    struct protoent *proto = NULL;
    if (NULL == (proto = getprotobyname("udp"))) {
        perror("getprotobyname()");
    }

    sockfd = -1;
    if (0 > (sockfd = socket(AF_INET, SOCK_DGRAM, proto->p_proto))) {
        perror("socket()");
    }

    struct hostent* server_host = NULL;
    if (NULL == (server_host = gethostbyname(host.toLatin1().data()))) {
        perror("gethostbyname()");
    }

    int res = -1;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr = *((struct in_addr *)server_host->h_addr_list[0]);
    server_addr.sin_port = htons(port);
}

void TFTPCThread::Connect()
{
    started = 1;
    if (postCONNECT()) {
        getDirList();
    }
}

void TFTPCThread::run()
{
    qDebug()<<"thread start";

    if ("get" == wrrq) {
        if (postRRQ(filename.toLatin1().data(), filename.size()+1)) {
            get(filename.toLatin1().data());
        }
    }
    else {
        QFileInfo info;
        info.setFile(filename);
        if (postWRQ(info.fileName().toLatin1().data(), info.fileName().size()+1)) {
            put(filename.toLatin1().data());
        }
    }

    emit postFinish(1);
}

int TFTPCThread::postCONNECT()
{
    uint16_t t_opcode = htons(OPCODE_CONNECT);

#ifdef Q_OS_WIN
    char *packet = (char *)malloc(sizeof(uint16_t));
#else
    void *packet = malloc(sizeof(uint16_t));
#endif

    memcpy(packet, &t_opcode, sizeof(t_opcode));

    ssize_t sendlen = -1;
    if (0 > (sendlen = sendto(sockfd, packet, sizeof(packet), 0, (struct sockaddr*)&server_addr, sizeof(struct sockaddr)))) {
        perror("postData sendto()");
        return 0;
    }

    return 1;
}

int TFTPCThread::getDirList()
{
    if (!get(".dir")) {
        return 0;
    }

    QFile file(".dir");
    if (file.open(QIODevice::ReadOnly)) {
        QString temp = QString(file.readAll());
        QStringList dirList = temp.split("/");
        foreach (QString x, dirList) {
            emit postFilename(x);
        }
        file.close();
    }

    return 1;

}

int TFTPCThread::postRRQ(char *filename, size_t filenamelen)
{
    struct TFTPHeader header;
    header.opcode = htons(OPCODE_RRQ);

    char mode[] = "octet";
    size_t modelen = strlen(mode)+1;

    size_t packetsize = sizeof(header)+filenamelen+modelen;

#ifdef Q_OS_WIN
    char *packet = (char *)malloc(packetsize);
#else
    void *packet = malloc(packetsize);
#endif

    memcpy(packet, &header, sizeof(header));
    memcpy(packet+sizeof(header), filename, filenamelen);
    memcpy(packet+sizeof(header)+filenamelen, mode, modelen);

    ssize_t res = -1;
    if (0 > (res = sendto(sockfd, packet, packetsize, 0, (struct sockaddr*)&server_addr, sizeof(struct sockaddr)))) {
        perror("postRRQ sendto()");
        return 0;
    }

    return 1;
}

int TFTPCThread::get(char *filename)
{
    FILE *fd = NULL;
    if (NULL == (fd = (fopen(filename, "wb")))) {
        perror("fw fopen()");
        return 0;
    }

    size_t recvlen = 1;
    short block;
    while (recvlen) {
        recvlen = getData(&block, fd);

        emit postProcess(recvlen);

        if (!postACK(block)) {
            return 0;
        }
    }

    fclose(fd);

    qDebug()<<"finish!";

    return 1;
}

size_t TFTPCThread::getData(short *block, FILE *fd)
{
    my_size_t packetsize = myioctl();

#ifdef Q_OS_WIN
    char *packet = (char *)malloc(packetsize);
#else
    void *packet = malloc(packetsize);
#endif

    ssize_t recvlen = -1;
    socklen_t addrlen = sizeof(server_addr);
    if (0 > (recvlen = recvfrom(sockfd, packet, packetsize, 0, (struct sockaddr *)&server_addr, &addrlen))) {
        perror("getData recvfrom()");
        return 0;
    }

    struct TFTPACK *ack = (struct TFTPACK *)packet;
    if (OPCODE_DATA != ntohs(ack->header.opcode)) {
        emit postMsg(QString("can not get DATA!"));
    }

    *block = ntohs(ack->block);
    emit postMsg(QString("block is ")+QString::number(*block, 10));

    size_t bufferlen = packetsize-sizeof(struct TFTPACK);
    emit postMsg(QString("bufferlen is : ")+QString::number((int)bufferlen, 10));
    void *buffer = malloc(bufferlen);
    memset(buffer, 0, bufferlen);
    memcpy(buffer, packet+sizeof(struct TFTPACK), bufferlen);

    size_t res = 0;
    if (0 == (res = fwrite(buffer, sizeof(void), bufferlen, fd))) {
        perror("getData fwrite()");
        return 0;
    }

    free(buffer);
    free(packet);

    return bufferlen;
}

int TFTPCThread::postACK(short block)
{
    struct TFTPACK ack;
    ack.header.opcode = htons(OPCODE_ACK);
    ack.block = htons(block);

    my_size_t packetsize = sizeof(ack);

#ifdef Q_OS_WIN
    char *packet = (char *)malloc(packetsize);
#else
    void *packet = malloc(packetsize);
#endif

    memcpy(packet, &ack, packetsize);

    ssize_t res = -1;
    if (0 > (res = sendto(sockfd, packet, packetsize, 0, (struct sockaddr*)&server_addr, sizeof(struct sockaddr)))) {
        perror("postRRQ sendto()");
        return 0;
    }

    return 1;
}

int TFTPCThread::postWRQ(char *filename, size_t filenamelen)
{
    struct TFTPHeader header;
    header.opcode = htons(OPCODE_WRQ);

    char mode[] = "octet";
    size_t modelen = strlen(mode)+1;

    size_t packetsize = sizeof(header)+filenamelen+modelen;

#ifdef Q_OS_WIN
    char *packet = (char *)malloc(packetsize);
#else
    void *packet = malloc(packetsize);
#endif

    memcpy(packet, &header, sizeof(header));
    memcpy(packet+sizeof(header), filename, filenamelen);
    memcpy(packet+sizeof(header)+filenamelen, mode, modelen);

    ssize_t res = -1;
    if (0 > (res = sendto(sockfd, packet, packetsize, 0, (struct sockaddr*)&server_addr, sizeof(struct sockaddr)))) {
        perror("postRRQ sendto()");
        return 0;
    }

    return 1;
}

int TFTPCThread::put(char *filename)
{
    FILE *fd = NULL;
    if (NULL == (fd = fopen(filename, "rb"))) {
        perror("fd fopen()");
        return 0;
    }

    short block;
    size_t bufferlen = 512;
    void *buffer = malloc(bufferlen);
    while (bufferlen) {
        bufferlen = fread(buffer, sizeof(void), 512, fd);
        postProcess(bufferlen);

        int res = -1;
        if (0 > (res = getACK(&block))) {
            return 0;
        }

        res = -1;
        if (0 > (res = postData(block, buffer, bufferlen))) {
            return 0;
        }
    }

    fclose(fd);

    return 1;
}

int TFTPCThread::getACK(short *block)
{
    my_size_t bufferlen = myioctl();

#ifdef Q_OS_WIN
    char *buffer = (char *)malloc(bufferlen);
#else
    void *buffer = malloc(bufferlen);
#endif

    ssize_t recvlen = -1;
    socklen_t addrlen = sizeof(server_addr);
    if (0 > (recvlen = recvfrom(sockfd, buffer, bufferlen, 0, (struct sockaddr *)&server_addr, &addrlen))) {
        perror("getACK recvfrom()");
        return 0;
    }

    struct TFTPACK *ack = (struct TFTPACK *)buffer;
    if (OPCODE_ACK != ntohs(ack->header.opcode)) {
        emit postMsg("can not get ACK");
        return 0;
    }

    *block = ntohs(ack->block);

    emit postMsg(QString("block is ")+QString::number(*block, 10));

    free(buffer);

    return 1;
}

int TFTPCThread::postData(short block, void *buffer, int bufferlen)
{
    struct TFTPACK ack;
    ack.header.opcode = htons(OPCODE_DATA);
    ack.block = htons(block);

    size_t packetsize = sizeof(ack)+bufferlen;

#ifdef Q_OS_WIN
    char *packet = (char *)malloc(packetsize);
#else
    void *packet = malloc(packetsize);
#endif

    memcpy(packet, &ack, sizeof(ack));
    memcpy(packet+sizeof(ack), buffer, bufferlen);

    ssize_t sendlen = -1;
    if (0 > (sendlen = sendto(sockfd, packet, packetsize, 0, (struct sockaddr*)&server_addr, sizeof(server_addr)))) {
        perror("postData sendto()");
        return -1;
    }

    emit postMsg(QString("bufferlen = ")+QString::number((int)bufferlen, 10));

    free(packet);

    return 0;
}

void TFTPCThread::getPort(const QString &str)
{
    port = str.toInt();
}

void TFTPCThread::getHost(const QString &str)
{
    host = str;
}

void TFTPCThread::getWRRQ(const QString &str)
{
    wrrq = str;
}

void TFTPCThread::getFilename(const QString &str)
{
    filename = str;
}
