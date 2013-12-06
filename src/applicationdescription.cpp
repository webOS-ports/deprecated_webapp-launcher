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
#include <QFile>
#include <QDebug>

#include "applicationdescription.h"

namespace luna
{

ApplicationDescription::ApplicationDescription() :
    mHeadless(false),
    mTrustScope(ApplicationDescription::TrustScopeSystem)
{
}

ApplicationDescription::ApplicationDescription(const ApplicationDescription& other) :
    mId(other.id()),
    mTitle(other.title()),
    mIcon(other.icon()),
    mEntryPoint(other.entryPoint()),
    mHeadless(other.headless()),
    mTrustScope(other.trustScope())
{
}

ApplicationDescription::ApplicationDescription(const QString &data, const QString &applicationBasePath) :
    mHeadless(false),
    mApplicationBasePath(applicationBasePath),
    mTrustScope(ApplicationDescription::TrustScopeSystem)
{
    initializeFromData(data);
}

ApplicationDescription::~ApplicationDescription()
{
}

void ApplicationDescription::initializeFromData(const QString &data)
{
    QJsonDocument document = QJsonDocument::fromJson(data.toUtf8());

    if (!document.isObject()) {
        qWarning() << "Failed to parse application description";
        return;
    }

    QJsonObject rootObject = document.object();

    if (rootObject.contains("id") && rootObject.value("id").isString())
        mId = rootObject.value("id").toString();

    if (rootObject.contains("main") && rootObject.value("main").isString())
        mEntryPoint = locateEntryPoint(rootObject.value("main").toString());

    if (rootObject.contains("noWindow") && rootObject.value("noWindow").isBool())
        mHeadless = rootObject.value("noWindow").toBool();

    if (rootObject.contains("title") && rootObject.value("title").isString())
        mTitle = rootObject.value("title").toString();

    if (rootObject.contains("icon") && rootObject.value("icon").isString()) {
        QString iconPath = rootObject.value("icon").toString();

        // we're only allow locally stored icons so we must prefix them with file:// to
        // store it in a QUrl object
        if (!iconPath.startsWith("file://"))
            iconPath.prepend("file://");

        mIcon = iconPath;
    }

    if (mIcon.isEmpty() || !mIcon.isLocalFile() || !QFile::exists(mIcon.toLocalFile()))
        mIcon = QUrl("qrc:///qml/images/default-app-icon.png");
}

QUrl ApplicationDescription::locateEntryPoint(const QString &entryPoint)
{
    QUrl entryPointAsUrl(entryPoint);

    if (entryPointAsUrl.scheme() != "file" && entryPointAsUrl.scheme() != "") {
        qWarning("Entry point %s for application %s is invalid",
                 entryPoint.toUtf8().constData(),
                 mId.toUtf8().constData());
        return QUrl("");
    }

    if (entryPointAsUrl.scheme() == "file")
        return entryPointAsUrl;

    return QUrl(QString("file://%1/%2").arg(mApplicationBasePath).arg(entryPoint));
}

QString ApplicationDescription::id() const
{
    return mId;
}

QString ApplicationDescription::title() const
{
    return mTitle;
}

QUrl ApplicationDescription::icon() const
{
    return mIcon;
}

QUrl ApplicationDescription::entryPoint() const
{
    return mEntryPoint;
}

bool ApplicationDescription::headless() const
{
    return mHeadless;
}

ApplicationDescription::TrustScope ApplicationDescription::trustScope() const
{
    return mTrustScope;
}

}
