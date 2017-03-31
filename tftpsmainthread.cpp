#include "tftpsmainthread.h"

TFTPSMainThread::TFTPSMainThread()
{

}

void TFTPSMainThread::stop()
{
    started = 0;
    shutdown(sockfd, 2);
    this->terminate();
    quit();
    wait();
}

my_size_t TFTPSMainThread::myioctl()
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

void TFTPSMainThread::init()
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

#ifdef Q_OS_WIN
    char opt;
#else
    int opt = 1;
#endif

    int res = -1;
    if(0 > (res = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)))) {
        perror("setsockopt()");
    }

    res = -1;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);
    if (0 > (res = bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)))) {
        perror("bind()");
    }

    memset(&client_addr, 0, sizeof(server_addr));
    client_addr.sin_family = AF_INET;
}

void TFTPSMainThread::run()
{
    qDebug()<<"thread start";
    started = 1;

    while (started) {
        if (getCONNECT() && postDirList()) {
            struct TFTPWRRQ wrrq;
            int res = 0;
            res = getWRRQ(&wrrq);
            emit postMsg(QString("filename is : ")+QString(wrrq.filename));
            emit postMsg(QString("mode is: ")+QString(wrrq.mode));
            if (1 == res) {
                put(wrrq.filename);
            }
            else if (2 == res)
            {
                get(wrrq.filename);
            }
        }
    }
}

int TFTPSMainThread::getCONNECT()
{
    my_size_t bufferlen = myioctl();

#ifdef Q_OS_WIN
    char *buffer = (char *)malloc(bufferlen);
#else
    void *buffer = malloc(bufferlen);
#endif

    ssize_t recvlen = -1;
    socklen_t addrlen = sizeof(struct sockaddr);
    if (0 > (recvlen = recvfrom(sockfd, buffer, bufferlen, 0, (struct sockaddr *)&client_addr, &addrlen))) {
        perror("getCONNECT recvfrom()");
    }

    struct TFTPHeader *header = (struct TFTPHeader *)buffer;
    if (OPCODE_CONNECT == ntohs(header->opcode)) {
        emit postMsg((QString)"get CONNECT from: " + inet_ntoa(client_addr.sin_addr) + (QString)" : " + QString::number(ntohs(client_addr.sin_port), 10));
        return 1;
    }

    return 0;
}

int TFTPSMainThread::postDirList()
{
    QDir dir;
    dir.setSorting(QDir::Name | QDir::IgnoreCase);
    dir.sorting();

    QStringList dirList;
    foreach (QFileInfo fileInfo, dir.entryList(QDir::NoDotAndDotDot | QDir::Files | QDir::Drives)) {
        QString temp0 = fileInfo.fileName() + ":" + QString::number(fileInfo.size(), 10);

        dirList.append(temp0);
    }

    QString temp1 = dirList.join("/");

    QFile file(".dir");
    if (file.open(QIODevice::WriteOnly)) {
        file.write(temp1.toLatin1());
        file.close();
    }
    else {
        emit postMsg("can not write .dir file!");
        return 0;
    }

    if (put(".dir")) {
        return 1;
    }
    else {
        return 0;
    }
}

int TFTPSMainThread::getWRRQ(struct TFTPWRRQ *wrrq)
{
    my_size_t bufferlen = myioctl();

#ifdef Q_OS_WIN
    char *buffer = (char *)malloc(bufferlen);
#else
    void *buffer = malloc(bufferlen);
#endif

    ssize_t recvlen = -1;
    socklen_t addrlen = sizeof(struct sockaddr);
    if (0 > (recvlen = recvfrom(sockfd, buffer, bufferlen, 0, (struct sockaddr *)&client_addr, &addrlen))) {
        perror("getReadReq recvfrom()");
        return -1;
    }

    char *temp = (char *)buffer+2;
    size_t filenamelen = strlen(temp)+1;
    wrrq->filename = (char *)malloc(filenamelen);
    memset(wrrq->filename, 0, filenamelen);
    memcpy(wrrq->filename, temp, filenamelen);

    temp = (char *)buffer+2+filenamelen;
    size_t modelen = strlen(temp)+1;
    wrrq->mode = (char *)malloc(modelen);
    memset(wrrq->mode, 0, modelen);
    memcpy(wrrq->mode, temp, modelen);

    struct TFTPHeader *header = (struct TFTPHeader *)buffer;
    if (OPCODE_RRQ == ntohs(header->opcode)) {
        emit postMsg(QString("get RRQ from : ")+QString(inet_ntoa(client_addr.sin_addr))+":"+QString::number(ntohs(client_addr.sin_port), 10));
        free(buffer);
        return 1;
    }
    else if (OPCODE_WRQ == ntohs(header->opcode)) {
        emit postMsg(QString("get WRQ from : ")+QString(inet_ntoa(client_addr.sin_addr))+":"+QString::number(ntohs(client_addr.sin_port), 10));
        free(buffer);
        return 2;
    }
    else
    {
        emit postMsg(QString("can not get 'RRQ' or 'WRQ'!"));
        free(buffer);
        return 0;
    }
}

int TFTPSMainThread::put(char *filename)
{
    short block = 1;
    emit postMsg("put : " + QString(filename) + "...");

    FILE *fd = NULL;
    if (NULL == (fd = fopen(filename, "rb"))) {
        perror("fd fopen()");
        return 0;
    }

    size_t bufferlen = 512;
    void *buffer = malloc(bufferlen);
    while (bufferlen) {
        bufferlen = fread(buffer, sizeof(void), 512, fd);

        if (!postData(block, buffer, bufferlen)) {
            return 0;
        }

        if (!getACK(block)) {
            return 0;
        }

        block++;
    }

    fclose(fd);

    return 1;
}

int TFTPSMainThread::postData(short block, void *buffer, int bufferlen)
{
    struct TFTPACK ack;
    ack.header.opcode = htons(OPCODE_DATA);
    ack.block = htons(block);

    uint16_t packetsize = sizeof(ack)+bufferlen;

#ifdef Q_OS_WIN
    char *packet = (char *)malloc(packetsize);
#else
    void *packet = malloc(packetsize);
#endif

    memcpy(packet, &ack, sizeof(ack));
    memcpy(packet+sizeof(ack), buffer, bufferlen);

    ssize_t sendlen = -1;
    if (0 > (sendlen = sendto(sockfd, packet, packetsize, 0, (struct sockaddr*)&client_addr, sizeof(client_addr)))) {
        perror("postData sendto()");
        return 0;
    }

    emit postMsg(QString("bufferlen = ")+QString::number((int)bufferlen, 10));

    free(packet);
}

int TFTPSMainThread::getACK(short block)
{
    my_size_t bufferlen = myioctl();

#ifdef Q_OS_WIN
    char *buffer = (char *)malloc(bufferlen);
#else
    void *buffer = malloc(bufferlen);
#endif

    ssize_t recvlen = -1;
    socklen_t addrlen = sizeof(client_addr);
    if (0 > (recvlen = recvfrom(sockfd, buffer, bufferlen, 0, (struct sockaddr *)&client_addr, &addrlen))) {
        perror("getACK recvfrom()");
        return 0;
    }

    struct TFTPACK *ack = (struct TFTPACK *)buffer;
    if (OPCODE_ACK != ntohs(ack->header.opcode)) {
        emit postMsg(QString("can not get ACK!"));
        return 0;
    }

    if (block != ntohs(ack->block)) {
        emit postMsg(QString("block ")+QString::number(ntohs(ack->block), 10)+QString("error!"));
        return 0;
    }

    emit postMsg(QString("block is ")+QString::number(block, 10));

    free(buffer);

    return 1;
}

int TFTPSMainThread::get(char *filename)
{
    short block = 1;

    FILE *fd = NULL;
    if (NULL == (fd = fopen(filename, "wb"))) {
        perror("fd fopen()");
        return -1;
    }

    size_t recvlen = 1;
    while (recvlen) {
        int res = -1;
        if (0 > (res = postACK(block))) {
            return -1;
        }

        recvlen = getData(block, fd);

        block++;
    }

    fclose(fd);

    return 0;
}

int TFTPSMainThread::postACK(short block)
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
    if (0 > (res = sendto(sockfd, packet, packetsize, 0, (struct sockaddr*)&client_addr, sizeof(struct sockaddr)))) {
        perror("postRRQ sendto()");
        return 0;
    }

    return 1;
}

size_t TFTPSMainThread::getData(short block, FILE *fd)
{
    my_size_t packetsize = myioctl();

#ifdef Q_OS_WIN
    char *packet = (char *)malloc(packetsize);
#else
    void *packet = malloc(packetsize);
#endif

    ssize_t recvlen = -1;
    socklen_t addrlen = sizeof(server_addr);
    if (0 > (recvlen = recvfrom(sockfd, packet, packetsize, 0, (struct sockaddr *)&client_addr, &addrlen))) {
        perror("getReadReq recvfrom()");
        return 0;
    }

    struct TFTPACK *ack = (struct TFTPACK *)packet;
    if (OPCODE_DATA != ntohs(ack->header.opcode));

    if (block != ntohs(ack->block)) {
        emit postMsg(QString("block ")+QString::number(ntohs(ack->block), 10)+QString("error!"));
        return 0;
    }

    emit postMsg(QString("block is : ")+QString::number(block, 10));

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

void TFTPSMainThread::getPort(const QString &str)
{
    port = str.toInt();
}
