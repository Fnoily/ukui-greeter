/* loginwindow.cpp
 * Copyright (C) 2018 Tianjin KYLIN Information Technology Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301, USA.
**/
#include "loginwindow.h"
#include <QMouseEvent>
#include <QDebug>
#include <QApplication>
#include <QStandardPaths>
#include <QScreen>
#include <QProcess>
#include <QFocusEvent>
#include <QWindow>
#include <QLightDM/SessionsModel>
#include "globalv.h"

LoginWindow::LoginWindow(QSharedPointer<GreeterWrapper> greeter, QWidget *parent)
    : QWidget(parent), m_greeter(greeter),
      m_sessionsModel(new QLightDM::SessionsModel(QLightDM::SessionsModel::LocalSessions, this)),
      m_config(new QSettings(configFile, QSettings::IniFormat)),
      m_timer(new QTimer(this))
{    
    initUI();
    connect(m_greeter.data(), SIGNAL(showMessage(QString,QLightDM::Greeter::MessageType)),
            this, SLOT(onShowMessage(QString,QLightDM::Greeter::MessageType)));
    connect(m_greeter.data(), SIGNAL(showPrompt(QString,QLightDM::Greeter::PromptType)),
            this, SLOT(onShowPrompt(QString,QLightDM::Greeter::PromptType)));
    connect(m_greeter.data(), SIGNAL(authenticationComplete()),
            this, SLOT(onAuthenticationComplete()));

    m_timer->setInterval(100);
    connect(m_timer, SIGNAL(timeout()), this, SLOT(updatePixmap()));
}

void LoginWindow::initUI()
{
    if (this->objectName().isEmpty())
        this->setObjectName(QStringLiteral("this"));
    this->resize(520, 135);

    QPalette plt;

    m_backLabel = new QLabel(this);
    m_backLabel->setObjectName(QStringLiteral("m_backLabel"));
    m_backLabel->setGeometry(QRect(0, 0, 32, 32));
    QPixmap back(":/resource/arrow_left.png");
    back = back.scaled(32, 32, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
    m_backLabel->setPixmap(back);
    m_backLabel->installEventFilter(this);

    m_faceLabel = new QLabel(this);
    m_faceLabel->setObjectName(QStringLiteral("m_faceLabel"));
    m_faceLabel->setGeometry(QRect(60, 0, 132, 132));
    m_faceLabel->setStyleSheet("QLabel{ border: 2px solid white}");

    m_sessionBg = new QWidget(this);
    m_sessionBg->setGeometry(QRect(width()-26, 0, 26, 26));
    m_sessionBg->hide();
    m_sessionLabel = new QSvgWidget(this);
    m_sessionLabel->setObjectName(QStringLiteral("m_sessionLabel"));
    m_sessionLabel->setGeometry(QRect(width()-23, 3, 20, 20));
    m_sessionLabel->installEventFilter(this);
    m_sessionLabel->hide();
    m_sessionLabel->setFocusPolicy(Qt::StrongFocus);

    plt.setColor(QPalette::WindowText, Qt::white);

    m_nameLabel = new QLabel(this);
    m_nameLabel->setObjectName(QStringLiteral("m_nameLabel"));
    QRect nameRect(220, 0, 200, 25);
    m_nameLabel->setGeometry(nameRect);
    m_nameLabel->setPalette(plt);
    m_nameLabel->setFont(QFont("ubuntu", fontSize+2));

    m_isLoginLabel = new QLabel(this);
    m_isLoginLabel->setObjectName(QStringLiteral("m_isLoginLabel"));
    QRect loginRect(220, nameRect.bottom()+5, 200, 30);
    m_isLoginLabel->setGeometry(loginRect);
    m_isLoginLabel->setPalette(plt);
    m_isLoginLabel->setFont(QFont("ubuntu", fontSize));

//    m_messageLabel = new QLabel(this);
//    m_messageLabel->setObjectName(QStringLiteral("m_messageLabel"));
//    m_messageLabel->setGeometry(QRect(220, 60, 300, 20));
//    plt.setColor(QPalette::WindowText, Qt::red);
//    m_messageLabel->setPalette(plt);

    m_passwordEdit = new IconEdit(QIcon(":/resource/arrow_right.png"), this);
    m_passwordEdit->setObjectName("m_passwordEdit");
    QRect pwdRect(220, 90, 300, 40);
    m_passwordEdit->setGeometry(pwdRect);
    m_passwordEdit->resize(QSize(300, 40));
    m_passwordEdit->setFocusPolicy(Qt::StrongFocus);
    m_passwordEdit->installEventFilter(this);
    m_passwordEdit->hide(); //收到请求密码的prompt才显示出来
    connect(m_passwordEdit, SIGNAL(clicked(const QString&)), this, SLOT(onLogin(const QString&)));

    setTabOrder(m_passwordEdit, m_sessionLabel);
}

bool LoginWindow::eventFilter(QObject *obj, QEvent *event)
{
    if(obj == m_backLabel) {
        if(!m_backLabel->isEnabled())
            return true;
        if(event->type() == QEvent::MouseButtonPress) {
            if(((QMouseEvent*)event)->button() == Qt::LeftButton){
                m_backLabel->setPixmap(scaledPixmap(32, 32, ":/resource/arrow_left_active.png"));
                return true;
            }
        }
        if(event->type() == QEvent::MouseButtonRelease) {
            if(((QMouseEvent*)event)->button() == Qt::LeftButton){
                m_backLabel->setPixmap(scaledPixmap(32, 32, ":/resource/arrow_left.png"));
                backToUsers();
                return true;
            }
        }
    } else if(obj == m_sessionLabel) {
        if(!m_sessionLabel->isEnabled())
            return true;
        if(event->type() == QEvent::MouseButtonRelease) {
            if(((QMouseEvent*)event)->button() == Qt::LeftButton){
                emit selectSession(m_session);
                return true;
            }
        } else if(event->type() == QEvent::FocusIn){
            m_sessionBg->setStyleSheet(QString::fromUtf8("QWidget{border:1px solid white; border-radius: 4px}"));
        } else if(event->type() == QEvent::FocusOut){
            m_sessionBg->setStyleSheet("");
        } else if(event->type() == QEvent::KeyRelease){
            if(((QKeyEvent*)event)->key() == Qt::Key_Return){
                emit selectSession(m_session);
                return true;
            }
        }
    }
    return QWidget::eventFilter(obj, event);
}

void LoginWindow::showEvent(QShowEvent *e)
{
    QWidget::showEvent(e);
//    m_passwordEdit->setFocus();
}

void LoginWindow::keyReleaseEvent(QKeyEvent *e)
{
    if(e->key() == Qt::Key_Escape){
        backToUsers();
    } else if(e->key() == Qt::Key_S && (e->modifiers() & Qt::ControlModifier)){
        Q_EMIT selectSession(m_session);
    }
}

/**
 * @brief LoginWindow::recover
 * 复原UI
 */
void LoginWindow::recover()
{
    m_nameLabel->clear();
    m_isLoginLabel->clear();
    m_passwordEdit->clear();
    m_passwordEdit->setType(QLineEdit::Password);
    m_passwordEdit->hide();
    clearMessage();
}

void LoginWindow::clearMessage()
{
    //清除所有的message label
    for(int i = 0; i < m_messageLabels.size(); i++) {
        QLabel *msgLabel = m_messageLabels[i];
        delete(msgLabel);
    }
    m_messageLabels.clear();
    //恢复密码输入框的位置
    m_passwordEdit->move(m_passwordEdit->geometry().left(), 90);
    //恢复窗口大小
    this->resize(520, 135);
}

/**
 * @brief LoginWindow::backToUsers
 * 返回到用户列表窗口
 */
void LoginWindow::backToUsers()
{
    recover();
    Q_EMIT back();
}

/**
 * @brief LoginWindow::setUserName
 * @param userName 用户名
 *
 * 设置用户名
 */
void LoginWindow::setUserName(const QString& userName)
{
    m_nameLabel->setText(userName);
}

/**
 * @brief LoginWindow::userName
 * @return 当前的用户名
 *
 * 获取当前用户名
 */
QString LoginWindow::getUserName()
{
    if(m_nameLabel->text() == tr("Login"))
        return "Login";
    return m_nameLabel->text();
}

/**
 * @brief LoginWindow::setFace
 * @param faceFile 用户头像文件
 *
 * 设置用户头像
 */
void LoginWindow::setFace(const QString& facePath)
{
    QFile faceFile(facePath);
    QPixmap faceImage;
    //如果头像文件不存在，则使用默认头像
    if(faceFile.exists())
        faceImage = scaledPixmap(128, 128, facePath);
    else
        faceImage = scaledPixmap(128, 128, ":/resource/default_face.png");

    m_faceLabel->setPixmap(faceImage);
}

/**
 * @brief LoginWindow::setLoggedIn
 * @param isLoggedIn
 *
 * 设置当前用户是否已已经登录
 */
void LoginWindow::setLoggedIn(bool isLoggedIn)
{
    m_isLoginLabel->setText(isLoggedIn ? tr("logged in") : "");
}

/**
 * @brief LoginWindow::setPrompt
 * @param text
 *
 * 设置密码框的提示信息
 */
void LoginWindow::setPrompt(const QString& text)
{
    m_passwordEdit->setPrompt(text);
}

/**
 * @brief LoginWindow::password
 * @return
 *
 * 获取密码
 */
QString LoginWindow::getPassword()
{
    return m_passwordEdit->text();
}

/**
 * @brief LoginWindow::setSession
 *
 * 设置session图标
 */
void LoginWindow::setSession(QString session)
{
    QString sessionIcon;

    if(session.isEmpty() || sessionIndex(session) == -1) {
        /* default */
        QString defaultSession = m_greeter->defaultSessionHint();
        if(defaultSession != session && sessionIndex(defaultSession) != -1) {
            session = defaultSession;
        }
        /* first session in session list */
        else if(m_sessionsModel && m_sessionsModel->rowCount() > 0) {
            session = m_sessionsModel->index(0, 0).data().toString();
        }
        else {
            session = "";
        }
    }
    m_session = session;

    sessionIcon = IMAGE_DIR + QString("badges/%1_badge-symbolic.svg").arg(session.toLower());
    m_sessionLabel->load(sessionIcon);
}

/**
 * @brief LoginWindow::session
 * @return
 *
 * 获取session标识
 */
QString LoginWindow::getSession()
{
    QString sessionKey;
    for(int i = 0; i < m_sessionsModel->rowCount(); i++) {
        QString session = m_sessionsModel->index(i, 0).data(Qt::DisplayRole).toString();
        if(m_session == session) {
            sessionKey = m_sessionsModel->index(i, 0).data(Qt::UserRole).toString();
            break;
        }
    }
    return sessionKey;
}

void LoginWindow::setUsersModel(QSharedPointer<QAbstractItemModel> model)
{
    if(model.isNull())
        return;
    m_usersModel = model;

    if(m_usersModel->rowCount() == 1) {
        m_backLabel->hide();
        setUserIndex(m_usersModel->index(0,0));
    }
}

bool LoginWindow::setUserIndex(const QModelIndex& index)
{
    if(!index.isValid()){
        return false;
    }
    //设置用户名
    QString name = index.data(Qt::DisplayRole).toString();
    setUserName(name);

    //设置头像
    QString facePath = index.data(QLightDM::UsersModel::ImagePathRole).toString();
    setFace(facePath);

    //显示是否已经登录
    bool isLoggedIn = index.data(QLightDM::UsersModel::LoggedInRole).toBool();
    setLoggedIn(isLoggedIn);

    //显示session图标
    if(!m_sessionsModel.isNull() && m_sessionsModel->rowCount() > 1) {
        m_sessionLabel->show();
        m_sessionBg->show();
        setSession(index.data(QLightDM::UsersModel::SessionRole).toString());
    }

    startAuthentication(name);

    return true;
}

void LoginWindow::setSessionsModel(QSharedPointer<QAbstractItemModel> model)
{
    if(model.isNull()){
        return ;
    }
    m_sessionsModel = model;
    //如果session有多个，则显示session图标，默认显示用户上次登录的session
    //如果当前还没有设置用户，则默认显示第一个session
    if(m_sessionsModel->rowCount() > 1) {
        if(!m_usersModel.isNull() && !m_nameLabel->text().isEmpty()) {
            for(int i = 0; i < m_usersModel->rowCount(); i++){
                QModelIndex index = m_usersModel->index(i, 0);
                if(index.data(Qt::DisplayRole).toString() == m_nameLabel->text()){
                    setSession(index.data(QLightDM::UsersModel::SessionRole).toString());
                    return;
                }
            }
        }
        setSession(m_sessionsModel->index(0, 0).data().toString());
    }
}

bool LoginWindow::setSessionIndex(const QModelIndex &index)
{
    //显示选择的session（如果有多个session则显示，否则不显示）
    if(!index.isValid()) {
        return false;
    }
    setSession(index.data(Qt::DisplayRole).toString());
    return true;
}

int LoginWindow::sessionIndex(const QString &session)
{
    if(!m_sessionsModel){
        return -1;
    }
    for(int i = 0; i < m_sessionsModel->rowCount(); i++){
        QString sessionName = m_sessionsModel->index(i, 0).data().toString();
        if(session.toLower() == sessionName.toLower()) {
            return i;
        }
    }
    return -1;
}

void LoginWindow::onSessionSelected(const QString &session)
{
    qDebug() << "select session: " << session;
    if(!session.isEmpty() && m_session != session) {
        m_session = session;
        m_greeter->setSession(m_session);
        setSession(m_session);

//        //查找session的标识
//        QString sessionName, sessionKey;
//        for(int i = 0; i < m_sessionsModel->rowCount(); i++){
//            sessionName = m_sessionsModel->index(i, 0).data().toString();
//            if(sessionName == session){
//                sessionKey = m_sessionsModel->index(i, 0).data(Qt::UserRole).toString();
//                m_greeter->setSession(sessionKey);
//                this->setSession(m_session);
//                return;
//            }
//        }
    }
}

void LoginWindow::startAuthentication(const QString &username)
{
    //用户认证
    if(username == tr("Guest")) {                       //游客登录
        qDebug() << "guest login";
        m_greeter->authenticateAsGuest();
    }
    else if(username == tr("Login")) {                  //手动输入用户名
        m_passwordEdit->setPrompt(tr("Username"));
        m_passwordEdit->setType(QLineEdit::Normal);
    }
    else {
        qDebug() << "login: " << username;
        m_greeter->authenticate(username);
    }
}

/**
 * @brief LoginWindow::startWaiting
 *
 * 等待认证结果
 */
void LoginWindow::startWaiting()
{
    m_passwordEdit->setWaiting(true);   //等待认证结果期间不能再输入密码
    m_backLabel->setEnabled(false);
    m_sessionLabel->setEnabled(false);
    m_timer->start();
}

void LoginWindow::stopWaiting()
{
    m_timer->stop();
    m_passwordEdit->setWaiting(false);
    m_passwordEdit->setIcon(":/resource/arrow_right.png");
    m_backLabel->setEnabled(true);
    m_sessionLabel->setEnabled(true);
    m_passwordEdit->showIcon("");
}

void LoginWindow::updatePixmap()
{
    QMatrix matrix;
    matrix.rotate(90.0);
    m_waiting = m_waiting.transformed(matrix, Qt::FastTransformation);
    m_passwordEdit->setIcon(QIcon(m_waiting));
}

void LoginWindow::saveLastLoginUser()
{
    m_config->setValue("lastLoginUser", m_nameLabel->text());
    m_config->sync();
    qDebug() << m_config->fileName();
}

void LoginWindow::onLogin(const QString &str)
{
    clearMessage();
    QString name = m_nameLabel->text();
    if(name == tr("Login")) {   //认证用户
        m_nameLabel->setText(str);
        m_passwordEdit->setText("");
        m_passwordEdit->setType(QLineEdit::Password);
        m_greeter->authenticate(str);
        qDebug() << "login: " << name;
    }
    else {  //发送密码
        m_greeter->respond(str);
        m_waiting.load(":/resource/waiting.png");
        m_passwordEdit->showIcon("*");  //当没有输入密码登录时，也显示等待提示
        m_passwordEdit->setIcon(QIcon(m_waiting));
        startWaiting();
    }
    m_passwordEdit->setText("");

}

void LoginWindow::onShowPrompt(QString text, QLightDM::Greeter::PromptType type)
{
    qDebug()<< "prompt: "<< text;
//    if(text == "") {    //显示生物识别窗口
//        int bioWid;
//        QWindow *bioWindow = QWindow::fromWinId(bioWid);
//        QWidget *bioWidget = QWidget::createWindowContainer(bioWindow, this, Qt::Widget);
//        bioWidget->setGeometry(m_passwordEdit->geometry());
//        bioWidget->show();
//        return;
//    }
    if(m_timer->isActive())
        stopWaiting();
    if(!text.isEmpty())
        m_passwordEdit->show();
    m_passwordEdit->setFocus();
    if(type != QLightDM::Greeter::PromptTypeSecret)
        m_passwordEdit->setType(QLineEdit::Normal);
    else
        m_passwordEdit->setType(QLineEdit::Password);

    m_passwordEdit->setPrompt(text);
}

void LoginWindow::onShowMessage(QString text, QLightDM::Greeter::MessageType type)
{
    qDebug()<< "message: "<< text;
    int lineNum = text.count('\n') + 1;
    int height = 20 * lineNum;  //label的高度
    if(m_messageLabels.size() >= 1) {
    //调整窗口大小
        this->resize(this->width(), this->height()+height);
    //移动密码输入框的位置
        m_passwordEdit->move(m_passwordEdit->geometry().left(),
                             m_passwordEdit->geometry().top() + height);
    }
    //每条message添加一个label
    int top = m_passwordEdit->geometry().top() - height - 10;
    QLabel *msgLabel = new QLabel(this);
    msgLabel->setObjectName(QStringLiteral("msgLabel"));
    msgLabel->setGeometry(QRect(220, top, 300, height));
    QPalette plt;
    if(type == QLightDM::Greeter::MessageTypeError)
        plt.setColor(QPalette::WindowText, Qt::red);
    else if(type == QLightDM::Greeter::MessageTypeInfo)
        plt.setColor(QPalette::WindowText, Qt::white);
    msgLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    msgLabel->setPalette(plt);
    msgLabel->setText(text);
    msgLabel->show();
    m_messageLabels.push_back(msgLabel);
}

void LoginWindow::onAuthenticationComplete()
{
    stopWaiting();
    if(m_greeter->isAuthenticated()) {
        qDebug()<< "authentication success";
//        m_greeter->startSession();
        saveLastLoginUser();
        Q_EMIT authenticationSuccess();
    } else {
        qDebug() << "authentication failed";
//        addMessage(tr("Incorrect password, please input again"));
        onShowMessage(tr("Incorrect password, please input again"), QLightDM::Greeter::MessageTypeInfo);
        m_passwordEdit->clear();
        m_greeter->authenticate(m_nameLabel->text());
    }
}