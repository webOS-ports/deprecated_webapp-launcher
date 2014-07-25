/*
 * Copyright (C) 2013 Simon Busch <morphis@gravedo.de>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>
 */

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QQuickView>
#include <QFile>
#include <QUrl>
#include <QtWebKitVersion>

#include <luna-service2++/message.hpp>
#include <luna-service2++/call.hpp>

#include <LocalePreferences.h>

#include "../webapplication.h"
#include "../webapplicationwindow.h"
#include "../systemtime.h"
#include "palmsystemextension.h"
#include "deviceinfo.h"

namespace luna
{

PalmSystemExtension::PalmSystemExtension(WebApplicationWindow *applicationWindow, QObject *parent) :
    BaseExtension("PalmSystem", applicationWindow, parent),
    mApplicationWindow(applicationWindow),
    mLunaPubHandle(NULL, true)
{
    applicationWindow->registerUserScript(QUrl("qrc:///extensions/PalmSystem.js"));

    mLunaPubHandle.attachToLoop(g_main_context_default());
}

void PalmSystemExtension::stageReady()
{
    qDebug() << __PRETTY_FUNCTION__;
    mApplicationWindow->stageReady();
}

void PalmSystemExtension::activate()
{
    qDebug() << __PRETTY_FUNCTION__;
    mApplicationWindow->focus();
}

void PalmSystemExtension::deactivate()
{
    qDebug() << __PRETTY_FUNCTION__;
    mApplicationWindow->unfocus();
}

void PalmSystemExtension::stagePreparing()
{
    qDebug() << __PRETTY_FUNCTION__;
    mApplicationWindow->stagePreparing();
}

void PalmSystemExtension::show()
{
    qDebug() << __PRETTY_FUNCTION__;
    mApplicationWindow->show();
}

void PalmSystemExtension::hide()
{
    qDebug() << __PRETTY_FUNCTION__;
    mApplicationWindow->hide();
}

void PalmSystemExtension::setWindowProperties(const QString &properties)
{
    qDebug() << __PRETTY_FUNCTION__ << properties;
}

void PalmSystemExtension::enableFullScreenMode(bool enable)
{
    qDebug() << __PRETTY_FUNCTION__ << enable;
}

void PalmSystemExtension::removeBannerMessage(int id)
{
    qDebug() << __PRETTY_FUNCTION__;

    QString appId = mApplicationWindow->application()->id();

    QJsonObject params;
    params.insert("id", id);

    QJsonDocument document(params);

    LS::Call call = mLunaPubHandle.callOneReply("luna://org.webosports.notifications/closeNotification",
                                                document.toJson().constData(),
                                                appId.toUtf8().constData());
}

void PalmSystemExtension::clearBannerMessages()
{
    qDebug() << __PRETTY_FUNCTION__;

    QString appId = mApplicationWindow->application()->id();

    LS::Call call = mLunaPubHandle.callOneReply("luna://org.webosports.notifications/closeAllNotifications",
                                                "{}", appId.toUtf8().constData());
}

void PalmSystemExtension::keepAlive(bool keep)
{
    qDebug() << __PRETTY_FUNCTION__ << keep;
    mApplicationWindow->setKeepAlive(keep);
}

void PalmSystemExtension::markFirstUseDone()
{
    QFile firstUseMarker("/var/luna/preferences/ran-first-use");
    firstUseMarker.open(QIODevice::ReadWrite);
    firstUseMarker.close();
}

void PalmSystemExtension::setProperty(const QString &name, const QVariant &value)
{
    qDebug() << __PRETTY_FUNCTION__ << name << value;
}

QString PalmSystemExtension::getProperty(const QJsonArray &params)
{
    qDebug() << __PRETTY_FUNCTION__ << params;

    if (params.count() != 1 || !params.at(0).isString())
        return QString("");

    QString name = params.at(0).toString();
    QString result = "";

    if (name == "launchParams")
        result = mApplicationWindow->application()->parameters();
    else if (name == "hasAlphaHole")
        result = QString("false");
    else if (name == "locale" || name == "locales.UI")
        result = QString::fromStdString(LocalePreferences::instance()->locale());
    else if (name == "localeRegion")
        result = QString::fromStdString(LocalePreferences::instance()->localeRegion());
    else if (name == "timeFormat")
        result = QString::fromStdString(LocalePreferences::instance()->timeFormat());
    else if (name == "timeZone" || name == "timezone")
        result = SystemTime::instance()->timezone();
    else if (name == "isMinimal")
        result = QString("false");
    else if (name == "identifier")
        result = mApplicationWindow->application()->identifier();
    else if (name == "screenOrientation")
        result = QString("");
    else if (name == "windowOrientation")
        result = QString("");
    else if (name == "specifiedWindowOrientation")
        result = QString("");
    else if (name == "videoOrientation")
        result = QString("");
    else if (name == "deviceInfo")
        result = DeviceInfo::instance()->jsonString();
    else if (name == "isActivated")
        result = QString(mApplicationWindow->active() ? "true" : "false");
    else if (name == "activityId")
        result = QString("%1").arg(mApplicationWindow->application()->activityId());
    else if (name == "phoneRegion")
        result = QString::fromStdString(LocalePreferences::instance()->phoneRegion());
    else if (name == "version")
        result = QString(QTWEBKIT_VERSION_STR);

    qDebug() << __PRETTY_FUNCTION__ << "result" << result;

    return result;
}

QString PalmSystemExtension::handleSynchronousCall(const QString& funcName, const QJsonArray& params)
{
    QString response = "{}";

    if (funcName == "getResource")
        response = getResource(params);
    else if (funcName == "getIdentifierForFrame")
        response = getIdentifierForFrame(params);
    else if (funcName == "getProperty")
        response = getProperty(params);
    else if (funcName == "addBannerMessage")
        response = addBannerMessage(params);

    return response;
}

QString PalmSystemExtension::getResource(const QJsonArray& params)
{
    qDebug() << __PRETTY_FUNCTION__ << params;

    if (params.count() != 2 || !params.at(0).isString())
        return QString("");

    QString path = params.at(0).toString();
    if (path.startsWith("file://"))
        path = path.right(path.size() - 7);

    if (!mApplicationWindow->application()->validateResourcePath(path)) {
        qDebug() << "WARNING: Access to path" << path << "is not allowed";
        return QString("");
    }

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return QString("");

    QByteArray data = file.readAll();

    return data;
}

QString PalmSystemExtension::getIdentifierForFrame(const QJsonArray &params)
{
    qDebug() << __PRETTY_FUNCTION__ << params;

    if (params.count() != 2 || !params.at(0).isString() || !params.at(0).isString())
        return QString("");

    QString id(params.at(0).toString());
    QString url(params.at(1).toString());

    return mApplicationWindow->getIdentifierForFrame(id, url);
}

QString PalmSystemExtension::addBannerMessage(const QJsonArray &params)
{
    qDebug() << __PRETTY_FUNCTION__ << params;

    if (params.count() != 7)
        return QString("");

    QString appId = mApplicationWindow->application()->id();

    QJsonObject notificationParams;
    notificationParams.insert("summary", params.at(0).toString());
    notificationParams.insert("appName", appId);
    notificationParams.insert("appIcon", params.at(2).toString());
    notificationParams.insert("expireTimeout", params.at(5).toInt());

    QJsonObject hints;
    hints.insert("params", params.at(1).toString());
    hints.insert("sound-class", params.at(3).toString());
    hints.insert("sound-file", params.at(4).toString());

    notificationParams.insert("hints", hints);

    QJsonDocument document(notificationParams);

    LS::Call call = mLunaPubHandle.callOneReply("luna://org.webosports.notifications/createNotification",
                                                document.toJson().constData(),
                                                appId.toUtf8().constData());
    LS::Message message(call.get());

    QJsonObject response = QJsonDocument::fromJson(message.getPayload()).object();

    if (!response.contains("id"))
        return QString("");

    return QString("%1").arg(response.value("id").toInt());
}


} // namespace luna
