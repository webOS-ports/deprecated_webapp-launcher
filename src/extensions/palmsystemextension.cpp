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

#include "../webapplication.h"
#include "../webapplicationwindow.h"
#include "palmsystemextension.h"

namespace luna
{

PalmSystemExtension::PalmSystemExtension(WebApplicationWindow *applicationWindow, QObject *parent) :
    BaseExtension("PalmSystem", applicationWindow, parent),
    mPropertyChangeHandlerCallbackId(0),
    mApplicationWindow(applicationWindow)
{
}

void PalmSystemExtension::stageReady()
{
    mApplicationWindow->stageReady();
}

void PalmSystemExtension::activate()
{
}

void PalmSystemExtension::deactivate()
{
}

void PalmSystemExtension::stagePreparing()
{
    mApplicationWindow->stagePreparing();
}

void PalmSystemExtension::show()
{
    mApplicationWindow->show();
}

void PalmSystemExtension::hide()
{
    mApplicationWindow->hide();
}

void PalmSystemExtension::setWindowProperties(const QString &properties)
{
}

void PalmSystemExtension::enableFullScreenMode(bool enable)
{
}

void PalmSystemExtension::addBannerMessage(int id, const QString &msg,
                                        const QString &params, const QString &icon, const QString &soundClass,
                                        const QString &soundFile, int duration, bool doNotSuppress)
{
}

void PalmSystemExtension::removeBannerMessage(int id)
{
}

void PalmSystemExtension::clearBannerMessages()
{
}

void PalmSystemExtension::keepAlive(bool keep)
{
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
}

void PalmSystemExtension::getProperty(int successCallbackId, int errorCallbackId, const QString &name)
{
    if (name == "launchParams") {
        callbackWithoutRemove(successCallbackId, mApplicationWindow->application()->parameters());
    }
    else if (name == "identifier") {
        callbackWithoutRemove(successCallbackId, mApplicationWindow->application()->id());
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
    rootObj.insert("identifier", QJsonValue(mApplicationWindow->application()->id()));
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

    return response;
}

QString PalmSystemExtension::getResource(const QJsonArray& params)
{
    if (params.count() != 2 || !params.at(0).isString())
        return QString("");

    QFile file(params.at(0).toString());
    if (!file.open(QIODevice::ReadOnly))
        return QString("");

    QByteArray data = file.readAll();

    return QString(data);
}

} // namespace luna
