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

#pragma once

#include "ui_mainwindow.h"
#include <QFileInfo>
#include <QMainWindow>
#include <QPointer>
#include <QProcess>
#include <QRunnable>
#include <QSystemTrayIcon>

#include "genericchatitemwidget.h"
#include "src/audio/iaudiocontrol.h"
#include "src/audio/iaudiosink.h"
#include "src/constdefine.h"
#include "src/core/bangpaefile.h"
#include "src/core/bangpaeid.h"
#include "src/core/bangpaepk.h"
#include "src/core/core.h"
#include "src/core/groupid.h"
#include "src/model/friendmessagedispatcher.h"
#include "src/model/groupmessagedispatcher.h"
#include "src/widget/form/reportform.h"
#include "src/widget/form/setfriendform.h"
#include "src/widget/form/updateform.h"
#include "src/widget/loginscreen.h"
#include "src/widget/updatedialog.h"

#if DESKTOP_NOTIFICATIONS
#include "src/platform/desktop_notifications/desktopnotify.h"
#endif

#define PIXELS_TO_ACT 7

namespace Ui {
class MainWindow;
}

class AlSink;
class Camera;
class ChatForm;
class ChatHistory;
class CircleWidget;
class ContentDialog;
class ContentLayout;
class Core;
class Friend;
class FriendChatroom;
class FriendListWidget;
class FriendWidget;
class GenericChatroomWidget;
class Group;
class GroupChatForm;
class GroupChatroom;
class GroupInvite;
class GroupInviteForm;
class GroupWidget;
class IChatLog;
class MaskablePixmapWidget;
class ProfileForm;
class ProfileInfo;
class QActionGroup;
class QMenu;
class QPushButton;
class QSplitter;
class QTimer;
class Settings;
class SettingsWidget;
class SystemTrayIcon;
class UpdateCheck;
class VideoSurface;

QString Byte2QString(const uint8_t* tKey, int nLen = 32);
bool QString2Byte(QString str, unsigned char* res);
//QByteArray QString2ByteArray(QString str);
bool bangpaeActivateEventHandler(const QByteArray& data);
void InitFileRelayStatus(int n = 1, int m = 1);
void MixPubKeyToRelayFileFormat(uint8_t* arrResPubKey, const uint8_t* srcTmpPeerPubKey, const uint8_t* dstTmpPeerPubKey) ;
void UnmixPubKeyFromRelayFileFormat(uint8_t* dstPeerPubKey, const uint8_t* srcTmpPeerPubKey, const uint8_t* arrResPubKey);


class QFileEncDecTask : public QObject, public QRunnable{
    Q_OBJECT
    Core* core;
    QString strSrcFilePath;
    QString strSrcFileName;

    /*
     * In the case of encryption, nFrndId is the destination friend.
     *                            arrPeerPubKey is the long term public key of destination friend.
     * In the case of decryption, nFrndId is the relay friend.
     *                            arrPeerPubKey is the long term public key of source friend.
     */

    bool bIsEnc;
    uint8_t arrPeerPubKey[32];
    int nFrndId;
public:
    QFileEncDecTask();
    ~QFileEncDecTask();
    void SetThreadData(Core* pCore, QString strTSrcPath, QString strTSrcName,
                       const uint8_t* arrTPeerPubKey, bool bTIsEnc, int nTFrndId = -1);
    void run() override;
signals:
    void fileDecryptionEnd(const QString&, const uint8_t* arrDstPubKey, const uint8_t* arrRelayPubKey );
    void fileEncryptionEnd(int, QString&, QString&, int);
};


class Widget final : public QMainWindow
{
    Q_OBJECT

private:
    enum class ActiveToolMenuButton
    {
        AddButton,
        GroupButton,
        TransferButton,
        SettingButton,
        None,
        ReportButton,
        SetFriendButton,
        TVButton,
        TVBPButton
    };

    enum class DialogType
    {
        AddDialog,
        TransferDialog,
        SettingDialog,
        ProfileDialog,
        GroupDialog,
        ReportDialog
    };

    enum class FilterCriteria
    {
        All = 0,
        Online,
        Offline,
        Friends,
        Groups
    };

    //int timeCnt;
    // QPushButton* friendRequestsButton;
    // SetFriendForm* setFriendForm;
    //FilesForm* filesForm;

    ContentLayout* contentLayout;
    Friend *m_tempf;
    GenericChatroomWidget* activeChatroomWidget;
    GenericChatroomWidget* clicked;
    GroupInviteForm* groupInviteForm;
    MaskablePixmapWidget* profilePicture;
    Profile& profile;
    ProfileForm* profileForm;
    QAction* actionLogout;
    QAction* actionQuit;
    QAction* actionShow;
    QAction* filterAllAction;
    QAction* filterDisplayActivity;
    QAction* filterDisplayName;
    QAction* filterFriendsAction;
    QAction* filterGroupsAction;
    QAction* filterOfflineAction;
    QAction* filterOnlineAction;
    QAction* statusAway;
    QAction* statusBusy;
    QAction* statusOnline;
    QActionGroup* filterDisplayGroup;
    QActionGroup* filterGroup;
    QFileEncDecTask* pFileEncDecTask;
    QMenu* filterMenu;
    QMenu* trayMenu;
    QMessageBox* pFileRelayWaitingMsg;
    QPoint dragPosition;
    QPointer<SettingsWidget> settingsWidget;
    QProcess* pTVProcess = nullptr;
    QPushButton* groupInvitesButton;
    QSplitter* centralLayout;
    QStringList blogForOneAdmin;
    QTimer *contactTimer = nullptr;
    QTimer *displayTimeTimer;
    QTimer* timer;
    QTimer *pTimerPoster = nullptr, *pTimerAppQuit = nullptr;
    ReportForm* reportForm = nullptr;
    Ui::MainWindow* ui;
    UpdateForm* updateForm = nullptr;
    bool eventFlag;
    bool eventIcon;
    bool autoAwayActive = false;
    bool isMessageBoxOpen;
    bool wasMaximized = false;
    int icon_size;
    mutable QMutex blogLock;
    static Widget* instance;
    std::unique_ptr<QSystemTrayIcon> icon;
    std::unique_ptr<UpdateCheck> updateCheck; // ownership should be moved outside Widget once non-singleton
    unsigned int unreadGroupInvites;

    FilterCriteria getFilterCriteria() const;
    Group* createGroup(uint32_t groupnumber, const GroupId& groupId, bool forMe, QString title = "");
    bool DecVidFile(QString strSrcFilePath, QString& strDstFilePath, QString strMachineNumber);
    bool event(QEvent* e) final;
    bool eventFilter(QObject* obj, QEvent* event) final;
    bool isBlogFile(QString fileName);
    bool newMessageAlert(QWidget* currentWindow, bool isActive, bool sound = true, bool notify = true);
    bool notify(QObject* receiver, QEvent* event);
    int versionCompare(QString versionOne, QString versionTwo);
    static bool filterGroups(FilterCriteria index);
    static bool filterOffline(FilterCriteria index);
    static bool filterOnline(FilterCriteria index);
    void addFriendDialog(const Friend* frnd, ContentDialog* dialog);
    void addGroupDialog(Group* group, ContentDialog* dialog);
    void clearSpecFileAndDir(const QString strPath, const QString strPattern,
                                    const int nMatchMode, bool bIsDir = false, const int nExpireTime = -1);
    void EncVidFile(QString strSrcFilePath, QString strDstFilePath);
    void SafeDelFile(QString strFilePath);
    void acceptFileTransfer(const BangPaeFile &file, const QString &path);
    void changeDisplayMode();
    void changeEvent(QEvent* event) final;
    void closeEvent(QCloseEvent* event) final;
    void cycleContacts(bool forward);
    void focusChatInput();
    void hideMainForms(GenericChatroomWidget* chatroomWidget);
    void moveEvent(QMoveEvent* event) final;
    void openDialog(GenericChatroomWidget* widget, bool newWindow);
    void removeFriend(Friend* f, bool fake = false);
    void removeGroup(Group* g, bool fake = false);
    void removeGroup(const GroupId& groupId);
    void resizeEvent(QResizeEvent* event) final;
    void retranslateUi();
    void saveSplitterGeometry();
    void saveWindowGeometry();
    void searchContacts();
    void setActiveToolMenuButton(ActiveToolMenuButton newActiveButton);
    void setJYJStatus();
    void updateFilterText();

public:
    Core* core = nullptr;
    FriendListWidget* contactListWidget;
    IAudioControl& audio;
    MessageProcessor::SharedParams sharedMessageProcessorParams;
    ProfileInfo* profileInfo;
    QString strWorkDirPath;
    Settings& settings;
    bool isLogout;
    char strSoundMeetingPassHash[PASS_HASH_LEN];
    int nPosterCnt = 0, nPosterId = 1, nPrvPosterId = -1;

    std::unique_ptr<IAudioSink> audioNotification;

    QMap<BangPaePk, FriendWidget*> friendWidgets;
    // Shared pointer because qmap copies stuff all over the place
    QMap<BangPaePk, std::shared_ptr<FriendMessageDispatcher>> friendMessageDispatchers;
    // Stop gap method of linking our friend messages back to a group id.
    // Eventual goal is to have a notification manager that works on
    // Messages hooked up to message dispatchers but we aren't there
    // yet
    QMap<BangPaePk, QMetaObject::Connection> friendAlertConnections;
    QMap<BangPaePk, std::shared_ptr<ChatHistory>> friendChatLogs;
    QMap<BangPaePk, std::shared_ptr<FriendChatroom>> friendChatrooms;
    QMap<BangPaePk, ChatForm*> chatForms;
    QMap<GroupId, GroupWidget*> groupWidgets;
    QMap<GroupId, std::shared_ptr<GroupMessageDispatcher>> groupMessageDispatchers;
    // Stop gap method of linking our group messages back to a group id.
    // Eventual goal is to have a notification manager that works on
    // Messages hooked up to message dispatchers but we aren't there
    // yet
    QMap<GroupId, QMetaObject::Connection> groupAlertConnections;
    QMap<GroupId, std::shared_ptr<IChatLog>> groupChatLogs;
    QMap<GroupId, std::shared_ptr<GroupChatroom>> groupChatrooms;
    QMap<GroupId, QSharedPointer<GroupChatForm>> groupChatForms;

    //Group* createGroup_ryc(uint32_t groupnumber, const GroupId& groupId, bool forMe, bool is_open);
    Camera* getCamera();
    ContentDialog* createContentDialog() const;
    ContentLayout* createContentDialog(DialogType type) const;
    QString generateRandTmpDirPath() const;
    bool getIsWindowMinimized();
    bool groupsVisible() const;
    bool newFriendMessageAlert(const BangPaePk& friendId, const QString& text, bool sound = true, bool file = false);
    bool newGroupMessageAlert(const GroupId& groupId, const BangPaePk& authorPk, const QString& message, bool notify);
    explicit Widget(Profile& _profile, IAudioControl& audio, QWidget* parent = nullptr);
    static QString fromDialogType(DialogType type);
    static Widget* getInstance(IAudioControl* audio = nullptr);
    static inline QIcon prepareIcon(QString path, int w = 0, int h = 0);
    static void confirmExecutableOpen(const QFileInfo& file);
    void cleanupNotificationSound();
    void clearAllReceipts();
    void init();
    void InitInAudioDev() ;
    void playNotificationSound(IAudioSink::Sound sound, bool loop = false);
    void reloadTheme();
    void removeFriend_jyj(Friend* f);
    void resetIcon();
//    void setCentralWidget(QWidget* widget, const QString& widgetName);
    void setProgress(uint32_t friendId, int value);
//    void showUpdateDownloadProgress();
    void updateIcons();
    ~Widget() override;

signals:
    void sigQuitApp();
    // void friendRequestAccepted(const BangPaePk& friendPk);
    // void friendRequested(const BangPaeId& friendAddress, const QString& message);
    void statusSet(Status::Status status);
    void statusSelected(Status::Status status);
    void usernameChanged(const QString& username);
    void changeGroupTitle(uint32_t groupnumber, const QString& title);
    void statusMessageChanged(const QString& statusMessage);
    void resized();
    void windowStateChanged(Qt::WindowStates states);

private slots:
    void connectCircleWidget(CircleWidget& circleWidget);
    void connectFriendWidget(FriendWidget& friendWidget);
    void copyFriendIdToClipboard(const BangPaePk& friendId);
    void dispatchFile(BangPaeFile file);
    void dispatchFileSendFailed(uint32_t friendId, const QString& fileName);
    void dispatchFileSendFailedForRelay(uint32_t friendId, const QString& fileName, const QString&);
    void dispatchFileWithBool(BangPaeFile file, bool);
    void friendListContextMenu(const QPoint& pos);
    // void friendRequestsUpdate();
    void FriendBusyNotification(BangPaePk friendId, bool bIsFromGroup = false);
    void FriendRefusedNotification(BangPaePk friendId);
    void groupInvitesClear();
    void groupInvitesUpdate();
    void incomingNotification(uint32_t friendId);
    void onCallEnd();
    void onChatroomWidgetClicked(GenericChatroomWidget* widget);
    void onDialogShown(GenericChatroomWidget* widget);
    void onEventIconTick();
    void onGetAllUserAppVersions(QVector<uint32_t>);
    void onGroupClicked();
    void onIconClick(QSystemTrayIcon::ActivationReason);
    void onNewFriendAVAlert(uint32_t friendId);
    void onNobodyAnswerNotification();
    void onRejectCall(uint32_t friendId);
    void onReportClicked();
    // void onSetFriendClicked();
    void onSendNewVerToAllUsers(QVector<uint32_t>);
    void onSetShowSystemTray(bool newValue);
    void onSplitterMoved(int pos, int index);
    void onStatusMessageChanged(const QString& newStatusMessage);
    void onStopNotification();
    void onTryCreateTrayIcon();
    void onViewKCTV();
    void onViewBPDFile();
    void onUpdateClicked();
    void onUserAwayCheck();
    void openNewDialog(GenericChatroomWidget* widget);
    void outgoingNotification();
    void registerContentDialog(ContentDialog& contentDialog) const;
    void removeFriend(const BangPaePk& friendId);
    void searchCircle(CircleWidget& circleWidget);
    void setStatusAway();
    void setStatusBusy();
    void setStatusOnline();
    void showProfile();
    void updateFriendActivity(const Friend& frnd);

public slots:
    void addFriend(uint32_t friendId, const BangPaePk& friendPk);
    void addFriendChatForm(const BangPaePk& friendPk);
    void addFriendFailed(const BangPaePk& userId, const QString& errorInfo = QString());
    void BroadcastTextRelayFrndReq(int nFrndId, int);
    void clickedContactWidget();
    void forceShow();
    void groupIncomingNotification();
    void groupOutgoingNotification();
    void nextContact();
    void onAppWorkingTimedOut();
    void onBadProxyCore();
    void onConnected();
    void onContactChangeColor();
    void onCoreChanged(Core& core);
    void onDisconnected();
    void onDisplayPoster();
    void onDisplayTime();
    void onEmptyGroupCreated(uint32_t groupnumber, const GroupId& groupId, const QString& title);
    void onFailedToStartCore();
    void onFileEncryptionEnd(int, QString&, QString&, int);
    void onFileReceiveRequested(const BangPaeFile& file);
    void onFriendAliasChanged(const BangPaePk& friendId, const QString& alias);
    void onFriendDialogShown(const Friend* f);
    void onFriendDisplayedNameChanged(const QString& displayed);
    void onFriendMessageReceived(uint32_t friendnumber, const QString& message, bool isAction);
    //void onFriendRequestReceived(const BangPaePk& friendPk, const QString& message);
    void onFriendStatusChanged(int friendId, Status::Status status);
    void onFriendStatusMessageChanged(int friendId, const QString& message);
    void onFriendTypingChanged(uint32_t friendnumber, bool isTyping);
    void onFriendUsernameChanged(int friendId, const QString& username);
    void onGroupDialogShown(Group* g);
    void onGroupInviteAccepted(const GroupInvite& inviteInfo);
    void onGroupInviteReceived(const GroupInvite& inviteInfo);
    void onGroupJoined(int groupNum, const GroupId& groupId);
    void onGroupMessageReceived(int groupnumber, int peernumber, const QString& message, bool isAction);
    void onGroupPeerAudioPlaying(int groupnumber, BangPaePk peerPk);
    void onGroupPeerNameChanged(uint32_t groupnumber, const BangPaePk& peerPk, const QString& newName);
    void onGroupPeerVideoReceived(int groupnumber, BangPaePk peerPk, QImage img);
    void onGroupPeerlistChanged(uint32_t groupnumber);
    void onGroupSendFailed(uint32_t groupnumber);
    void onGroupTitleChanged(uint32_t groupnumber, const QString& author, const QString& title);
    void onNoFileRelay(int, QString, QString);
    void onNoTxtRelay(int, int, bool);
    void onQuitApp();
    void onReceiptReceived(int friendId, ReceiptNum receipt);
    void onSelfAvatarLoaded(const QPixmap& pic);
    void onSeparateWindowChanged(bool separate, bool clicked);
    void onSeparateWindowClicked(bool separate);
    void onShowSettings();
    void onStatusSet(Status::Status status);
    void onUpdateAvailable();
    void previousContact();
    void refreshPeerListsLocal(const QString& username);
    void removeBlackMan(QStringList blackPbList);
    void setStatusMessage(const QString& statusMessage);
    void setUsername(const QString& username);
    void setWindowTitle(const QString& title);
    void showRelayedFileDecryptionEndMsg(const QString& strSrcName, const uint8_t* arrDstPubKey, const uint8_t* arrRelayPubKey);
    void titleChangedByUser(const QString& title);
    void toggleFullscreen();

#if DESKTOP_NOTIFICATIONS
    DesktopNotify notifier;
#endif

};

