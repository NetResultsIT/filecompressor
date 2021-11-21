/********************************************************************************
 *   Copyright (C) 2018 by NetResults S.r.l. ( http://www.netresults.it )       *
 *   Author(s):                                                                 *
 *              Francesco Lamonica  <f.lamonica@netresults.it>                  *
 ********************************************************************************/

#ifndef NRFILECOMPRESSOR_H
#define NRFILECOMPRESSOR_H

#include <QString>
class QFile;


#ifdef USE_NRFILECOMPRESSOR_NAMESPACE
#define NRFILECOMPRESSOR_NAMESPACE NRFileCompressorV0_0
#define BEGIN_NRFILECOMPRESSOR_NAMESPACE namespace NRFILECOMPRESSOR_NAMESPACE {
#define END_NRFILECOMPRESSOR_NAMESPACE }
#else
#define NRFILECOMPRESSOR_NAMESPACE
#define BEGIN_NRFILECOMPRESSOR_NAMESPACE
#define END_NRFILECOMPRESSOR_NAMESPACE
#endif


BEGIN_NRFILECOMPRESSOR_NAMESPACE

class NrFileCompressor
{

public:
    enum compressedFileFormatEnum
    {
        NO_COMPRESSION,
        GZIP_ARCHIVE,
        ZIP_ARCHIVE
    };

    enum CompressErrorType {
        E_FILE_NOT_OPEN         =  -1,
        E_FILE_NOT_WRITEABLE    =  -2,
        E_MINIZ_ERROR           =  -3,
    };

private:
    static quint8 getByte(quint32 var, quint8 bytenum);
    static int writeGzipHeader(QFile *pFile, quint32 i_mtime);
    static int writeGzipFooter(QFile *pFile, quint32 i_crc32, quint32 i_size);

public:
    NrFileCompressor();
    static int fileCompress(const QString &filename, const QString &filepath, NrFileCompressor::compressedFileFormatEnum algo, int lev=6); //Default compression level
    static int fileCompress(const QString &filename, const QString &srcpath, const QString &dstpath, NrFileCompressor::compressedFileFormatEnum algo, int lev=6); //Default compression level
    static QString getCompressedFilename(const QString &filename, NrFileCompressor::compressedFileFormatEnum algo);

    static int compressZipFile(const QString &filename, const QString &srcpath, const QString &dstpath, int level);
    static int uncompressZipFile(const QString &filename, const QString &destDir);

    static int compressGzipFile(const QString &filename, const QString &srcpath, const QString &dstpath, int level);

};

END_NRFILECOMPRESSOR_NAMESPACE

#endif // NRFILECOMPRESSOR_H
