/*
    Copyright © 2020-2025 by The qBangPae Project Contributors

    This file is part of qBangPae, a Qt-based graphical interface for BangPae.

    qBangPae is libre software: you can redistribute it and/or modify
    it under the terms of the RyongNamSan General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    qBangPae is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    RyongNamSan General Public License for more details.

    You should have received a copy of the RyongNamSan General Public License
    along with qBangPae.  If not, see <http://www.ryongnamsan.edu.kp/licenses/>.
*/
#include "widget.h"

#include <QApplication>
#include <QClipboard>
#include <QDebug>
#include <QDesktopServices>
#include <QDesktopWidget>
#include <QFileDialog>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPainter>
#include <QShortcut>
#include <QString>
#include <QStorageInfo>
#include <QSvgRenderer>
#include <QThreadPool>
#include <QWindow>
#include <string.h>
#include "bangpaecore/util.h"
#include "bangpaecore/net_crypto.h"
#include "circlewidget.h"
#include "contentdialog.h"
#include "contentlayout.h"
#include "form/groupchatform.h"
#include "friendlistlayout.h"
#include "friendlistwidget.h"
#include "friendwidget.h"
#include "groupwidget.h"
#include "maskablepixmapwidget.h"
#include "splitterrestorer.h"
#include "src/chatlog/content/filetransferwidget.h"
#include "src/constdefine.h"
#include "src/core/coreav.h"
#include "src/core/corefile.h"
#include "src/friendlist.h"
#include "src/grouplist.h"
#include "src/model/chathistory.h"
#include "src/model/chatroom/friendchatroom.h"
#include "src/model/chatroom/groupchatroom.h"
#include "src/model/friend.h"
#include "src/model/group.h"
#include "src/model/grouphistory.h"
#include "src/model/groupinvite.h"
#include "src/model/profile/profileinfo.h"
#include "src/model/status.h"
#include "src/net/updatecheck.h"
#include "src/nexus.h"
#include "src/persistence/offlinemsgengine.h"
#include "src/persistence/profile.h"
#include "src/persistence/settings.h"
#include "src/platform/timer.h"
#include "src/widget/chatformheader.h"
#include "src/widget/contentdialogmanager.h"
#include "src/widget/form/chatform.h"
#include "src/widget/form/groupinviteform.h"
#include "src/widget/form/profileform.h"
#include "src/widget/form/setpassworddialog.h"
#include "src/widget/form/settingswidget.h"
#include "src/widget/gui.h"
#include "src/widget/loginscreen.h"
#include "src/widget/style.h"
#include "src/widget/translator.h"
#include "tool/removefrienddialog.h"


#ifdef Q_OS_WIN
#include <windows.h>
#endif

UpdateDialog *updateDialog;
QMap<QString, uint32_t> name2intMap;

struct RelayFileStruct {
    int nDstFrndId;
    QString strFilePath;
    int nRelayFrndId;
};
RelayFileStruct* arrRelayFileData = nullptr;

extern bool bAutoLogin;
bool arrIsExistGroupPrivateCall[30] = {false};  //MAX_GRP_CNT 30 그룹최대수
bool bSoundMeetingMode = GRP_MEETINGMODE_ONEPEERTABLE;   // 현재의 그룹회의방식 (고정열쇠방식, 일반회의방식)
uint8_t StaticKey[32];                          // 고정열쇠
extern bool bSoundMeetingMode;                  // 현재의 그룹회의방식  (고정열쇠방식, 일반회의방식)
extern uint32_t nCallPeerFrndId;                // 현재 그룹보고를 하고 있는 마디점의 번호
extern bool bIsPrivateGroupCalling;             // 현재 그룹보고상태
int nRelayTextDataCnt;                          //현재 중계전송하고 있는 통보문수
int nRelayTextBufId = 0;                        //현재 중계전송하고 있는 통보문의 완충기에서의 색인번호
int nRelayFileDataCnt;                          // 중계전송파일개수
bool nArrRelayFileStatus[NUM_TOTAL_PEOPLES];    // 매 친구에 대해서 현재 파일중계전송을 하고 있는가 하는 상태보관
uint8_t nRelaySearchStartId = 0;                // 중계전송탐색첫마디점의 색인번호
int nArrRelayFrndId[NUM_TOTAL_PEOPLES];         // 매 친구에 대해서 현재 파일중계전송하고 있는 친구번호보관
int arrRelayTextFrndId[NUM_TOTAL_PEOPLES];      // 매 친구에 대해서 현재 통보문중계전송하고 있는 친구번호보관
QString arrRelayTextBuf[RELAY_TEXT_BUF_LEN];    // 중계전송하고 있는 통보문배렬


// 바이트렬을 QString형으로 변환
QString Byte2QString(const uint8_t* tKey, int nLen) {
    int i = 0;
    // char tmpStr[nLen*2 + 1];// 결과를 보관하는 char*배렬

    char* tmpStr = static_cast<char*> (malloc( (nLen * 2 + 1) * sizeof(char)));

    for (i = 0; i < nLen; i ++) {
        uint8_t t1 = tKey[i] / 16, t2 = tKey[i] % 16;
        char ch;

        // 2자리 16진수형 1바이트자료를 읽고
        // 매 바이트자료를 문자렬형식으로 변환한다.
        if (t1 < 10) ch = '0' + t1;
        else ch = 'A' + t1 - 10;
        tmpStr[i * 2] = ch;

        if (t2 < 10) ch = '0' + t2;
        else ch = 'A' + t2 - 10;

        tmpStr[i * 2 + 1] = ch;
    }
    tmpStr[nLen*2] = '\0';
    QString retStr = QString(tmpStr);
    free(tmpStr);
    return retStr;
}

bool QString2Byte(QString str, uint8_t* res) {
    int i, j = 0;
    for (i = 0; i < str.length(); i += 2) {
        char ch1 = str[i].toLatin1(), ch2 = str[i + 1].toLatin1();

        if (ch1 >= '0' && ch1 <= '9') res[j] = ch1 - '0';
        else res[j] = ch1 - 'A' + 10;

        res[j] = res[j] << 4;

        if (ch2 >= '0' && ch2 <= '9') res[j] |= (ch2 - '0');
        else res[j] |= (ch2 - 'A' + 10);
        j ++;
    }
    return true;
}

//QByteArray QString2ByteArray(QString str) {
//    QByteArray array;
//    int i;
//    char v;
//    for (i = 0; i < str.length(); i += 2) {
//        char ch1 = str[i].toLatin1(), ch2 = str[i + 1].toLatin1();

//        // 문자렬을 읽은 후 론리합연산을 리용하여 1바이트의 바이트자료로 변환한다.
//        if (ch1 >= '0' && ch1 <= '9') v = ch1 - '0';
//        else v = ch1 - 'A' + 10;

//        v = v << 4;

//        if (ch2 >= '0' && ch2 <= '9') v |= (ch2 - '0');
//        else v |= (ch2 - 'A' + 10);
//        array.push_back(v);
//    }
//    return array;
//}


void InitFileRelayStatus(int nFileCnt, int nFrndCnt) {
    for (int i = 0; i < NUM_TOTAL_PEOPLES; i ++) {
        nArrRelayFileStatus[i] = false;
    }

    memset(nArrRelayFrndId, 0xff, sizeof(int) * NUM_TOTAL_PEOPLES );
    nRelayFileDataCnt = 0;

    if (nullptr != arrRelayFileData)
        delete [] arrRelayFileData;
    arrRelayFileData = new RelayFileStruct[nFileCnt * nFrndCnt];
}

void MixWithSwap(uint8_t* peerPubKey) {
    int i;
    for (i = 0; i < 32; i += 2)  {
        uint8_t tmp = peerPubKey[i];
        peerPubKey[i] = peerPubKey[i + 1];
        peerPubKey[i + 1] = tmp;
    }
}

/**
 * Z = Swap(X1) ^ Swap(X2) ^ Y
 *
 * X1 : srcTmpPeerPubKey
 * X2 : dstTmpPeerPubKey
 * Z : arrResPubKey
 * Y : StaticKey
 */
void MixPubKeyToRelayFileFormat(uint8_t* arrResPubKey,
                                const uint8_t* srcTmpPeerPubKey, const uint8_t* dstTmpPeerPubKey) {
    uint8_t srcPeerPubKey[32], dstPeerPubKey[32];

    memcpy(srcPeerPubKey, srcTmpPeerPubKey, 32);
    memcpy(dstPeerPubKey, dstTmpPeerPubKey, 32);

    MixWithSwap(srcPeerPubKey);
    MixWithSwap(dstPeerPubKey);

    for (int i = 0; i < 32; i ++) {
        arrResPubKey[i] = srcPeerPubKey[i] ^ dstPeerPubKey[i] ^ StaticKey[i];
    }
}

/**
 * X2 = Swap(Z ^ Swap(X1) ^ Y)
 *
 * X2 : dstPeerPubKey
 * X1 : srcTmpPeerPubKey
 * Z : arrResPubKey
 * Y : StaticKey
 */

void UnmixPubKeyFromRelayFileFormat(uint8_t* dstPeerPubKey,
                                    const uint8_t* srcTmpPeerPubKey, const uint8_t* arrResPubKey) {
    uint8_t srcPeerPubKey[32];

    memcpy(srcPeerPubKey, srcTmpPeerPubKey, 32);
    MixWithSwap(srcPeerPubKey);

    for (int i = 0; i < 32; i ++) {
        dstPeerPubKey[i] = arrResPubKey[i] ^ srcPeerPubKey[i] ^ StaticKey[i];
    }

    MixWithSwap(dstPeerPubKey);
}


// ROC 2022.07.25
QString Widget::generateRandTmpDirPath() const
{
    uint8_t randNum[8];
    randombytes(randNum, RAND_DEC_DIR_NAME_LEN);

    QString strPath = "";
#ifdef Q_OS_WIN
    strPath = QDir::cleanPath(QStandardPaths::writableLocation(QStandardPaths::HomeLocation)
                           + WORK_DIR + WORK_DIR_PREFIX + Byte2QString(randNum, RAND_DEC_DIR_NAME_LEN) );
#elif defined(Q_OS_OSX)
    strPath = QDir::cleanPath(QStandardPaths::writableLocation(QStandardPaths::HomeLocation)
                           + QDir::separator() + "Library" + QDir::separator()
                           + "Application Support" + QDir::separator() + "BangPae" + QDir::separator()
                           + WORK_DIR_PREFIX + Byte2QString(randNum, RAND_DEC_DIR_NAME_LEN) );
#else
    strPath = QDir::cleanPath(QStandardPaths::writableLocation(QStandardPaths::ConfigLocation)
                           + QDir::separator() + "BangPae" + QDir::separator()
                            + WORK_DIR_PREFIX + Byte2QString(randNum, RAND_DEC_DIR_NAME_LEN));
#endif

    QDir{}.mkpath(strPath);
    return strPath;
}

// ROC 2022.07.19, 17:20 END

// 게시판의 방패전용파일 strFilepath를 암호화하여 보관한다.
void Widget::EncVidFile(QString strSrcFilePath, QString strDstFilePath)
{
    QFile srcFile(strSrcFilePath), dstFile(strDstFilePath);

    if (dstFile.exists()) return;
    const int nKeyLen = 32;

    QString strExtension = strSrcFilePath.mid(strSrcFilePath.lastIndexOf(".") + 1);
    if ("docx" == strExtension) strExtension = "dox";
    else if ("pptx" == strExtension) strExtension = "ppx";
    else if ("xlsx" == strExtension) strExtension = "xlx";

    int nEnDecDataLen;
    srcFile.open(QFile::ReadOnly);
    // If the file size is less than 10MB.
    if (srcFile.size() >= 10 * 1024 * 1024) {
        // Only encrypt the first 128KB of the file.
        nEnDecDataLen = 32768;
    } else {
        // Only encrypt the entire file, except the last 32byte data.
        nEnDecDataLen = srcFile.size() - nKeyLen;
    }

    uint8_t *arrBufIn, *arrEnDecKey;
    arrBufIn = reinterpret_cast<uint8_t*>(malloc( (nEnDecDataLen + 3) * sizeof(uint8_t)));
    arrEnDecKey = reinterpret_cast<uint8_t*>(malloc( (nKeyLen) * sizeof(uint8_t)));

    srcFile.seek(nEnDecDataLen);
    srcFile.read(reinterpret_cast<char*>(arrEnDecKey), nKeyLen);
    int i;
    for (i = 0; i < nKeyLen; i += 2) {
        uint8_t tmpVal = arrEnDecKey[i];
        arrEnDecKey[i] = arrEnDecKey[i + 1];
        arrEnDecKey[i + 1] = tmpVal;
    }

    for (i = 0; i < nKeyLen/ 2; i ++) {
        uint8_t tmpVal = arrEnDecKey[i];
        arrEnDecKey[i] = arrEnDecKey[32 - 1 - i];
        arrEnDecKey[32 - 1 - i] = tmpVal;
    }

    crypto_hash_sha256(arrEnDecKey, arrEnDecKey, nKeyLen);

    srcFile.seek(0);
    memcpy(arrBufIn, strExtension.toLocal8Bit().data(), 3);
    srcFile.read(reinterpret_cast<char*>(arrBufIn + 3), nEnDecDataLen);

    crypto_stream_xchacha20_xor(arrBufIn, arrBufIn, nEnDecDataLen + 3, BPD_ENDEC_NONCE, arrEnDecKey);

    dstFile.open(QFile::WriteOnly);
    dstFile.write(reinterpret_cast<const char*>(arrBufIn), nEnDecDataLen + 3);

    int nReadLen;
    do {
        nReadLen = srcFile.read(reinterpret_cast<char*>(arrBufIn), 8192);
        dstFile.write(reinterpret_cast<const char*>(arrBufIn), nReadLen);
    } while (nReadLen >= 8192);

    srcFile.close();
    dstFile.close();

    free(arrBufIn);
    free(arrEnDecKey);
}

/*
The extension of strSrcFilePath is "*.bpd".
*/
bool Widget::DecVidFile(QString strSrcFilePath, QString& strDstFilePath, QString strMachineNumber)
{
    QFile srcFile(strSrcFilePath);
    srcFile.open(QFile::ReadOnly);

    unsigned long long plaintext_len = 32;
    unsigned long long ciphertext_len = plaintext_len + crypto_aead_xchacha20poly1305_ietf_ABYTES;

    uint8_t* arrBufIn0;
    arrBufIn0 = static_cast<uint8_t*>(malloc(ciphertext_len * sizeof(uint8_t)));

    srcFile.seek(srcFile.size() - ciphertext_len);
    srcFile.read(reinterpret_cast<char*>(arrBufIn0), ciphertext_len);

    uint8_t* MachineNum = reinterpret_cast<uint8_t*>(
                                    strMachineNumber.toUtf8().data());

    if (crypto_aead_xchacha20poly1305_ietf_decrypt(
                            arrBufIn0, &plaintext_len, nullptr,
                            arrBufIn0, ciphertext_len,
                            ADD_ENDEC_DATA1, ADD_ENDEC_DATA1_LEN,
                            BPD_ENDEC_NONCE, MachineNum
                            ) != 0)
    {
        QMessageBox::information(NULL, "방패전용파일 열기오유", "방패전용파일은 내리적재한 콤퓨터에서만 열수 있습니다.", QMessageBox::Ok);
        return false;
    }

    const int nKeyLen = 32;
    int nEnDecDataLen;

    // If the file size is less than 10MB.
    if (srcFile.size()  >= 10 * 1024 * 1024 + 3 + 48) {
        // Only encrypt the first 128KB of the file.
        nEnDecDataLen = 32768 + 3;
    } else {
        // Only encrypt the entire file, except the last 32byte data.
        nEnDecDataLen = srcFile.size() - (nKeyLen + 48);
    }

    uint8_t *arrBufIn, *arrEnDecKey;
    arrBufIn = static_cast<uint8_t*>( malloc(nEnDecDataLen * sizeof (uint8_t)) );
    arrEnDecKey = static_cast<uint8_t*>( malloc(nKeyLen * sizeof (uint8_t)) );

    srcFile.seek(nEnDecDataLen);
    srcFile.read(reinterpret_cast<char*>(arrEnDecKey), nKeyLen);

    int i;
    for (i = 0; i < nKeyLen; i += 2) {
        uint8_t tmpVal = arrEnDecKey[i];
        arrEnDecKey[i] = arrEnDecKey[i + 1];
        arrEnDecKey[i + 1] = tmpVal;
    }

    for (i = 0; i < nKeyLen/ 2; i ++) {
        uint8_t tmpVal = arrEnDecKey[i];
        arrEnDecKey[i] = arrEnDecKey[nKeyLen - 1 - i];
        arrEnDecKey[nKeyLen - 1 - i] = tmpVal;
    }
    crypto_hash_sha256(arrEnDecKey, arrEnDecKey, 32);

    srcFile.seek(0);
    srcFile.read(reinterpret_cast<char*>(arrBufIn), nEnDecDataLen);

    crypto_stream_xchacha20_xor(arrBufIn, arrBufIn, nEnDecDataLen , BPD_ENDEC_NONCE, arrEnDecKey);

    char strTmpExtension[5] = {0};
    memcpy(strTmpExtension, arrBufIn, 3);
    strTmpExtension[3] = 0;

    QString strDstFileName = QFileInfo{strSrcFilePath}.fileName();
    strDstFileName = strDstFileName.left(strDstFileName.lastIndexOf(".") );

    QString strExtension (strTmpExtension);
    if ("dox" == strExtension) strExtension = "docx";
    else if ("ppx" == strExtension) strExtension = "pptx";
    else if ("xlx" == strExtension) strExtension = "xlsx";

    // ROC 2022.09.24 START
    strDstFilePath = strDstFilePath + "/" + strDstFileName + "." + strExtension ;
    // ROC 2022.09.24 END

    QFile dstFile(strDstFilePath);
    dstFile.open(QFile::WriteOnly);
    dstFile.write(reinterpret_cast<const char*>(arrBufIn + 3), nEnDecDataLen - 3);

    int nReadLen;
    do {
        nReadLen = srcFile.read(reinterpret_cast<char*>(arrBufIn), 8192);
        dstFile.write(reinterpret_cast<const char*>(arrBufIn), nReadLen);
    } while (nReadLen >= 8192);

    srcFile.close();
    dstFile.close();

    dstFile.open(QFile:: Append | QFile::ReadWrite);
    dstFile.resize(dstFile.size() - ciphertext_len);
    dstFile.close();

    free(arrBufIn0);
    free(arrBufIn);
    free(arrEnDecKey);

    return true;
}

// 게시판의 방패전용파일 strFilepath를 암호화하여 보관한다.
//void Widget:: EncVidFileWithMachineNum(QString strFilePath, QString strMachineNumber)
//{
//    QFile srcFile(strFilePath);
//    srcFile.open(QFile::ReadWrite);

//    unsigned long long plaintext_len = 32;
//    unsigned long long ciphertext_len = plaintext_len + crypto_aead_xchacha20poly1305_ietf_ABYTES;

//    uint8_t *arrBufIn;
//    arrBufIn = static_cast<uint8_t*>(malloc(ciphertext_len * sizeof(uint8_t)));

//    srcFile.seek(srcFile.size() - plaintext_len);
//    srcFile.read(reinterpret_cast<char*>(arrBufIn), plaintext_len);

//    uint8_t* MachineNum = reinterpret_cast<uint8_t*>(
//                                    strMachineNumber.toUtf8().data());

//    // 암호화를 진행한다.
//    crypto_aead_xchacha20poly1305_ietf_encrypt(
//                   arrBufIn, &ciphertext_len,
//                   arrBufIn, plaintext_len,
//                   ADD_ENDEC_DATA1, ADD_ENDEC_DATA1_LEN,
//                   nullptr,
//                   BPD_ENDEC_NONCE, MachineNum
//                   );

//    srcFile.seek(srcFile.size());
//    srcFile.write(reinterpret_cast<const char*>(arrBufIn), ciphertext_len);
//    srcFile.close();

//    free(arrBufIn);
//}


QFileEncDecTask::QFileEncDecTask() {

}


QFileEncDecTask::~QFileEncDecTask() {

}

void QFileEncDecTask::SetThreadData(Core* pCore, QString strTSrcPath, QString strTSrcName,
                   const uint8_t* arrTPeerPubKey, bool bTIsEnc, int nTFrndId ) {
    core = pCore;
    strSrcFilePath = strTSrcPath;
    strSrcFileName = strTSrcName;
    bIsEnc = bTIsEnc;
    memcpy(arrPeerPubKey, arrTPeerPubKey, 32);
    nFrndId = nTFrndId;
}


void QFileEncDecTask::run() {

    uint8_t  header[crypto_secretstream_xchacha20poly1305_HEADERBYTES];
    unsigned long long out_len;
    size_t         rlen;
    bool            eof;
    uint8_t  tag;

    uint8_t sharedKey[32], selfSecretKey[32];
    bangpae_self_get_secret_key(core->bangpae.get(), selfSecretKey);


    if (bIsEnc) {
        uint8_t selfLongPubKey[32], arrResPubKey[32];

        bangpae_self_get_public_key(core->bangpae.get(), selfLongPubKey);

        if (crypto_box_beforenm(sharedKey, arrPeerPubKey, selfSecretKey) != 0)
            return;

        // Mix the long term key of source and destination peer.
        MixPubKeyToRelayFileFormat(arrResPubKey, selfLongPubKey, arrPeerPubKey);

        QString strDstFileName = SM_FUNCTXT_REQ_FILERLY
                                     + Byte2QString(arrResPubKey, 32)
                                     // + QByteArray{}.insert(0, reinterpret_cast<char*>(arrResPubKey), 32).toHex().toUpper()
                                     // ROC 2022.11.12 START
                                     // 화일이름은 바이트렬문자렬로 변환.
                                     + Byte2QString (reinterpret_cast<const uint8_t*>(strSrcFileName.toLocal8Bit().constData())
                                     , strSrcFileName.toLocal8bit().length() );
                                     // ROC 2022.11.12 END

        QString strDstFilePath = Nexus::getDesktopGUI()->strWorkDirPath + "/" + strDstFileName;

        uint8_t  buf_in[CHUNK_SIZE];
        uint8_t  buf_out[CHUNK_SIZE + crypto_secretstream_xchacha20poly1305_ABYTES];

        crypto_secretstream_xchacha20poly1305_state st;
        QFile fp_t(strDstFilePath), fp_s(strSrcFilePath);
        fp_t.open(QFile::ReadWrite);
        fp_s.open(QFile::ReadOnly);

        crypto_secretstream_xchacha20poly1305_init_push(&st, header, sharedKey);
        fp_t.write(reinterpret_cast<const char*>(header), sizeof header);
        do {
            rlen = fp_s.read(reinterpret_cast<char*>(buf_in), sizeof buf_in);
            eof = fp_s.atEnd();
            tag = eof ? crypto_secretstream_xchacha20poly1305_TAG_FINAL : 0;
            crypto_secretstream_xchacha20poly1305_push(&st, buf_out, &out_len, buf_in, rlen,
                                                       NULL, 0, tag);
            fp_t.write(reinterpret_cast<char*>(buf_out), static_cast<size_t>(out_len));
        } while (! eof);
        fp_t.close();
        fp_s.close();

        QFileInfo fileInfo(strDstFilePath);
        qint64 nEncSize = fileInfo.size();

        emit fileEncryptionEnd(nFrndId, strDstFileName, strDstFilePath, nEncSize);

    } else {

        int res = crypto_box_beforenm(sharedKey, arrPeerPubKey, selfSecretKey);

        QString strDstFileName = strSrcFileName.mid(SM_FUNCTXT_REQ_FILERLY.length() + 64);
        QString strDstFilePath = strSrcFilePath.left(
                            strSrcFilePath.length() - strSrcFileName.length()) + strDstFileName;
#ifdef Q_OS_LINUX
        strDstFilePath = "/home/rg/Documents/" + strDstFileName;
#endif

        uint8_t  buf_in[CHUNK_SIZE + crypto_secretstream_xchacha20poly1305_ABYTES];
        uint8_t  buf_out[CHUNK_SIZE];

        crypto_secretstream_xchacha20poly1305_state st;
        QFile fp_t(strDstFilePath), fp_s(strSrcFilePath);
        int            ret = -1;

         //If the decoded file already exists in the same directory, then append the suffix at the tail of filename.
         //ex: 1.jpg => 1 (1).jpg, 1(2).jpg
        if (fp_t.exists()) {
            //@brief strDstDir : "D:/BangPaeDownLoad/"
            QString strDstDir = strDstFilePath.left(fp_t.fileName().lastIndexOf("/") + 1);
            // @brief strDstFileName : "1.jpg"
            QString strDstFileName = strDstFilePath.mid(fp_t.fileName().lastIndexOf("/") + 1);
            // @brief strDstExtention : ".jpg"
            QString strDstExtention = "";

            if (strDstFileName.contains("."))
                strDstExtention = strDstFileName.mid(strDstFileName.lastIndexOf("."));
            int i = 1;

            while (true) {
                QString strNewDstFilePath = strDstDir + strDstFileName
                                            + QString("").sprintf(" (%d)", i) + strDstExtention;
                i ++;
                if (QFileInfo(strNewDstFilePath).exists()) continue;
                else {
                    strDstFilePath = strNewDstFilePath;
                    break;
                }
            }
            fp_t.setFileName(strDstFilePath);
        }

        fp_t.open(QFile::ReadWrite);
        fp_s.open(QFile::ReadOnly);
        fp_s.read(reinterpret_cast<char*>(header), sizeof header);
        // fread(header, 1, sizeof header, fp_s);

        if (crypto_secretstream_xchacha20poly1305_init_pull(&st, header, sharedKey) != 0) {
            goto ret; /* incomplete header */
        }
        do {
            rlen = fp_s.read(reinterpret_cast<char*>(buf_in), sizeof buf_in);
            eof = fp_s.atEnd();
            if (crypto_secretstream_xchacha20poly1305_pull(&st, buf_out, &out_len, &tag,
                                                           buf_in, rlen, NULL, 0) != 0) {
                goto ret; /* corrupted chunk */
            }
            if (tag == crypto_secretstream_xchacha20poly1305_TAG_FINAL && ! eof) {
                goto ret; /* premature end (end of file reached before the end of the stream) */
            }
            fp_t.write(reinterpret_cast<const char*>(buf_out), static_cast<size_t>(out_len));
            // fwrite(buf_out, 1, static_cast<size_t>(out_len), fp_t);
        } while (! eof);

        ret = 0;
    ret:
        fp_t.close();
        fp_s.close();
        res = ret;

        QFile::remove(strSrcFilePath);

        if (!strDstFileName.startsWith("###"))
            emit fileDecryptionEnd(strDstFileName, arrPeerPubKey , FriendList::id2Key(nFrndId).getData());
    }

}

void Widget::onFileEncryptionEnd(int nFrndId, QString& strNewEncFileName, QString& strNewEncFilePath, int nEncSize) {
    if (!pFileRelayWaitingMsg->isHidden())
        pFileRelayWaitingMsg->hide();
    core->getCoreFile()->sendFile(nFrndId, strNewEncFileName,
                                  strNewEncFilePath, nEncSize);
}


void Widget::showRelayedFileDecryptionEndMsg(const QString& strSrcName, const uint8_t* arrSrcPubKey, const uint8_t* arrRelayPubKey) {

    /* Tell the source friend that file has successfully been sent to destination.
     * Pattern = Prefix + SourceFriendPublicKey + MyPubKey + FileName*/
    friendMessageDispatchers[BangPaePk(arrRelayPubKey)]->sendMessage(false, SM_FILE_RELAY_SUCCESS_TRANSFER
                + Byte2QString(arrSrcPubKey)
                + Byte2QString(core->getSelfPublicKey().getData())
               + strSrcName);

    QMessageBox::information(NULL, "파일중계전송", FriendList::findFriend(BangPaePk {arrSrcPubKey})->getDisplayedName() + "동지가 "
                             + FriendList::findFriend(BangPaePk {arrRelayPubKey})->getDisplayedName()
                             + "동지를 통하여 파일(" + strSrcName + ")을 전송하였습니다.", QMessageBox::Ok);

    filterAllAction->setChecked(true);
    changeDisplayMode();
}

bool bangpaeActivateEventHandler(const QByteArray&) {
    Widget* widget = Nexus::getDesktopGUI();
    if (!widget) {
        return true;
    }

    //q() << "Handling [activate] event from other instance";
    widget->forceShow();

    return true;
}

namespace {

/**
 * @brief Dangerous way to find out if a path is writable.
 * @param filepath Path to file which should be deleted.
 * @return True, if file writeable, false otherwise.
 */
bool tryRemoveFile(const QString& filepath)
{
    QFile tmp(filepath);
    bool writable = tmp.open(QIODevice::WriteOnly);
    tmp.remove();
    return writable;
}
} // namespace

void Widget::acceptFileTransfer(const BangPaeFile& file, const QString& path)
{
    QString filepath;
    int number = 0;

    QString suffix = QFileInfo(file.fileName).completeSuffix();
    QString base = QFileInfo(file.fileName).baseName();

    do {
        filepath = QString("%1/%2%3.%4")
                       .arg(path, base,
                            number > 0 ? QString(" (%1)").arg(QString::number(number)) : QString(),
                            suffix);
        ++number;
    } while (QFileInfo(filepath).exists());

    // Do not automatically accept the file-transfer if the path is not writable.
    // The user can still accept it manually.
    if (tryRemoveFile(filepath)) {
        CoreFile* coreFile = core->getCoreFile();
        coreFile->acceptFileRecvRequest(file.friendId, file.fileNum, filepath);
    }
    else {
        qWarning() << "Cannot write to " << filepath;
    }
}

Widget* Widget::instance{nullptr};

Widget::Widget(Profile &_profile, IAudioControl& audio, QWidget* parent)
    : QMainWindow(parent)
    , activeChatroomWidget{nullptr}
    , profile{_profile}
    , trayMenu{nullptr}
    , ui(new Ui::MainWindow)
    , eventFlag(false)
    , eventIcon(false)
    , audio(audio)
    , settings(Settings::getInstance())
{
    isLogout = false;       
    installEventFilter(this);
    QString locale = settings.getTranslation();
    Translator::translate(locale);
    updateDialog = NULL;
    isMessageBoxOpen = false;
    //strDecDirPath = generateRandTmpDirPath();
}
void Widget::init()
{
    ui->setupUi(this);

    // JYJ START
    // 새판본인경우
    if(Settings::getInstance().last_version != PROGRAM_VERSION)
    {
        // 갱신내용현시
        QMessageBox box;
        box.setStyleSheet("QLabel{"
                              "min-width:800px;"
                              "min-height:100px; "
                              "font-size:14px;"
                              "}");
        box.setText(UPDATE_NOTICE);
        box.setWindowTitle(UPDATE_NOTICE_TITLE);
        box.exec();
    }
    // 판본정보갱신
    Settings::getInstance().last_version = PROGRAM_VERSION;

    if (Settings::getInstance().lastRecordInit.daysTo(QDate::currentDate()) >= 10)
    {
        QMessageBox::critical(nullptr, "경고", "대화기록이 많아지면 통신에 영향을 줄수 있습니다.\n"
                                             "대화기록을 초기화한때로부터 10일이상이 지났습니다. 기록을 초기화하여 주십시오.\n"
                                             "1--설정창에서 대화기록보관단추를 눌러서 대화기록을 보관하십시오.\n"
                                             "2--대화기록초기화단추를 눌러서 초기화하고 방패를 재기동하십시오.");
    }
     
    

    // 1초간격으로 시간현시
    displayTimeTimer = new QTimer(this);
    connect(displayTimeTimer, SIGNAL(timeout()), this, SLOT(onDisplayTime()));
    displayTimeTimer->start(DISPLAY_TIME_INTERVAL);

    // 주소록배경색변경
    if (Settings::getInstance().getAutoChangeColor()
      && Settings::getInstance().getThemeColor() != DARK_THEME_IDX)
    {
        contactTimer = new QTimer(this);
        connect(contactTimer, SIGNAL(timeout()), this, SLOT(onContactChangeColor()));
        contactTimer->start(TIME_CONTACTLIST_COLOR_CHANGE * 1000);
    }
    // JYJ END


    // ROC START
    // 림시파일등록부정돈
    clearSpecFileAndDir(settings.getGlobalAutoAcceptDir(), ".bpd",
                       FILENAME_ENDSWITH_MATCH_MODE, false, BPD_DEADLINE_MILLISECONDS);

    clearSpecFileAndDir(settings.getGlobalAutoAcceptDir(), FILENAME_PART_KCTV, FILENAME_CONTAIN_MATCH_MODE);//"D:/BangPaeDownload"
    clearSpecFileAndDir("E:/BangPaeDownload", FILENAME_PART_KCTV, FILENAME_CONTAIN_MATCH_MODE);//"E:/BangPaeDownload"
    clearSpecFileAndDir(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
                FILENAME_PART_KCTV, FILENAME_CONTAIN_MATCH_MODE);//"MyDocument"

    clearSpecFileAndDir(settings.getGlobalAutoAcceptDir(), PROGRAM_VERSION, FILENAME_CONTAIN_MATCH_MODE);//"D:/BangPaeDownload"
    clearSpecFileAndDir("E:/BangPaeDownload", PROGRAM_VERSION, FILENAME_CONTAIN_MATCH_MODE);//"E:/BangPaeDownload"
    clearSpecFileAndDir(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
                PROGRAM_VERSION, FILENAME_CONTAIN_MATCH_MODE);//"MyDocument"

    clearSpecFileAndDir(QDir::cleanPath(QStandardPaths::writableLocation(QStandardPaths::HomeLocation)
                               + QDir::separator() + WORK_DIR), WORK_DIR_PREFIX,
                       FILENAME_CONTAIN_MATCH_MODE, true);//"AppData/Local/Temp/"

    strWorkDirPath = generateRandTmpDirPath();

    // 문자중계전송상태변수에 대한 초기화조작진행
    memset(arrRelayTextFrndId, 0xff, NUM_TOTAL_PEOPLES * sizeof(int));

    // 1시간후에 프로그람끄기계수기동작
    pTimerAppQuit = new QTimer(nullptr);
    connect(pTimerAppQuit, SIGNAL(timeout()), this, SLOT(onAppWorkingTimedOut()));
    pTimerAppQuit->start(TIME_AUTO_LOGIN * 1000);

    // 이모쉰복호화
    if ( !Settings::getInstance().getSmileyPack().isEmpty() )
            Settings::getInstance().setSmileyPack(Settings::getInstance().getSmileyPack() );

    char data[EMO_ENCDATA_PREFIX_LEN + 1];
    QString strSrcDirPath = Settings::getInstance().getExePath() + EMO_ENCDATA_DIR;
    QString strSrcFilePath, strDstFilePath, strDstFileName;
    QDir dirEmojione(strSrcDirPath);
    QFile dstFile;

    for (QString fileName : dirEmojione.entryList(QDir::Files) ) {
        if (fileName.endsWith("eef")) {

            strDstFileName = fileName;
            strDstFileName.replace("eef", "png");
            strSrcFilePath = strSrcDirPath + "/" + fileName;
            strDstFilePath = strWorkDirPath + "/" + strDstFileName;
            QFile{strSrcFilePath}.copy(strDstFilePath);

            dstFile.setFileName(strDstFilePath);
            dstFile.open(QFile::ReadWrite);
            dstFile.read(data, EMO_ENCDATA_PREFIX_LEN);
            for (int i = 0; i < EMO_ENCDATA_PREFIX_LEN; i ++) {
                data[i] ^=  EMO_ENCDATA_CONSTANT;
            }
            dstFile.seek(0);
            dstFile.write(reinterpret_cast<const char*>(&data), EMO_ENCDATA_PREFIX_LEN);
            dstFile.close();
        }
    }

    // 선전화복호화
    QDir dirPoster(Settings::getInstance().getExePath() + "Poster/");
    if(dirPoster.isEmpty())
        nPosterCnt = 1;
    else
        nPosterCnt =  dirPoster.count()-2;

    // 선전화현시에 리용된 Timer설정.
    QTimer::singleShot(2000, this, SLOT(onDisplayPoster()) );
    pTimerPoster = new QTimer(this);
    connect(pTimerPoster, SIGNAL(timeout()), this, SLOT(onDisplayPoster()));
    pTimerPoster->start(1000 * TIME_POSTER_SHOW);

    // 파일중계전송알림대화칸창조
    pFileRelayWaitingMsg = new QMessageBox(this);
    pFileRelayWaitingMsg->setWindowTitle("알림");
    pFileRelayWaitingMsg->setText("파일중계전송대기중...");
    pFileRelayWaitingMsg->hide();

    // 보고기록대화칸창조
    int permission = Nexus::getProfile()->bpxPerm;
    if (permission != LEADER_PERMISSION
            || Nexus::getProfile()->bpxPermission.contains(STR_WORKER_BOSS)
            || Nexus::getProfile()->bpxPermission.contains(STR_YOUTH_BOSS) ) {
        ui->reportButton->setVisible(false);
    } else {
        reportForm = new ReportForm;
    }

    QThreadPool::globalInstance()->setMaxThreadCount(NUM_MAX_RLYFILE_ENCTHREAD);

    pTVProcess=new QProcess(this);

    // ROC END


    QIcon themeIcon = QIcon::fromTheme("qbangpae");
    if (!themeIcon.isNull()) {
        setWindowIcon(themeIcon);
    }

    timer = new QTimer();
    timer->start(1000);

    icon_size = 15;

    actionShow = new QAction(this);
    connect(actionShow, &QAction::triggered, this, &Widget::forceShow);

    // Preparing icons and set their size
    statusOnline = new QAction(this);
    statusOnline->setIcon(prepareIcon(Status::getIconPath(Status::Status::Online), icon_size, icon_size));
    connect(statusOnline, &QAction::triggered, this, &Widget::setStatusOnline);

    statusAway = new QAction(this);
    statusAway->setIcon(prepareIcon(Status::getIconPath(Status::Status::Away), icon_size, icon_size));
    connect(statusAway, &QAction::triggered, this, &Widget::setStatusAway);

    statusBusy = new QAction(this);
    statusBusy->setIcon(prepareIcon(Status::getIconPath(Status::Status::Busy), icon_size, icon_size));
    connect(statusBusy, &QAction::triggered, this, &Widget::setStatusBusy);

    actionLogout = new QAction(this);
    actionLogout->setIcon(prepareIcon(":/img/others/logout-icon.svg", icon_size, icon_size));

    actionQuit = new QAction(this);
    actionQuit->setIcon(prepareIcon(Style::getImagePath("rejectCall/rejectCall.svg"), icon_size, icon_size));
    connect(actionQuit, &QAction::triggered, this, &Widget::onQuitApp);

    layout()->setContentsMargins(0, 0, 0, 0);

    profilePicture = new MaskablePixmapWidget(this, QSize(40, 40), ":/img/avatar_mask.svg");
    profilePicture->setPixmap(QPixmap(":/img/contact_dark.svg"));
    profilePicture->setClickable(true);
    profilePicture->setObjectName("selfAvatar");
    profilePicture->setIsSelf(true);

    ui->myProfile->insertWidget(0, profilePicture);
    ui->myProfile->insertSpacing(1, 7);

    filterMenu = new QMenu(this);
    filterGroup = new QActionGroup(this);
    filterDisplayGroup = new QActionGroup(this);

    filterDisplayName = new QAction(this);
    filterDisplayName->setCheckable(true);
    filterDisplayName->setChecked(true);
    filterDisplayGroup->addAction(filterDisplayName);
    filterDisplayActivity = new QAction(this);
    filterDisplayActivity->setCheckable(true);
    filterDisplayGroup->addAction(filterDisplayActivity);
    settings.getFriendSortingMode() == FriendListWidget::SortingMode::Name
        ? filterDisplayName->setChecked(true)
        : filterDisplayActivity->setChecked(true);

    filterAllAction = new QAction(this);
    filterAllAction->setCheckable(true);
    filterAllAction->setChecked(true);
    filterGroup->addAction(filterAllAction);
    filterMenu->addAction(filterAllAction);
    filterOnlineAction = new QAction(this);
    filterOnlineAction->setCheckable(true);
    filterGroup->addAction(filterOnlineAction);
    filterMenu->addAction(filterOnlineAction);
    filterOfflineAction = new QAction(this);
    filterOfflineAction->setCheckable(true);
    filterGroup->addAction(filterOfflineAction);
    filterMenu->addAction(filterOfflineAction);
    filterFriendsAction = new QAction(this);
    filterFriendsAction->setCheckable(true);
    filterGroup->addAction(filterFriendsAction);
    filterMenu->addAction(filterFriendsAction);
    filterGroupsAction = new QAction(this);
    filterGroupsAction->setCheckable(true);
    filterGroup->addAction(filterGroupsAction);
    filterMenu->addAction(filterGroupsAction);

    ui->searchContactFilterBox->setMenu(filterMenu);
    ui->searchContactFilterBox->setStyleSheet("color:black;");
    ui->searchContactText->setStyleSheet("color:black;");

    contactListWidget = new FriendListWidget(this, settings.getGroupchatPosition());
    connect(contactListWidget, &FriendListWidget::searchCircle, this, &Widget::searchCircle);
    connect(contactListWidget, &FriendListWidget::connectCircleWidget, this, &Widget::connectCircleWidget);
    ui->friendList->setWidget(contactListWidget);
    ui->friendList->setLayoutDirection(Qt::LeftToRight);
    ui->friendList->setContextMenuPolicy(Qt::CustomContextMenu);

    ui->statusLabel->setEditable(true);

    QMenu* statusButtonMenu = new QMenu(ui->statusButton);
    statusButtonMenu->addAction(statusOnline);
    statusButtonMenu->addAction(statusBusy);
    ui->statusButton->setMenu(statusButtonMenu);

    // disable proportional scaling
    ui->mainSplitter->setStretchFactor(0, 0);
    ui->mainSplitter->setStretchFactor(1, 1);

    onStatusSet(Status::Status::Offline);

    // Disable some widgets until we're connected to the DHT
    ui->statusButton->setEnabled(false);

    Style::setThemeColor(settings.getThemeColor());

    groupInviteForm = new GroupInviteForm;

    core = &profile.getCore();

    settingsWidget = new SettingsWidget(updateCheck.get(), audio, core, this);

    CoreFile* coreFile = core->getCoreFile();
    Profile* profile = Nexus::getProfile();
    profileInfo = new ProfileInfo(core, profile);
    profileForm = new ProfileForm(profileInfo, core);
    profileForm->m_widget = this;
//    ui->setFriendButton->setVisible(false);
    ui->transferButton->setVisible(false);
    ui->groupButton->setVisible(false);


    // JYJ START
    // 첫기동암호인경우 암호변경
    if(profile->is_simple_pass)
    {
        QMessageBox::critical(nullptr, "주의", "반드시 영문환경으로 놓고 기정암호를 변경하십시오.");
        const QString title = tr("안전성과 관련하여 기정암호를 변경하십시오.");
        SetPasswordDialog* dialog = new SetPasswordDialog(title, QString{}, nullptr);
        while (dialog->exec() == QDialog::Rejected) {
            QMessageBox::warning(dialog, "오유", "암호를 꼭 변경하여야 합니다.");
        }

        QString newPass = dialog->getPassword();
        if (!profileInfo->setPassword(newPass)) {
            QMessageBox::critical(dialog, "오유", "암호변경이 실패하였습니다.");
        }
    }

    // 개발자인경우 갱신창문창조 및 현시
    if (core->getUsername().contains(UPDATE_PEER_NAME))
    {
        connect(ui->addButton, &QPushButton::clicked, this, &Widget::onUpdateClicked);
        Nexus::getProfile()->bpxPerm = DEVELOPER_PERMISSION; // Our Teacher
        updateForm = new UpdateForm();
        connect(updateForm, SIGNAL(onSendNewVerToAllUsers(QVector<uint32_t>)), this,
                SLOT(onSendNewVerToAllUsers(QVector<uint32_t>)));
        connect(updateForm, SIGNAL(onGetAllUserAppVersions(QVector<uint32_t>)), this,
                SLOT(onGetAllUserAppVersions(QVector<uint32_t>)));
    }
    else {
        ui->addButton->setVisible(false);
    }
    // JYJ END


    ui->timelcdNumber->setVisible(false);

    // connect logout tray menu action
    connect(actionLogout, &QAction::triggered, profileForm, &ProfileForm::onLogoutClicked);
    connect(coreFile, &CoreFile::fileReceiveRequested, this, &Widget::dispatchFile);
    connect(coreFile, &CoreFile::fileReceiveRequested, this, &Widget::onFileReceiveRequested);
    connect(coreFile, &CoreFile::fileSendFailed, this, &Widget::dispatchFileSendFailed);
    connect(coreFile, &CoreFile::fileSendFailedForRelay, this, &Widget::dispatchFileSendFailedForRelay);
    connect(coreFile, &CoreFile::fileSendStarted, this, &Widget::dispatchFile);
    connect(coreFile, &CoreFile::fileTransferAccepted, this, &Widget::dispatchFile);
    connect(coreFile, &CoreFile::fileTransferBrokenUnbroken, this, &Widget::dispatchFileWithBool);
    connect(coreFile, &CoreFile::fileTransferCancelled, this, &Widget::dispatchFile);
    connect(coreFile, &CoreFile::fileTransferFinished, this, &Widget::dispatchFile);
    connect(coreFile, &CoreFile::fileTransferInfo, this, &Widget::dispatchFile);
    connect(coreFile, &CoreFile::fileTransferPaused, this, &Widget::dispatchFile);
    connect(coreFile, &CoreFile::fileTransferRemotePausedUnpaused, this, &Widget::dispatchFileWithBool);
    connect(filterDisplayGroup, &QActionGroup::triggered, this, &Widget::changeDisplayMode);
    connect(filterGroup, &QActionGroup::triggered, this, &Widget::searchContacts);
    connect(groupInviteForm, &GroupInviteForm::groupCreate, core, &Core::createGroup);
    connect(profile, &Profile::selfAvatarChanged, profileForm, &ProfileForm::onSelfAvatarLoaded);
    connect(profilePicture, &MaskablePixmapWidget::clicked, this, &Widget::showProfile);
    connect(timer, &QTimer::timeout, this, &Widget::onEventIconTick);
    connect(timer, &QTimer::timeout, this, &Widget::onTryCreateTrayIcon);
    connect(timer, &QTimer::timeout, this, &Widget::onUserAwayCheck);
    connect(ui->TVBPButton, &QPushButton::clicked, this, &Widget::onViewBPDFile);
    connect(ui->TVButton, &QPushButton::clicked, this, &Widget::onViewKCTV);
    connect(ui->friendList, &QWidget::customContextMenuRequested, this, &Widget::friendListContextMenu);
    connect(ui->mainSplitter, &QSplitter::splitterMoved, this, &Widget::onSplitterMoved);
    connect(ui->nameLabel, &CroppingLabel::clicked, this, &Widget::showProfile);
    connect(ui->reportButton, &QPushButton::clicked, this, &Widget::onReportClicked);
    connect(ui->searchContactText, &QLineEdit::textChanged, this, &Widget::searchContacts);
    connect(ui->settingsButton, &QPushButton::clicked, this, &Widget::onShowSettings);
    connect(ui->statusLabel, &CroppingLabel::editFinished, this, &Widget::onStatusMessageChanged);

    // keyboard shortcuts
    auto* const quitShortcut = new QShortcut(Qt::CTRL + Qt::Key_Q, this);
    connect(quitShortcut, &QShortcut::activated, this, &Widget::onQuitApp);
    new QShortcut(Qt::CTRL + Qt::SHIFT + Qt::Key_Tab, this, SLOT(previousContact()));
    new QShortcut(Qt::CTRL + Qt::Key_Tab, this, SLOT(nextContact()));
    new QShortcut(Qt::CTRL + Qt::Key_PageUp, this, SLOT(previousContact()));
    new QShortcut(Qt::CTRL + Qt::Key_PageDown, this, SLOT(nextContact()));
    new QShortcut(Qt::Key_F11, this, SLOT(toggleFullscreen()));


    contentLayout = nullptr;
    onSeparateWindowChanged(settings.getSeparateWindow(), false);

    ui->TVBPButton->setCheckable(true);
    ui->TVBPButton->setToolTip("게시판자료열람");
    ui->TVButton->setCheckable(true);
    ui->TVButton->setToolTip("조선중앙텔레비죤");
    ui->groupButton->setCheckable(true);
    ui->reportButton->setCheckable(true);
    ui->reportButton->setToolTip("보고정형");
    ui->settingsButton->setCheckable(true);
    ui->addButton->setCheckable(true);
    //ui->setFriendButton->setCheckable(true);
    //ui->setFriendButton->setToolTip("주소록변경");

    if (contentLayout) {
        showProfile();
    }

    // restore window state
    restoreState(settings.getWindowState());
    restoreGeometry(settings.getWindowGeometry());
    SplitterRestorer restorer(ui->mainSplitter);
    restorer.restore(settings.getSplitterState(), size());

    groupInvitesButton = nullptr;
    unreadGroupInvites = 0;

    connect(groupInviteForm, &GroupInviteForm::groupInvitesSeen, this, &Widget::groupInvitesClear);
    connect(groupInviteForm, &GroupInviteForm::groupInviteAccepted, this, &Widget::onGroupInviteAccepted);

    // settings
    connect(&settings, &Settings::showSystemTrayChanged, this, &Widget::onSetShowSystemTray);
    connect(&settings, &Settings::separateWindowChanged, this, &Widget::onSeparateWindowClicked);
    connect(&settings, &Settings::compactLayoutChanged, contactListWidget, &FriendListWidget::onCompactChanged);
    connect(&settings, &Settings::groupchatPositionChanged, contactListWidget, &FriendListWidget::onGroupchatPositionChanged);

    reloadTheme();
    updateIcons();
    retranslateUi();
    Translator::registerHandler(std::bind(&Widget::retranslateUi, this), this);

    if (!settings.getShowSystemTray()) {
        show();
    }

    filterOnlineAction->setChecked(true);
    FilterCriteria filter = getFilterCriteria();

    contactListWidget->searchChatrooms("", filterOnline(filter), filterOffline(filter), filterGroups(filter), false);
    updateFilterText();
    contactListWidget->reDraw();

    Settings::getInstance().setTimestampFormat("MM-dd HH:mm:ss");
    srand(static_cast<unsigned int>(time(NULL)));

}


// ROC 2022.07.11, 10:00 START
void Widget::onAppWorkingTimedOut() {
    bool bIsTvOn = false;
#ifdef Q_OS_WIN
    HWND hwnd;
    hwnd = nullptr;
    hwnd = FindWindowA(nullptr, "KCTV-");

    if (nullptr != hwnd) {
        bIsTvOn = true;
    }
#endif

    // Check if exists transmitting file.
    bool bIsTransFile = false;
    for (BangPaeFile tmpFile : core->getCoreFile()->fileMap.values()) {
        if (tmpFile.status == BangPaeFile::INITIALIZING
            || tmpFile.status == BangPaeFile::PAUSED
            || tmpFile.status == BangPaeFile::TRANSMITTING ) {
            bIsTransFile = true;
            break;
        }
    }

    if ( bIsTvOn || bIsTransFile || core->getAv()->isCallBusy() ) {
        return;
    } else {

        bAutoLogin = true;

        QMessageBox* pMsgBx = new QMessageBox();
        QTimer::singleShot(TIME_AUTO_LOGIN_MSG_SHOW * 1000, this, [this, &pMsgBx]() {
           QWidgetList topWidgets = QApplication::topLevelWidgets();
           foreach (QWidget *w, topWidgets) {
                 if (QMessageBox *mb = qobject_cast<QMessageBox *>(w)) {
                    mb->close();
                    delete pMsgBx;
                    pMsgBx = nullptr;
                    profileInfo->logout();
                    return;
                 }
           }
        });

        pMsgBx->information(nullptr, "알림", " 프로그람기동시간이 1시간이상되였습니다. 프로그람보안과 관련하여 다시 기동하겠습니다.");
        if (nullptr != pMsgBx) {
           profileInfo->logout();
           return;
        }
    }

}
// ROC 2022.07.11, 10:00 END

bool Widget::eventFilter(QObject* obj, QEvent* event)
{
    QWindowStateChangeEvent* ce = nullptr;
    Qt::WindowStates state = windowState();

    switch (event->type()) {
    case QEvent::Close:
        // It's needed if user enable `Close to tray`
        wasMaximized = state.testFlag(Qt::WindowMaximized);
        break;

    case QEvent::WindowStateChange:
        ce = static_cast<QWindowStateChangeEvent*>(event);
        if (state.testFlag(Qt::WindowMinimized) && obj) {
            wasMaximized = ce->oldState().testFlag(Qt::WindowMaximized);
        }

        break;
    default:
        break;
    }

    if(event->type() == QEvent::WindowActivate)
        this->setStatusOnline();
    return false;
}

void Widget::updateIcons()
{
    if (!icon) {
        return;
    }

    const QString assetSuffix = Status::getAssetSuffix(static_cast<Status::Status>(
                                    ui->statusButton->property("status").toInt()))
                                + (eventIcon ? "_event" : "");

    // Some builds of Qt appear to have a bug in icon loading:
    // QIcon::hasThemeIcon is sometimes unaware that the icon returned
    // from QIcon::fromTheme was a fallback icon, causing hasThemeIcon to
    // incorrectly return true.
    //
    // In qBangPae this leads to the tray and window icons using the static qBangPae logo
    // icon instead of an icon based on the current presence status.
    //
    // This workaround checks for an icon that definitely does not exist to
    // determine if hasThemeIcon can be trusted.
    //
    // On systems with the Qt bug, this workaround will always use our included
    // icons but user themes will be unable to override them.
    static bool checkedHasThemeIcon = false;
    static bool hasThemeIconBug = false;

    if (!checkedHasThemeIcon) {
        hasThemeIconBug = QIcon::hasThemeIcon("qbangpae-asjkdfhawjkeghdfjgh");
        checkedHasThemeIcon = true;

    }

    QIcon ico;
    if (!hasThemeIconBug && QIcon::hasThemeIcon("qbangpae-" + assetSuffix)) {
        ico = QIcon::fromTheme("qbangpae-" + assetSuffix);
    } else {
        QString color = settings.getLightTrayIcon() ? "light" : "dark";
        QString path = ":/img/taskbar/" + color + "/taskbar_" + assetSuffix + ".svg";
        QSvgRenderer renderer(path);

        // Prepare a QImage with desired characteritisc
        QImage image = QImage(250, 250, QImage::Format_ARGB32);
        image.fill(Qt::transparent);
        QPainter painter(&image);
        renderer.render(&painter);
        ico = QIcon(QPixmap::fromImage(image));
    }

    setWindowIcon(ico);
    if (icon) {
        icon->setIcon(ico);
    }
}

Widget::~Widget()
{

    clearSpecFileAndDir(settings.getGlobalAutoAcceptDir(), ".bpd",
                       FILENAME_ENDSWITH_MATCH_MODE, false, BPD_DEADLINE_MILLISECONDS);

    if ( QDir{strWorkDirPath}.exists() ) QDir{strWorkDirPath}.removeRecursively();

#ifdef Q_OS_WIN
    HWND hwnd;
    hwnd = nullptr;
    hwnd = FindWindowA(nullptr, "KCTV");

    if (nullptr != hwnd) {


        QString cmd = "wmic process where \"name='KCTV.exe'\" get ExecutablePath";
        QProcess p;
        p.start(cmd);
        p.waitForFinished();
        QString result = QString::fromLocal8Bit(p.readAllStandardOutput());
        QStringList list = cmd.split(" ");
        result = result.remove(list.last(), Qt::CaseInsensitive).replace("\r", "").replace("\n", "").simplified();
        QString strKCTVPath = result.left( result.lastIndexOf("\\", result.indexOf(".exe")) );

        cmd = "taskkill /IM \"KCTV.exe\" /F";
        p.start(cmd);
        p.waitForFinished();
        Sleep(1000);
        if (!strKCTVPath.isEmpty())
        {
            QMessageBox::information(nullptr, "KCTV deleting", "Path:" + strKCTVPath);
            QDir{strKCTVPath}.removeRecursively();
        }
    }
#endif

    if(pTVProcess != NULL){
        pTVProcess->close();
        delete pTVProcess;
        pTVProcess = NULL;
    }

    QWidgetList windowList = QApplication::topLevelWidgets();

    for (QWidget* window : windowList) {
        if (window != this) {
            window->close();
        }
    }

    Translator::unregister(this);
    if (icon) {
        icon->hide();
    }

    for (Group* g : GroupList::getAllGroups()) {
        removeGroup(g, true);
    }

    for (Friend* f : FriendList::getAllFriends()) {
        removeFriend(f, true);
    }

    for (auto form : chatForms) {
        delete form;
    }

    delete profileForm;
    delete profileInfo;
    if (core->getUsername().contains(UPDATE_PEER_NAME))
        delete updateForm;
    delete groupInviteForm;
    delete timer;


    delete pTimerAppQuit;
    delete pTimerPoster;
    if(contactTimer != nullptr)
        delete contactTimer;
    delete displayTimeTimer;

    delete contentLayout;
    delete settingsWidget;

    if (Nexus::getProfile()->bpxPerm == LEADER_PERMISSION && reportForm != nullptr) {
        delete reportForm;
    }

    FriendList::clear();
    GroupList::clear();
    delete trayMenu;
    delete ui;
    instance = nullptr;
}

/**
 * @brief Switches to the About settings page.
 */
//void Widget::showUpdateDownloadProgress()
//{
//    onShowSettings();
//    settingsWidget->showAbout();
//}

void Widget::clearSpecFileAndDir(const QString strPath, const QString strPattern, const int nMatchMode, bool bIsDir,
                            const int nExpireTime) {
    QDir dir(strPath);
    if(!dir.exists()) {
        return;
    }
    dir.setFilter(QDir::Dirs | QDir::Files);
    dir.setSorting(QDir::DirsFirst);
    QFileInfoList list = dir.entryInfoList();
    if (list.isEmpty()) return;

    int i = 0;

    do {
        QFileInfo fileInfo = list.at(i);
        QString filename = fileInfo.fileName();
        if(filename == "." || filename == "..") {
            ++i;
            continue;
        }

        if(fileInfo.isDir()) {
            if (bIsDir) {
                if (fileInfo.filePath().contains(strPattern) )
                    QDir{fileInfo.filePath()}.removeRecursively();
            }
            else {
                clearSpecFileAndDir(fileInfo.filePath(), strPattern, nMatchMode, bIsDir, nExpireTime);
            }
        }
        else {
            if (!bIsDir) {
                if (-1 == nExpireTime ||
                        fileInfo.birthTime().secsTo(QDateTime::currentDateTime()) >= nExpireTime) {
                    if ( (FILENAME_CONTAIN_MATCH_MODE == nMatchMode && filename.contains(strPattern) )
                        || (FILENAME_ENDSWITH_MATCH_MODE == nMatchMode && filename.endsWith(strPattern))
                        || (FILENAME_STARTSWITH_MATCH_MODE == nMatchMode && filename.startsWith(strPattern)) ) {
                        QFile::remove(fileInfo.filePath());
                    }
                }
            }
        }

        ++i;
    }while(i < list.size());
}

void Widget::moveEvent(QMoveEvent* event)
{
    if (event->type() == QEvent::Move) {
        saveWindowGeometry();
        saveSplitterGeometry();
    }

    QWidget::moveEvent(event);
}

void Widget::closeEvent(QCloseEvent* event)
{
    if (settings.getShowSystemTray() && settings.getCloseToTray()) {
        QWidget::closeEvent(event);
    } else {
        if (autoAwayActive) {
            emit statusSet(Status::Status::Online);
            autoAwayActive = false;
        }
        saveWindowGeometry();
        saveSplitterGeometry();
        QWidget::closeEvent(event);
        qApp->quit();
    }
}

void Widget::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::WindowStateChange) {
        if (isMinimized() && settings.getShowSystemTray() && settings.getMinimizeToTray()) {
            this->hide();
        }
    }
}

void Widget::resizeEvent(QResizeEvent* event)
{
    saveWindowGeometry();

    QMainWindow::resizeEvent(event);
}

void Widget::onSelfAvatarLoaded(const QPixmap& pic)
{
    profilePicture->setPixmap(pic);
}

void Widget::onCoreChanged(Core& core)
{
    connect(&core, &Core::connected, this, &Widget::onConnected);
    connect(&core, &Core::disconnected, this, &Widget::onDisconnected);
    connect(&core, &Core::statusSet, this, &Widget::onStatusSet);
    connect(&core, &Core::usernameSet, this, &Widget::setUsername);
    connect(&core, &Core::statusMessageSet, this, &Widget::setStatusMessage);
    connect(&core, &Core::friendAdded, this, &Widget::addFriend);
    connect(&core, &Core::failedToAddFriend, this, &Widget::addFriendFailed);
    connect(&core, &Core::friendUsernameChanged, this, &Widget::onFriendUsernameChanged);
    connect(&core, &Core::friendStatusChanged, this, &Widget::onFriendStatusChanged);
    connect(&core, &Core::friendStatusMessageChanged, this, &Widget::onFriendStatusMessageChanged);
    //connect(&core, &Core::friendRequestReceived, this, &Widget::onFriendRequestReceived);
    connect(&core, &Core::friendMessageReceived, this, &Widget::onFriendMessageReceived);
    connect(&core, &Core::receiptRecieved, this, &Widget::onReceiptReceived);
    connect(&core, &Core::groupInviteReceived, this, &Widget::onGroupInviteReceived);
    connect(&core, &Core::groupMessageReceived, this, &Widget::onGroupMessageReceived);
    connect(&core, &Core::groupPeerlistChanged, this, &Widget::onGroupPeerlistChanged);
    connect(&core, &Core::groupPeerNameChanged, this, &Widget::onGroupPeerNameChanged);
    connect(&core, &Core::groupTitleChanged, this, &Widget::onGroupTitleChanged);
    connect(&core, &Core::groupPeerVideoReceived, this, &Widget::onGroupPeerVideoReceived);
    connect(&core, &Core::groupPeerAudioPlaying, this, &Widget::onGroupPeerAudioPlaying);
    connect(&core, &Core::emptyGroupCreated, this, &Widget::onEmptyGroupCreated);
    connect(&core, &Core::groupJoined, this, &Widget::onGroupJoined);
    connect(&core, &Core::friendTypingChanged, this, &Widget::onFriendTypingChanged);
    connect(&core, &Core::groupSentFailed, this, &Widget::onGroupSendFailed);
    connect(&core, &Core::usernameSet, this, &Widget::refreshPeerListsLocal);
    connect(this, &Widget::statusSet, &core, &Core::setStatus);
    // JYJ 09.03
    // connect(this, &Widget::friendRequested, &core, &Core::requestFriendship);
    // connect(this, &Widget::friendRequestAccepted, &core, &Core::acceptFriendRequest);
    connect(this, &Widget::changeGroupTitle, &core, &Core::changeGroupTitle);

    sharedMessageProcessorParams.setPublicKey(core.getSelfPublicKey().toString());
}

void Widget::onConnected()
{
    ui->statusButton->setEnabled(true);
    emit core->statusSet(core->getStatus());
}

void Widget::onDisconnected()
{
    ui->statusButton->setEnabled(false);
    emit core->statusSet(Status::Status::Offline);
}

void Widget::onFailedToStartCore()
{
    QMessageBox critical(this);
    critical.setText(tr(
        "BangPaecore failed to start, the application will terminate after you close this message."));
    critical.setIcon(QMessageBox::Critical);
    critical.exec();
    qApp->exit(EXIT_FAILURE);
}

void Widget::onBadProxyCore()
{
    settings.setProxyType(Settings::ProxyType::ptNone);
    QMessageBox critical(this);
    critical.setText(tr("BangPaecore failed to start with your proxy settings. "
                        "qBangPae cannot run; please modify your "
                        "settings and restart.",
                        "popup text"));
    critical.setIcon(QMessageBox::Critical);
    critical.exec();
    onShowSettings();
}

void Widget::onStatusSet(Status::Status status)
{
    ui->statusButton->setProperty("status", static_cast<int>(status));
    ui->statusButton->setIcon(prepareIcon(getIconPath(status), icon_size, icon_size));
    updateIcons();
}

void Widget::onSeparateWindowClicked(bool separate)
{
    onSeparateWindowChanged(separate, true);
}

void Widget::onSeparateWindowChanged(bool separate, bool clicked)
{
    if (!separate) {
        QWindowList windowList = QGuiApplication::topLevelWindows();

        for (QWindow* window : windowList) {
            if (window->objectName() == "detachedWindow") {
                window->close();
            }
        }

        QWidget* contentWidget = new QWidget(this);
        contentWidget->setObjectName("contentWidget");

        contentLayout = new ContentLayout(contentWidget);
        // JYJ 04.10
        //ui->mainSplitter->addWidget(contentWidget);
        settings.firstContactRight = settings.getContactRight();
        if (settings.getContactRight())
            ui->mainSplitter->insertWidget(0, contentWidget);
        else
            ui->mainSplitter->addWidget(contentWidget);
        // ROC START 06.05
        QDesktopWidget* pWid = QApplication::desktop();
        int nScreenWidth = pWid->availableGeometry().right();

        int v = 0;
        if (settings.getContactRight())
            v = 1;

        if (nScreenWidth <= SMLDIS_SCR_WIDTH) {
            // JYJ 04.10
            // ui->mainSplitter->widget(0)->setMaximumWidth(nScreenWidth * 0.25);
            // ui->mainSplitter->widget(0)->setMinimumWidth(nScreenWidth * 0.25);
            ui->mainSplitter->widget(v)->setMaximumWidth(nScreenWidth * 0.25);
            ui->mainSplitter->widget(v)->setMinimumWidth(nScreenWidth * 0.25);
        } else {
            // JYJ 0410
            // ui->mainSplitter->widget(0)->setMaximumWidth(NMLDIS_CONTRACT_MAX_WIDTH);
            // ui->mainSplitter->widget(0)->setMinimumWidth(NMLDIS_CONTRACT_MAX_WIDTH);
            ui->mainSplitter->widget(v)->setMaximumWidth(NMLDIS_CONTRACT_MAX_WIDTH);
            ui->mainSplitter->widget(v)->setMinimumWidth(NMLDIS_CONTRACT_MAX_WIDTH);
        }
        // ROC END 06.05

        setMinimumWidth(COTRACT_MIN_WIDTH);

        SplitterRestorer restorer(ui->mainSplitter);
        restorer.restore(settings.getSplitterState(), size());

        showProfile();      
    } else {
        int width = ui->friendList->size().width();
        QSize size;
        QPoint pos;

        if (contentLayout) {
            pos = mapToGlobal(ui->mainSplitter->widget(1)->pos());
            size = ui->mainSplitter->widget(1)->size();
        }

        if (contentLayout) {
            contentLayout->clear();
            contentLayout->parentWidget()->setParent(nullptr); // Remove from splitter.
            contentLayout->parentWidget()->hide();
            contentLayout->parentWidget()->deleteLater();
            contentLayout->deleteLater();
            contentLayout = nullptr;
        }

        setMinimumWidth(ui->tooliconsZone->sizeHint().width());

        if (clicked) {
            showNormal();
            resize(width, height());

            if (settingsWidget) {
                ContentLayout* contentLayout = createContentDialog((DialogType::SettingDialog));
                contentLayout->parentWidget()->resize(size);
                contentLayout->parentWidget()->move(pos);
                settingsWidget->show(contentLayout);
                setActiveToolMenuButton(ActiveToolMenuButton::None);
            }
        }

        setWindowTitle(QString());
        setActiveToolMenuButton(ActiveToolMenuButton::None);
    }
}

void Widget::setWindowTitle(const QString& title)
{
    if (title.isEmpty()) {
        QMainWindow::setWindowTitle(QApplication::applicationName());
    } else {
        QString tmp = title;
        /// <[^>]*> Regexp to remove HTML tags, in case someone used them in title
        QMainWindow::setWindowTitle(QApplication::applicationName() + QStringLiteral(" - ")
                                    + tmp.remove(QRegExp("<[^>]*>")));
    }
}

void Widget::forceShow()
{
    hide(); // Workaround to force minimized window to be restored
    show();
    activateWindow();
}

void Widget::onGroupClicked()
{
    if (settings.getSeparateWindow()) {
        if (!groupInviteForm->isShown()) {
            groupInviteForm->show(createContentDialog(DialogType::GroupDialog));
        }

        setActiveToolMenuButton(ActiveToolMenuButton::None);
    } else {
        hideMainForms(nullptr);
        groupInviteForm->show(contentLayout);
        setWindowTitle(fromDialogType(DialogType::GroupDialog));
        setActiveToolMenuButton(ActiveToolMenuButton::GroupButton);
    }
}

void Widget::confirmExecutableOpen(const QFileInfo& file)
{
    static const QStringList dangerousExtensions = {"app",  "bat",     "com",    "cpl",  "dmg",
                                                    "exe",  "hta",     "jar",    "js",   "jse",
                                                    "lnk",  "msc",     "msh",    "msh1", "msh1xml",
                                                    "msh2", "msh2xml", "mshxml", "msi",  "msp",
                                                    "pif",  "ps1",     "ps1xml", "ps2",  "ps2xml",
                                                    "psc1", "psc2",    "py",     "reg",  "scf",
                                                    "sh",   "src",     "vb",     "vbe",  "vbs",
                                                    "ws",   "wsc",     "wsf",    "wsh"};

    if (dangerousExtensions.contains(file.suffix())) {
        bool answer = GUI::askQuestion(tr("Executable file", "popup title"),
                                       tr("You have asked qBangPae to open an executable file. "
                                          "Executable files can potentially damage your computer. "
                                          "Are you sure want to open this file?",
                                          "popup text"),
                                       false, true);
        if (!answer) {
            return;
        }

        // The user wants to run this file, so make it executable and run it
        QFile(file.filePath())
            .setPermissions(file.permissions() | QFile::ExeOwner | QFile::ExeUser | QFile::ExeGroup
                            | QFile::ExeOther);
    }

    QDesktopServices::openUrl(QUrl::fromLocalFile(file.filePath()));
}

void Widget::onIconClick(QSystemTrayIcon::ActivationReason reason)
{
    if (reason == QSystemTrayIcon::Trigger) {
        if (isHidden() || isMinimized()) {
            if (wasMaximized) {
                showMaximized();
            } else {
                showNormal();
            }

            activateWindow();
        } else if (!isActiveWindow()) {
            activateWindow();
        } else {
            wasMaximized = isMaximized();
            hide();
        }
    } else if (reason == QSystemTrayIcon::Unknown) {
        if (isHidden()) {
            forceShow();
        }
    }
}

void Widget::onShowSettings()
{
    if (settings.getSeparateWindow()) {
        if (!settingsWidget->isShown()) {
            settingsWidget->show(createContentDialog(DialogType::SettingDialog));
        }

        setActiveToolMenuButton(ActiveToolMenuButton::None);
    } else {
        hideMainForms(nullptr);
        settingsWidget->show(contentLayout);
        setWindowTitle(fromDialogType(DialogType::SettingDialog));
        setActiveToolMenuButton(ActiveToolMenuButton::SettingButton);
    }
}

void Widget::showProfile() // onAvatarClicked, onUsernameClicked
{
    if (settings.getSeparateWindow()) {
        if (!profileForm->isShown()) {
            profileForm->show(createContentDialog(DialogType::ProfileDialog));
        }

        setActiveToolMenuButton(ActiveToolMenuButton::None);
    } else {
        hideMainForms(nullptr);
        profileForm->show(contentLayout);
        setWindowTitle(fromDialogType(DialogType::ProfileDialog));
        setActiveToolMenuButton(ActiveToolMenuButton::None);
    }
}

void Widget::hideMainForms(GenericChatroomWidget* chatroomWidget)
{
    setActiveToolMenuButton(ActiveToolMenuButton::None);

    if (contentLayout != nullptr) {
        contentLayout->clear();
    }

    if (activeChatroomWidget != nullptr) {
        activeChatroomWidget->setAsInactiveChatroom();
    }

    activeChatroomWidget = chatroomWidget;
}

void Widget::setUsername(const QString& username)
{
    if (username.isEmpty()) {
        ui->nameLabel->setText(tr("Your name"));
        ui->nameLabel->setToolTip(tr("Your name"));
    } else {
        ui->nameLabel->setText(username);
        ui->nameLabel->setToolTip(
            Qt::convertFromPlainText(username, Qt::WhiteSpaceNormal)); // for overlength names
    }

    sharedMessageProcessorParams.onUserNameSet(username);
}

void Widget::onStatusMessageChanged(const QString& newStatusMessage)
{
    // Keep old status message untifl Core tells us to set it.
    core->setStatusMessage(newStatusMessage);
}

void Widget::setStatusMessage(const QString& statusMessage)
{
    ui->statusLabel->setText(statusMessage);
    // escape HTML from tooltips and preserve newlines
    // TODO: move newspace preservance to a generic function
    ui->statusLabel->setToolTip("<p style='white-space:pre'>" + statusMessage.toHtmlEscaped() + "</p>");
}

/**
 * @brief Plays a sound via the audioNotification AudioSink
 * @param sound Sound to play
 * @param loop if true, loop the sound until onStopNotification() is called
 */
void Widget::playNotificationSound(IAudioSink::Sound sound, bool loop)
{
    if (!settings.getAudioOutDevEnabled()) {
        // don't try to play sounds if audio is disabled
        return;
    }

    if (audioNotification == nullptr) {
        audioNotification = std::unique_ptr<IAudioSink>(audio.makeSink());
        if (audioNotification == nullptr) {
            //qDebug() << "Failed to allocate AudioSink";
            return;
        }
    }

    audioNotification->connectTo_finishedPlaying(this, [this](){ cleanupNotificationSound(); });

    audioNotification->playMono16Sound(sound);

    if (loop) {
        audioNotification->startLoop();
    }
}

void Widget::cleanupNotificationSound()
{
    audioNotification.reset();
}

void Widget::onNewFriendAVAlert(uint32_t friendnumber)
{
    const auto& friendId = FriendList::id2Key(friendnumber);
    newFriendMessageAlert(friendId, {}, false);
}


void Widget::incomingNotification(uint32_t friendnumber)
{
    const auto& friendId = FriendList::id2Key(friendnumber);
    newFriendMessageAlert(friendId, {}, false);

    friendWidgets[friendId]->chatroomWidgetClicked(friendWidgets[friendId]);

    // loop until call answered or rejected

    auto& s = Settings::getInstance();
    QPair<int, int> callingMusic = s.getFriendCallingMusic(friendId);

    // 호출착신음보관정보에 따라 해당한 착신음 재생진행
    if(callingMusic.first == TYPE_OUR_PHONE_BELL
            && callingMusic.second >= 1
            && callingMusic.second <= NUM_OUR_PHONE_BELL)
        playNotificationSound(IAudioSink::Sound(NUM_ORIGINAL_PHONE_BELL + callingMusic.second), true);
    else
        playNotificationSound(IAudioSink::Sound::IncomingCall_0, true);

}


void Widget::outgoingNotification()
{
    // loop until call answered or rejected
    playNotificationSound(IAudioSink::Sound::OutgoingCall, true);
}


void Widget::onNobodyAnswerNotification()
{
    // loop until call answered or rejected
    playNotificationSound(IAudioSink::Sound::NobodyAnswer, false);
}

void Widget::onCallEnd()
{
    playNotificationSound(IAudioSink::Sound::CallEnd);
}

/**
 * @brief Widget::onStopNotification Stop the notification sound.
 */
void Widget::onStopNotification()
{
    audioNotification.reset();
}


/**
 * @brief Dispatches file to the appropriate chatlog and accepts the transfer if necessary
 */
void Widget::dispatchFile(BangPaeFile file)
{
    const auto& friendId = FriendList::id2Key(file.friendId);
    if(!chatForms.contains(friendId)) addFriendChatForm(friendId);
    Friend* f = FriendList::findFriend(friendId);
    if (!f) {
        return;
    }

    auto pk = f->getPublicKey();

    if (file.status == BangPaeFile::INITIALIZING && file.direction == BangPaeFile::RECEIVING) {
        auto sender =
            (file.direction == BangPaeFile::SENDING) ? core->getSelfPublicKey() : pk;

        const Settings& settings = Settings::getInstance();
//        QString autoAcceptDir = settings.getAutoAcceptDir(f->getPublicKey());

//        if (autoAcceptDir.isEmpty() && settings.getAutoSaveEnabled()) {
//            autoAcceptDir = settings.getGlobalAutoAcceptDir();
//        }
        QString autoAcceptDir = settings.getGlobalAutoAcceptDir();


        auto maxAutoAcceptSize = settings.getMaxAutoAcceptSize();
        bool autoAcceptSizeCheckPassed = maxAutoAcceptSize == 0 || maxAutoAcceptSize >= file.filesize;

        if (!autoAcceptDir.isEmpty() && autoAcceptSizeCheckPassed) {
            acceptFileTransfer(file, autoAcceptDir);
        }
    }    

    if (file.fileName.startsWith(SM_FUNCTXT_REQ_FILERLY)) {
        uint8_t selfLongTermKey[32];
        uint8_t pubKeyDest[32], pubKeySrc[32], arrResPubKey[32];

        memcpy(selfLongTermKey, core->getSelfPublicKey().getData(), 32);
        QString strResPubKey = file.fileName.mid(SM_FUNCTXT_REQ_FILERLY.length(), 64);
        QString2Byte(strResPubKey, arrResPubKey);

        //Unmix the long term key of source and destination peer.
        if (BangPaeFile::SENDING == file.direction) {
            UnmixPubKeyFromRelayFileFormat(pubKeyDest, selfLongTermKey, arrResPubKey);

             /* If pubKeyDest is not in my friend list, then it means I am not a source friend, I am a relay friend.
             file.friendId is a destination friend.*/
            if (nullptr == FriendList::findFriend(BangPaePk(pubKeyDest))) {
                memcpy(pubKeyDest, friendId.getData(), 32);
                UnmixPubKeyFromRelayFileFormat(pubKeySrc, pubKeyDest, arrResPubKey);
            }
            //If not, pubKeyDest is in my friend list, then it means I am a source friend.
            else {
                memcpy(pubKeySrc, selfLongTermKey, 32);
            }
        }
        else if (BangPaeFile::RECEIVING == file.direction) {
            UnmixPubKeyFromRelayFileFormat(pubKeySrc, selfLongTermKey, arrResPubKey);

             //If pubKeySrc is not in my friend list, then it means I am a relay friend.file.friendId is a source friend.
            if (nullptr == FriendList::findFriend(BangPaePk(pubKeySrc))) {
                memcpy(pubKeySrc, friendId.getData(), 32);
                UnmixPubKeyFromRelayFileFormat(pubKeyDest, pubKeySrc, arrResPubKey);
            }
            //If not, pubKeyDest is in my friend list, then it means I am a destination friend.
            else {
                memcpy(pubKeyDest, selfLongTermKey, 32);
            }
        }

        if (BangPaeFile::FINISHED == file.status ) {
             //If the destination key in the filename is equal to myself, then decrypt file and delete that file.
            if (memcmp(pubKeyDest, selfLongTermKey, 32) == 0) {
                QString strDstFileName = file.fileName.mid(SM_FUNCTXT_REQ_FILERLY.length() + 64);
                QString strDstFilePath = file.filePath.left(file.filePath.length() - file.fileName.length()) + strDstFileName;

                pFileEncDecTask = new QFileEncDecTask();
                connect(pFileEncDecTask, SIGNAL(fileDecryptionEnd(const QString&, const uint8_t*, const uint8_t*)),
                        this, SLOT(showRelayedFileDecryptionEndMsg(const QString&, const uint8_t*, const uint8_t*)),
                        Qt::BlockingQueuedConnection);

                pFileEncDecTask->SetThreadData(core, file.filePath, file.fileName, pubKeySrc, false/*복호화*/, file.friendId);
                pFileEncDecTask->setAutoDelete(true);
                QThreadPool::globalInstance()->start(pFileEncDecTask);

                file.fileName = strDstFileName;
                file.setFilePath(strDstFilePath);

                 /* If the relayed file is Image file, then wait for finish decrypting. This is for previewing that image.
                 * If I only set the path of decrypted image and don't wait for decrypting then the preview will be failed.*/
                static const QStringList previewExtensions = {"png", "jpeg", "jpg", "gif", "svg",
                                                              "PNG", "JPEG", "JPG", "GIF", "SVG"};
                if (previewExtensions.contains(QFileInfo(strDstFilePath).suffix())) {
                    QThreadPool::globalInstance()->waitForDone(1000);
                }
            }
             /* If the source key in the filename is equal to my secret key, then delete that file.
             * It means that the temporary encryption file has already been sent sucessfully. */
            else if (memcmp(pubKeySrc, selfLongTermKey, 32) == 0){
                QString strFileName = file.fileName.mid(SM_FUNCTXT_REQ_FILERLY.length() + 64);
                QString strFilePath = file.filePath.left(file.filePath.length() - file.fileName.length()) + strFileName;

                SafeDelFile(file.filePath);

                file.fileName = strFileName;
                file.setFilePath(strFilePath);
            }
             /* If not, then relay the file to the destination or delete the relayed temporary file.
             * Relay the file only if the direction is RECEIVING. Delete the file only if the direction is SENDING.*/
            else {
                if (file.direction == BangPaeFile::RECEIVING) {

                    Friend* pFriend = FriendList::findFriend( BangPaePk {pubKeyDest} );
                    if (nullptr != pFriend &&
                            (pFriend->getStatus() == Status::Status::Online || pFriend->getStatus() == Status::Status::Away)) {
                        
                        // ROC 2022.11.12 START
                        QString strFileName = QFileInfo(file.filePath).fileName();
                        QString strFileDir = file.filePath.left(file.filePath.length() - file.fileName.length());

                        // 화일이름복호화바이트렬부분얻기
                        QString strFileDecName = strFileName.right( strFileName.length() 
                                                - SM_FUNCTXT_REQ_FILERLY.length() 
                                                - 64);

                        // 얻어진 바이트렬로부터 원래의 파일이름복원.
                        uint8_t buf[100];
                        QString2Byte(strFileDecName, buf);
                        QByteArray byteArr{reinterpret_cast<const char*>(buf), strFileDecName.length() / 2};
                        strFileDecName = QString::fromLocal8Bit(byteArr);

                        QFile::rename(file.filePath, strFileDir + strFileDecName);
                        file.filePath = strFileDir + strFileDecName;
                        // ROC 2022.11.12 END

                        core->getCoreFile()->sendFile(
                                    pFriend->getId(), file.fileName, file.filePath, file.filesize);
                    }
                } else if (file.direction == BangPaeFile::SENDING) {
                    SafeDelFile(file.filePath);
                }
            }
        }
        else if (BangPaeFile::CANCELED == file.status || BangPaeFile::BROKEN == file.status) {
            //If the file transfer has failed, then delete the temp encrypted file.
            if (file.filePath.compare("") == 0)
                SafeDelFile(Settings::getInstance().getGlobalAutoAcceptDir() + "/" + file.fileName);
            else
                SafeDelFile(file.filePath);

             /* If I am a relay friend and an error occurred while sending to the destination friend
             then send the "file relay fail" spcial message to the source of relay file. */
            if (memcmp(pubKeyDest, selfLongTermKey, 32) != 0
                    && memcmp(pubKeySrc, selfLongTermKey, 32) != 0
                    && file.direction == BangPaeFile::SENDING) {

                //Tell the source friend that file relaying has been fault.
                friendMessageDispatchers[BangPaePk(pubKeySrc)]->sendMessage(false, SM_FILE_RELAY_FAILED + file.fileName);
            }
        }

         //If I am is a receiver of relayed file, then show the file transfering message in FreindChatLog of source friend.
        if (memcmp(pubKeyDest, selfLongTermKey, 32) == 0)
            pk = BangPaePk {pubKeySrc};
         //If I am is a sender of relayed file, then show the file transfering message in FreindChatLog of destination friend.
        else if (memcmp(pubKeySrc, selfLongTermKey, 32) == 0)
            pk = BangPaePk {pubKeyDest};
        //If I am a relay friend, then don't show file transfering message.
        else
            return;

    }

    /**/// Avoid Wrong Avatar Sending
    if (file.fileName.isEmpty() || file.fileKind == BANGPAE_FILE_KIND_AVATAR)
    {
        return;
    }

    const auto senderPk = (file.direction == BangPaeFile::SENDING) ? core->getSelfPublicKey() : pk;

    if (!friendChatLogs.contains(pk)) {
        addFriendChatForm(pk);
    }

    friendChatLogs[pk]->onFileUpdated(senderPk, file);

    /**/// If I get file from blogs
    if (file.direction == BangPaeFile::RECEIVING && file.status == BangPaeFile::FINISHED)
    {
        QStringList blogs = Settings::getInstance().blogs;
        for (int i = 0; i < blogs.length(); i ++)
        {
            QStringList items = blogs[i].split("@");
            if (items.length() != ProfileForm::usefulColumnCnt)
                continue;

            QString fileName = QFileInfo(items[ProfileForm::TitleIdx]).fileName();
            int st = fileName.lastIndexOf(".");
            fileName = fileName.left(st);
            if (file.fileName.startsWith(fileName))
            {
                friendMessageDispatchers[f->getPublicKey()]->sendMessage
                        (false, SM_BLOG_DOWNLOAD_SUCCESS + items[ProfileForm::TitleIdx]);
                // JYJ 2022.07.25, 22:30 START
                if (items[ProfileForm::LoadmodeIdx] == BLOG_MODE_UNNORMAL)
                // JYJ 2022.07.25, 22:30 END
                {
                    QStringList downloadedBlogs = Settings::getInstance().downloadedBlogs;
                    if (downloadedBlogs.length() > MAX_BLOG_RECORD_CNT) downloadedBlogs.removeAt(0);

                    downloadedBlogs.append(
                        items[ProfileForm::DateIdx] + "#" +
                        items[ProfileForm::ContributorIdx] + "#" +
                        items[ProfileForm::TitleIdx]);

                    Settings::getInstance().downloadedBlogs = downloadedBlogs;

                    //기대번호로 암호화
                    QFile srcFile(file.filePath);
                    srcFile.open(QFile::ReadWrite);

                    unsigned long long plaintext_len = 32;
                    unsigned long long ciphertext_len = plaintext_len + crypto_aead_xchacha20poly1305_ietf_ABYTES;
                    uint8_t *arrBufIn = static_cast<uint8_t*>(malloc(ciphertext_len * sizeof(uint8_t)));
                    srcFile.seek(srcFile.size() - plaintext_len);
                    srcFile.read(reinterpret_cast<char*>(arrBufIn), plaintext_len);

                    uint8_t* MachineNum = reinterpret_cast<uint8_t*>(Nexus::getProfile()->getStrMachineNum().toUtf8().data());

                    // 암호화를 진행한다.
                    crypto_aead_xchacha20poly1305_ietf_encrypt(
                                    arrBufIn, &ciphertext_len,
                                    arrBufIn, plaintext_len,
                                    ADD_ENDEC_DATA1, ADD_ENDEC_DATA1_LEN,
                                    nullptr,
                                    BPD_ENDEC_NONCE, MachineNum
                                    );

                    srcFile.seek(srcFile.size());
                    srcFile.write(reinterpret_cast<const char*>(arrBufIn), ciphertext_len);
                    srcFile.close();
                    free(arrBufIn);

                    break;
                }
            }
        }
        /**/// If that is poster merge file then do something
        if (file.fileName.startsWith("poster_sb"))
        {
            QFile inFile(file.filePath);
            if (inFile.open(QIODevice::ReadOnly))
            {
                QByteArray inArray = inFile.readAll();
                QDataStream inStream(&inArray, QIODevice::ReadOnly);

                int eraseMode;
                inStream >> eraseMode;

                QDir dir("Poster");
                dir.setFilter(QDir::AllEntries | QDir::NoDotAndDotDot);
                QFileInfoList fileList = dir.entryInfoList();

                int fileCnt = 0;
                foreach (QFileInfo file, fileList){
                    if (file.isFile()){
                        fileCnt ++;
                        if (eraseMode == 0)
                            file.dir().remove(file.fileName());
                    }
                }

                fileCnt = (fileCnt * eraseMode) + 1;

                bool suc = true;
                QString path = settings.getGlobalAutoAcceptDir() + "Poster/";

                {
                    dir = QDir(path);
                    dir.setFilter(QDir::AllEntries | QDir::NoDotAndDotDot);
                    fileList = dir.entryInfoList();
                    foreach (QFileInfo file, fileList){
                        file.dir().remove(file.fileName());
                    }
                }


                while(!inStream.atEnd())
                {
                    QByteArray array;
                    inStream >> array;
                    array = integrated_xchacha_encrypt(array, POSTER_ENDEC_NONCE, POSTER_ENDEC_KEY);

                    QFile outFile(QString("Poster/Poster-%1.dat").arg(fileCnt));
                    if (outFile.open(QIODevice::WriteOnly)) {
                        outFile.write(array);
                        outFile.close();
                    }
                    else {
                        QDir dir;
                        if(!dir.exists(path))
                            dir.mkpath(path);

                        QFile outFile(QString(path + "Poster-%1.dat").arg(fileCnt));
                        outFile.open(QIODevice::WriteOnly);
                        outFile.write(array);
                        outFile.close();
                        suc = false;
                    }
                    fileCnt ++;
                }
                inFile.close();

                if (!suc) {
                    if (eraseMode == 0) {
                        QMessageBox::information(nullptr, "알림",
                             QString("Windows체계구동기가 보호걸렸으므로 선전화파일복사가 실패하였습니다. %1의 파일들을 %2등록부안에 덧쓰기 복사하십시오.").
                             arg(path, QCoreApplication::applicationDirPath() + "/Poster"));
                    }
                    else {
                        QMessageBox::information(nullptr, "알림",
                             QString("Windows체계구동기가 보호걸렸으므로 선전화파일복사가 실패하였습니다. %1의 파일들을 %2등록부안에 추가하십시오.").
                             arg(path, QCoreApplication::applicationDirPath() + "/Poster"));
                    }
                }

                QMessageBox::StandardButton mboxResult =
                        QMessageBox::question(NULL, "다시가입",
                                                "선전화파일을 정확히 내리적재하였습니다.\n"
                                                "다음번 가입시 반영되게 됩니다. 다시 가입하시겠습니까?",
                                              QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

                if (mboxResult == QMessageBox::Yes) {
                    profileInfo->logout();
                }
            }
        }
    }

}

void Widget::dispatchFileWithBool(BangPaeFile file, bool)
{
    dispatchFile(file);
}

void Widget::dispatchFileSendFailedForRelay(uint32_t friendId, const QString& fileName, const QString& filepath)
{
    const auto& friendPk = FriendList::id2Key(friendId);

     /* Broadcast file relaying request to all my friend.
     * The index of first friend which send the request increases every once broadcasting.*/

    QList<Friend*> pFriendList = FriendList::getAllFriends();
    int nListLen = pFriendList.length();
    for (int i = 0; i < nListLen; ++ i) {
        int j = (nRelaySearchStartId + i) % nListLen;
        Friend* pFriend = pFriendList[j];
        if (pFriend->getStatus() == Status::Status::Online || pFriend->getStatus() == Status::Status::Away) {
            friendMessageDispatchers[pFriend->getPublicKey()]->sendMessage(false, SM_FILE_RELAY_IS_DEST_CONNECTED
                                            + Byte2QString(friendPk.getData(), 32)
                                            + filepath);
        }
    }

    nRelaySearchStartId = (nRelaySearchStartId + 1) % nListLen;

    pFileRelayWaitingMsg->show();

    QTimer::singleShot(TIME_FILELY_WAITING * 1000, this, [this, friendId, fileName, filepath](){
        this->onNoFileRelay(friendId, fileName, filepath);
    }
    );
}


bool Widget::isBlogFile(QString fileName)
{
    QStringList tempBlogs = Settings::getInstance().blogs;
    QString contributor;
    if(profileForm->isBlogPeer != -1)
        contributor = ProfileForm::int2contributor[profileForm->isBlogPeer];
    int i;
    if (!contributor.isEmpty()) {
        for (i = 0; i < tempBlogs.size(); i ++)
        {
            QStringList item = tempBlogs.at(i).split("@");
            if (item.size() != ProfileForm::usefulColumnCnt)
                continue;

            if (item[ProfileForm::TitleIdx].contains(fileName) &&
                    item[ProfileForm::ContributorIdx].compare(contributor) == 0)
                break;
        }
        if (i < tempBlogs.size())
            return true;
    }
    return false;
}


/**
 * @brief Widget::onNoTxtRelay
 * @param nFrndId : the index of destination friend.
 * @param nCurRelayTxtBufId : the index of message which you are going to send in arrRelayTextBuf
 * @param bIsForBroadCast : true if this function is called for response to broadcasting,
 *                          false if this functions is called for response to requesting
 *                                the already connected relay friend.
 */
void Widget::onNoTxtRelay(int nFrndId, int nCurRelayTxtBufId, bool bIsForBroadCast) {

    //If the message already have sent to destination friend, then return.
    if (-1 != arrRelayTextFrndId[nFrndId]) {
        return;
    }

    //If this function is response for broadcast request, then show the message, return the function.
    if (bIsForBroadCast) {
        QString strDestFrndName = "";
        for (Friend* p : FriendList::getAllFriends()) {
            if (static_cast<int>(p->getId()) == nFrndId) {
                strDestFrndName = p->getUserName();
                break;
            }
        }
        if (strDestFrndName.compare("") == 0) {
            QMessageBox::information(nullptr, "전송실패", "보내려는 대상이 주소록에 있는지 다시 확인하여 주십시오.");
            return;
        }

        //If this is for blog, then don't show the message.
        if (arrRelayTextBuf[nCurRelayTxtBufId].contains(SM_BLOG_CONTENT_REQUEST) ||
                arrRelayTextBuf[nCurRelayTxtBufId].contains(SM_BLOG_CONTENT) ||
                arrRelayTextBuf[nCurRelayTxtBufId].contains(SM_BLOG_REQUEST_FILE_PATH) ) {
            return;
        }

        if (arrRelayTextBuf[nCurRelayTxtBufId].startsWith(SMH)) {
            return;
        }

        QMessageBox::information(nullptr, "본문전송실패:"
                                 + arrRelayTextBuf[nCurRelayTxtBufId]
                                 + (bIsForBroadCast ? ", for broadcast" : ", not for broadcast"),
                                 "현재 수신자("+ strDestFrndName
                                 + ")가 방패망에 없거나 혹은 당신의 주소록에 련결된 대상들중 수신자와 직접 련결되여 중계할수 있는 사람이 없습니다.");
        return;
    }

     /* Otherwise, then you have to broadcast again,
     * because the relay friend which is alreay connected isn't connected with destination friend now.*/
    BroadcastTextRelayFrndReq(nFrndId, nCurRelayTxtBufId);
    QTimer::singleShot(TIME_TXTRLY_WAITING * 1000, this, [this, nFrndId, nCurRelayTxtBufId](){
            this->onNoTxtRelay(nFrndId, nCurRelayTxtBufId, true);
        }
    );
}

void Widget::BroadcastTextRelayFrndReq(int nFrndId, int nCurRelayTxtBufId) {
    QString strDstPubKey = Byte2QString(FriendList::id2Key(nFrndId).getData(), 32);
    for (Friend* f : FriendList::getAllFriends()) {
        if (Status::isOnline(f->getStatus() ) ) {
            friendMessageDispatchers[f->getPublicKey()]->sendMessage(false, SM_TEXT_RELAY_IS_DEST_CONNECTED
                                                            + strDstPubKey + QString("%1").arg(nCurRelayTxtBufId) );
        }
    }
}

void Widget::onNoFileRelay(int nFrndId, QString strFileName, QString strFilePath) {
     /* nFrndId : the index of friend who receives the relayed file.
     * If -1 == nArrRelayFrndId[nFrndId], it means there is no relay friend,*/
    if (!pFileRelayWaitingMsg->isHidden())
        pFileRelayWaitingMsg->hide();

    if (-1 == nArrRelayFrndId[nFrndId]) {
        QString strDestFrndName = "";
        for (Friend* p : FriendList::getAllFriends()) {
            if (static_cast<int>(p->getId()) == nFrndId) {
                strDestFrndName = p->getUserName();
                break;
            }
        }
        if (strDestFrndName.compare("") == 0) {
            QMessageBox::information(nullptr, "전송실패", "보내려는 대상이 주소록에 있는지 다시 확인하여 주십시오.");
            return;
        }

        if(!isBlogFile(strFileName))
        	QMessageBox::information(nullptr, "파일전송실패:" + strFilePath,
                                 "현재 수신자("
                                 + strDestFrndName
                                 + ")가 방패망에 없거나 혹은 당신의 주소록에 련결된 대상들중 수신자와 직접 련결되여 중계할수 있는 사람이 없습니다.");
        return;
    }

    // If you already had sended relay file to nFrndId, then return the function.
    int i;
    for (i = 0; i < nRelayFileDataCnt; i ++) {
        if (arrRelayFileData[i].nDstFrndId == nFrndId
                && arrRelayFileData[i].strFilePath.compare(strFilePath) == 0) {
            return;
        }
    }

    arrRelayFileData[nRelayFileDataCnt].nDstFrndId = nFrndId;
    arrRelayFileData[nRelayFileDataCnt].strFilePath = strFilePath;
    arrRelayFileData[nRelayFileDataCnt].nRelayFrndId = nArrRelayFrndId[nFrndId];
    nRelayFileDataCnt ++;

    pFileEncDecTask = new QFileEncDecTask();
    connect(pFileEncDecTask, SIGNAL(fileEncryptionEnd(int, QString&, QString&, int)),
            this, SLOT(onFileEncryptionEnd(int, QString&, QString&, int)), Qt::BlockingQueuedConnection);

    pFileEncDecTask->SetThreadData(core, strFilePath, strFileName,
                                     FriendList::id2Key(nFrndId).getData(),
                                     true, nArrRelayFrndId[nFrndId]);
    pFileEncDecTask->setAutoDelete(true);

    QThreadPool::globalInstance()->start(pFileEncDecTask);
}

void Widget::dispatchFileSendFailed(uint32_t friendId, const QString& fileName)
{
    const auto& friendPk = FriendList::id2Key(friendId);

    auto chatForm = chatForms.find(friendPk);
    if (chatForm == chatForms.end()) {
        return;
    }
    chatForm.value()->addSystemInfoMessage(tr("Failed to send file \"%1\"").arg(fileName),
                                           ChatMessage::MODIFIED_ERROR, QDateTime::currentDateTime());
}

void Widget::onRejectCall(uint32_t friendId)
{
    CoreAV* const av = core->getAv();
    av->cancelCall(friendId);
}

void Widget::onFriendStatusChanged(int friendId, Status::Status status)
{
    const auto& friendPk = FriendList::id2Key(friendId);
    if(!chatForms.contains(friendPk))
        addFriendChatForm(friendPk);
    Friend* f = FriendList::findFriend(friendPk);
    if (!f) {
        return;
    }

    bool isActualChange = f->getStatus() != status;

    FriendWidget* widget = friendWidgets[f->getPublicKey()];
    if (isActualChange) {
        if (!Status::isOnline(f->getStatus())) {
            f->setStatus(status);
            contactListWidget->moveWidget(widget, Status::Status::Online);
        } else if (status == Status::Status::Offline) {
            f->setStatus(status);
            contactListWidget->moveWidget(widget, Status::Status::Offline);
        }
        setJYJStatus();

        // JYJ 2022.07.16, 08:20 START
        if (core->getUsername().contains(UPDATE_PEER_NAME)
                && settings.autoUpdate
                && Status::isOnline(f->getStatus())
                && !f->getUserName().contains(Nexus::getProfile()->bpxServerName))
        {
            QTimer::singleShot(TIME_UPDATE_MESSAGE * 1000, this, [this, f](){
                friendMessageDispatchers[f->getPublicKey()]->sendMessage(false, SM_PLEASE_UPDATE + PROGRAM_VERSION);
            });
        }
        // JYJ 2022.07.16, 08:20 END

        if (f->getUserName().contains(Nexus::getProfile()->bpxServerName)
                && Status::isOnline(f->getStatus()))
        {
             QTimer::singleShot(TIME_BLOG_MESSAGE * 1000, this, [this](){profileForm->on_refreshBlog_clicked();});
        }

        // JYJ 2022.07.25, 17:00 START
        if (Status::isOnline(f->getStatus()))
        {
            QStringList &ownBlogs = profileForm->ownBlogs;
            for (int i = 0; i < ownBlogs.size(); i ++)
            {
               QTimer::singleShot(TIME_BLOG_MESSAGE * 1000, this, [this, f, ownBlogs, i]() {
               friendMessageDispatchers[f->getPublicKey()]->sendMessage(false, SM_BLOG_DOWNLOADCNT_UPDATED +
                                                                        ownBlogs.at(i));
               });
            }
        }
        // JYJ 2022.07.25, 17:00 END

    }

    f->setStatus(status);
    widget->updateStatusLight();
    if (widget->isActive()) {
        setWindowTitle(widget->getTitle());
    }

    ContentDialogManager::getInstance()->updateFriendStatus(friendPk);

    QString searchString = ui->searchContactText->text();
    FilterCriteria filter = getFilterCriteria();

    contactListWidget->searchChatrooms(searchString, filterOnline(filter), filterOffline(filter),
                                       filterGroups(filter));

    updateFilterText();

    contactListWidget->reDraw();
}

void Widget::onFriendStatusMessageChanged(int friendId, const QString& message)
{
    const auto& friendPk = FriendList::id2Key(friendId);
    Friend* f = FriendList::findFriend(friendPk);
    if (!f) {
        return;
    }

    QString str = message;
    str.replace('\n', ' ').remove('\r').remove(QChar('\0'));
    f->setStatusMessage(str);

    friendWidgets[friendPk]->setStatusMsg(str);
    if(chatForms.contains(friendPk))
        chatForms[friendPk]->setStatusMessage(str);
}

void Widget::onFriendDisplayedNameChanged(const QString& displayed)
{
    Friend* f = qobject_cast<Friend*>(sender());
    const auto& friendPk = f->getPublicKey();
    for (Group* g : GroupList::getAllGroups()) {
        if (g->getPeerList().contains(friendPk)) {
            g->updateUsername(friendPk, displayed);
        }
    }

    FriendWidget* friendWidget = friendWidgets[f->getPublicKey()];
    if (friendWidget->isActive()) {
        GUI::setWindowTitle(displayed);
    }
}

void Widget::onFriendUsernameChanged(int friendId, const QString& username)
{
    const auto& friendPk = FriendList::id2Key(friendId);
    Friend* f = FriendList::findFriend(friendPk);
    if (!f) {
        return;
    }

    QString str = username;
    str.replace('\n', ' ').remove('\r').remove(QChar('\0'));
    f->setName(str);
}

void Widget::onFriendAliasChanged(const BangPaePk& friendId, const QString& alias)
{
    Friend* f = qobject_cast<Friend*>(sender());

    // TODO(sudden6): don't update the contact list here, make it update itself
    FriendWidget* friendWidget = friendWidgets[friendId];
    Status::Status status = f->getStatus();
    contactListWidget->moveWidget(friendWidget, status);
    FilterCriteria criteria = getFilterCriteria();
    bool filter = status == Status::Status::Offline ? filterOffline(criteria) : filterOnline(criteria);
    friendWidget->searchName(ui->searchContactText->text(), filter);

    settings.setFriendAlias(friendId, alias);
    settings.savePersonal();

}

void Widget::onChatroomWidgetClicked(GenericChatroomWidget* widget)
{
    if(widget->getFriend() && !chatForms.contains(widget->getFriend()->getPublicKey()))
    {
        addFriendChatForm(widget->getFriend()->getPublicKey());
        clicked = widget;
        QTimer::singleShot(1, this, SLOT(clickedContactWidget()));
    }
    else
        openDialog(widget, /* newWindow = */ false);
}


void Widget::clickedContactWidget()
{
    if(clicked)
        openDialog(clicked, /* newWindow = */ false);
}

void Widget::openNewDialog(GenericChatroomWidget* widget)
{
    if(chatForms.contains(widget->getFriend()->getPublicKey()))
    {
        addFriendChatForm(widget->getFriend()->getPublicKey());
    }
    openDialog(widget, /* newWindow = */ true);
}

void Widget::openDialog(GenericChatroomWidget* widget, bool newWindow)
{
    widget->resetEventFlags();
    widget->updateStatusLight();

    GenericChatForm* form;
    GroupId id;
    const Friend* frnd = widget->getFriend();
    const Group* group = widget->getGroup();
    if (frnd) {
        if(chatForms.contains(frnd->getPublicKey()))
            form = chatForms[frnd->getPublicKey()];
        else
        {
            addFriendChatForm(frnd->getPublicKey());
            form = chatForms[frnd->getPublicKey()];
        }
    } else {
        id = group->getPersistentId();
        form = groupChatForms[id].data();
    }
    form->headWidget->headTextLayout->insertWidget(0, ChatFormHeader::pPosterLabel);
    bool chatFormIsSet;
    ContentDialogManager::getInstance()->focusContact(id);
    chatFormIsSet = ContentDialogManager::getInstance()->contactWidgetExists(id);


    if ((chatFormIsSet || form->isVisible()) && !newWindow) {
        return;
    }

    if (settings.getSeparateWindow() || newWindow) {
        ContentDialog* dialog = nullptr;

        if (!settings.getDontGroupWindows() && !newWindow) {
            dialog = ContentDialogManager::getInstance()->current();
        }

        if (dialog == nullptr) {
            dialog = createContentDialog();
        }

        dialog->show();

        if (frnd) {
            addFriendDialog(frnd, dialog);
        } else {
            Group* group = widget->getGroup();
            addGroupDialog(group, dialog);
        }

        dialog->raise();
        dialog->activateWindow();
    } else {
        hideMainForms(widget);
        if (frnd) {
            chatForms[frnd->getPublicKey()]->show(contentLayout);
        } else {
            groupChatForms[group->getPersistentId()]->show(contentLayout);
        }
        widget->setAsActiveChatroom();
        setWindowTitle(widget->getTitle());
    }
}


void Widget::onFriendMessageReceived(uint32_t friendnumber, const QString& message, bool isAction)
{
    // ROC 2022.09.26 START
    //WriteStringToFile("D:/BangPaeDownLoad/roc_debug.txt", "FMR_bgn\n", "a+");
    // ROC 2022.09.26 END

    QString strMsgBuf = message;

    const auto& friendId = FriendList::id2Key(friendnumber);
    Friend* f = FriendList::findFriend(friendId);

    if (!f) return;

/*************************철회기능과 관련하여 받은 통보문처리***************************/
    if(message.startsWith(SM_RECALL_FILE))
    {
        QString fileName = message.mid(SM_RECALL_FILE.length());
        chatForms[friendId]->recallFileMessage(fileName);

        int idx = fileName.lastIndexOf(".");
        QString name, ext;
        if (idx >= 0)
        {
            name = fileName.mid(0, idx);
            ext = fileName.mid(idx + 1);
        }
        else
            name = fileName;

        int mxIdx = -1;
        QString candidate;
        while(true)
        {
            if (mxIdx == -1)
                if (QFile::exists(settings.getGlobalAutoAcceptDir() + QDir::separator() + fileName))
                {
                    mxIdx = 0;
                    candidate = settings.getGlobalAutoAcceptDir() + QDir::separator() + fileName;
                }
                else
                    break;
            else
                if (QFile::exists(QString("%1%2%3 (%4).%5").arg(settings.getGlobalAutoAcceptDir())
                                  .arg(QDir::separator())
                                  .arg(name)
                                  .arg(mxIdx + 1)
                                  .arg(ext)))
                {
                    candidate = QString("%1%2%3 (%4).%5").arg(settings.getGlobalAutoAcceptDir())
                            .arg(QDir::separator())
                            .arg(name)
                            .arg(mxIdx + 1)
                            .arg(ext);
                    mxIdx ++;
                }
                else
                   break;
        }

        if (mxIdx == -1 || !QFile::remove(candidate))
        {
            friendMessageDispatchers[friendId]->sendMessage(false, SM_RECALL_FILE_FAIL + fileName);
        }
        else
        {
            friendMessageDispatchers[friendId]->sendMessage(false, SM_RECALL_FILE_SUCCESS + fileName);
        }
    }
    if(message.startsWith(SM_RECALL_FILE_RESULT))
    {
        QString fileName = message.mid(SM_RECALL_FILE_SUCCESS.length());
        QString result;
        if (message.count(SM_RECALL_FILE_SUCCESS) > 0)
        {
            result = "성공";
            chatForms[friendId]->recallMyLastFileMessage(fileName);
        }
        else
            result = "실패";
        chatForms[friendId]->addSystemInfoMessage(
                    QString("파일 %1에 대한 철회가 %2하였습니다.").arg(
                        fileName).arg(
                        result
                        ),
                    ChatMessage::INFO,
                    QDateTime::currentDateTime());

    }
    else if(message.startsWith(SM_RECALL))
    {
        QString recallMessage = message.mid(SM_RECALL.length());
        m_tempf = f;
        chatForms[m_tempf->getPublicKey()]->recallMessage(recallMessage);
    }

/*************************일반통화와 관련하여 받은 통보문처리***************************/
    // 상대방으로부터 림시공개열쇠받기처리진행
    else if(message.startsWith(SM_SEND_MY_PUB_KEY_NONCE_DATA))
    {
        // 특수문자렬에서 열쇠부분을 분리해내고 genSharedKey()함수를 리용하여 친구와의 공유열쇠를 생성.
        uint8_t friendPubKeyData[PUBKEYDATALEN];
        QString2Byte(message.mid(SM_SEND_MY_PUB_KEY_NONCE_DATA.length(), PUBKEYDATALEN), friendPubKeyData);

        uint8_t rlt = genSharedKey(friendPubKeyData,
                              Nexus::getProfile()->getServerED25519LongPubKey(),    //Ppub
                              Nexus::getProfile()->getSelfED25519LongPrivKey(),     //sA
                              Nexus::getProfile()->getSelfED25519SessionPrivKey(),  //tA
                              Nexus::getProfile()->getID()   ,                      //IDA
                              friendnumber );

        if( rlt == 4)
        {
            QMessageBox::critical(nullptr, "서명검증실패", f->getUserName() + "동지에 대한 신분인증이 실패하였습니다. \n\n\
보안상원칙으로부터 프로그람을 끄겠습니다. \n\
5분정도 있다가 프로그람을 다시 기동하여 보십시오. \n\
만일 계속 실패하면 개발자에게 알려주기 바랍니다.");
            qApp->quit();
        };

        if( rlt > 0 && rlt < 4)
        {
            QMessageBox::critical(nullptr, "열쇠공유오유", "열쇠공유가 실패하였습니다. 프로그람을 재기동하겠습니다.");
            profileInfo->logout();
            return;
        };

//        qDebug() << "SharedKey:" << Byte2QString(GetSharedKeyFrmFrndId(friendnumber), 32);

        // ROC 2022.07.10, 10:00 START
        // uint8_t RecvNonce[NONCELEN];
        // QString2Byte(message.mid(SM_SEND_MY_PUB_KEY_NONCE_DATA.length() + PUBKEYDATALEN, NONCESTRLEN), RecvNonce);
        // SetRecvNonceFrmFrndId(friendnumber, RecvNonce);
        // ROC 2022.07.10, 10:00 END

        // 뒤붙이에 대한 판단진행, "Y"인경우 자기의 림시공개열쇠를 상대방에게 전송.
        if (message.right(1) == "Y") {
            // ROC 2022.07.10, 10:00 START
            // GenerateSentNonceFrmFrndId(friendnumber);
//            friendMessageDispatchers[friendId]->sendMessage(false, SM_SEND_MY_PUB_KEY_NONCE_DATA +
//                                   Nexus::getProfile()->mySendPubKeyData +                                  //320byte
//                                   Byte2QString(GetSentNonceFrmFrndId(friendnumber), NONCELEN) + "N");      //STR48byte
            friendMessageDispatchers[friendId]->sendMessage(false, SM_SEND_MY_PUB_KEY_NONCE_DATA +
                                   Nexus::getProfile()->mySendPubKeyData);                                   //320byte
            // ROC 2022.07.10, 10:00 END
        }

        // ROC 2022.07.09, 19:30 START
        /*
        qDebug() << "Group Report Shared Key:"
                 << Byte2QString(GetSharedKeyFrmFrndId(f->getId()), 32) << endl;

        WriteStringToFile("D:/BangPaeDownLoad/GroupReportDebug.txt",
                         (getUsername() + ":" + QDateTime::currentDateTime().toString() + ":" + f->getDisplayedName() + "(1대1)").toUtf8().constData(),
                         "a+");
        WriteStringToFile("D:/BangPaeDownLoad/GroupReportDebug.txt", "\n", "a+");
        WriteStringToFile("D:/BangPaeDownLoad/GroupReportDebug.txt",
                         ("Shared Key:" + Byte2QString(GetSharedKeyFrmFrndId(f->getId()), 32)).toLocal8Bit().constData(),
                         "a+");
        WriteStringToFile("D:/BangPaeDownLoad/GroupReportDebug.txt", "\n", "a+");
        */
        // ROC 2022.07.09, 19:30 END
    }
    else if(message == SM_AUDIO_CALL_BUSY)
    {
        chatForms[friendId]->setCallBusyReceived = true;
        FriendBusyNotification(friendId);
    }
    else if(message == SM_AUDIO_CALL_REFUSED)
    {
        chatForms[friendId]->setCallRefusedReceived = true;
        FriendRefusedNotification(friendId);
    }

/*************************그룹과 관련하여 받은 통보문처리******************************/
    else if(message.startsWith(SM_IS_GROUP_AVAILABLE))
    {
        QString group_string = message.mid(SM_IS_GROUP_AVAILABLE.length());

        auto groups = GroupList::getAllGroups();
        bool exists = false;
        for(int i = 0; i < groups.count(); i++)
        {
            Group* g = groups.at(i);
            // JYJ 2022.07.24, 16:20 START
            if(Core::getInstance()->getSelfPublicKey().toString() == g->getGroupCreatorPK() &&
               g->getPersistentId().toString() == group_string)
            {
                exists = true;
            }
            // JYJ 2022.07.24, 16:20 END
        }
        for(auto it=Settings::getInstance().avaliable_groups.begin();
            it!=Settings::getInstance().avaliable_groups.end();)
        {
            if(it->id == group_string)
                exists = true;
            ++it;    //指向下一个位置
        }
        if(exists == false)
        {
            friendMessageDispatchers[friendId]->sendMessage(false, SM_GROUP_NOT_EXIST + group_string);
        }
    }
    else if(message.startsWith(SM_GROUP_NOT_EXIST))
    {
        QString group_string = message.mid(SM_GROUP_NOT_EXIST.length());

        auto groups = GroupList::getAllGroups();
        Group* del_group = NULL;
        for(int i = 0; i < groups.count(); i++)
        {
            Group* g = groups.at(i);
            // JYJ 2022.07.24, 16:20 START
            if(f->getPublicKey().toString() == g->getGroupCreatorPK() && g->getPersistentId().toString() == group_string)
            {
                del_group = g;
            }
            // JYJ 2022.07.24, 16:20 END
        }
        if(del_group)
        {
            QString groupName = del_group->getName();
            QMessageBox::information(NULL, "그룹삭제", f->getDisplayedName() + "동지가 그룹<" +
                                     groupName.mid(GROUP_NAME_MIN_SIZE_1) + ">을 해산하였으므로 자동탈퇴하였습니다.");
            removeGroup(del_group);
        }
    }

/*************************중계전송과 관련하여 받은 통보문처리***************************/
        //1--원천마디점------------------
    else if(message.startsWith(SM_FILE_RELAY_IS_DEST_CONNECTED)) {

        //If I am a ChongBanZhang or SamChon, then don't reponse the realy request.
        // JYJ 2022.07.25, 22:00 START
        if (core->getUsername().contains(STR_BOSS) && LEADER_PERMISSION == Nexus::getProfile()->bpxPerm ) return;
        // JYJ 2022.07.25, 22:00 END
        uint8_t pubKeyDest[32];
        QString strDestPubKey = message.mid(SM_FILE_RELAY_IS_DEST_CONNECTED.length(), 64);
        QString2Byte(strDestPubKey, pubKeyDest);

        QList<Friend*> allFriend = FriendList::getAllFriends();
        for (Friend* pFriend : allFriend) {
            if ( (pFriend->getStatus() == Status::Status::Online
                    || pFriend->getStatus() == Status::Status::Away)
                    && memcmp(pFriend->getPublicKey().getData(), pubKeyDest, 32) == 0 ) {
                friendMessageDispatchers[friendId]->sendMessage(false, SM_FILE_RELAY_DEST_IS_CONNECTED
                                                                         + strDestPubKey
                                                                         + message.mid(SM_FILE_RELAY_IS_DEST_CONNECTED.length() + 64));
                break;
            }
        }
    }
    else if(message.startsWith(SM_TEXT_RELAY_IS_DEST_CONNECTED)) {

        //If I am a ChongBanZhang or SamChon, then don't reponse the realy request.
        // JYJ 2022.07.25, 22:00 START
        if (core->getUsername().contains(STR_BOSS) && LEADER_PERMISSION == Nexus::getProfile()->bpxPerm ) return;
        // JYJ 2022.07.25, 22:00 END

        uint8_t pubKeyDest[32];
        QString strDestPubKey = message.mid(
                    SM_TEXT_RELAY_IS_DEST_CONNECTED.length(), 64);
        QString strMsgIndex = message.mid(
                    SM_TEXT_RELAY_IS_DEST_CONNECTED.length() + 64);
        QString2Byte(strDestPubKey, pubKeyDest);

        QList<Friend*> allFriend = FriendList::getAllFriends();
        for (Friend* pFriend : allFriend) {
            if ( (pFriend->getStatus() == Status::Status::Online
                    || pFriend->getStatus() == Status::Status::Away)
                    && memcmp(pFriend->getPublicKey().getData(), pubKeyDest, 32) == 0 ) {
                friendMessageDispatchers[friendId]->sendMessage(false, SM_TEXT_RELAY_DEST_IS_CONNECTED +
                                                                         strDestPubKey + strMsgIndex);
                break;
            }
        }
    }
    else if(message.startsWith(SM_FILE_RELAY_SUCCESS)) {

        QString strDstPubKey
                = message.mid(SM_FILE_RELAY_SUCCESS.length(), 64);

        QString strFileName
                = message.mid(SM_FILE_RELAY_SUCCESS.length() + 64);

        uint8_t arrDstPubKey[32];
        QString2Byte(strDstPubKey, arrDstPubKey);

        Friend* pDstFrnd = FriendList::findFriend(BangPaePk(arrDstPubKey));
        Friend* pRelayFrnd = FriendList::findFriend(FriendList::id2Key(friendnumber));

        if (!isBlogFile(strFileName))
            QMessageBox::information(nullptr, "파일중계전송성공", pRelayFrnd->getUserName()
                                + "동지를 통하여 " + pDstFrnd->getUserName() + "동지에게 파일(" + strFileName
                                + ")을 성과적으로 전송하였습니다.");
    }
    else if(message.startsWith(SM_FILE_RELAY_FAILED)) {
        QString strResPubKey = message.mid(SM_FILE_RELAY_FAILED.length() +
                                           SM_FUNCTXT_REQ_FILERLY.length(), 64);
        QString strFileName = message.mid(SM_FILE_RELAY_FAILED.length() +
                                          SM_FUNCTXT_REQ_FILERLY.length() + 64);
        uint8_t arrDstPubKey[32], arrResPubKey[32];
        QString2Byte(strResPubKey, arrResPubKey);

        UnmixPubKeyFromRelayFileFormat(arrDstPubKey, core->getSelfPublicKey().getData(), arrResPubKey);

        Friend* pDstFrnd = FriendList::findFriend(BangPaePk(arrDstPubKey));
        Friend* pRelayFrnd = FriendList::findFriend(FriendList::id2Key(friendnumber));

        if (!isBlogFile(strFileName)) {

            /* If nRelayFileDataCnt == 0, it means there is no file sent in relay mode.
             * Therefore, this is error case and is not processed. */
            if (nRelayFileDataCnt > 0) {
                for (int i = 0; i < nRelayFileDataCnt; i ++) {
                    if ( QFileInfo(arrRelayFileData[i].strFilePath).fileName() == strFileName ) {
                        if (-1 != arrRelayFileData[i].nRelayFrndId) {
                            arrRelayFileData[i].nRelayFrndId = -1;

                            QMessageBox::information(nullptr, "파일중계전송실패", pRelayFrnd->getUserName()
                                                + "동지를 통하여 " + pDstFrnd->getUserName() + "동지에게 파일(" + strFileName
                                                + ")을 중계전송하던중 실패하였습니다.");
                        }
                        break;
                    }
                }
            }
        }
    }
        //2--중계마디점-------------------
    else if(message.startsWith(SM_TEXT_RELAY_FUNCTXT_REQ)) {

        uint8_t pubKeySrc[32], arrResPubKey[32], pubKeyDest[32];
        QString strResPubKey = message.mid(SM_TEXT_RELAY_FUNCTXT_REQ.length(), 64);
        QString2Byte(strResPubKey, arrResPubKey);

        //Unmix the long term key of source and destination peer.
        UnmixPubKeyFromRelayFileFormat(pubKeySrc, core->getSelfPublicKey().getData(), arrResPubKey);

         /* If pubKeySrc is not in my friend list, then it means I am a relay friend.
         * file.friendId is a source friend.*/
        if (nullptr == FriendList::findFriend(BangPaePk(pubKeySrc))) {
            memcpy(pubKeySrc, friendId.getData(), 32);
            UnmixPubKeyFromRelayFileFormat(pubKeyDest, pubKeySrc, arrResPubKey);

            friendMessageDispatchers[BangPaePk(pubKeyDest)]->sendMessage(false, message);

        }
        //If not, pubKeyDest is in my friend list, then it means I am a destination friend.
        else {
            memcpy(pubKeyDest, core->getSelfPublicKey().getData(), 32);

            if (!friendChatLogs.contains(BangPaePk(pubKeySrc))) {
                addFriendChatForm(BangPaePk(pubKeySrc));
            }

            f = FriendList::findFriend(BangPaePk(pubKeySrc));
            strMsgBuf = message.mid(SM_TEXT_RELAY_FUNCTXT_REQ.length() + 64);

            if ( !(strMsgBuf.isEmpty() || strMsgBuf.startsWith(SMH) ) ) {
                QMessageBox::information(NULL, "통보문중계전송" + strMsgBuf,
                                                     FriendList::findFriend( BangPaePk{pubKeySrc} )->getDisplayedName() +
                                                    "동지가 중계전송방식으로 통보문을 전송하였습니다. (중계자:" +
                                                     FriendList::findFriend(FriendList::id2Key(friendnumber))->getDisplayedName() + ")", QMessageBox::Ok);
                filterAllAction->setChecked(true);
                changeDisplayMode();
            }
        }
    }
    else if(message.startsWith(SM_FILE_RELAY_SUCCESS_TRANSFER)) {
        // Source Friend of Relayed file
        QString strSrcPubKey = message.mid(SM_FILE_RELAY_SUCCESS_TRANSFER.length(), 64);

        // Destination Friend of Relayed file
        QString strDstPubKey = message.mid(SM_FILE_RELAY_SUCCESS_TRANSFER.length() + 64, 64);
        QString strFileName = message.mid(SM_FILE_RELAY_SUCCESS_TRANSFER.length() + 128);

        uint8_t pubKeySrc[32];
        QString2Byte(strSrcPubKey, pubKeySrc);

         /* If i have received the request to trasfer "file relay success" message to source friend of relay file,
         * then get the source friend, make pattern, and send to source friend.
         * Pattern = PREFIX + DestinationFriendPublicKey + FileName */
        friendMessageDispatchers[BangPaePk(pubKeySrc)]->sendMessage(false, SM_FILE_RELAY_SUCCESS + strDstPubKey + strFileName);

    }
    else if(message.startsWith(SM_FILE_RELAY_DEST_IS_CONNECTED)) {

        //At first, calcuate the friendId of friend with public key in the response and put into nDstFrndId.
        QString strDestPubKey = message.mid(SM_FILE_RELAY_DEST_IS_CONNECTED.length(), 64);
        QString strFilePath = message.mid(SM_FILE_RELAY_DEST_IS_CONNECTED.length() + 64);

        uint8_t pubKeyDst[32];
        QString2Byte(strDestPubKey, pubKeyDst);

        int nDstFrndId = -1;
        int i = 0;
        for (Friend* p : FriendList::getAllFriends()) {
            if (memcmp(p->getPublicKey().getData(), pubKeyDst, 32) == 0) {
                nDstFrndId = p->getId();
                break;
            }
            i ++;
        }

        if (-1 == nDstFrndId) {
            QMessageBox::information(nullptr, "Group File Sending Error",
                                     "Friend in relay respond is not in my list.", QMessageBox::Ok);
            return;
        }

         /* If that friend(nDstFrndId) has no relay friend,then put the friendnumber to nArrRelayFrndId[].
         * nArrRelayFrndId[] is used in onNoFileRelay(), in the case of number of directed friend
         * is smaller than destination friend.*/
        if (-1 == nArrRelayFrndId[nDstFrndId]) {
            nArrRelayFrndId[nDstFrndId] = friendnumber;
        }

        //If you already had sended file using friendnumber relay friend, then return the function.
        if (nArrRelayFileStatus[friendnumber]) {
            return;
        }

        //If you already had relayed the fil-strFilePath to nDstFrndId, then return the function.
        for (i = 0; i < nRelayFileDataCnt; i ++) {
            if (arrRelayFileData[i].nDstFrndId == nDstFrndId
                && arrRelayFileData[i].strFilePath.compare(strFilePath) == 0 ) {
                return;
            }
        }

        //Insert RelayFileData to arrRelayFileData[], and send the file.
        arrRelayFileData[nRelayFileDataCnt].nDstFrndId = nDstFrndId;
        arrRelayFileData[nRelayFileDataCnt].strFilePath = strFilePath;
        arrRelayFileData[nRelayFileDataCnt].nRelayFrndId = friendnumber;
        nRelayFileDataCnt ++;
        nArrRelayFileStatus[friendnumber] = true;
        QString strFileName = strFilePath.mid(strFilePath.lastIndexOf("/") + 1);

        pFileEncDecTask = new QFileEncDecTask();
        connect(pFileEncDecTask, SIGNAL(fileEncryptionEnd(int, QString&, QString&, int)),
                this, SLOT(onFileEncryptionEnd(int, QString&, QString&, int)), Qt::BlockingQueuedConnection);

        pFileEncDecTask->SetThreadData(core, strFilePath, strFileName, pubKeyDst, true, f->getId());
        pFileEncDecTask->setAutoDelete(true);
        QThreadPool::globalInstance()->start(pFileEncDecTask);
    }
    else if(message.startsWith(SM_TEXT_RELAY_DEST_IS_CONNECTED)) {

        //At first, calcuate the friendId of friend with public key in the response and put into nDstFrndId.
        uint8_t pubKeyDst[32];
        QString strDestPubKey = message.mid(SM_TEXT_RELAY_DEST_IS_CONNECTED.length(), 64);
        QString strMsgIndex = message.mid(SM_TEXT_RELAY_DEST_IS_CONNECTED.length() + 64);;
        QString2Byte(strDestPubKey, pubKeyDst);

        int nDstFrndId = -1;
        for (Friend* p : FriendList::getAllFriends()) {
            if (memcmp(p->getPublicKey().getData(), pubKeyDst, 32) == 0) {
                nDstFrndId = p->getId();
                break;
            }
        }

        if (-1 == nDstFrndId) {
            QMessageBox::information(nullptr, "Text relay error",
                                     "Friend in relay respond is not in my list.", QMessageBox::Ok);
            return;
        }

        if (arrRelayTextFrndId[nDstFrndId] != -1) return;

        arrRelayTextFrndId[nDstFrndId] = friendnumber;

        //Mix the long term key of source and destination peer.
        uint8_t arrResPubKey[32];
        MixPubKeyToRelayFileFormat(arrResPubKey, core->getSelfPublicKey().getData(), pubKeyDst);
        friendMessageDispatchers[friendId]->sendMessage(false, SM_TEXT_RELAY_FUNCTXT_REQ
                                                                + Byte2QString(arrResPubKey)
                                                                + arrRelayTextBuf[strMsgIndex.toInt()]);
    }

/*************************게시판과 관련하여 받은 통보문처리**************************/
    else if(message.startsWith(SM_BLOG_CONTENT_REQUEST))
    {
        QMutexLocker{&blogLock};
        QString contributor;
        contributor = profileForm->publishName;

        friendMessageDispatchers[friendId]->sendMessage(false, SM_BLOG_CONTENT + contributor + "#" + "START");

        QStringList tempBlogs = Settings::getInstance().blogs;
        for (int i = 0; i < tempBlogs.size(); i ++)
        {
            QStringList item = tempBlogs.at(i).split("@");
            if (item.size() != ProfileForm::usefulColumnCnt)
                continue;

            if (item.at(ProfileForm::ContributorIdx) == profileForm->publishName)
            {
                friendMessageDispatchers[friendId]->sendMessage(false, SM_BLOG_CONTENT
                                                                        + contributor + "#" + tempBlogs.at(i));
            }
        }

        friendMessageDispatchers[friendId]->sendMessage(false, SM_BLOG_CONTENT + contributor + "#" + "END");
    }
    else if(message.startsWith(SM_BLOG_CONTENT))
    {
        QMutexLocker{&blogLock};
        QString info = message.mid(SM_BLOG_CONTENT.length());
        int idx = info.indexOf("#");
        QString contributor = info.left(idx);

        if (message.endsWith("START"))
        {
            blogForOneAdmin.clear();
        }
        else if (message.endsWith("END"))
        {
            QStringList oldblogs = Settings::getInstance().blogs;
            int i;

            QStringList tempBlogs = blogForOneAdmin;
            QStringList newBlogs;

            for (i = 0; i < tempBlogs.size(); i ++)
            {
                QStringList items = tempBlogs.at(i).split("@");

                if (items.size() != ProfileForm::usefulColumnCnt)
                    continue;

                int j;
                bool flag = false;
                for (j = 0;j < oldblogs.size(); j ++) {
                    QStringList oldItems = oldblogs.at(j).split("@");
                    int k;
                    for (k = 0; k < ProfileForm::usefulColumnCnt; k ++)
                        if (k != ProfileForm::DownloadNumIdx && oldItems.at(k) != items.at(k))
                            break;
                    if (k == ProfileForm::usefulColumnCnt) {
                        int d1 = items.at(ProfileForm::DownloadNumIdx).toInt();
                        int d2 = oldItems.at(ProfileForm::DownloadNumIdx).toInt();
                        if (d2 > d1) {
                            newBlogs << oldblogs.at(j);
                            flag = true;
                        }
                        break;
                    }
                }
                if (flag) continue;

                if (items.at(ProfileForm::ContributorIdx) == profileForm->publishName)
                {
                    newBlogs << tempBlogs.at(i);
                    continue;
                }

                if (items.at(ProfileForm::PermissionIdx) == ProfileForm::permissionStrs.at(0))
                {
                    // JYJ 2022.07.26, 18:00 START
                     if(Nexus::getProfile()->bpxPerm == NORMAL_PERMISSION ||
                        Nexus::getProfile()->bpxPermission.contains(STR_WORKER_BOSS) ||
                        Nexus::getProfile()->bpxPermission.contains(STR_YOUTH_BOSS))
                     // JYJ 2022.07.26, 18:00 START
                         continue;
                }

                // JYJ 2022.07.26, 18:30 START
                if (items.at(ProfileForm::AgreementIdx) != BLOG_STATUS_ACCEPT)
                // JYJ 2022.07.26, 18:30 START
                    continue;

                newBlogs << tempBlogs.at(i);
            }

            Settings::getInstance().blogs = newBlogs;

            profileForm->refreshBlogTwt();

            for (i = 0; i < newBlogs.count(); i ++)
            {
                QStringList items = newBlogs.at(i).split("@");
                if (items.size() != ProfileForm::usefulColumnCnt)
                    continue;
                QString testString = items[0] + "@" + items[1];
                int j;
                for (j = 0; j < oldblogs.count(); j ++)
                {
                    if (oldblogs[j].startsWith(testString))
                        break;
                }
                if (j == oldblogs.count())
                    break;
            }
            if (i < newBlogs.count())
            {
                profileForm->isChanged = true;
                showProfile();
            }
        }
        else{
            QString blog = info.mid(idx + 1);
            // JYJ 09.03
            blogForOneAdmin.append(blog);
        }

    }
    else if(message.startsWith(SM_BLOG_REQUEST_FILE_PATH))
    {
        QMutexLocker{&blogLock};
        QString path = message.mid(SM_BLOG_REQUEST_FILE_PATH.length());

        QStringList blogs = Settings::getInstance().blogs;
        int i;
        for (i = 0; i < blogs.size(); i ++)
        {
            QStringList item = blogs.at(i).split("@");
            if (item.at(ProfileForm::TitleIdx) == path)
                break;
        }

        // 08.28
        if (!f->getDisplayedName().contains(Nexus::getProfile()->bpxServerName) && i == blogs.size())
        {
            friendMessageDispatchers[friendId]->sendMessage(false, SM_BLOG_FILE_DOWNLOAD_FAILED);
        }

        else
        {
            /**/// Do not increase the number

            QFile file(path);
            QString fileName = QFileInfo(path).fileName();
            if (!file.exists() || !file.open(QIODevice::ReadOnly))
                friendMessageDispatchers[friendId]->sendMessage(false, SM_BLOG_FILE_DOWNLOAD_FAILED);
            else
            {
                file.close();
                qint64 filesize = QFileInfo(path).size();

                // JYJ 2022.07.25, 22:30 START
                if(blogs.at(i).split("@").at(ProfileForm::LoadmodeIdx) == BLOG_MODE_UNNORMAL)
                // JYJ 2022.07.25, 22:30 END
                {
                    QString strNewFilePath = path.left(path.length() - 4) + ".bpd";
                    QString fileName1 = fileName.left(fileName.length() - 4) + ".bpd";

                    EncVidFile(path, strNewFilePath);

                    core->getCoreFile()->sendFile(f->getId(), fileName1, strNewFilePath, filesize + 3);
                }
                else
                    core->getCoreFile()->sendFile(f->getId(), fileName, path, filesize);

            }
        }
    }
    else if(message.startsWith(SM_BLOG_DELETE))
    {
        QMutexLocker{&blogLock};
        QString path = message.mid(SM_BLOG_DELETE.length());

        QStringList blogs = Settings::getInstance().blogs;
        int i;
        for (i = 0; i < blogs.size(); i ++)
        {
            QStringList item = blogs.at(i).split("@");
            if (path == item.at(ProfileForm::TitleIdx))
                break;
        }

        if (i < blogs.size())
        {
            blogs.removeAt(i);
            Settings::getInstance().blogs = blogs;
            profileForm->refreshBlogTwt();
        }
    }
    else if(message.startsWith(SM_BLOG_DOWNLOAD_SUCCESS))
    {
        QMutexLocker{&blogLock};
        QString path = message.mid(SM_BLOG_DOWNLOAD_SUCCESS.length());
        QStringList blogs = Settings::getInstance().blogs;
        int i;
        for (i = 0; i < blogs.size(); i ++)
        {
            QStringList item = blogs.at(i).split("@");
            if (item.size() != ProfileForm::usefulColumnCnt)
                continue;
            /* JYJ 07.12 */
            if (path.compare(item.at(ProfileForm::TitleIdx)) == 0
                    && item.at(ProfileForm::ContributorIdx).compare(profileForm->publishName) == 0)
                break;
        }

        if (i < blogs.size())
        {
            QStringList items = blogs.at(i).split("@");
            int nDownloads = items.at(ProfileForm::DownloadNumIdx).toInt();

            items.removeAt(ProfileForm::DownloadNumIdx);
            items.insert(ProfileForm::DownloadNumIdx, QString::number(nDownloads + 1));

            blogs.removeAt(i); blogs.insert(i, items.join("@"));

            Settings::getInstance().blogs = blogs;
            profileForm->refreshBlogTwt();

            QVector<uint32_t> friendList = core->getFriendList();
            for (int i = 0; i < friendList.length(); i ++)
            {
                const auto& friendPk = FriendList::id2Key(friendList[i]);
                Friend* f = FriendList::findFriend(friendPk);
                if (!f || !Status::isOnline(f->getStatus()))
                    continue;
                friendMessageDispatchers[friendId]->sendMessage(false, SM_BLOG_DOWNLOADCNT_UPDATED +
                                                                         items.join("@"));

            }
        }
    }
    else if(message.startsWith(SM_BLOG_DOWNLOADCNT_UPDATED))
    {
        QMutexLocker{&blogLock};
        QString changedBlog = message.mid(SM_BLOG_DOWNLOADCNT_UPDATED.length());
        QStringList blogs = Settings::getInstance().blogs;
        int i;
        for (i = 0; i < blogs.size(); i ++)
        {
            QStringList item = blogs.at(i).split("@");
            if (item.size() != ProfileForm::usefulColumnCnt)
                continue;
            if (changedBlog.startsWith(item.at(0) + "@" + item.at(1) + "@" + item.at(2)))
                break;
        }
        if (i < blogs.size())
        {
            if (blogs.at(i) != changedBlog) {
                blogs.removeAt(i); blogs.insert(i, changedBlog);
                Settings::getInstance().blogs = blogs;
                profileForm->refreshBlogTwt();
            }
        }
    }
    else if(message.startsWith(SM_BLOG_FILE_DOWNLOAD_FAILED))
    {
       QMessageBox::information(this, "오유", "원천봉사기측에서 오유가 발생하였습니다");
    }

/*************************이부분은 말단에만 있음**********************************/
    else if(message.startsWith(SM_MY_VERSION))
    {
        QString version = message.mid(SM_MY_VERSION.length());
        f->setVersionString(version);
    }
    else if(message.startsWith(SM_GET_VERSION))
    {
        friendMessageDispatchers[friendId]->sendMessage(false, SM_MY_VERSION + PROGRAM_VERSION);
    }
    else if(message.startsWith(SM_PLEASE_UPDATE))
    {
        friendMessageDispatchers[friendId]->sendMessage(false, SM_MY_VERSION + PROGRAM_VERSION);
        QString version = message.mid(SM_PLEASE_UPDATE.length());

        if (versionCompare(version, PROGRAM_VERSION) == 1 && updateDialog == NULL && !isMessageBoxOpen)
        {
            // JYJ 2022.07.20, START 16:40 START
            profileForm->setNewVersionLabel(version);
            // JYJ 2022.07.20, START 16:40 END
            isMessageBoxOpen = true;
            QMessageBox::StandardButton mboxResult =
                    QMessageBox::question(NULL, "확인",
                                          QString("새로운 판본") + version +
                                          QString("이 나왔습니다. 갱신하시겠습니까?\n(바쁘면 후에 다시 가입할때 해도됩니다.)"),
                                          QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
            isMessageBoxOpen = false;
            if (mboxResult == QMessageBox::No) {
                return;
            }
            else {
                friendMessageDispatchers[friendId]->sendMessage(false, SM_REQUEST_UPDATE + PROGRAM_VERSION);
                updateDialog = new UpdateDialog;
                updateDialog->show();
                updateDialog->getProgressBar()->setValue(0);
            }
        }
    }
    else if(message.startsWith(SM_REQUEST_UPDATE))
    {
        QString path = settings.setupFilePath;
        QFile file(path);
        QString fileName = QFileInfo(path).fileName();
        if (!file.exists() || !file.open(QIODevice::ReadOnly))
            friendMessageDispatchers[friendId]->sendMessage(false, SM_UPDATE_FAILED);
        else
        {
            file.close();
            qint64 filesize = file.size();
            core->getCoreFile()->sendFile(f->getId(), fileName, path, filesize, BANGPAE_FILE_KIND_UPDATE_SETUP_FILE);
        }
    }
    else if(message.startsWith(SM_UPDATE_FAILED))
    {
       QMessageBox::information(this, "오유", "원천봉사기측에서 오유가 발생하였습니다. 갱신을 진행할수 없습니다.");
       if (updateDialog)
       {
           updateDialog->close();
           delete updateDialog;
           updateDialog = NULL;
       }
    }
    else if(message.startsWith(SM_SERVER_CANCEL_CALL))
    {
        CoreAV* coreav = core->getAv();
        if (coreav->isCallBusy())
            chatForms[friendId]->onCallTriggered();
    }

/*****************************************************************************/
    // ROC 2022.06.28, 17:38, START
    friendMessageDispatchers[f->getPublicKey()]->onMessageReceived(isAction, strMsgBuf);
    // ROC 2022.06.28, 17:38, END

    // ROC 2022.09.26 START
    //WriteStringToFile("D:/BangPaeDownLoad/roc_debug.txt", "FMR_end\n", "a+");
    // ROC 2022.09.26 END
}


void Widget::onReceiptReceived(int friendId, ReceiptNum receipt)
{
    const auto& friendKey = FriendList::id2Key(friendId); 
    Friend* f = FriendList::findFriend(friendKey);
    if (!f) {
        return;
    }

    friendMessageDispatchers[friendKey]->onReceiptReceived(receipt);
}

void Widget::addFriendDialog(const Friend* frnd, ContentDialog* dialog)
{
    const BangPaePk& friendPk = frnd->getPublicKey();
    ContentDialog* contentDialog = ContentDialogManager::getInstance()->getFriendDialog(friendPk);
    bool isSeparate = settings.getSeparateWindow();
    FriendWidget* widget = friendWidgets[friendPk];
    bool isCurrent = activeChatroomWidget == widget;
    if (!contentDialog && !isSeparate && isCurrent) {
    }

    auto form = chatForms[friendPk];
    auto chatroom = friendChatrooms[friendPk];
    FriendWidget* friendWidget =
        ContentDialogManager::getInstance()->addFriendToDialog(dialog, chatroom, form);

    friendWidget->setStatusMsg(widget->getStatusMsg());

#if (QT_VERSION >= QT_VERSION_CHECK(5, 7, 0))
    auto widgetRemoveFriend = QOverload<const BangPaePk&>::of(&Widget::removeFriend);
#else
    auto widgetRemoveFriend = static_cast<void (Widget::*)(const BangPaePk&)>(&Widget::removeFriend);
#endif
    connect(friendWidget, &FriendWidget::removeFriend, this, widgetRemoveFriend);
    connect(friendWidget, &FriendWidget::middleMouseClicked, dialog,
            [=]() { dialog->removeFriend(friendPk); });
    connect(friendWidget, &FriendWidget::copyFriendIdToClipboard, this,
            &Widget::copyFriendIdToClipboard);
    connect(friendWidget, &FriendWidget::newWindowOpened, this, &Widget::openNewDialog);

    // Signal transmission from the created `friendWidget` (which shown in
    // ContentDialog) to the `widget` (which shown in main widget)
    // FIXME: emit should be removed
    connect(friendWidget, &FriendWidget::contextMenuCalled, widget,
            [=](QContextMenuEvent* event) { emit widget->contextMenuCalled(event); });

    connect(friendWidget, &FriendWidget::chatroomWidgetClicked, [=](GenericChatroomWidget* w) {
        Q_UNUSED(w)
        emit widget->chatroomWidgetClicked(widget);
    });
    connect(friendWidget, &FriendWidget::newWindowOpened, [=](GenericChatroomWidget* w) {
        Q_UNUSED(w)
        emit widget->newWindowOpened(widget);
    });
    // FIXME: emit should be removed
    emit widget->chatroomWidgetClicked(widget);

    Profile* profile = Nexus::getProfile();
    connect(profile, &Profile::friendAvatarSet, friendWidget, &FriendWidget::onAvatarSet);
    connect(profile, &Profile::friendAvatarRemoved, friendWidget, &FriendWidget::onAvatarRemoved);

    QPixmap avatar = Nexus::getProfile()->loadAvatar(frnd->getPublicKey());
    if (!avatar.isNull()) {
        friendWidget->onAvatarSet(frnd->getPublicKey(), avatar);
    }
}

void Widget::addGroupDialog(Group* group, ContentDialog* dialog)
{
    const GroupId& groupId = group->getPersistentId();
    ContentDialog* groupDialog = ContentDialogManager::getInstance()->getGroupDialog(groupId);
    bool separated = settings.getSeparateWindow();
    GroupWidget* widget = groupWidgets[groupId];
    bool isCurrentWindow = activeChatroomWidget == widget;
    if (!groupDialog && !separated && isCurrentWindow) {
    }

    auto chatForm = groupChatForms[groupId].data();
    auto chatroom = groupChatrooms[groupId];
    auto groupWidget =
        ContentDialogManager::getInstance()->addGroupToDialog(dialog, chatroom, chatForm);

#if (QT_VERSION >= QT_VERSION_CHECK(5, 7, 0))
    auto removeGroup = QOverload<const GroupId&>::of(&Widget::removeGroup);
#else
    auto removeGroup = static_cast<void (Widget::*)(const GroupId&)>(&Widget::removeGroup);
#endif
    connect(groupWidget, &GroupWidget::removeGroup, this, removeGroup);
    connect(groupWidget, &GroupWidget::chatroomWidgetClicked, chatForm, &GroupChatForm::focusInput);
    connect(groupWidget, &GroupWidget::middleMouseClicked, dialog,
            [=]() { dialog->removeGroup(groupId); });
    connect(groupWidget, &GroupWidget::chatroomWidgetClicked, chatForm, &ChatForm::focusInput);
    connect(groupWidget, &GroupWidget::newWindowOpened, this, &Widget::openNewDialog);

    // Signal transmission from the created `groupWidget` (which shown in
    // ContentDialog) to the `widget` (which shown in main widget)
    // FIXME: emit should be removed
    connect(groupWidget, &GroupWidget::chatroomWidgetClicked, [=](GenericChatroomWidget* w) {
        Q_UNUSED(w)
        emit widget->chatroomWidgetClicked(widget);
    });

    connect(groupWidget, &GroupWidget::newWindowOpened, [=](GenericChatroomWidget* w) {
        Q_UNUSED(w)
        emit widget->newWindowOpened(widget);
    });

    // FIXME: emit should be removed
    emit widget->chatroomWidgetClicked(widget);
}

bool Widget::newFriendMessageAlert(const BangPaePk& friendId, const QString& text, bool sound, bool file)
{
    bool hasActive;
    QWidget* currentWindow;
    ContentDialog* contentDialog = ContentDialogManager::getInstance()->getFriendDialog(friendId);
    Friend* f = FriendList::findFriend(friendId);

    if (contentDialog != nullptr) {
        currentWindow = contentDialog->window();
        hasActive = ContentDialogManager::getInstance()->isContactActive(friendId);
    } else {
        if (settings.getSeparateWindow() && settings.getShowWindow()) {
            if (settings.getDontGroupWindows()) {
                contentDialog = createContentDialog();
            } else {
                contentDialog = ContentDialogManager::getInstance()->current();
                if (!contentDialog) {
                    contentDialog = createContentDialog();
                }
            }

            addFriendDialog(f, contentDialog);
            currentWindow = contentDialog->window();
            hasActive = ContentDialogManager::getInstance()->isContactActive(friendId);
        } else {
            currentWindow = window();
            FriendWidget* widget = friendWidgets[friendId];
            hasActive = widget == activeChatroomWidget;
        }
    }

    if (newMessageAlert(currentWindow, hasActive, sound)) {
        FriendWidget* widget = friendWidgets[friendId];
        f->setEventFlag(true);
        widget->updateStatusLight();
        ui->friendList->trackWidget(widget);

        if (contentDialog == nullptr) {
            if (hasActive) {
                setWindowTitle(widget->getTitle());
            }
        } else {
            ContentDialogManager::getInstance()->updateFriendStatus(friendId);
        }

        return true;
    }

    return false;
}

bool Widget::newGroupMessageAlert(const GroupId& groupId, const BangPaePk& authorPk,
                                  const QString& message, bool notify)
{
    bool hasActive;
    QWidget* currentWindow;
    ContentDialog* contentDialog = ContentDialogManager::getInstance()->getGroupDialog(groupId);
    Group* g = GroupList::findGroup(groupId);
    GroupWidget* widget = groupWidgets[groupId];

    if (contentDialog != nullptr) {
        currentWindow = contentDialog->window();
        hasActive = ContentDialogManager::getInstance()->isContactActive(groupId);
    } else {
        currentWindow = window();
        hasActive = widget == activeChatroomWidget;
    }

    if (!newMessageAlert(currentWindow, hasActive, true, notify)) {
        return false;
    }

    g->setEventFlag(true);
    widget->updateStatusLight();

    if (contentDialog == nullptr) {
        if (hasActive) {
            setWindowTitle(widget->getTitle());
        }
    } else {
        ContentDialogManager::getInstance()->updateGroupStatus(groupId);
    }

    return true;
}

QString Widget::fromDialogType(DialogType type)
{
    switch (type) {
    case DialogType::AddDialog:
        return tr("Add friend", "title of the window");
    case DialogType::GroupDialog:
        return tr("Group invites", "title of the window");
    case DialogType::TransferDialog:
        return tr("File transfers", "title of the window");
    case DialogType::SettingDialog:
        return tr("Settings", "title of the window");
    case DialogType::ProfileDialog:
        return tr("My profile", "title of the window");
    case DialogType::ReportDialog:
        return tr("보고정형", "title of the window");
    }

    assert(false);
    return QString();
}

bool Widget::newMessageAlert(QWidget* currentWindow, bool isActive, bool sound, bool notify)
{
    bool inactiveWindow = isMinimized() || !currentWindow->isActiveWindow();

    if (!inactiveWindow && isActive) {
        return false;
    }

    if (notify) {
        if (settings.getShowWindow()) {
            currentWindow->show();
        }

        if (settings.getNotify()) {
            if (inactiveWindow) {
#if DESKTOP_NOTIFICATIONS
                if (!settings.getDesktopNotify()) {
                    QApplication::alert(currentWindow);
                }
#else
                QApplication::alert(currentWindow);
#endif
                eventFlag = true;
            }
            bool isBusy = core->getStatus() == Status::Status::Busy;
            bool busySound = settings.getBusySound();
            bool notifySound = settings.getNotifySound();

            CoreAV* coreav = core->getAv();
            bool isPhoneBusy = coreav->isCallBusy();
            if (notifySound && sound && (!isBusy || busySound) && !isPhoneBusy) {
                playNotificationSound(IAudioSink::Sound::NewMessage);
            }
        }
    }

    return true;
}

void Widget::onFileReceiveRequested(const BangPaeFile& file)
{
    if (file.fileKind == BANGPAE_FILE_KIND_UPDATE_EXE_FILE)
        return;
    if (file.fileKind != BANGPAE_FILE_KIND_UPDATE_SETUP_FILE) {

        //If this file is encrypted relayed file, then show the alert on the FriendChatLog of source node.
        BangPaePk friendPk = FriendList::id2Key(file.friendId);
        if (file.fileName.startsWith(SM_FUNCTXT_REQ_FILERLY)) {
            uint8_t pubKeySrc[32], arrResPubKey[32];

            QString strResPubKey = file.fileName.mid(SM_FUNCTXT_REQ_FILERLY.length(), 64);
            QString2Byte(strResPubKey, arrResPubKey);

            //Unmix the long term key of source and destination peer.
            UnmixPubKeyFromRelayFileFormat(pubKeySrc, core->getSelfPublicKey().getData(), arrResPubKey);

             /*If the source friend is in my friend list, then show the Alert on the FriendLogChat of source node,
             * not the relayed node.*/
            if (nullptr != FriendList::findFriend(BangPaePk(pubKeySrc))) {
                friendPk = BangPaePk(pubKeySrc);
            }

            //If I am a is a relayed node, then don't show the Alert.
            else {
                return;
            }
        }

        newFriendMessageAlert(friendPk,
                              file.fileName + " ("
                                  + FileTransferWidget::getHumanReadableSize(file.filesize) + ")",
                              true, true);
    }
    else {
        const BangPaePk& friendPk = FriendList::id2Key(file.friendId);
        newFriendMessageAlert(friendPk,
                              file.fileName + " ("
                                  + FileTransferWidget::getHumanReadableSize(file.filesize) + ")",
                              false, true);
    }
}

void Widget::updateFriendActivity(const Friend& frnd)
{
    const BangPaePk& pk = frnd.getPublicKey();
    const auto oldTime = settings.getFriendActivity(pk);
    const auto newTime = QDateTime::currentDateTime();
    settings.setFriendActivity(pk, newTime);
    FriendWidget* widget = friendWidgets[frnd.getPublicKey()];
    contactListWidget->moveWidget(widget, frnd.getStatus());
    contactListWidget->updateActivityTime(oldTime); // update old category widget
}

void Widget::removeFriend(Friend* f, bool fake)
{
    if (!fake) {
        RemoveFriendDialog ask(this, f);
        ask.exec();

        if (!ask.accepted()) {
            return;
        }

        if (ask.removeHistory()) {
            Nexus::getProfile()->getHistory()->removeFriendHistory(f->getPublicKey());
        }
    }

    const BangPaePk friendPk = f->getPublicKey();
    auto widget = friendWidgets[friendPk];
    widget->setAsInactiveChatroom();
    if (widget == activeChatroomWidget) {
        activeChatroomWidget = nullptr;
        //showProfile();
    }

    friendAlertConnections.remove(friendPk);
    contactListWidget->removeFriendWidget(widget);

    ContentDialog* lastDialog = ContentDialogManager::getInstance()->getFriendDialog(friendPk);
    if (lastDialog != nullptr) {
        lastDialog->removeFriend(friendPk);
    }

    FriendList::removeFriend(friendPk, fake);
    if (!fake) {
        core->removeFriend(f->getId());
        // aliases aren't supported for non-friend peers in groups, revert to basic username
        for (Group* g : GroupList::getAllGroups()) {
            if (g->getPeerList().contains(friendPk)) {
                g->updateUsername(friendPk, f->getUserName());
            }
        }
    }

    friendWidgets.remove(friendPk);
    delete widget;
    auto chatForm = chatForms[friendPk];
    chatForms.remove(friendPk);
    delete chatForm;
    delete f;

    if(!fake){
        if (contentLayout && contentLayout->mainHead->layout()->isEmpty()) {
            showProfile();
        }
        contactListWidget->reDraw();
    }
}

void Widget::removeFriend(const BangPaePk& friendId)
{
    removeFriend(FriendList::findFriend(friendId), false);
}

void Widget::onDialogShown(GenericChatroomWidget* widget)
{
    widget->resetEventFlags();
    widget->updateStatusLight();

    ui->friendList->updateTracking(widget);
    resetIcon();
}

void Widget::onFriendDialogShown(const Friend* f)
{
    onDialogShown(friendWidgets[f->getPublicKey()]);
}

void Widget::onGroupDialogShown(Group* g)
{
    const GroupId& groupId = g->getPersistentId();
    onDialogShown(groupWidgets[groupId]);
}

void Widget::toggleFullscreen()
{
    if (windowState().testFlag(Qt::WindowFullScreen)) {
        setWindowState(windowState() & ~Qt::WindowFullScreen);
    } else {
        setWindowState(windowState() | Qt::WindowFullScreen);
    }
}

void Widget::onUpdateAvailable()
{
    ui->settingsButton->setProperty("update-available", true);
    ui->settingsButton->style()->unpolish(ui->settingsButton);
    ui->settingsButton->style()->polish(ui->settingsButton);
}

ContentDialog* Widget::createContentDialog() const
{
    ContentDialog* contentDialog = new ContentDialog();

    registerContentDialog(*contentDialog);
    return contentDialog;
}

void Widget::registerContentDialog(ContentDialog& contentDialog) const
{
    ContentDialogManager::getInstance()->addContentDialog(contentDialog);
    connect(&contentDialog, &ContentDialog::friendDialogShown, this, &Widget::onFriendDialogShown);
    connect(&contentDialog, &ContentDialog::groupDialogShown, this, &Widget::onGroupDialogShown);
    connect(core, &Core::usernameSet, &contentDialog, &ContentDialog::setUsername);
    connect(&settings, &Settings::groupchatPositionChanged, &contentDialog,
            &ContentDialog::reorderLayouts);
    connect(&contentDialog, &ContentDialog::addFriendDialog, this, &Widget::addFriendDialog);
    connect(&contentDialog, &ContentDialog::addGroupDialog, this, &Widget::addGroupDialog);
    connect(&contentDialog, &ContentDialog::connectFriendWidget, this, &Widget::connectFriendWidget);

}

ContentLayout* Widget::createContentDialog(DialogType type) const
{
    class Dialog : public ActivateDialog
    {
    public:
        explicit Dialog(DialogType type, Settings& settings, Core* core)
            : ActivateDialog(nullptr, Qt::Window)
            , type(type)
            , settings(settings)
            , core{core}
        {
            restoreGeometry(settings.getDialogSettingsGeometry());
            Translator::registerHandler(std::bind(&Dialog::retranslateUi, this), this);
            retranslateUi();
            setWindowIcon(QIcon(":/img/icons/qbangpae.svg"));
            setStyleSheet(Style::getStylesheet("window/general.css"));

            connect(core, &Core::usernameSet, this, &Dialog::retranslateUi);
        }

        ~Dialog()
        {
            Translator::unregister(this);
        }

    public slots:

        void retranslateUi()
        {
            setWindowTitle(core->getUsername() + QStringLiteral(" - ") + Widget::fromDialogType(type));
        }

    protected:
        void resizeEvent(QResizeEvent* event) override
        {
            settings.setDialogSettingsGeometry(saveGeometry());
            QDialog::resizeEvent(event);
        }

        void moveEvent(QMoveEvent* event) override
        {
            settings.setDialogSettingsGeometry(saveGeometry());
            QDialog::moveEvent(event);
        }

    private:
        DialogType type;
        Settings& settings;
        Core* core;
    };

    Dialog* dialog = new Dialog(type, settings, core);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    ContentLayout* contentLayoutDialog = new ContentLayout(dialog);

    dialog->setObjectName("detached");
    dialog->setLayout(contentLayoutDialog);
    dialog->layout()->setMargin(0);
    dialog->layout()->setSpacing(0);
    dialog->setMinimumSize(720, 400);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->show();

    return contentLayoutDialog;
}

void Widget::copyFriendIdToClipboard(const BangPaePk& friendId)
{
    Friend* f = FriendList::findFriend(friendId);
    if (f != nullptr) {
        QClipboard* clipboard = QApplication::clipboard();
        clipboard->setText(friendId.toString(), QClipboard::Clipboard);
    }
}

void Widget::onGroupInviteReceived(const GroupInvite& inviteInfo)
{
    const uint32_t friendId = inviteInfo.getFriendId();
    const BangPaePk& friendPk = FriendList::id2Key(friendId);
    const Friend* f = FriendList::findFriend(friendPk);
    updateFriendActivity(*f);

    const uint8_t confType = inviteInfo.getType();
    if (confType == BANGPAE_CONFERENCE_TYPE_TEXT || confType == BANGPAE_CONFERENCE_TYPE_AV) {
        if (settings.getAutoGroupInvite(f->getPublicKey())) {
            onGroupInviteAccepted(inviteInfo);
        } else {
            if (!groupInviteForm->addGroupInvite(inviteInfo)) {
                return;
            }

            ++unreadGroupInvites;
            groupInvitesUpdate();
            newMessageAlert(window(), isActiveWindow(), true, true);
        }
    } else {
        qWarning() << "onGroupInviteReceived: Unknown groupchat type:" << confType;
        return;
    }
}

void Widget::onGroupInviteAccepted(const GroupInvite& inviteInfo)
{
    const uint32_t groupId = core->joinGroupchat(inviteInfo);
    if (groupId == std::numeric_limits<uint32_t>::max()) {
        qWarning() << "onGroupInviteAccepted: Unable to accept group invite";
        return;
    }
}

// 그룹에서 새통보문을 받을때 처리
void Widget::onGroupMessageReceived(int groupnumber, int peernumber, const QString& message,
                                    bool isAction)
{
    // JYJ 2022.09.24 START
    //WriteStringToFile("D:/BangPaeDownLoad/jyj_debug.txt", "onGroupMessageReceived_1\n", "a+");
    // JYJ 2022.09.24 END
    const GroupId& groupId = GroupList::id2Key(groupnumber);
    assert(GroupList::findGroup(groupId));

    BangPaePk author = core->getGroupPeerPk(groupnumber, peernumber);
    Friend* f = FriendList::findFriend(author);
    groupChatForms[groupId]->pPrivCallFriend = f;

    // JYJ 2022.09.24 START
    //WriteStringToFile("D:/BangPaeDownLoad/jyj_debug.txt", "onGroupMessageReceived_2\n", "a+");
    // JYJ 2022.09.24 END
    QString messageTemp = message;
    // 음성회의 소집신호를 보내왔을때 회의에 이미 참가한 사람은 건너띄도록 설정
    if(message.startsWith(SM_GROUP_AUDIO_RECALL) && groupChatForms[groupId]->inCall == true)
                    messageTemp = SMH;
    bool flag = groupMessageDispatchers[groupId]->onMessageReceived(author, isAction, messageTemp);

    /*--------------------------------------------------------음성회의-----------------------------------------------*/
    // 음성회의 소집신호를 보내왔을때(특수문자렬의 뒤에 회의방식번호가 붙여진다. 1(true): 일반방식, 0(false): 고정열쇠방식)
    if( flag && message.startsWith(SM) )
    {
        if (message.mid(SM.length(), 1) == "0")
        {
            // 고정열쇠방식에서 특수문자렬의 뒤에 따라온 고정열쇠의 하쉬값을 분리해낸다.
            // 회의참가자가 회의참가시 입력하는 암호의 정확성판단에 리용된다.
            bSoundMeetingMode = GRP_MEETINGMODE_STCKEY;
            memcpy(strSoundMeetingPassHash,
                   message.mid(SM.length() + 1).toLocal8Bit().constData(),
                   message.mid(SM.length() + 1).toLocal8Bit().length() );
        } else {
            bSoundMeetingMode = GRP_MEETINGMODE_ONEPEERTABLE;
        }

        // 자기의 그룹회의상태를 "이미참가"상태로 만들고 상태변수들에 대한 설정을 진행한다.
        groupChatForms[groupId].data()->setIncomingButton();
        groupWidgets[groupId]->chatroomWidgetClicked(groupWidgets[groupId]);
        groupIncomingNotification();
    }

    // 어떤 그룹성원이 자기가 음성회의에 새로 참가하였다는 신호를 보내왔을때 (해당회의참가자와 관련한 정보들을 자기 대면부에 갱신)
    else if( flag && message == SM_JOINCALL)
    {
        QString authorPKstr = author.toString();
        groupChatForms[groupId]->addSystemInfoMessage(f->getDisplayedName() + "동지가 음성회의에 참가하였습니다.",
                                                      ChatMessage::INFO, QDateTime::currentDateTime());


        // 이미 참가한 마디점들속에 새로 참가한 마디점이 있는가 판단진행, 존재하지 않는 경우 현시상태 갱신진행
        if(!groupChatForms[groupId]->inCallPeerPKs.contains(authorPKstr)){
            groupChatForms[groupId]->inCallPeerPKs.append(authorPKstr);
            groupChatForms[groupId]->updateUserNames();
        }

        if(groupChatForms[groupId].data()->group->isIMadeGroup()){
            groupChatForms[groupId]->groupCallJoined();
        }

        // 자기가 이미 회의에 참가하는 상태인 경우, SM_IAMJOINEDCALL을 통하여 자기가 이미 회의에 참가중이라는 상태를 그룹에 방송.
        if(groupChatForms[groupId]->inCall)
            groupMessageDispatchers[groupId]->sendMessage(false, SM_GROUP_AUDIO_I_AM_JOINED);
    }

    // 어떤 그룹성원이 자기가 음성회의에 이미 참가하고 있다는 신호를 보내왔을때
    else if( flag && message == SM_GROUP_AUDIO_I_AM_JOINED)
    {
        QString authorPKstr = author.toString();
        if(!groupChatForms[groupId]->inCallPeerPKs.contains(authorPKstr))
        {
            groupChatForms[groupId]->inCallPeerPKs.append(authorPKstr);
            groupChatForms[groupId]->updateUserNames();
        }
    }

    // 지각생음성회의에 참가하라는 허락신호를 보내왔을때(지각생측에서 실행, 자기회의참가상태를 그룹방송, 이미 참가하고 있는 마디점들상태를 자기대면부에 현시)
    else if( flag && message.startsWith(SM_GROUP_AUDIO_RECALL) && groupChatForms[groupId]->inCall == false)
    {
        if (message.mid(SM_GROUP_AUDIO_RECALL.length(), 1) == QString("0"))
        {
            bSoundMeetingMode = GRP_MEETINGMODE_STCKEY;
            memcpy(strSoundMeetingPassHash,
                   message.mid(SM_GROUP_AUDIO_RECALL.length() + 1).toLocal8Bit().constData(),
                   message.mid(SM_GROUP_AUDIO_RECALL.length() + 1).toLocal8Bit().length() );
        } else {
            bSoundMeetingMode = GRP_MEETINGMODE_ONEPEERTABLE;
        }

        groupChatForms[groupId].data()->setIncomingButton();
        groupIncomingNotification();
    }

    // 음성회의 끝마침신호를 보내왔을때, 혹은 그룹보고시 그룹조직자가 보고 다 받고 전화를 놓는 신호를 보내왔을때(그룹보고자측에서 실행)
    else if( flag && message == SM_END)
    {
        // ROC 2022.07.20, 16:00 START
        arrIsExistGroupPrivateCall[groupnumber] = false;
        // ROC 2022.07.20, 16:00 END

        if (groupChatForms[groupId].data()->headWidget->getCallState() == 1)
            return;

        // 그룹회의를 끝내기 위해 상태변수들에 대한 초기화진행. 마이크와 고성기를 끄고 회의참가성원들에 대한 현시상태에 대한 초기화진행.
        groupChatForms[groupId]->bIsInGroupCall = false;
        groupChatForms[groupId].data()->groupCallFinished();
        groupChatForms[groupId]->inCallPeerPKs.clear();
        groupChatForms[groupId].data()->privateCallName = "";
        groupChatForms[groupId]->setIsPrivateCall(false);
        groupChatForms[groupId]->updateUserNames();

        //If the reporter received SM_END, then set the bIsPrivateGroupCalling false.
        bIsPrivateGroupCalling = false;
        bSoundMeetingMode = GRP_MEETINGMODE_ONEPEERTABLE;
    }
    /*--------------------------------------------------------그룹보고-----------------------------------------------*/
    // 그룹보고하려고 호출하는 신호를 그룹보고자가 보내왔을때 (그룹조직자측에서 실행)
    else if (flag && message.startsWith(SM_GROUP_REPORT_CALLING_SERVER)
             && groupChatForms[groupId].data()->group->isIMadeGroup())
    {
        // 그룹조직자가 현재 통화중인경우 바쁨통보문을 보고자에게 전송.
        if (core->getAv()->isCallBusy() ||
            core->getAv()->IsPrivateCalling() ||
            groupChatForms[groupId].data()->headWidget->getCallState() != 1)
        {
            // 상대방의 통화시도를 알리는 통보문현시진행
            groupChatForms[groupId].data()->addSystemInfoMessage(f->getDisplayedName() + " 동지가 개별전화를 시도하였습니다.",
                                                                 ChatMessage::INFO, QDateTime::currentDateTime());
            // 자기의 상태판단에 따라 상대방에게 바쁨상태를 알려준다.
            groupMessageDispatchers[groupId]->sendMessage(false, SM_GROUP_REPORT_BUSY);
            // 전화가 들어왔댔다는 표식을 설정.
            groupWidgets[groupId]->updateStatusLight(2);
        }
        else // 그룹조직자가 바쁨상태가 아닌경우 호출요청접수, 착신음 울림.
        {
            groupChatForms[groupId].data()->addSystemInfoMessage(f->getDisplayedName() + " 동지가 그룹에서 개별전화를 요청합니다.",
                                                                ChatMessage::INFO, QDateTime::currentDateTime());
            // 호출요청문자렬에 속한 특수열쇠문자렬얻기진행 및 열쇠생성진행
            if (f != nullptr  && !Status::isOnline(f->getStatus()) ) {

                uint8_t friendPubKeyData[PUBKEYDATALEN];
                QString2Byte(message.mid(SM_GROUP_REPORT_CALLING_SERVER.length(), PUBKEYDATALEN), friendPubKeyData);

                uint8_t rlt = genSharedKey(friendPubKeyData,
                                      Nexus::getProfile()->getServerED25519LongPubKey(),    //Ppub
                                      Nexus::getProfile()->getSelfED25519LongPrivKey(),     //sA
                                      Nexus::getProfile()->getSelfED25519SessionPrivKey(),  //tA
                                      Nexus::getProfile()->getID(),                          //IDA
                                      f->getId() );

                if( rlt == 4)
                {
                    QMessageBox::critical(nullptr, "서명검증실패", f->getUserName() + "동지에 대한 신분인증이 실패하였습니다. \n\n\
        보안상원칙으로부터 프로그람을 끄겠습니다. \n\
        5분정도 있다가 프로그람을 다시 기동하여 보십시오. \n\
        만일 계속 실패하면 개발자에게 알려주기 바랍니다.");
                    qApp->quit();
                };

                if( rlt > 0 && rlt < 4)
                {
                    QMessageBox::critical(nullptr, "열쇠공유오유", "error:=" + QString("%1").arg(rlt) + "-- 열쇠공유가 실패하였습니다. 프로그람을 재기동하겠습니다.");
                    profileInfo->logout();
                    return;
                };

                // ROC 2022.07.10, 10:00 START
//                qDebug() << "SharedKey:" << Byte2QString(GetSharedKeyFrmFrndId(f->getId()), 32);
                // uint8_t RecvNonce[NONCELEN];
                // QString2Byte(message.mid(SM_GROUP_REPORT_CALLING_SERVER.length() + PUBKEYDATALEN, NONCESTRLEN), RecvNonce);
                // SetRecvNonceFrmFrndId(f->getId(), RecvNonce);
                // ROC 2022.07.10, 10:00 END
            }

            // ROC 2022.07.09, 19:30 START
            /*
            qDebug() << "Group Report Shared Key:"
                     << Byte2QString(GetSharedKeyFrmFrndId(f->getId()), 32) << endl;

            WriteStringToFile("D:/BangPaeDownLoad/GroupReportDebug.txt",
                             (getUsername() + ":" + QDateTime::currentDateTime().toString() + ":" + f->getDisplayedName() + "(그룹보고)").toUtf8().constData(),
                             "a+");
            WriteStringToFile("D:/BangPaeDownLoad/GroupReportDebug.txt", "\n", "a+");
            WriteStringToFile("D:/BangPaeDownLoad/GroupReportDebug.txt",
                             ("Shared Key:" + Byte2QString(GetSharedKeyFrmFrndId(f->getId()), 32)).toLocal8Bit().constData(),
                             "a+");
            WriteStringToFile("D:/BangPaeDownLoad/GroupReportDebug.txt", "\n", "a+");
            */
            // ROC 2022.07.09, 19:30 END

            groupWidgets[groupId]->chatroomWidgetClicked(groupWidgets[groupId]);
            groupChatForms[groupId]->setIncomingButton();
            groupChatForms[groupId]->createPrivateCallConfirm(f->getUserName() + "동지의 개별전화요청");
            groupChatForms[groupId]->privateCallName = f->getDisplayedName();
            playNotificationSound(IAudioSink::Sound::IncomingCall, true);

            nCallPeerFrndId = f->getId();
        }
    }

    // 그룹보고하려고 호출하다가 그룹보고자가 자체중지신호를 보내왔을때(그룹조직자측에서 실행, 착신음을 꺼준다.)
    else if( flag && message.startsWith(SM_GROUP_REPORT_STOP_CALLING_SERVER))
    {
        if (groupChatForms[groupId].data()->bIsPrvCallConfirmCreated)
        {
            groupChatForms[groupId].data()->removePrivateCallConfirm();
            groupChatForms[groupId].data()->onClientStopCalling();
            groupChatForms[groupId].data()->privateCallName = "";
        }
    }

    // 그룹보고하려고 호출하는데 그룹조직자가 접수신호를 보내왔을때 (그룹보고자측에서 실행, 그룹조직자는 호출접수통보문뒤에 그룹보고자의 Alias를 따라 보낸다.
    else if( flag && message.startsWith(SM_GROUP_REPORT_ACCEPT_CALL))
    {
        if (groupChatForms[groupId].data()->bIsPrivateCall)
        {
            if (f != nullptr  && !Status::isOnline(f->getStatus())) {
                uint8_t friendPubKeyData[PUBKEYDATALEN];
                QString2Byte(message.mid(SM_GROUP_REPORT_ACCEPT_CALL.length(), PUBKEYDATALEN), friendPubKeyData);

                uint8_t rlt = genSharedKey(friendPubKeyData,
                                      Nexus::getProfile()->getServerED25519LongPubKey(),    //Ppub
                                      Nexus::getProfile()->getSelfED25519LongPrivKey(),     //sA
                                      Nexus::getProfile()->getSelfED25519SessionPrivKey(),  //tA
                                      Nexus::getProfile()->getID(),                          //IDA
                                      f->getId() );

                if( rlt == 4)
                {
                    QMessageBox::critical(nullptr, "서명검증실패", f->getUserName() + "동지에 대한 신분인증이 실패하였습니다. \n\n\
        보안상원칙으로부터 프로그람을 끄겠습니다. \n\
        5분정도 있다가 프로그람을 다시 기동하여 보십시오. \n\
        만일 계속 실패하면 개발자에게 알려주기 바랍니다.");
                    qApp->quit();
                };

                if( rlt > 0 && rlt < 4)
                {
                    QMessageBox::critical(nullptr, "열쇠공유오유", "error:=" + QString("%1").arg(rlt) + "-- 열쇠공유가 실패하였습니다. 프로그람을 재기동하겠습니다.");
                    profileInfo->logout();
                    return;
                };

                // ROC 2022.07.10, 10:00 START
                // uint8_t RecvNonce[NONCELEN];
                // QString2Byte(message.mid(SM_GROUP_REPORT_ACCEPT_CALL.length() + PUBKEYDATALEN, NONCESTRLEN), RecvNonce);
                // SetRecvNonceFrmFrndId(f->getId(), RecvNonce);
                // ROC 2022.07.10, 10:00 END
            }
            groupChatForms[groupId]->privateCallName = f->getDisplayedName();
            groupChatForms[groupId].data()->updateUserNames();
            groupChatForms[groupId]->groupCallJoined();
            bIsPrivateGroupCalling = true;

            nCallPeerFrndId = f->getId();

            // ROC 2022.07.09, 19:30 START
            /*
            qDebug() << "Group Report Shared Key:"
                     << Byte2QString(GetSharedKeyFrmFrndId(f->getId()), 32) << endl;

            WriteStringToFile("D:/BangPaeDownLoad/GroupReportDebug.txt",
                             (getUsername() + ":" + QDateTime::currentDateTime().toString() + ":" + f->getDisplayedName() + "(그룹보고)").toUtf8().constData(),
                             "a+");
            WriteStringToFile("D:/BangPaeDownLoad/GroupReportDebug.txt", "\n", "a+");
            WriteStringToFile("D:/BangPaeDownLoad/GroupReportDebug.txt",
                             ("Shared Key:" + Byte2QString(GetSharedKeyFrmFrndId(f->getId()), 32)).toLocal8Bit().constData(),
                             "a+");
            WriteStringToFile("D:/BangPaeDownLoad/GroupReportDebug.txt", "\n", "a+");
            */
            // ROC 2022.07.09, 19:30 END
        } else {
            // 자신이 그룹보고자가 아닌경우 그룹보고시의 중계마디점역할을 하게 된다. 따라서 그룹보고상태배렬변수의 값을 설정하게 된다.
            // ROC 2022.07.20, 16:00 START
            // arrIsExistGroupPrivateCall[get_group_id(groupId.getData())] = true;
            arrIsExistGroupPrivateCall[groupnumber] = true;
            // ROC 2022.07.20, 16:00 END
        }
    }

    // 그룹보고하려고 호출하는데 그룹조직자가 바쁨신호를 보내왔을때 (그룹보고자측에서 실행)
    else if( flag && message.startsWith(SM_GROUP_REPORT_BUSY)
             && groupChatForms[groupId].data()->bIsPrivateCall)
    {
        groupChatForms[groupId]->addSystemInfoMessage("그룹관리자는 지금 통화중입니다.",
                                                    ChatMessage::INFO, QDateTime::currentDateTime());
        groupChatForms[groupId]->onClientStopCalling();
        groupChatForms[groupId]->setIsPrivateCall(false);
        BangPaePk pkId;
        FriendBusyNotification(pkId, true);
    }

    // 그룹보고하려고 호출하는데 그룹조직자가 거절신호를 보내왔을때 (그룹보고자측에서 실행, 상대방이 거절하였다는 알림문현시)
    else if (flag && message.startsWith(SM_GROUP_REPORT_REFUSE_PRIVATE_CALL)
             && groupChatForms[groupId].data()->bIsPrivateCall)
    {
        // 알림문을 현시해주는 동시에 음성알림문을 재생시키고 상태변수에 대한 초기화를 진행한다.
        groupChatForms[groupId]->addSystemInfoMessage("상대방이 응답을 거절하였습니다. 후에 다시하십시오.",
                                                      ChatMessage::INFO, QDateTime::currentDateTime());
        groupChatForms[groupId].data()->onClientStopCalling();
        playNotificationSound(IAudioSink::Sound::FriendCallRefused, false);
        groupChatForms[groupId].data()->setIsPrivateCall(false);
    }
    /*--------------------------------------------------------화상회의-----------------------------------------------*/
    // 화상회의 소집신호를 보내왔을때
    else if( flag && message == SM_GROUP_VIDEO_START)
    {
        groupChatForms[groupId].data()->setIncomingButton(true);
        groupWidgets[groupId]->chatroomWidgetClicked(groupWidgets[groupId]);
        groupIncomingNotification();
    }

    // 화상회의 소집신호에 대한 접수신호를 보내왔을때
    else if( flag && message == SM_GROUP_VIDEO_JOIN)
    {
        groupChatForms[groupId]->addSystemInfoMessage(f->getDisplayedName() + "동지가 화상회의에 참가하였습니다.",
                                                      ChatMessage::INFO, QDateTime::currentDateTime());
        QString authorPKstr = author.toString();

        // inCallPeerPks배렬에 보관되여 있는 이미 회의에 참가한 마디점들속에 새로 참가한 마디점이 있는가에 대한 판단진행.
        // 그렇지 않는 경우  현시상태에 대한 갱신을 진행(파란색으로 설정)
        if(!groupChatForms[groupId]->inCallPeerPKs.contains(authorPKstr)){
                groupChatForms[groupId]->inCallPeerPKs.append(authorPKstr);
                groupChatForms[groupId]->updateUserNames();
        }

        if(groupChatForms[groupId].data()->group->isIMadeGroup()){
                groupChatForms[groupId]->groupVideoCallJoined();
        }

        // 자기가 이미 회의에 참가하는 상태인경우 SM_IAMJOINEDCALL을 통하여 자기가 이미 회의에 참가중이라는 상태를 그룹에 방송.
        if(groupChatForms[groupId]->inCall)
                groupMessageDispatchers[groupId]->sendMessage(false, SM_GROUP_AUDIO_I_AM_JOINED);
    }

    // 화상회의 끝마침신호를 보내왔을때(회의탈퇴와 관련한 조작들을 진행)
    else if( flag && message == SM_GROUP_VIDEO_END)
    {
        groupChatForms[groupId].data()->groupVideoCallFinished();
        groupChatForms[groupId]->inCallPeerPKs.clear();
        groupChatForms[groupId]->updateUserNames();
    }

    /*--------------------------------------------------------기타-----------------------------------------------*/
    // 그룹통보문철회신호를 보내왔을때
    else if( message.startsWith(SM_RECALL))
    {
        QString recallMessage = message.mid(SM_RECALL.length());
        groupChatForms[groupId]->recallMessageForGroup(recallMessage, author);
    }
    // JYJ 2022.09.24 START
    //WriteStringToFile("D:/BangPaeDownLoad/jyj_debug.txt", "onGroupMessageReceived_3\n", "a+");
    // JYJ 2022.09.24 END
}

void Widget::groupOutgoingNotification()
{
    // loop until call answered or rejected
    playNotificationSound(IAudioSink::Sound::OutgoingCall, true);
}

void Widget::groupIncomingNotification()
{
    // loop until call answered or rejected
    playNotificationSound(IAudioSink::Sound::GroupIncomingCall, true);
}
void Widget::onGroupPeerlistChanged(uint32_t groupnumber)
{
    const GroupId& groupId = GroupList::id2Key(groupnumber);
    Group* g = GroupList::findGroup(groupId);
    if (nullptr == g) return;
    assert(g);
    g->regeneratePeerList();
}

void Widget::onGroupPeerNameChanged(uint32_t groupnumber, const BangPaePk& peerPk, const QString& newName)
{
    const GroupId& groupId = GroupList::id2Key(groupnumber);
    Group* g = GroupList::findGroup(groupId);
    assert(g);

    const QString setName = FriendList::decideNickname(peerPk, newName);
    g->updateUsername(peerPk, newName);
}

void Widget::onGroupTitleChanged(uint32_t groupnumber, const QString& author, const QString& title)
{
    const GroupId& groupId = GroupList::id2Key(groupnumber);
    Group* g = GroupList::findGroup(groupId);
    assert(g);

    GroupWidget* widget = groupWidgets[groupId];
    if (widget->isActive()) {
        // JYJ 2022.07.24, 16:00 START
        GUI::setWindowTitle(getRealGroupName(title));
        // JYJ 2022.07.24, 16:00 END
    }

    g->setTitle(author, title);
    FilterCriteria filter = getFilterCriteria();
    widget->searchName(ui->searchContactText->text(), filterGroups(filter));
}

void Widget::titleChangedByUser(const QString& title)
{
    const auto* group = qobject_cast<Group*>(sender());
    assert(group != nullptr);
    emit changeGroupTitle(group->getId(), title);
}

void Widget::onGroupPeerAudioPlaying(int groupnumber, BangPaePk peerPk)
{
    const GroupId& groupId = GroupList::id2Key(groupnumber);
    assert(GroupList::findGroup(groupId));

    auto form = groupChatForms[groupId].data();
    form->peerAudioPlaying(peerPk);
}

void Widget::removeGroup(Group* g, bool fake)
{
    const auto& groupId = g->getPersistentId();
    const auto groupnumber = g->getId();
    auto groupWidgetIt = groupWidgets.find(groupId);
    if (groupWidgetIt == groupWidgets.end()) {
        //qWarning()<< "Tried to remove group" << groupnumber << "but GroupWidget doesn't exist";
        return;
    }
    auto widget = groupWidgetIt.value();
    widget->setAsInactiveChatroom();
    if (static_cast<GenericChatroomWidget*>(widget) == activeChatroomWidget) {
        activeChatroomWidget = nullptr;
        showProfile();      
    }

    GroupList::removeGroup(groupId, fake);
    ContentDialog* contentDialog = ContentDialogManager::getInstance()->getGroupDialog(groupId);
    if (contentDialog != nullptr) {
        contentDialog->removeGroup(groupId);
    }

    if (!fake) {
        core->removeGroup(groupnumber);
    }
    contactListWidget->removeGroupWidget(widget); // deletes widget

    groupWidgets.remove(groupId);
    auto groupChatFormIt = groupChatForms.find(groupId);
    if (groupChatFormIt == groupChatForms.end()) {
        //qWarning() << "Tried to remove group" << groupnumber << "but GroupChatForm doesn't exist";
        return;
    }
    groupChatForms.erase(groupChatFormIt);
    delete g;
    if (contentLayout && contentLayout->mainHead->layout()->isEmpty()) {
        showProfile();      
    }

    contactListWidget->reDraw();
}

void Widget::removeGroup(const GroupId& groupId)
{
    removeGroup(GroupList::findGroup(groupId));
}

Group* Widget::createGroup(uint32_t groupnumber, const GroupId& groupId, bool forMe, QString title)
{
    Group* g = GroupList::findGroup(groupId);
    if (g) {
        //qWarning() << "Group already exists";
        return g;
    }
    const auto groupName = "#" + core->getSelfPublicKey().toString() + "#" +
                                tr("Groupchat %1").arg(groupId.toString().left(8));
    const bool enabled = core->getGroupAvEnabled(groupnumber);

    // ROC 2022.06.23 START
    Group* newgroup;
    if (title.isEmpty()) {
        newgroup =
            GroupList::addGroup(groupnumber, groupId, groupName, enabled, core->getUsername(), forMe);
    } else {
        newgroup =
            GroupList::addGroup(groupnumber, groupId, title, enabled, core->getUsername(), forMe);
    }
    // ROC 2022.06.23 END

    auto dialogManager = ContentDialogManager::getInstance();
    auto rawChatroom = new GroupChatroom(newgroup, dialogManager);
    std::shared_ptr<GroupChatroom> chatroom(rawChatroom);

    const auto compact = settings.getCompactLayout();
    auto widget = new GroupWidget(chatroom, compact);
    auto history = Nexus::getProfile()->getHistory();
    auto messageProcessor = MessageProcessor(sharedMessageProcessorParams);
    auto messageDispatcher =
        std::make_shared<GroupMessageDispatcher>(*newgroup, std::move(messageProcessor), *core,
                                                 *core, Settings::getInstance());
    auto groupChatLog = std::make_shared<SessionChatLog>(*core);
    auto groupHistory =
        std::make_shared<GroupHistory>(*newgroup, history, *core, Settings::getInstance(),
                                      *messageDispatcher);
    auto notifyReceivedCallback = [this, groupId](const BangPaePk& author, const Message& message) {
        auto isTargeted = std::any_of(message.metadata.begin(), message.metadata.end(),
                                      [](MessageMetadata metadata) {
                                          return metadata.type == MessageMetadataType::selfMention;
                                      });
        newGroupMessageAlert(groupId, author, message.content,
                             isTargeted || settings.getGroupAlwaysNotify());
    };

    auto notifyReceivedConnection =
        connect(messageDispatcher.get(), &IMessageDispatcher::messageReceived, notifyReceivedCallback);

    assert(core != nullptr);
    auto form = new GroupChatForm(*core, newgroup, *groupHistory, *messageDispatcher, this);
    connect(form, &GroupChatForm::stopNotification, this, &Widget::onStopNotification);
    connect(form, &GroupChatForm::groupOutgoingNotification, this, &Widget::groupOutgoingNotification);
    connect(&settings, &Settings::nameColorsChanged, form, &GenericChatForm::setColorizedNames);
    form->setColorizedNames(settings.getEnableGroupChatsColor());
    groupMessageDispatchers[groupId] = messageDispatcher;
    groupChatLogs[groupId] = groupHistory;
    groupWidgets[groupId] = widget;
    groupChatrooms[groupId] = chatroom;
    groupChatForms[groupId] = QSharedPointer<GroupChatForm>(form);

    contactListWidget->addGroupWidget(widget);
    widget->updateStatusLight();
    contactListWidget->activateWindow();

    connect(widget, &GroupWidget::chatroomWidgetClicked, this, &Widget::onChatroomWidgetClicked);
    connect(widget, &GroupWidget::newWindowOpened, this, &Widget::openNewDialog);
#if (QT_VERSION >= QT_VERSION_CHECK(5, 7, 0))
    auto widgetRemoveGroup = QOverload<const GroupId&>::of(&Widget::removeGroup);
#else
    auto widgetRemoveGroup = static_cast<void (Widget::*)(const GroupId&)>(&Widget::removeGroup);
#endif
    connect(widget, &GroupWidget::removeGroup, this, widgetRemoveGroup);
    connect(widget, &GroupWidget::middleMouseClicked, this, [=]() { removeGroup(groupId); });
    connect(widget, &GroupWidget::chatroomWidgetClicked, form, &ChatForm::focusInput);
    connect(newgroup, &Group::titleChangedByUser, this, &Widget::titleChangedByUser);
    connect(core, &Core::usernameSet, newgroup, &Group::setSelfName);

    FilterCriteria filter = getFilterCriteria();
    widget->searchName(ui->searchContactText->text(), filterGroups(filter));

    return newgroup;
}

void Widget::onEmptyGroupCreated(uint32_t groupnumber, const GroupId& groupId, const QString& title)
{
    bool forMe = true;
    if(!title.isEmpty())
        forMe = title.startsWith("#" + core->getSelfPublicKey().toString() + "#");
    Group* group = createGroup(groupnumber, groupId, forMe, title);

    if (!group) {
        return;
    }
    if (title.isEmpty()) {
        // Only rename group if groups are visible.
        if (groupsVisible()) {
            group->createName();
            groupWidgets[groupId]->editName();
        }
    } else {
        group->setTitle(QString(), title);
    }
}

void Widget::onGroupJoined(int groupId, const GroupId& groupPersistentId)
{
    createGroup(groupId, groupPersistentId, false, "");
}

/**
 * @brief Used to reset the blinking icon.
 */
void Widget::resetIcon()
{
    eventIcon = false;
    eventFlag = false;
    updateIcons();
}

bool Widget::event(QEvent* e)
{
    switch (e->type()) {
    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonDblClick:
        focusChatInput();
        break;
    case QEvent::Paint:
        ui->friendList->updateVisualTracking();
        break;
    case QEvent::WindowActivate:
        if (activeChatroomWidget) {
            activeChatroomWidget->resetEventFlags();
            activeChatroomWidget->updateStatusLight();
            setWindowTitle(activeChatroomWidget->getTitle());
        }

        if (eventFlag) {
            resetIcon();
        }

        focusChatInput();

        break;
    default:
        break;
    }

    return QMainWindow::event(e);
}

void Widget::onUserAwayCheck()
{
#ifdef QBANGPAE_PLATFORM_EXT
    uint32_t autoAwayTime = settings.getAutoAwayTime() * 60 * 1000;
    bool online = static_cast<Status::Status>(ui->statusButton->property("status").toInt())
                  == Status::Status::Online;
    bool away = autoAwayTime && Platform::getIdleTime() >= autoAwayTime;

    if (online && away) {
//        //qDebug() << "auto away activated at" << QTime::currentTime().toString();
        emit statusSet(Status::Status::Away);
        autoAwayActive = true;
    } else if (autoAwayActive && !away) {
//        //qDebug() << "auto away deactivated at" << QTime::currentTime().toString();
        emit statusSet(Status::Status::Online);
        autoAwayActive = false;
    }
#endif
}

void Widget::onEventIconTick()
{
    if (eventFlag) {
        eventIcon ^= true;
        updateIcons();
    }
}

void Widget::onTryCreateTrayIcon()
{
    static int32_t tries = 15;
    if (!icon && tries--) {
        if (QSystemTrayIcon::isSystemTrayAvailable()) {
            icon = std::unique_ptr<QSystemTrayIcon>(new QSystemTrayIcon);
            updateIcons();
            trayMenu = new QMenu(this);

            // adding activate to the top, avoids accidentally clicking quit
            trayMenu->addAction(actionShow);
            trayMenu->addSeparator();
            trayMenu->addAction(statusOnline);
            trayMenu->addAction(statusBusy);
            trayMenu->addSeparator();
            trayMenu->addAction(actionLogout);
            trayMenu->addAction(actionQuit);
            icon->setContextMenu(trayMenu);

            connect(icon.get(), &QSystemTrayIcon::activated, this, &Widget::onIconClick);

            if (settings.getShowSystemTray()) {
                icon->show();
                setHidden(settings.getAutostartInTray());
            } else {
                show();
            }

        } else if (!isVisible()) {
            show();
        }
    } else {
        disconnect(timer, &QTimer::timeout, this, &Widget::onTryCreateTrayIcon);
        if (!icon) {
            //qWarning() << "No system tray detected!";
            show();
        }
    }
}

void Widget::setStatusOnline()
{
    if (!ui->statusButton->isEnabled()) {
        return;
    }

    core->setStatus(Status::Status::Online);
}

void Widget::setStatusAway()
{
    if (!ui->statusButton->isEnabled()) {
        return;
    }

    core->setStatus(Status::Status::Away);
}

void Widget::setStatusBusy()
{
    if (!ui->statusButton->isEnabled()) {
        return;
    }

    core->setStatus(Status::Status::Busy);
}

void Widget::onGroupSendFailed(uint32_t groupnumber)
{
    const auto& groupId = GroupList::id2Key(groupnumber);
    assert(GroupList::findGroup(groupId));

    const auto message = tr("Message failed to send");
    const auto curTime = QDateTime::currentDateTime();
    auto form = groupChatForms[groupId].data();
    form->addSystemInfoMessage(message, ChatMessage::INFO, curTime);
}

void Widget::onFriendTypingChanged(uint32_t friendnumber, bool isTyping)
{
    const auto& friendId = FriendList::id2Key(friendnumber);
    Friend* f = FriendList::findFriend(friendId);
    if (!f) {
        return;
    }

//     qDebug() << "实验5:onFriendTypingChanged(uint32_t friendnumber, bool isTyping)";
    chatForms[friendId]->setFriendTyping(isTyping);
//     qDebug() << "实验6:onFriendTypingChanged(uint32_t friendnumber, bool isTyping)";
}

void Widget::onSetShowSystemTray(bool newValue)
{
    if (icon) {
        icon->setVisible(newValue);
    }
}

void Widget::saveWindowGeometry()
{
    settings.setWindowGeometry(saveGeometry());
    settings.setWindowState(saveState());
}

void Widget::saveSplitterGeometry()
{
    if (!settings.getSeparateWindow()) {
        settings.setSplitterState(ui->mainSplitter->saveState());
    }
}

void Widget::onSplitterMoved(int pos, int index)
{
    Q_UNUSED(pos)
    Q_UNUSED(index)
    saveSplitterGeometry();
}

void Widget::cycleContacts(bool forward)
{
    contactListWidget->cycleContacts(activeChatroomWidget, forward);
}

bool Widget::filterGroups(FilterCriteria index)
{
    switch (index) {
    case FilterCriteria::Offline:
    case FilterCriteria::Friends:
        return true;
    default:
        return false;
    }
}

bool Widget::filterOffline(FilterCriteria index)
{
    switch (index) {
    case FilterCriteria::Online:
    case FilterCriteria::Groups:
        return true;
    default:
        return false;
    }
}

bool Widget::filterOnline(FilterCriteria index)
{
    switch (index) {
    case FilterCriteria::Offline:
    case FilterCriteria::Groups:
        return true;
    default:
        return false;
    }
}

void Widget::clearAllReceipts()
{
    QList<Friend*> frnds = FriendList::getAllFriends();
    for (Friend* f : frnds) {
        friendMessageDispatchers[f->getPublicKey()]->clearOutgoingMessages();
    }
}

void Widget::reloadTheme()
{
    this->setStyleSheet(Style::getStylesheet("window/general.css"));
    QString statusPanelStyle = Style::getStylesheet("window/statusPanel.css");
    ui->tooliconsZone->setStyleSheet(Style::getStylesheet("tooliconsZone/tooliconsZone.css"));
    ui->statusPanel->setStyleSheet(statusPanelStyle);
    ui->statusHead->setStyleSheet(statusPanelStyle);
    ui->friendList->setStyleSheet(Style::getStylesheet("friendList/friendList.css"));
    ui->statusButton->setStyleSheet(Style::getStylesheet("statusButton/statusButton.css"));
    contactListWidget->reDraw();

    profilePicture->setStyleSheet(Style::getStylesheet("window/profile.css"));

    if (contentLayout != nullptr) {
        contentLayout->reloadTheme();
    }

    for (Friend* f : FriendList::getAllFriends()) {
        friendWidgets[f->getPublicKey()]->reloadTheme();
    }

    for (Group* g : GroupList::getAllGroups()) {
        groupWidgets[g->getPersistentId()]->reloadTheme();
    }
    for (auto f : FriendList::getAllFriends()) {
       if(chatForms.contains(f->getPublicKey()))
           chatForms[f->getPublicKey()]->reloadTheme();
    }

    for (auto g : GroupList::getAllGroups()) {
        groupChatForms[g->getPersistentId()]->reloadTheme();
    }
}

void Widget::nextContact()
{
    cycleContacts(true);
}

void Widget::previousContact()
{
    cycleContacts(false);
}

// Preparing needed to set correct size of icons for GTK tray backend
inline QIcon Widget::prepareIcon(QString path, int w, int h)
{
#ifdef Q_OS_LINUX

    QString desktop = getenv("XDG_CURRENT_DESKTOP");
    if (desktop.isEmpty()) {
        desktop = getenv("DESKTOP_SESSION");
    }

    desktop = desktop.toLower();
    if (desktop == "xfce" || desktop.contains("gnome") || desktop == "mate" || desktop == "x-cinnamon") {
        if (w > 0 && h > 0) {
            QSvgRenderer renderer(path);

            QPixmap pm(w, h);
            pm.fill(Qt::transparent);
            QPainter painter(&pm);
            renderer.render(&painter, pm.rect());
            return QIcon(pm);
        }
    }
#endif
    return QIcon(path);
}

void Widget::searchContacts()
{
    QString searchString = ui->searchContactText->text();
    FilterCriteria filter = getFilterCriteria();

    contactListWidget->searchChatrooms(searchString, filterOnline(filter), filterOffline(filter),
                                       filterGroups(filter), false);
    updateFilterText();
    contactListWidget->reDraw();
}

void Widget::changeDisplayMode()
{
    filterDisplayGroup->setEnabled(false);

    if (filterDisplayGroup->checkedAction() == filterDisplayActivity) {
        contactListWidget->setMode(FriendListWidget::SortingMode::Activity);
    } else if (filterDisplayGroup->checkedAction() == filterDisplayName) {
        contactListWidget->setMode(FriendListWidget::SortingMode::Name);
    }

    searchContacts();
    filterDisplayGroup->setEnabled(true);
    updateFilterText();
}

void Widget::updateFilterText()
{
    QString action = filterDisplayGroup->checkedAction()->text();
    QString text = filterGroup->checkedAction()->text();
    text = action + QStringLiteral(" | ") + text;
    ui->searchContactFilterBox->setText(text);
}

Widget::FilterCriteria Widget::getFilterCriteria() const
{
    QAction* checked = filterGroup->checkedAction();

    if (checked == filterOnlineAction)
        return FilterCriteria::Online;
    else if (checked == filterOfflineAction)
        return FilterCriteria::Offline;
    else if (checked == filterFriendsAction)
        return FilterCriteria::Friends;
    else if (checked == filterGroupsAction)
        return FilterCriteria::Groups;

    return FilterCriteria::All;
}

void Widget::searchCircle(CircleWidget& circleWidget)
{
    FilterCriteria filter = getFilterCriteria();
    QString text = ui->searchContactText->text();
    circleWidget.search(text, true, filterOnline(filter), filterOffline(filter));
}

bool Widget::groupsVisible() const
{
    FilterCriteria filter = getFilterCriteria();
    return !filterGroups(filter);
}

void Widget::friendListContextMenu(const QPoint& pos)
{
    QMenu menu(this);
    QAction* createGroupAction = menu.addAction(tr("Create new group..."));
    QAction* addCircleAction = menu.addAction(tr("Add new circle..."));

    if(Nexus::getProfile()->bpxPerm == NORMAL_PERMISSION){
        createGroupAction->setEnabled(false);
    }

    QAction* chosenAction = menu.exec(ui->friendList->mapToGlobal(pos));

    if (chosenAction == addCircleAction) {
        contactListWidget->addCircleWidget();
    } else if (chosenAction == createGroupAction) {
        core->createGroup();
    }
}

void Widget::groupInvitesUpdate()
{
    if (unreadGroupInvites == 0) {
        delete groupInvitesButton;
        groupInvitesButton = nullptr;
    } else if (!groupInvitesButton) {
        groupInvitesButton = new QPushButton(this);
        groupInvitesButton->setObjectName("green");
        ui->statusLayout->insertWidget(2, groupInvitesButton);

        connect(groupInvitesButton, &QPushButton::released, this, &Widget::onGroupClicked);
    }

    if (groupInvitesButton) {
        groupInvitesButton->setText(tr("%n New Group Invite(s)", "", unreadGroupInvites));
    }
}

void Widget::groupInvitesClear()
{
    unreadGroupInvites = 0;
    groupInvitesUpdate();
}



void Widget::setActiveToolMenuButton(ActiveToolMenuButton newActiveButton)
{
    ui->addButton->setChecked(newActiveButton == ActiveToolMenuButton::AddButton);
    ui->addButton->setDisabled(newActiveButton == ActiveToolMenuButton::AddButton);
    ui->groupButton->setChecked(newActiveButton == ActiveToolMenuButton::GroupButton);
    ui->groupButton->setDisabled(newActiveButton == ActiveToolMenuButton::GroupButton);
    ui->settingsButton->setChecked(newActiveButton == ActiveToolMenuButton::SettingButton);
    ui->settingsButton->setDisabled(newActiveButton == ActiveToolMenuButton::SettingButton);
    ui->reportButton->setChecked(newActiveButton == ActiveToolMenuButton::ReportButton);
    ui->reportButton->setDisabled(newActiveButton == ActiveToolMenuButton::ReportButton);
//    ui->setFriendButton->setChecked(newActiveButton == ActiveToolMenuButton::SetFriendButton);
//    ui->setFriendButton->setDisabled(newActiveButton == ActiveToolMenuButton::SetFriendButton);

    ui->TVButton->setChecked(newActiveButton == ActiveToolMenuButton::TVButton);
    ui->TVButton->setDisabled(newActiveButton == ActiveToolMenuButton::TVButton);
    ui->TVBPButton->setChecked(newActiveButton == ActiveToolMenuButton::TVBPButton);
    ui->TVBPButton->setDisabled(newActiveButton == ActiveToolMenuButton::TVBPButton);
}

void Widget::retranslateUi()
{
    ui->retranslateUi(this);
    setUsername(core->getUsername());
    setStatusMessage(core->getStatusMessage());

    filterDisplayName->setText(tr("정렬"));
    filterDisplayActivity->setText(tr("활동으로"));
    filterAllAction->setText(tr("All"));
    filterOnlineAction->setText(tr("Online"));
    filterOfflineAction->setText(tr("Offline"));
    filterFriendsAction->setText(tr("Friends"));
    filterGroupsAction->setText(tr("Groups"));
    setJYJStatus();
    ui->searchContactText->setPlaceholderText(tr("주소록검색"));
    updateFilterText();
    ui->searchContactText->setPlaceholderText(tr("Search Contacts"));

    statusOnline->setText(tr("Online", "Button to set your status to 'Online'"));
    statusAway->setText(tr("Away", "Button to set your status to 'Away'"));
    statusBusy->setText(tr("Busy", "Button to set your status to 'Busy'"));
    actionLogout->setText(tr("Logout", "Tray action menu to logout user"));
    actionQuit->setText(tr("Exit", "Tray action menu to exit BangPae"));
    actionShow->setText(tr("Show", "Tray action menu to show qBangPae window"));

    if (!settings.getSeparateWindow() && (settingsWidget && settingsWidget->isShown())) {
        setWindowTitle(fromDialogType(DialogType::SettingDialog));
    }
    groupInvitesUpdate();

}

void Widget::focusChatInput()
{
    if (activeChatroomWidget) {
        if (const Friend* f = activeChatroomWidget->getFriend()) {
            chatForms[f->getPublicKey()]->focusInput();
        } else if (Group* g = activeChatroomWidget->getGroup()) {
            groupChatForms[g->getPersistentId()]->focusInput();
        }
    }
}

void Widget::refreshPeerListsLocal(const QString& username)
{
    for (Group* g : GroupList::getAllGroups()) {
        g->updateUsername(core->getSelfPublicKey(), username);
    }
}

void Widget::connectCircleWidget(CircleWidget& circleWidget)
{
    connect(&circleWidget, &CircleWidget::searchCircle, this, &Widget::searchCircle);
    connect(&circleWidget, &CircleWidget::newContentDialog, this, &Widget::registerContentDialog);
}

void Widget::connectFriendWidget(FriendWidget& friendWidget)
{
    connect(&friendWidget, &FriendWidget::searchCircle, this, &Widget::searchCircle);
    connect(&friendWidget, &FriendWidget::updateFriendActivity, this, &Widget::updateFriendActivity);
}

void Widget::InitInAudioDev() {
    settingsWidget.data()->InitInAudioDev();
}

void Widget::onGroupPeerVideoReceived(int groupnumber, BangPaePk peerPk, QImage img)
{
    const GroupId& groupId = GroupList::id2Key(groupnumber);
    assert(GroupList::findGroup(groupId));

    auto form = groupChatForms[groupId].data();
    form->peerVideoReceived(peerPk, img);
}

void Widget::removeBlackMan(QStringList blackPbList)
{
    int i;
    for (i = 0; i < blackPbList.size(); i ++)
    {

        QByteArray byte_arr;

        if(blackPbList[i].size() % 2 != 0)
            continue;
        bool ok;
        for(int j = 0; j < blackPbList[i].size() ; j += 2)
             byte_arr.append(char(blackPbList[i].mid(j, 2).toUShort(&ok, 16)));

        BangPaePk bangpaePk(byte_arr);
        friendWidgets[bangpaePk]->setBlockStatus();
    }
}

void Widget::onUpdateClicked()
{
    if (settings.getSeparateWindow()) {
        if (!updateForm->isShown()) {
            updateForm->show(createContentDialog(DialogType::AddDialog));
        }

        setActiveToolMenuButton(ActiveToolMenuButton::None);
    } else {
        hideMainForms(nullptr);
        updateForm->show(contentLayout);
        setWindowTitle("갱신목록");
        setActiveToolMenuButton(ActiveToolMenuButton::AddButton);
    }
}

void Widget::onReportClicked() {

    if (settings.getSeparateWindow()) {
        QMessageBox::information(NULL, "1", "1");

        if (!reportForm->isShown()) {
            reportForm->show(createContentDialog(DialogType::ReportDialog));
        }

        setActiveToolMenuButton(ActiveToolMenuButton::None);
    } else {
        hideMainForms(nullptr);
        reportForm->show(contentLayout);
        setWindowTitle(fromDialogType(DialogType::ReportDialog));
        setActiveToolMenuButton(ActiveToolMenuButton::ReportButton);
    }
}

void Widget::FriendBusyNotification(BangPaePk friendId, bool bIsFromGroup)
{
    playNotificationSound(IAudioSink::Sound::FriendCallBusy, false);
    if (!bIsFromGroup) {
        const auto curTime = QDateTime::currentDateTime();
        chatForms[friendId]->addSystemInfoMessage("현재 상대방은 통화중입니다. 후에 다시하십시오.", ChatMessage::INFO, curTime);
    }
}

void Widget::FriendRefusedNotification(BangPaePk friendId)
{
    playNotificationSound(IAudioSink::Sound::FriendCallRefused, false);
    const auto curTime = QDateTime::currentDateTime();
    chatForms[friendId]->addSystemInfoMessage("상대방이 응답을 거절하였습니다. 후에 다시하십시오.", ChatMessage::INFO, curTime);
}

void Widget::addFriend(uint32_t friendId, const BangPaePk& friendPk)
{
    settings.updateFriendAddress(friendPk.toString());

    Friend* newfriend = FriendList::addFriend(friendId, friendPk);
    QString displayName = newfriend->getDisplayedName();
    int firstIdx = displayName.indexOf(".") + 1;
    int endIdx = displayName.indexOf("(");
    if (firstIdx >=1 && endIdx >= 0 && endIdx >= firstIdx)
        displayName = displayName.mid(firstIdx, endIdx-firstIdx);

    displayName = displayName + "동지 :";
    name2intMap[displayName] = friendId;
    auto dialogManager = ContentDialogManager::getInstance();

    auto rawChatroom = new FriendChatroom(newfriend, dialogManager);
    std::shared_ptr<FriendChatroom> chatroom(rawChatroom);
    const auto compact = settings.getCompactLayout();

    auto widget = new FriendWidget(chatroom, compact, this);
    connect(widget, &FriendWidget::addBlackMan, settingsWidget, &SettingsWidget::addBlackMan);
    connectFriendWidget(*widget);
    friendWidgets[friendPk] = widget;
    friendChatrooms[friendPk] = chatroom;

    const auto activityTime = settings.getFriendActivity(friendPk);


    contactListWidget->addFriendWidget(widget, Status::Status::Offline,
                                       settings.getFriendCircleID(friendPk));


    auto notifyReceivedCallback = [this, friendPk](const BangPaePk& author, const Message& message) {
        newFriendMessageAlert(friendPk, message.content);
    };

    connect(newfriend, &Friend::aliasChanged, this, &Widget::onFriendAliasChanged);
    connect(newfriend, &Friend::displayedNameChanged, this, &Widget::onFriendDisplayedNameChanged);
    connect(widget, &FriendWidget::chatroomWidgetClicked, this, &Widget::onChatroomWidgetClicked);
    connect(widget, &FriendWidget::copyFriendIdToClipboard, this, &Widget::copyFriendIdToClipboard);
    connect(widget, &FriendWidget::contextMenuCalled, widget, &FriendWidget::onContextMenuCalled);
    connect(widget, SIGNAL(removeFriend(const BangPaePk&)), this, SLOT(removeFriend(const BangPaePk&)));

    Profile* profile = Nexus::getProfile();
    connect(profile, &Profile::friendAvatarSet, widget, &FriendWidget::onAvatarSet);
    connect(profile, &Profile::friendAvatarRemoved, widget, &FriendWidget::onAvatarRemoved);

    // Try to get the avatar from the cache
    QPixmap avatar = Nexus::getProfile()->loadAvatar(friendPk);
    if (!avatar.isNull()) {
        widget->onAvatarSet(friendPk, avatar);
    }


    FilterCriteria filter = getFilterCriteria();
    widget->search(ui->searchContactText->text(), filterOffline(filter));
}

void Widget::addFriendChatForm(const BangPaePk& friendPk)
{
    Friend* newfriend = FriendList::findFriend(friendPk);

    auto widget = friendWidgets[friendPk];

    auto history = Nexus::getProfile()->getHistory();

    auto messageProcessor = MessageProcessor(sharedMessageProcessorParams);
    auto friendMessageDispatcher =
        std::make_shared<FriendMessageDispatcher>(*newfriend, std::move(messageProcessor), *core);

    // Note: We do not have to connect the message dispatcher signals since
    // ChatHistory hooks them up in a very specific order
    auto chatHistory =
        std::make_shared<ChatHistory>(*newfriend, history, *core, Settings::getInstance(),
                                      *friendMessageDispatcher);
    auto friendForm = new ChatForm(profile, newfriend, *chatHistory, *friendMessageDispatcher, this);
    connect(friendForm, &ChatForm::updateFriendActivity, this, &Widget::updateFriendActivity);

    friendMessageDispatchers[friendPk] = friendMessageDispatcher;
    friendChatLogs[friendPk] = chatHistory;
    chatForms[friendPk] = friendForm;


    const auto activityTime = settings.getFriendActivity(friendPk);
    const auto chatTime = friendForm->getLatestTime();
    if (chatTime > activityTime && chatTime.isValid()) {
        settings.setFriendActivity(friendPk, chatTime);
    }

    auto notifyReceivedCallback = [this, friendPk](const BangPaePk& author, const Message& message) {
        newFriendMessageAlert(friendPk, message.content);
    };

    auto notifyReceivedConnection =
        connect(friendMessageDispatcher.get(), &IMessageDispatcher::messageReceived,
                notifyReceivedCallback);

    friendAlertConnections.insert(friendPk, notifyReceivedConnection);

    connect(friendForm, &ChatForm::newFriendAVAlert, this, &Widget::onNewFriendAVAlert);
    connect(friendForm, &ChatForm::incomingNotification, this, &Widget::incomingNotification);
    connect(friendForm, &ChatForm::outgoingNotification, this, &Widget::outgoingNotification);
    connect(friendForm, &ChatForm::nobodyAnswerNotification, this, &Widget::onNobodyAnswerNotification);
    connect(friendForm, &ChatForm::stopNotification, this, &Widget::onStopNotification);
    connect(friendForm, &ChatForm::rejectCall, this, &Widget::onRejectCall);

    // Try to get the avatar from the cache
    QPixmap avatar = Nexus::getProfile()->loadAvatar(friendPk);
    if (!avatar.isNull()) {
        friendForm->onAvatarChanged(friendPk, avatar);
        widget->onAvatarSet(friendPk, avatar);
    }

    if(core->isFriendOnline(newfriend->getId()))
    {
        auto status = newfriend->getStatus();
        newfriend->setStatus(Status::Status::Online);
        friendForm->onFriendStatusChanged(newfriend->getId(), Status::Status::Online);
        newfriend->setStatus(status);
    }
}

void Widget::addFriendFailed(const BangPaePk&, const QString& errorInfo)
{
    QString info = QString(tr("친구요청을 보낼수 없습니다."));
    if (!errorInfo.isEmpty()) {
        info = info + QStringLiteral(": ") + errorInfo;
    }

    QMessageBox::critical(nullptr, "오유", info);
}

void Widget::setJYJStatus()
{
    if (core == nullptr)
        return;

    // 주소록의 웃쪽에 현재 활동중이 사용자수량현시진행.
    QVector<uint32_t> friendList = core->getFriendList();

    int i, cnt = 0;
    for (i = 0; i < friendList.size(); i ++)
    {
        const auto& friendPk = FriendList::id2Key(friendList[i]);
        Friend* f = FriendList::findFriend(friendPk);
        if(f && Status::isOnline(f->getStatus()))
        {
            cnt ++;
        }
    }

    ui->jyjStatusLabel->setText(QString("현재사용자:") + QString::number(cnt)
                                + "/" + QString::number(friendList.size()));
}
void Widget::onDisplayTime()
{
    ui->timeLabel->setText(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"));
}

void Widget::onContactChangeColor()
{
    settingsWidget->setWidgetThemeByInt(rand() % 8);
}

void Widget::onDisplayPoster()
{
    // ROC 2022.09.24 START
    //WriteStringToFile("D:/BangPaeDownLoad/roc_debug.txt", "DP_bgn\n", "a+");
    // ROC 2022.09.24 END
    
    do {
        nPosterId = (rand() % nPosterCnt) + 1;
    } while (nPosterId == nPrvPosterId);

    if (!QDir{}.exists(strWorkDirPath)){
        strWorkDirPath = generateRandTmpDirPath();
    }

    if (chatForms.values().count() > 0)
        chatForms.values()[0]->SetPoster();
    if (groupChatForms.values().count() > 0)
        groupChatForms.values()[0]->SetPoster();

    // ROC 2022.09.24 START
    //WriteStringToFile("D:/BangPaeDownLoad/roc_debug.txt", "DP_end\n", "a+");
    // ROC 2022.09.24 END
}

int Widget::versionCompare(QString versionOne, QString versionTwo)
{
    QStringList listOne = versionOne.split(".");
    QStringList listTwo = versionTwo.split(".");
    int i;
    for (i = 0; i < listOne.size() && i < listTwo.size(); i ++)
    {
        int numberOne = listOne[i].toInt();
        int numberTwo = listTwo[i].toInt();
        if (numberOne > numberTwo)
            return 1;
        if (numberOne < numberTwo)
            return -1;
    }
    if (listOne.size() == listTwo.size())
        return 0;
    if (listOne.size() > listTwo.size())
        return 1;
    return -1;
}

void Widget::setProgress(uint32_t friendId, int value)
{
    if (updateForm)
        updateForm->setProgress(friendId, value);
}


void Widget::onSendNewVerToAllUsers(QVector<uint32_t> selectedFriends)
{
    int i;

    for (i = 0; i < selectedFriends.size(); i ++)
    {
        const auto& friendPk = FriendList::id2Key(selectedFriends[i]);
        Friend* f = FriendList::findFriend(friendPk);
        if(f && Status::isOnline(f->getStatus()))
        {
            friendMessageDispatchers[f->getPublicKey()]->sendMessage(false, SM_PLEASE_UPDATE + PROGRAM_VERSION);
        }
    }
}

void Widget::onGetAllUserAppVersions(QVector<uint32_t> selectedFriends)
{
    int i;

    for (i = 0; i < selectedFriends.size(); i ++)
    {
        const auto& friendPk = FriendList::id2Key(selectedFriends[i]);
        Friend* f = FriendList::findFriend(friendPk);
        if(f && Status::isOnline(f->getStatus()))
        {
            friendMessageDispatchers[f->getPublicKey()]->sendMessage(false, SM_GET_VERSION);
        }
    }
}

void Widget::onViewKCTV()
{
    if(pTVProcess == NULL){
        pTVProcess=new QProcess(this);
    }

    QStringList param;
    param << PROGRAMM_NAME << PROGRAMM_DEVELOP_AGENCY << strWorkDirPath;

    QFile tvTmpfile(KCTVPATH32);
    if (tvTmpfile.exists())
        pTVProcess->start(QString("\"") + KCTVPATH32 + "\"", param);
    else
        pTVProcess->start(QString("\"") + KCTVPATH64 + "\"", param);

    setActiveToolMenuButton(ActiveToolMenuButton::None);
}


void Widget::onViewBPDFile()
{
    setActiveToolMenuButton(ActiveToolMenuButton::None);

    QString filePath = QFileDialog::getOpenFileName(this, "열람할 파일을 선택하여 주십시오.",
                                                                Settings::getInstance().getGlobalAutoAcceptDir(), "*.bpd");
    if (filePath != "" && QFileInfo(filePath).birthTime().secsTo(QDateTime::currentDateTime()) <= BPD_DEADLINE_MILLISECONDS) {

        // Check if the free space of all dirves.
        QString strDrvPath = "";
        for (QFileInfo info : QDir::drives()) {
            if (QStorageInfo{info.absolutePath()}.bytesFree() > QFileInfo{filePath}.size()) {
                strDrvPath = info.absolutePath();
                break;
            }
        }

        if (strDrvPath == "") {
            QMessageBox::information(NULL, "용량초과통보", "기억공간이 부족하므로 파일을 열수 없습니다. 기억기청소를 진행하여주십시오.",
                                     QMessageBox::Ok);
            return;
        }

        // ROC 2022.09.24 START
        QString strTmpWorkingDirPath = strWorkDirPath;
        if(!DecVidFile(filePath, strTmpWorkingDirPath, Nexus::getProfile()->getStrMachineNum()))
             return;
        // ROC 2022.09.24 END

        if(pTVProcess == NULL) {
            pTVProcess=new QProcess(this);
        }
        QStringList lstParam;

        // 1st param: *.pdf file path
        // 2st param:  *.bpd file path
        lstParam.append(strTmpWorkingDirPath);
        lstParam.append( filePath );

        QFile tvTmpfile(KCTVPATH32);
        if (tvTmpfile.exists())
            pTVProcess->start(QString("\"") + KCTVPATH32 + "\"", lstParam);
        else
            pTVProcess->start(QString("\"") + KCTVPATH64 + "\"", lstParam);
    }
    else if (filePath != "") {
        SafeDelFile(filePath);
        QMessageBox::information(NULL, "기한초과통보", "기한이 지났으므로 파일은 자동적으로 삭제됩니다.", QMessageBox::Ok);
    }

}

void Widget::SafeDelFile(QString strFilePath) {
    QFile file(strFilePath);
    if (file.isOpen()) {
        file.close();
    }

    QTimer::singleShot(TIME_SAFE_DEL_INTERVAL * 1000, this, [this, strFilePath]() {
        bool res = QFile::remove(strFilePath);
        if (!res) {
            this->SafeDelFile(strFilePath);
        }
    });
}


void Widget::onQuitApp() {
    qApp->quit();
}


void Widget::removeFriend_jyj(Friend* f)
{

    // 주소록창문에서의 친구삭제진행
    const BangPaePk friendPk = f->getPublicKey();   // 09.23
    auto widget = friendWidgets[friendPk];
    widget->setAsInactiveChatroom();
    if (widget == activeChatroomWidget) {
        activeChatroomWidget = nullptr;
        showProfile();
    }

    // friendAlertConnections.remove(friendPk);
    contactListWidget->removeFriendWidget(widget);

    ContentDialog* lastDialog = ContentDialogManager::getInstance()->getFriendDialog(friendPk);
    if (lastDialog != nullptr) {
        lastDialog->removeFriend(friendPk);
    }

    // Core에서의 친구삭제진행.
    FriendList::removeFriend(friendPk, false);

    core->removeFriend(f->getId());
    // aliases aren't supported for non-friend peers in groups, revert to basic username
    for (Group* g : GroupList::getAllGroups()) {
        if (g->getPeerList().contains(friendPk)) {
            g->updateUsername(friendPk, f->getUserName());
        }
    }

    friendWidgets.remove(friendPk);
    delete widget;

    // 친구대화칸객체 삭제진행
    auto chatForm = chatForms[friendPk];
    chatForms.remove(friendPk);
    delete chatForm;

    delete f;
    if (contentLayout && contentLayout->mainHead->layout()->isEmpty()) {
        showProfile();
    }

    contactListWidget->reDraw();
}
