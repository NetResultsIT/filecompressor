/********************************************************************************
 *   Copyright (C) 2018 by NetResults S.r.l. ( http://www.netresults.it )       *
 *   Author(s):                                                                 *
 *              Francesco Lamonica  <f.lamonica@netresults.it>                  *
 ********************************************************************************/

#include "NrFileCompressor.h"
#include "miniz.h"

#include <QFile>
#include <QFileInfo>
#include <QDateTime>
#include <QDir>

#include <iostream>
#include <memory>

#if defined(__GNUC__)
  // Ensure we get the 64-bit variants of the CRT's file I/O calls
  #ifndef _FILE_OFFSET_BITS
    #define _FILE_OFFSET_BITS 64
  #endif
  #ifndef _LARGEFILE64_SOURCE
    #define _LARGEFILE64_SOURCE 1
  #endif
#endif


#define GZIP_EXT ".gz"
#define ZIP_EXT ".zip"


BEGIN_NRFILECOMPRESSOR_NAMESPACE


NrFileCompressor::NrFileCompressor()
{
    /* empty ctor */
}


/*!
 * \brief NrFileCompressor::fileCompress the basic function to be called to compress a file (zip file will be created in current working dir)
 * \param i_fileName the filename (without path) of the file to be compressed
 * \param i_srcpath the path where the file to be compressed is located
 * \param i_algo the type of compression to be used, either GZIP or ZIP
 * \param i_level the compression level (0=none(faster) .. 9=max (slower))
 * \return 0 if successful, a negative error code otherwise
 */
int NrFileCompressor::fileCompress(QString const& i_fileName, const QString &i_srcpath, NrFileCompressor::compressedFileFormatEnum i_algo, int i_lev)
{
    return fileCompress(i_fileName, i_srcpath, QDir::currentPath(), i_algo, i_lev);
}

/*!
 * \brief NrFileCompressor::fileCompress the basic function to be called to compress a file
 * \param i_fileName the filename (without path) of the file to be compressed
 * \param i_srcpath the path where the file to be compressed is located
 * \param i_dstpath the path where the compressed file should be created (it must exists and be writable)
 * \param i_algo the type of compression to be used, either GZIP or ZIP
 * \param i_level the compression level (0=none(faster) .. 9=max (slower))
 * \return 0 if successful, a negative error code otherwise
 */
int NrFileCompressor::fileCompress(QString const& i_fileName, const QString &i_srcpath, const QString &i_dstpath, NrFileCompressor::compressedFileFormatEnum i_algo, int i_lev)
{
    if (i_algo == NrFileCompressor::GZIP_ARCHIVE) {
        return compressGzipFile(i_fileName, i_srcpath, i_dstpath, i_lev);
    } else {
        return compressZipFile(i_fileName, i_srcpath, i_dstpath, i_lev);
    }
}



QString calculateNameCompliantWithZipAlgoMiniZ(const QString &filename)
{
    QString s = filename;
    s.replace("\\", "_");
    s.replace("/", "_");
    s.replace(":", "_");
    return s;
}

QString calculateFilenameWithPath(const QString &i_dstPath, const QString & filename)
{
    QString destfileWithpath = QDir::fromNativeSeparators(i_dstPath);
    if (!destfileWithpath.endsWith("/")) {
        destfileWithpath.append("/");
    }
    destfileWithpath.append(filename);
    return destfileWithpath;
}


/*!
 * \brief NrFileCompressor::getCompressedFilename
 * \param i_fileName the name of the file to compress
 * \param i_algo the algorithm that should be used to compress
 * \return the filename with the correct extension denoting the compression
 */
QString
NrFileCompressor::getCompressedFilename(const QString &i_fileName, NrFileCompressor::compressedFileFormatEnum i_algo)
{
    switch(i_algo) {
        case NrFileCompressor::GZIP_ARCHIVE:
            return i_fileName + GZIP_EXT;
        case NrFileCompressor::ZIP_ARCHIVE:
            return calculateNameCompliantWithZipAlgoMiniZ(i_fileName) + ZIP_EXT;
        default:
            return i_fileName;
    }
}

/*******************
 *     ZIP PART    *
 * *****************/

/*!
 * \brief NrFileCompressor::compressZipFile
 * \param i_filename the filename (without path) of the file to be compressed
 * \param i_srcpath the path where the file to be compressed is located
 * \param i_dstpath the path where the compressed file should be created (it must exists and be writable)
 * \param level the level of compression to be used while compressing the ZIP file (0=storing, 6=default, 9=maximum)
 * \return a integer return code, 0 meaning the process was successfull
 * \warning the filename \em cannot contain characters '\', '/' or ':' if it does they will be replaced by "_"
 */
int NrFileCompressor::compressZipFile(const QString &i_filename, const QString &i_srcpath, const QString &i_dstpath, int level)
{
    std::cout << "Compressing (ZIP) file " << i_filename.toStdString() << std::endl;
    const char *s_pComment = "Zipped with NrFileCompressor! Invalid chars replaced with _";

    QString compressedfilename = getCompressedFilename(i_filename, NrFileCompressor::ZIP_ARCHIVE);

    QString destfilename = calculateFilenameWithPath(i_dstpath, compressedfilename);
    QString srcfilename = calculateFilenameWithPath(i_srcpath, i_filename);

    if (!QFile::exists(srcfilename)) {
        std::cerr << "Cannot find file to compress: " << srcfilename.toStdString() << std::endl;
        return NrFileCompressor::E_FILE_NOT_OPEN;
    }

    mz_zip_archive zip_archive;

    //reset the zip archive
    memset(&zip_archive, 0, sizeof(zip_archive));

    //init for writing
    bool res = mz_zip_writer_init_file(&zip_archive, destfilename.toLatin1().constData(), 0);

    if (!res)
    {
        std::cerr << "" << mz_zip_get_error_string(mz_zip_get_last_error(&zip_archive)) << std::endl;
        return EXIT_FAILURE;
    }

    // add "filename" file to the archive with a (possibly) modified name "destfilename" (the one that it will be unzipped with)
    // as the original might have some invalid characters in it: '/', '\' or ':'
    res = mz_zip_writer_add_file(&zip_archive, compressedfilename.toLatin1().constData(),
                                  srcfilename.toLatin1().constData(),
                                  s_pComment, (quint16)strlen(s_pComment), level);
    if (!res)
    {
        std::cerr << "Error while adding a zip file (" << i_filename.toStdString() << ") to zip archive: "
                  << mz_zip_get_error_string(mz_zip_get_last_error(&zip_archive)) << std::endl;
        return EXIT_FAILURE;
    }

    res = mz_zip_writer_finalize_archive(&zip_archive);
    if (!res)
    {
        std::cerr << "Error while finalizing zip archive: " << mz_zip_get_error_string(mz_zip_get_last_error(&zip_archive)) << std::endl;
        return EXIT_FAILURE;
    }

    res = mz_zip_writer_end(&zip_archive);
    if (!res)
    {
        std::cerr << "Error while closing zip archive: " << mz_zip_get_error_string(mz_zip_get_last_error(&zip_archive)) << std::endl;
        return EXIT_FAILURE;
    }

    return 0;
}

/*!
 * \brief NrFileCompressor::uncompressZipFile method to uncompress a zip archive file
 * \param filename the full path of the zip archive file to be uncompresses
 * \param destDir the destination directory where extracted files will be stored
 * \return 0 if successful, a negative error code otherwise
 * \note Currently the method skips directories and doesn't preserve files relative path.
 *       Furthermore, files in destDir are overwritten from extracted files having the same name.
 */
int NrFileCompressor::uncompressZipFile(const QString &filename, const QString &destDir)
{
    std::cout << "Uncompressing (ZIP) file " << filename.toStdString() << std::endl;

    mz_zip_archive zip_archive;
    memset(&zip_archive, 0, sizeof(zip_archive));

    // init for read
    bool res = mz_zip_reader_init_file(&zip_archive, filename.toLatin1().constData(), 0);
    if (!res)
    {
        std::cerr << "" << mz_zip_get_error_string(mz_zip_get_last_error(&zip_archive)) << std::endl;
        return EXIT_FAILURE;
    }

    uint count = mz_zip_reader_get_num_files(&zip_archive);
    if (count == 0)
    {
        mz_zip_reader_end(&zip_archive);
        return 0;
    }

    // extract the files
    for (uint i = 0; i < count; ++i)
    {
        mz_zip_archive_file_stat file_stat;
        if (!mz_zip_reader_file_stat(&zip_archive, i, &file_stat))
        {
            continue;
        }

        if (mz_zip_reader_is_file_a_directory(&zip_archive, i))
        {
            // skip directories
            continue;
        }

        QString destfn = QFileInfo(QString(file_stat.m_filename)).fileName();
        QString destfilename = QString("%1/%2").arg(!destDir.isEmpty() ? destDir : ".", destfn);
        
        res = mz_zip_reader_extract_to_file(&zip_archive, i, destfilename.toLatin1().constData(), 0);
        if (!res)
        {
            std::cerr << "Error while extracting file from zip archive: " << mz_zip_get_error_string(mz_zip_get_last_error(&zip_archive)) << std::endl;
            return EXIT_FAILURE;
        }
    }

    // close the archive
    mz_zip_reader_end(&zip_archive);

    return 0;
}


/*********************
 *     gZIP PART     *
 * *******************/


quint8 NrFileCompressor::getByte(quint32 var, quint8 bytenum)
{
    int shift;
    switch(bytenum) {
    case 1:
        shift = 8;
        break;
    case 2:
        shift = 16;
        break;
    case 3:
        shift = 24;
        break;
    default: //(LSB)
        shift = 0;
        break;
    }
    return (var >> shift) & 0xFF;
}


int NrFileCompressor::writeGzipHeader(QFile *pFile, quint32 i_mtime)
{
    /*
        It builds the GZIP file structure with his header and footer in this format:
        (http://www.zlib.org/rfc-gzip.html)

        +---+---+---+---+---+---+---+---+---+---+=============================+---+---+---+---+---+---+---+---+
        |ID1|ID2|CM |FLG|     MTIME     |XFL|OS |   ....COMPRESSED DATA....   |     CRC32     |     ISIZE     |
        +---+---+---+---+---+---+---+---+---+---+=============================+---+---+---+---+---+---+---+---+

        The first 10 bytes header is fixed, the CRC32 is the checksum of uncompressed data
        and the ISIZE is the size of uncompressed data in bytes.
     */
    const char id1      = 31;   //Fixed value identifying GZip
    const char  id2     = 139;  //Fixed value identifying GZip
    const char  cm      = 8;    //(Fixed value identifying GZip) is the standard deflate method
    const char  flg     = 0;    //0 means no extra flags
    quint8 mtime[4];
    const char  xflg    = 0;    //0 means no extra flags

#ifdef WIN32
    const char  os      = 0;    //0 is Dos/win, 3 is Unix-style (used for line endings)
#else
    const char  os      = 3;    //0 is Dos/win, 3 is Unix-style (used for line endings)
#endif

    mtime[0] = getByte(i_mtime, 0);
    mtime[1] = getByte(i_mtime, 1);
    mtime[2] = getByte(i_mtime, 2);
    mtime[3] = getByte(i_mtime, 3);


    pFile->write(&id1, 1);
    pFile->write(&id2, 1);
    pFile->write(&cm, 1);
    pFile->write(&flg, 1);
    pFile->write((const char*) &mtime, 4);
    pFile->write(&xflg, 1);
    pFile->write(&os, 1);

    return 0;
}


int NrFileCompressor::writeGzipFooter(QFile *pFile, quint32 i_crc32, quint32 i_size)
{
    quint8 size[4];
    size[0] = getByte(i_size, 0);
    size[1] = getByte(i_size, 1);
    size[2] = getByte(i_size, 2);
    size[3] = getByte(i_size, 3);

    quint8 crc32[4];
    crc32[0] = getByte(i_crc32, 0);
    crc32[1] = getByte(i_crc32, 1);
    crc32[2] = getByte(i_crc32, 2);
    crc32[3] = getByte(i_crc32, 3);

    pFile->write((const char*) &crc32, 4);
    pFile->write((const char*) &size, 4);

    return 0;
}


/*!
 * \brief NrFileCompressor::compressGzipFile
 * \param i_filename the filename (without path) of the file to be compressed
 * \param i_srcpath the path where the file to be compressed is located
 * \param i_dstpath the path where the compressed file should be created (it must exists and be writable)
 * \param level the level of compression to be used while compressing the GZIP file (0=storing, 6=default, 9=maximum)
 * \return a integer return code, 0 meaning the process was successfull
 */
int NrFileCompressor::compressGzipFile(const QString &i_filename, const QString &i_srcpath, const QString &i_dstpath, int level)
{
    std::cout << "Compressing (GZIP) file " << i_filename.toStdString() << std::endl;
    //int level = Z_BEST_COMPRESSION;
    z_stream stream;

    QString compressedfilename = getCompressedFilename(i_filename, NrFileCompressor::GZIP_ARCHIVE);
    QString destfilename = calculateFilenameWithPath(i_dstpath, compressedfilename);
    QString srcfilename = calculateFilenameWithPath(i_srcpath, i_filename);

    if (!QFile::exists(srcfilename)) {
        std::cerr << "Cannot find file to compress: " << srcfilename.toStdString() << std::endl;
        return NrFileCompressor::E_FILE_NOT_OPEN;
    }

    const qint64 BUF_SIZE = (1024 * 1024);

    ///////////////////////////////////////////////////////////////////
    ///
    /// We need to create those buffers in the heap, cause in Visual
    /// Studio the stack memory is limited to 1MB and the following
    /// instructions:
    /// quint8 s_inbuf[BUF_SIZE];
    /// quint8 s_outbuf[BUF_SIZE];
    /// make the gzip compression crash at runtime

    std::unique_ptr<unsigned char[]> s_inbuf(new unsigned char [BUF_SIZE]);
    std::unique_ptr<unsigned char[]> s_outbuf(new unsigned char [BUF_SIZE]);

    ///
    ///
    ///
    ///////////////////////////////////////////////////////////////////

    // Init the z_stream
    memset(&stream, 0, sizeof(stream));
    stream.next_in = s_inbuf.get();
    stream.avail_in = 0;
    stream.next_out = s_outbuf.get();
    stream.avail_out = BUF_SIZE;

    QFile fin(srcfilename);
    QFile fout(destfilename);

    bool b = true;
    b &= fin.open(QIODevice::ReadOnly);
    b &= fout.open(QIODevice::WriteOnly);

    if(!b) {
        fin.close();
        fout.close();
        return NrFileCompressor::E_FILE_NOT_OPEN;
    }

    //write the GZip file header
    QFileInfo finfo(fin);
    writeGzipHeader(&fout, static_cast<quint32>(finfo.lastModified().toMSecsSinceEpoch()/1000));

    // Compression.
    qint64 finSize = fin.size();
    qint64 infile_remaining = finSize;

    //if (deflateInit(&stream, level) != Z_OK)
    if (deflateInit2(&stream, level, MZ_DEFLATED, -MZ_DEFAULT_WINDOW_BITS, 9, MZ_DEFAULT_STRATEGY) != Z_OK)
    {
        std::cerr << "deflateInit2() failed!" << std::endl;
        return NrFileCompressor::E_MINIZ_ERROR;
    }

    //init the crc for uncompressed data
    ulong crc = mz_crc32(0, nullptr, 0);

    //reading loop
    for ( ; ; )
    {
        int status;
        if (!stream.avail_in)
        {
          // Input buffer is empty, so read more bytes from input file.
          uint n = qMin((qint64)BUF_SIZE, infile_remaining);

          if (fin.read((char*)s_inbuf.get(), n) != n)
          {
            std::cerr << "Failed reading from input file!" << std::endl;
            return NrFileCompressor::E_MINIZ_ERROR;
          }

          //update the crc
          crc = mz_crc32(crc, s_inbuf.get(), n);

          stream.next_in = s_inbuf.get();
          stream.avail_in = n;

          infile_remaining -= n;
          std::cout << "Input bytes remaining: " << infile_remaining << std::endl;
        }

        status = deflate(&stream, infile_remaining ? Z_NO_FLUSH : Z_FINISH);

        if ((status == Z_STREAM_END) || (!stream.avail_out))
        {
          // Output buffer is full, or compression is done, so write buffer to output file.
          uint n = BUF_SIZE - stream.avail_out;
          if (fout.write((char*)s_outbuf.get(), n) != n)
          {
            std::cerr << "Failed writing to output file!" << std::endl;
            return NrFileCompressor::E_MINIZ_ERROR;
          }
          stream.next_out = s_outbuf.get();
          stream.avail_out = BUF_SIZE;
        }

        if (status == Z_STREAM_END)
            break;
        else if (status != Z_OK)
        {
            std::cerr << "deflate() failed with status: " << status << std::endl;
            return NrFileCompressor::E_MINIZ_ERROR;
        }
    }

    if (deflateEnd(&stream) != Z_OK)
    {
        std::cerr << "deflateEnd() failed!" << std::endl;
        return NrFileCompressor::E_MINIZ_ERROR;
    }

    //This is a fast modulo to power-of-2 numbers
    quint32 modsize = static_cast<quint32>(finSize & (LONG_MAX - 1));
    //write the GZIP file footer
    writeGzipFooter(&fout, static_cast<quint32>(crc), modsize);

    fin.close();
    fout.close();

    return Z_OK;
}


END_NRFILECOMPRESSOR_NAMESPACE

