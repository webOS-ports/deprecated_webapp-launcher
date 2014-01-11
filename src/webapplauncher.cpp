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

#include <QDebug>
#include <QDir>
#include <QtWebKit/private/qquickwebview_p.h>
#include <QTimer>

#include "applicationdescription.h"
#include "webapplauncher.h"
#include "webapplication.h"

namespace luna
{

WebAppLauncher::WebAppLauncher(int &argc, char **argv)
    : QGuiApplication(argc, argv),
      mLaunchedApp(0)
{
    setApplicationName("WebAppLauncher");

    QQuickWebViewExperimental::setFlickableViewportEnabled(false);

    connect(this, SIGNAL(aboutToQuit()), this, SLOT(onAboutToQuit()));

    // We're using a static list here to mark specific applications allowed to run in
    // headless mode (primary window will be not visible). The list should only contain
    // legacy applications. All new applications should not use the headless mode anymore
    // and will refuse to start. There should really no need to extend this list and
    // therefore it will be kept static forever.
    mAllowedHeadlessApps << "com.palm.app.email";
    mAllowedHeadlessApps << "com.palm.app.calendar";
    mAllowedHeadlessApps << "com.palm.app.clock";
    mAllowedHeadlessApps << "org.webosinternals.tweaks";
}

WebAppLauncher::~WebAppLauncher()
{
    onAboutToQuit();
}

bool WebAppLauncher::validateApplication(const ApplicationDescription& desc)
{
    if (desc.id().length() == 0)
        return false;

    if (desc.entryPoint().isLocalFile() && !QFile::exists(desc.entryPoint().toLocalFile()))
        return false;

    if (desc.headless() && !mAllowedHeadlessApps.contains(desc.id()))
        return false;

    return true;
}

void WebAppLauncher::launchApp(const QString &manifestPath, const QString &parameters)
{
    QFile manifestFile(manifestPath);
    if (!manifestFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning("Failed to read application manifest %s",
                 manifestPath.toUtf8().constData());
        return;
    }

    QString manifestData = QTextStream(&manifestFile).readAll();
    manifestFile.close();

    QString applicationBasePath = QFileInfo(manifestPath).absoluteDir().path();
    qDebug() << "applicationBasePath" << applicationBasePath;
    ApplicationDescription desc(manifestData, applicationBasePath);

    if (!validateApplication(desc)) {
        qWarning("Got invalid application description for app %s",
                 desc.id().toUtf8().constData());
        return;
    }

    // We set the application id as application name so that locally stored things for
    // each application are separated and remain after the application was stopped.
    QCoreApplication::setApplicationName(desc.id());

    QQuickWebViewExperimental::setFlickableViewportEnabled(desc.flickable());

    QString processId = QString("%0").arg(applicationPid());
    QString windowType = "card";
    QUrl entryPoint = desc.entryPoint();
    WebApplication *app = new WebApplication(this, entryPoint, windowType,
                                             desc, parameters, processId);
    connect(app, SIGNAL(closed()), this, SLOT(onApplicationWindowClosed()));

    this->setQuitOnLastWindowClosed(false);

    mLaunchedApp = app;

    this->exec();
}

void WebAppLauncher::onAboutToQuit()
{
    if( mLaunchedApp )
        delete mLaunchedApp;
    mLaunchedApp = NULL;
}

void WebAppLauncher::onApplicationWindowClosed()
{
    quit();
}

} // namespace luna
