#ifndef BASE_H
#define BASE_H

#include <QFileDialog>
#include <QMessageBox>
#include <QDebug>
#include <QThread>
#include <QCloseEvent>
#include <QDir>
#include <QFile>
#include <QFileInfo>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <assert.h>
#include <stdint.h>

#ifdef Q_OS_WIN

#include <winsock2.h>
#define socklen_t int
#define my_size_t u_long
#else

#include <sys/socket.h>
#include <netdb.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#define SOCKET		int
#define my_size_t size_t
#endif

#define OPCODE_RRQ (1)
#define OPCODE_WRQ (2)
#define OPCODE_DATA (3)
#define OPCODE_ACK (4)
#define OPCODE_ERR (5)
#define OPCODE_CONNECT (6)

#define BLOCKSIZE (512)

struct TFTPHeader{
    short opcode;
}__attribute__((packed));

struct TFTPWRRQ{
    struct TFTPHeader header;
    char *filename;	// Terminal as \0x00
    char *mode;	// Terminal as \0x00
}__attribute__((packed));

struct TFTPData{
    struct TFTPHeader header;
    short block;
    char *data;
}__attribute__((packed));

struct TFTPACK{
    struct TFTPHeader header;
    short block;
}__attribute__((packed));

struct TFTPERR{
    struct TFTPHeader header;
    short errcode;
    char *errmsg;	// Terminal as \0x00
}__attribute__((packed));

#endif // BASE_H
