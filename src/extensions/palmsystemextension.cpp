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

#include <luna-service2++/message.hpp>
#include <luna-service2++/call.hpp>

#include "../webapplication.h"
#include "../webapplicationwindow.h"
#include "palmsystemextension.h"

namespace luna
{

PalmSystemExtension::PalmSystemExtension(WebApplicationWindow *applicationWindow, QObject *parent) :
    BaseExtension("PalmSystem", applicationWindow, parent),
    mPropertyChangeHandlerCallbackId(0),
    mApplicationWindow(applicationWindow),
    mLunaPubHandle(NULL, true)
{
    applicationWindow->registerUserScript(QUrl("qrc:///extensions/PalmSystem.js"));

    connect(applicationWindow->application(), SIGNAL(parametersChanged()), this, SLOT(onParametersChanged()));

    mLunaPubHandle.attachToLoop(g_main_context_default());
}

void PalmSystemExtension::onParametersChanged()
{
    mAppEnvironment->executeScript(QString("__PalmSystem.launchParams = '%1';")
                                   .arg(mApplicationWindow->application()->parameters()));
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
}

void PalmSystemExtension::clearBannerMessages()
{
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

void PalmSystemExtension::registerPropertyChangeHandler(int successCallbackId, int errorCallbackId)
{
    mPropertyChangeHandlerCallbackId = successCallbackId;
}

void PalmSystemExtension::setProperty(const QString &name, const QVariant &value)
{
    qDebug() << __PRETTY_FUNCTION__ << name << value;
}

void PalmSystemExtension::getProperty(int successCallbackId, int errorCallbackId, const QString &name)
{
    if (name == "launchParams") {
        callbackWithoutRemove(successCallbackId, mApplicationWindow->application()->parameters());
    }
    else if (name == "identifier") {
        callbackWithoutRemove(successCallbackId, mApplicationWindow->application()->identifier());
    }
    else if (name == "activityId") {
        callbackWithoutRemove(successCallbackId, QString("%1").arg(mApplicationWindow->application()->activityId()));
    }
}

void PalmSystemExtension::initializeProperties(int successCallbackId, int errorCallbackId)
{
    QJsonObject rootObj;

    rootObj.insert("launchParams", QJsonValue(mApplicationWindow->application()->parameters()));
    rootObj.insert("hasAlphaHole", QJsonValue(false));
    rootObj.insert("locale", QJsonValue(QString("")));
    rootObj.insert("localeRegion", QJsonValue(QString("")));
    rootObj.insert("timeFormat", QJsonValue(QString("")));
    rootObj.insert("timeZone", QJsonValue(QString("")));
    rootObj.insert("isMinimal", QJsonValue(QString("")));
    rootObj.insert("identifier", QJsonValue(mApplicationWindow->application()->identifier()));
    rootObj.insert("version", QJsonValue(QString("")));
    rootObj.insert("screenOrientation", QJsonValue(QString("")));
    rootObj.insert("windowOrientation", QJsonValue(QString("")));
    rootObj.insert("specifiedWindowOrientation", QJsonValue(QString("")));
    rootObj.insert("videoOrientation", QJsonValue(QString("")));
    rootObj.insert("deviceInfo", QJsonValue(QString("{\"modelName\":\"unknown\",\"platformVersion\":\"0.0.0\"}")));
    rootObj.insert("isActivated", QJsonValue(true));
    rootObj.insert("activityId", QJsonValue(mApplicationWindow->application()->activityId()));
    rootObj.insert("phoneRegion", QJsonValue(QString("")));

    QJsonDocument document(rootObj);

    callback(successCallbackId, document.toJson());
}

QString PalmSystemExtension::handleSynchronousCall(const QString& funcName, const QJsonArray& params)
{
    QString response = "{}";

    if (funcName == "getResource")
        response = getResource(params);
    else if (funcName == "getIdentifierForFrame")
        response = getIdentifierForFrame(params);
    else if (funcName == "getActivityId")
        response = getActivityId(params);
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

QString PalmSystemExtension::getActivityId(const QJsonArray& params)
{
    return QString("%1").arg(mApplicationWindow->application()->activityId());
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

    LS::Call call = mLunaPubHandle.callOneReply("luna://org.webosports.luna/createNotification",
                                                document.toJson().constData(),
                                                appId.toUtf8().constData());
    LS::Message message(&mLunaPubHandle, call.get());

    QJsonObject response = QJsonDocument::fromJson(message.getPayload()).object();

    if (!response.contains("id"))
        return QString("");

    return QString("%1").arg(response.value("id").toInt());
}


} // namespace luna
