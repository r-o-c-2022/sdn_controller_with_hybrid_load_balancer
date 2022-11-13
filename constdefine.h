#ifndef CONSTDEFINE_H
#define CONSTDEFINE_H

#include <QObject>
#include "sodium.h"

#define PROGRAMM_NAME                           "BANGPAE"
#define PROGRAMM_DEVELOP_AGENCY                 "BUPT"
#define WORK_DIR                                "\\AppData\\Local\\Temp\\"
#define IMAGE_DIR                               "images"
#define WORK_DIR_PREFIX                         "BPTMP"                 // 림시등록부이름앞붙이
#define KCTVPATH64                              "C:\\Program Files\\KCTV\\KCTV.exe"
#define KCTVPATH32                              "C:\\Program Files (x86)\\KCTV\\KCTV.exe"
#define DISPLAY_TIME_INTERVAL                   1000                    // 시계현시간격

#define CHUNK_SIZE                              8192                    //size of block for En/Decrypt
#define PUBKEYDATALEN                           320
#define NONCELEN                                24
#define NONCESTRLEN                             48
#define OUR_MAX_CRYPTO_PACKET_SIZE              1400
#define PASS_HASH_LEN                           32

#define NORMAL_PERMISSION                       0                       // 일반권한
#define LEADER_PERMISSION                       1                       // 지구책임자, 직맹위원장, 청년동맹위원장권한
#define SMALL_LEADER_PERMISSION                 2                       // 단위책임자 권한
#define ADMIN_PERMISSION                        3                       // 관리자권한
#define DEVELOPER_PERMISSION                    4                       // 개발자권한

#define NUM_TOTAL_UNIVS                         25                      //최대기관수
#define NUM_PEOLPES_UNIV                        50                      //최대기관인원수
#define NUM_TOTAL_PEOPLES                       NUM_TOTAL_UNIVS * NUM_PEOLPES_UNIV//명단자료전체인원수

#define GROUP_NAME_MIN_SIZE                     65                      // 그룹이름최소길이
#define GROUP_NAME_MIN_SIZE_1                   66                      // 그룹이름최소길이


#define RAND_DEC_DIR_NAME_LEN                    8
#define EMO_ENCDATA_DIR                         "Emojione"              // 암호화된 그림기호보관등록부이름
#define EMO_ENCDATA_PREFIX_LEN                  4                       // emotion자료암호화 앞붙이길이
#define EMO_ENCDATA_CONSTANT                    0x55                    // emotion자료암호화용상수
#define EMO_DECCACHE_DIR                        "decemo"                // 복호화된 그림기호보관등록부이름
#define EMO_NUM_COL                             9                       // 그림기호배렬렬수
#define EMO_NUM_ROW                             9                       // 그림기호배렬행수
#define EMO_NUM                                 EMO_NUM_COL*EMO_NUM_ROW // 그림기호수
#define EMO_SIZE                                45                      // 그림기호크기
#define SEND_IMAGE_ENCDATA_CONSTANT             0xA9                    // 보낸 그림자료암호화용상수
#define TYPE_OUR_PHONE_BELL                     2
#define NUM_OUR_PHONE_BELL                      7
#define NUM_ORIGINAL_PHONE_BELL                 9

#define SMLDIS_SCR_WIDTH                        1680                    // 축소방식에서의 화면넓이
#define COTRACT_MIN_WIDTH                       775                     // 주소록의 최소넓이
#define NMLDIS_CONTRACT_MAX_WIDTH               350                     // 일반화면방식에서의 주소록최대넓이
#define HEAD_LAYOUT_SPACING                     5                       // 머리부에서 Layout내에서의 Widget들사이 간격
#define MIC_BUTTONS_LAYOUT_SPACING              4                       // 마이크단추사이간격
#define BUTTONS_LAYOUT_HOR_SPACING              4                       // 단추들의 수평간격
#define BIG_WIDHEI_RATE                         10                      // 큰 선전화의 너비높이비률 1200.0 / 120.0;
#define SMA_WIDHEI_RATE                         9.09                    // 작은 선전화의 너비높이비률 800.0 / 88.0;

#define CIRCLE_CNT_OFFSET                       10                      // 기관모임편차
#define BANGPAE_FILE_KIND_UPDATE_EXE_FILE       9                       // 갱신파일종류번호
#define BANGPAE_FILE_KIND_UPDATE_SETUP_FILE     3                       // 설치판갱신파일종류번호
#define BANGPAE_FILE_KIND_UAF_FILE              4                       // 주소록갱신파일종류번호
#define UAF_RECORD_SEPERATOR_NUM                4                       // 주소록갱신파일 :에의한 분류개수

#define MAX_BLOG_RECORD_CNT                     19                      //게시판에 있는 자료 최대내리적재기록수
#define BPD_DEADLINE_MILLISECONDS               21600
#define RELAY_TEXT_BUF_LEN                      3                       //통보문중계전송완충기크기
#define NUM_MAX_RLYFILE_ENCTHREAD               20

#define FILENAME_CONTAIN_MATCH_MODE              0                      // 파일이름포함방식맞추기
#define FILENAME_ENDSWITH_MATCH_MODE             1                      // 파일이름끝나기방식맞추기
#define FILENAME_STARTSWITH_MATCH_MODE           2                      // 파일이름시작방식맞추기

#define TIME_POSTER_FADE_IN                     3                       // 선전화가 현시, 비현시되는 동화시간
#define TIME_POSTER_UPDATE_INTERVAL             60                      // 선전화사이의 현시시간간격
#define TIME_TXTRLY_WAITING                     1                       // 본문중계전송기다림시간
#define TIME_FILELY_WAITING                     5                       // 파일중계전송기다림시간
#define TIME_SAFE_DEL_INTERVAL                  1                       // 파일완전지우기시간간격
#define TIME_LOGIN_AUTO_EXIT                    60                      // Login창문현시시간
#define TIME_POSTER_SHOW                        30                      // 선전화현시시간간격
#define TIME_AUTO_LOGIN                         3600                     // 프로그람자동탈퇴시간
#define TIME_AUTO_LOGIN_MSG_SHOW                2                       // 프로그람탈퇴통보문현시후 없어지는 시간
#define TIME_CONTACTLIST_COLOR_CHANGE           1200                    // 주소록배경색변경시간
#define TIME_CALL                               1                       // 1대1통화후 단추활성화시간 (통화끝난후 다시 호출방지)
#define TIME_RELOAD_CHAT                        2                       // 통보문철회후 기록재적재시간
#define TIME_ENABLE_BLOG_REFRESH_BUTTON         2                       // 게시판내용투고후 투고단추활성화시간
#define TIME_BLOG_START                         2                       // 프로그람기동하여 게시판렬간격변경시간
#define TIME_REFRESH_UPDATE_TABLE               3                       // 갱신창문에서 판본얻은후 표갱신시간
#define TIME_UPDATE_MESSAGE                     3                       // 련결후 개발자가 갱신통보문 보내는 시간
#define TIME_BLOG_MESSAGE                       3                       // 련결후 게시판자료요구통보문 보내는 시간


#define CHATBOX_HEIGHT                          200                     // 통보문입력창높이
#define FILENAME_PART_KCTV                      "KCTV"                  // 텔레비죤관련프로그람삭제를 위한 문자렬
#define DARK_THEME_IDX                          8

#define STR_BOSS                                "지구책임자"
#define STR_WORKER_BOSS                         "직맹위원장"
#define STR_YOUTH_BOSS                          "청년동맹위원장"
#define STR_UNIT_BOSS                           "단위책임자"

#define STR_RESEARCHER                          "연구생"
#define STR_INTERN                              "실습생"
#define STR_STUDENT                             "학생"

#define BLOG_ADMINISTRATOR                      "관리자"                  // 게시판투고자
#define BLOG_BOSS                               STR_BOSS                // 게시판투고자 : 지구책임자
#define BLOG_WORKER_BOSS                        STR_WORKER_BOSS         // 게시판투고자 : 직맹위원장
#define BLOG_YOUTH_BOSS                         STR_YOUTH_BOSS          // 게시판투고자 : 청년동맹위원장
#define BLOG_DEVELOPER                          "개발자"                 // 게시판투고자

#define BLOG_PERMISSION_NORMAL                  "일반"                   // 게시판권한 : 일반
#define BLOG_PERMISSION_UNNORMAL                STR_UNIT_BOSS           // 게시판권한 : 단위책임자

#define BLOG_MODE_NORMAL                        "일반적재"                // 게시판적재방식
#define BLOG_MODE_UNNORMAL                      "방패전용"                // 게시판적재방식

#define BLOG_STATUS_ACCEPT                      "승인함"                  // 게시판내용상태
#define BLOG_STATUS_UNDEFINED                   "미정"                    // 게시판내용상태
#define BLOG_STATUS_REJECT                      "부결"                    // 게시판내용상태

static const QStringList BLOG_TYPE_STRS = {"포치", "갱신","연구자료", "학습자료", "상식자료", "도움요청", "요청회답"}; //게시판류형

#define REPORT_TEMPLATE_FILE_NAME               "RecordSample.sam"      // 보고기록파일 Template
#define TO_ENC_BYTES                            3                       // 림시그림파일 암호화할 바이트수

static const QByteArray IMAGE_SUFFIX("Encrypted");                      // 암호된 그림파일 뒤붙이
static constexpr int SCREENSHOT_GRABBER_OPENING_DELAY = 500;            //화면캡쳐단추열림지연시간 :
static constexpr int CHAT_WIDGET_MIN_HEIGHT = 50;                       // 본문입력칸의 최소높이설정
static constexpr int TYPING_NOTIFICATION_DURATION = 3000;               // 상대방의 본문입력상태에 대한 알림시간간격
static const QString ACTION_PREFIX = QStringLiteral("/me ");

static const QString UPDATE_FILE_PREFIX = "BangPae-Client-";
static const QString UPDATE_FILE_SUFFIX = ".exe";
static const QString BANGPAE_EXT = ".bpx";
static const QString UAF_VESION = "UAFVersion2";
static const char BPX_CONTENT_HEADER[] = "BangPaeProfileVer5";
static const QByteArray TOTAL_BPX_HEADER = "TotalBPX2.0";
static const uint8_t ADD_ENDEC_DATA1[37] = "RYONGNAM-UNIV-MATH-KUMSONG2-BUPT-5.0";
static const unsigned long long ADD_ENDEC_DATA1_LEN = 36;
static const QString PROGRAM_VERSION = "5.4.13";
static const QString CORE_VERSION = "3.0";
static const QString DEV_DATE= "2022.09.16";
static const QString TITLE_NAME = "방패(사용자)-Ver."+PROGRAM_VERSION;
static const QString UPDATE_PEER_NAME = "김영진";
static const QString ABSTRACT = "      프로그람판본 : "+PROGRAM_VERSION+ \
                                                   "      핵심부판본 : "+CORE_VERSION+ \
                                                   "      Qt판본: 5.14.2\n\n       \
      《방패》프로그람의 저작권은 2020년부터 2025년까지 베이징체신대학이 소유하고 있습니다.\n\n     \
      프로그람갱신은 베이징체신대학의 허가를 받아야 합니다. 이 프로그람이 많은 도움이 되기를 바랍니다. \n\n     \
      문제점이 있으면 베이징체신대학에 문의해주십시오. \n\n     \
      개발자 : 베이징체신대학:  mathkyy@163.com, 18410305071     개발날자 : " + DEV_DATE;

static const QString UPDATE_NOTICE_TITLE = PROGRAM_VERSION + "판본갱신";
static const QString UPDATE_NOTICE = PROGRAM_VERSION + \
"판본에서 새로 갱신된 내용은 다음과 같습니다.\n \
\n\n \
1--방패전용파일기능의 오유를 수정하였습니다.\n \
2--작업등록부를 통일하고 최량화를 진행하였습니다.\n \
3--이모쉰기능을 비롯한 일련의 오유를 퇴치하였습니다.\n \
4--체계의 원활한 운영을 위하여 10일에 한번씩 대화기록을 보관하고 초기화하할것을 귄고하도록 하였습니다.\n \
5--KCTV에서 일부 불합리한점들을 수정하였습니다.\n \
6--프로그람이 꺼질때 최량화처리를 진행하였습니다.\n \
7--수동갱신단추를 새로 첨가하였습니다.";


static const QString SMH = "#BANGPAESENTENCE#";                                             // 특수문자렬앞붙이

static const QString SM_SEND_MY_PUB_KEY_NONCE_DATA = SMH + "#PUBKEY AND NONCE OF MYPARTNER#"; // 열쇠공유시 상대방에게 열쇠알림
static const QString SM_AUDIO_CALL_BUSY = SMH + "#CALLBUSY#";                               // 1대1호출요청시 바쁨상태통보문
static const QString SM_AUDIO_CALL_REFUSED = SMH + "#RGCALLREFUSED#";                       // 1대1호출요청시 거절상태통보문
static const QString SM_SERVER_CANCEL_CALL = SMH + "#SERVER_CANCEL_CALL#";                  // 봉사기측에서 1대1통화를 끝냈다는것을 알리기
static const QString SM_IS_GROUP_AVAILABLE = SMH + "#IS_GROUP_AVAILABLE#";                  // 그룹이 존재하는가 따지기
static const QString SM_GROUP_NOT_EXIST = SMH + "#GROUP_NOT_EXIST#";                        // 그룹이 존재하지 않는다는것을 알리기

static const QString SM = "#GROUPCALL#";                                                    // 음성회의 소집신호
static const QString SM_END = "#GROUPCALLEND#";                                             // 음성회의 끝냄신호
static const QString SM_JOINCALL = "#GROUPCALLJOIN#";                                       // 음성회의 접속신호
static const QString SM_GROUP_AUDIO_I_AM_JOINED = SMH + "#GROUPCALLIAMJOINED#";             // 음성회의 접속상태 알림신호
static const QString SM_GROUP_AUDIO_RECALL = SMH + "#GROUPRECALL#";                         // 음성회의 재접속신호

static const QString SM_GROUP_VIDEO_START = SMH + "#GROUPVIDEOCALL#";                       // 비데오회의 소집신호
static const QString SM_GROUP_VIDEO_END = SMH + "#GROUPVIDEOCALLEND#";                      // 비데오회의 끝냄신호
static const QString SM_GROUP_VIDEO_JOIN = SMH + "#GROUPCALLVIDEOJOIN#";                    // 비데오회의 접속신호

static const QString SM_GROUP_REPORT_CALLING_SERVER = SMH + "#GROUP_REPORT_CALLING_SERVER#";           // 그룹보고에서 그룹관리자를 호출
static const QString SM_GROUP_REPORT_ACCEPT_CALL = SMH + "#GROUP_REPORT_ACCEPT_CALL#";                 // 그룹보고에서 그룹관리자가 호출접수
static const QString SM_GROUP_REPORT_BUSY = SMH + "#GROUP_REPORT_BUSY#";                               // 그룹보고에서 그룹관리자가 통화중
static const QString SM_GROUP_REPORT_REFUSE_PRIVATE_CALL = SMH + "#GROUP_REPORT_REFUSE_PRIVATE_CALL#"; // 그룹보고에서 그룹관리자가 호출거절
static const QString SM_GROUP_REPORT_STOP_CALLING_SERVER = SMH + "#GROUP_REPORT_STOP_CALLING_SERVER#"; // 그룹보고에서 그룹관리자를 호출중지

static const QString SM_FILE_RELAY_IS_DEST_CONNECTED = SMH + "IS_DEST_CONNECTED";           // 파일중계전송시 중계마디점의 련결상태요청
static const QString SM_FILE_RELAY_DEST_IS_CONNECTED = SMH + "DEST_IS_CONNECTED";           // 파일중계전송시 중계마디점의 련결상태알림
static const QString SM_FILE_RELAY_SUCCESS = SMH + "FILE_RELAY_SUCCESS";                    // 파일중계전송시 성공에 대한 알림(중계마디점에서 원천마디점한데 알림)
static const QString SM_FILE_RELAY_FAILED = SMH + "FILE_RELAY_FAILED";                      // 파일중계전송시 실패에 대한 알림
static const QString SM_FILE_RELAY_SUCCESS_TRANSFER = SMH + "FILE_REALY_SUCCESS_TRANSFER";  // 파일중계전송시 성공에 대한 알림(목적마디점에서 중계마디점한데 알림)
static const QString SM_FUNCTXT_REQ_FILERLY = "#RFR";                                       // 파일중계전송시 중계전송되는 파일이름의 앞붙이

static const QString SM_TEXT_RELAY_FUNCTXT_REQ = SMH + "#RTR";                              // 통보문중계전송시 중계전송되는 본문의 앞붙이
static const QString SM_TEXT_RELAY_IS_DEST_CONNECTED = SMH + "FOR_TEXT_IS_DEST_CONNECTED";  // 통보문중계전송시 중계마디점의 련결정보요청진행
static const QString SM_TEXT_RELAY_DEST_IS_CONNECTED = SMH + "FOR_TEXT_DEST_IS_CONNECTED";  // 통보문중계전송시 중계마디점의 련결정보알림진행

static const QString SM_BLOG_DOWNLOAD_SUCCESS = SMH + "#BLOG_DOWNLOAD_SUCCESS#";            // 게시판내용의 내리적재성공 알리기
static const QString SM_BLOG_DOWNLOADCNT_UPDATED = SMH + "#BLOG_DOWNLOADCNT_UPDATED#";      // 게시판내용의 내리적재회수 갱신
static const QString SM_BLOG_FILE_DOWNLOAD_FAILED = SMH + "#BLOG_FILE_DOWNLOAD_FAILED#";    // 게시판내용의 내리적재실패
static const QString SM_BLOG_DELETE = SMH + "#BLOG_DELETE#";                                // 게시판내용의 삭제
static const QString SM_BLOG_CONTENT = SMH + "#BLOG_CONTENT";                               // 게시판내용의 전송
static const QString SM_BLOG_CONTENT_REQUEST = SMH + "#BLOG_CONTENT_REQUEST#";              // 게시판내용의 요청
static const QString SM_BLOG_REQUEST_FILE_PATH = SMH + "#BLOG_REQUEST_FILE_PATH#";          // 게시판내용의 경로전송

static const QString SM_RECALL = "#RECALL#" ;                                               // 통보문철회
static const QString SM_RECALL_FILE = "#RECALLFILE#";                                       // 파일철회
static const QString SM_RECALL_FILE_SUCCESS = SMH + "#RECALLFILES#";                        // 파일철회성공
static const QString SM_RECALL_FILE_FAIL = SMH + "#RECALLFILEF#";                           // 파일철회실패--
static const QString SM_RECALL_FILE_RESULT = SMH + "#RECALLFILE";                           // 파일철회결과--

static const QString SM_MY_VERSION = "#MyVersion#";                                         // 자기의 판본알리기
static const QString SM_DEVDATE = SMH + "#DEVDATE#";                                        // 개발자에게 자기의 개발날자 및 현재판본을 알리기
static const QString SM_PLEASE_UPDATE = "#PleaseUpdate#";                                   // 갱신하라고 알리기
static const QString SM_GET_VERSION = SMH + "#GetVersion";
static const QString SM_REQUEST_UPDATE = "#RequestUpdate#";                                 // 갱신파일 요청
static const QString SM_UPDATE_FAILED = "#UpdateFailed#";                                   // 갱신파일 내리적재실패

//static const QString SM_GROUPRECALLJOIN = "#GROUPRECALLJOIN#";
//static const QString SM_GROPU_BROADCAST_MY_PUBLICKEY = SMH + "#MYGROUP#";
//static const QString SPECIAL_FUNCTEXT_START_WRITE_DEBUG_FILE = "#STARTDEBUG#";
//static const QString SPECIAL_FUNCTEXT_STOP_WRITE_DEBUG_FILE = "#STOPDEBUG#";
//static const QString SPECIAL_FUNCTEXT_SHOW_ONE_SHARED_KEYS = "#ONE#";
//static const QString SPECIAL_FUNCTEXT_SHOW_GROUP_SHARED_KEYS = "#GROUP#";

//passwoed의 마스크
static const uint8_t CLNT_GEN_BPK_IMPORT_BPX_PASSWORD_MASK[32] = {
    139, 121, 137, 178, 87, 228, 56, 44, 129, 142, 159, 20, 38, 188, 83, 163,
    180, 94, 149, 229, 76, 153, 184, 14, 216, 34, 234, 244, 22, 232, 230, 144};
static const uint8_t CLNT_IMPORT_BPX_PWHASH_SALT[16] ={
    212, 68, 77, 15, 217, 210, 82, 153, 91, 245, 251, 171, 105, 193, 175, 203};
static const uint8_t CLNT_GEN_SN_PWHASH_SALT[16] ={
    30, 65, 81, 221, 62, 177, 168, 107, 242, 209, 78, 133, 93, 134, 100, 227};

//공유열쇠-2 생성에 필요한 마스크
static const uint8_t SHARE_KEY_2_MASK[32] = {
    116, 254, 134, 103, 34, 176, 46, 149, 50, 56, 161, 96, 236, 2, 28, 150, 
    144, 251, 66, 123, 132, 11, 141, 168, 234, 192, 69, 105, 162, 210, 199, 82};

//증명서내용의 암/복호화에 필요한 NONCE, 암호화열쇠는 공유열쇠-2
static const uint8_t BPX_CONTENT_ENDEC_NONCE[24] = {
    38, 80, 11, 134, 237, 250, 48, 199, 133, 191, 157, 231,
    230, 189, 180, 247, 79, 132, 110, 19, 15, 2, 43, 158};

//bpx파일을 암/복호화하는데 필요한 NONCE, 암호화열쇠는 기대번호
static const uint8_t BPX_ENDEC_NONCE[24] = {
    217, 210, 150, 205, 204, 27, 100, 214, 220, 87, 157, 127,
    75, 169, 98, 4, 190, 252, 246, 159, 73, 130, 162, 67};

//ini파일을 암/복호화하는데 필요한 NONCE, 암호화열쇠는 기대번호
static const uint8_t INI_ENDEC_NONCE[24] = {
    169, 253, 30, 107, 118, 157, 65, 119, 227, 194, 246, 196,
    220, 17, 225, 230, 154, 244, 28, 75, 255, 117, 158, 126};

//주소록갱신파일을 암/복호화하는데 필요한 NONCE와 암호화열쇠
static const uint8_t UAF_ENDEC_NONCE[24] = {
    230, 191, 43, 95, 162, 216, 85, 12, 62, 87, 203, 205,
    0, 165, 169, 121, 3, 213, 70, 134, 164, 220, 46, 231};
static const uint8_t UAF_ENDEC_KEY[32] = {
    144, 121, 161, 139, 239, 158, 104, 103, 93, 100, 188, 157, 26, 4, 60, 171,
    141, 130, 5, 245, 178, 136, 37, 57, 217, 174, 228, 176, 186, 45, 198, 97};

//봉사기림시공개열쇠-2의 복호화에 필요한 NONCE와 고정열쇠
static const uint8_t SRV_PUBKEY_2_ENDEC_NONCE[24] = {
    75, 226, 199, 49, 209, 127, 212, 64, 84, 242, 254, 0,
    83, 135, 172, 235, 8, 69, 92, 29, 33, 225, 255, 196};
static const uint8_t SRV_PUBKEY_2_ENDEC_KEY[32] = {
    208, 160, 95, 39, 56, 141, 171, 32, 190, 106, 129, 157, 175, 216, 110, 6,
    66, 47, 59, 29, 234, 83, 167, 70, 99, 64, 42, 191, 246, 55, 221, 33};

//선전물파일을 암/복호화하는데 필요한 NONCE와 암호화열쇠
static const uint8_t POSTER_ENDEC_NONCE[24] = {
    241, 135, 175, 198, 112, 181, 139, 222, 232, 173, 226, 170,
    114, 192, 194, 73, 94, 205, 82, 61, 242, 75, 20, 7};
static const uint8_t POSTER_ENDEC_KEY[32] = {
    124, 35, 45, 152, 117, 247, 89, 71, 140, 188, 214, 243, 125, 92, 251, 191, 
    178, 97, 190, 244, 27, 66, 24, 44, 98, 110, 241, 54, 103, 65, 197, 175};

static const uint8_t BPD_ENDEC_NONCE[24] = {
    51, 167, 247, 28, 129, 191, 155, 34, 66, 252, 25, 42,
    121, 98, 170, 219, 70, 148, 19, 127, 113, 64, 137, 182};

// xchacha를 리용하여 암호화진행
static const QByteArray integrated_xchacha_encrypt(QByteArray data, const uint8_t nonce[],
                                                   const uint8_t key[])
{
    uint8_t* message = reinterpret_cast<uint8_t *>(data.data());
    unsigned long long message_len = data.length();
    unsigned long long ciphertext_len = message_len + crypto_aead_xchacha20poly1305_ietf_ABYTES;
    uint8_t *ciphertext = reinterpret_cast<uint8_t *>(malloc(ciphertext_len));
    // uint8_t ciphertext[ciphertext_len];

    if(crypto_aead_xchacha20poly1305_ietf_encrypt(
                   ciphertext, &ciphertext_len,
                   message, message_len,
                   ADD_ENDEC_DATA1, ADD_ENDEC_DATA1_LEN,
                   nullptr,
                   nonce, key
                   ) != 0)
    {
        return nullptr;
    }
    QByteArray temp = QByteArray(reinterpret_cast<char*>(ciphertext), ciphertext_len);
    free(ciphertext);
    return temp;
}

// xchacha를 리용하여 복호화진행
static const QByteArray integrated_xchacha_decrypt(QByteArray data, const uint8_t *nonce,
                                                     const uint8_t *key)
{
    uint8_t *ciphertext = reinterpret_cast<uint8_t*>(data.data());
    unsigned long long ciphertext_len = data.length();
    unsigned long long decrypted_len = ciphertext_len - crypto_aead_xchacha20poly1305_ietf_ABYTES;
    uint8_t *decrypted = reinterpret_cast<uint8_t *>(malloc(decrypted_len));
    // uint8_t decrypted[decrypted_len];

    if (crypto_aead_xchacha20poly1305_ietf_decrypt(
                    decrypted, &decrypted_len,
                    nullptr,
                    ciphertext, ciphertext_len,
                    ADD_ENDEC_DATA1, ADD_ENDEC_DATA1_LEN,
                    nonce, key
                    ) != 0)
    {
        return nullptr;
    }
    QByteArray temp = QByteArray(reinterpret_cast<char *>(decrypted), decrypted_len);
    free(decrypted);
    return temp;
}

// JYJ 2022.07.24, 16:00 START

static const bool isValidGroupName(QString groupName)
{
    return groupName.size() > GROUP_NAME_MIN_SIZE &&
            groupName.at(0) == '#' &&
            groupName.at(GROUP_NAME_MIN_SIZE) == '#';
}

static const QString getRealGroupName(QString groupName)
{
    if (isValidGroupName(groupName))
        return groupName.mid(GROUP_NAME_MIN_SIZE_1);
    return groupName;
}

static const QString getGroupCreator(QString groupName)
{
    if (isValidGroupName(groupName))
        return groupName.mid(1, 64);
    return "None";
}

// JYJ 2022.07.24, 16:00 END


#endif // CONSTDEFINE_H
