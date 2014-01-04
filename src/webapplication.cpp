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
#include <QQmlContext>
#include <QJsonObject>
#include <QJsonDocument>

#include <QtWebKit/private/qquickwebview_p.h>
#ifndef WITH_UNMODIFIED_QTWEBKI
#include <QtWebKit/private/qwebnewpagerequest_p.h>
#endif

#include <set>
#include <string>

#include "webapplauncher.h"
#include "applicationdescription.h"
#include "webapplication.h"
#include "webapplicationwindow.h"
#include "webapplicationplugin.h"

#include <Settings.h>

#include <webos_application.h>

#include <sys/types.h>
#include <unistd.h>

namespace luna
{

struct webos_application_event_handlers event_handlers = {
    .activate = NULL,
    .deactivate = NULL,
    .suspend = NULL,
    .relaunch = WebApplication::relaunch_cb,
    .lowmemory = NULL
};

class ResourcePathValidator
{
public:
    static ResourcePathValidator& instance()
    {
        static ResourcePathValidator instance;
        return instance;
    }

    bool validate(const QString &path, bool privileged)
    {
        if (findPathInList(mAllowedTargetPaths, path))
            return true;
        if (privileged && findPathInList(mPrivilegedAppPaths, path))
            return true;
        if (!privileged && findPathInList(mUnprivilegedAppPaths, path))
            return true;

        return false;
    }

private:
    ResourcePathValidator()
    {
        // NOTE: below set of paths are taken from the configuration set in the webkit used in
        // webOS 3.0.5. See http://downloads.help.palm.com/opensource/3.0.5/webcore-patch.gz

        // paths allowed for every app
        mAllowedTargetPaths << "/usr/palm/frameworks";
        mAllowedTargetPaths << "/media/internal";
        mAllowedTargetPaths << "/usr/lib/luna/luna-media";
        mAllowedTargetPaths << "/var/luna/files";
        mAllowedTargetPaths << "/var/luna/data/extractfs";
        mAllowedTargetPaths << "/var/luna/data/im-avatars";
        mAllowedTargetPaths <<  "/usr/palm/applications/com.palm.app.contacts/sharedWidgets/";
        mAllowedTargetPaths << "/usr/palm/sysmgr/";
        mAllowedTargetPaths << "/usr/palm/public";
        mAllowedTargetPaths << "/var/file-cache/";
        mAllowedTargetPaths << "/usr/lib/luna/system/luna-systemui/images/";
        mAllowedTargetPaths << "/usr/lib/luna/system/luna-systemui/app/FilePicker";

        // paths only allowed for privileged apps
        mPrivilegedAppPaths << "/usr/lib/luna/system/";   // system ui apps
        mPrivilegedAppPaths << "/usr/palm/applications/";  // Palm apps
        mPrivilegedAppPaths << "/var/usr/palm/applications/com.palm.";  // privileged apps like facebook
        mPrivilegedAppPaths << "/media/cryptofs/apps/usr/palm/applications/com.palm.";  // privileged 3rd party apps
        mPrivilegedAppPaths << "/usr/palm/sysmgr/";
        mPrivilegedAppPaths << "/var/usr/palm/applications/com/palm/";
        mPrivilegedAppPaths << "/media/cryptofs/apps/usr/palm/applications/com/palm/";

        // additional paths allowed for unprivileged apps
        mUnprivilegedAppPaths << "/var/usr/palm/applications/";
        mUnprivilegedAppPaths << "/media/cryptofs/apps/usr/palm/applications/";
    }

    bool findPathInList(const QStringList &list, const QString &path)
    {
        Q_FOREACH(QString item, list) {
            if (path.startsWith(item))
                return true;
        }
        return false;
    }

    QStringList mAllowedTargetPaths;
    QStringList mPrivilegedAppPaths;
    QStringList mUnprivilegedAppPaths;
};

WebApplication::WebApplication(WebAppLauncher *launcher, const QUrl& url, const QString& windowType,
                               const ApplicationDescription& desc, const QString& parameters,
                               const QString& processId, QObject *parent) :
    QObject(parent),
    mLauncher(launcher),
    mDescription(desc),
    mProcessId(processId),
    mIdentifier(mDescription.id() + " " + mProcessId),
    mParameters(parameters),
    mMainWindow(0),
    mLaunchedAtBoot(false),
    mPrivileged(false),
    mPlugin(0),
    mActivity(mIdentifier, desc.id(), processId)
{
    webos_application_init(desc.id().toUtf8().constData(), &event_handlers, this);
    webos_application_attach(g_main_loop_new(g_main_context_default(), TRUE));

    loadPlugin();

    // Only system applications with a specific id prefix are privileged to access
    // the private luna bus
    if (mDescription.trustScope() == ApplicationDescription::TrustScopeSystem &&
        (mDescription.id().startsWith("org.webosports") ||
         mDescription.id().startsWith("com.palm")) ||
         mDescription.id().startsWith("org.webosinternals"))
        mPrivileged = true;

    mMainWindow = new WebApplicationWindow(this, url, windowType, mDescription.headless());
    connect(mMainWindow, SIGNAL(closed()), this, SLOT(windowClosed()));

    const std::set<std::string> appsToLaunchAtBoot = Settings::LunaSettings()->appsToLaunchAtBoot;
    mLaunchedAtBoot = (appsToLaunchAtBoot.find(id().toStdString()) != appsToLaunchAtBoot.end());
}

WebApplication::~WebApplication()
{
}

void WebApplication::relaunch_cb(const char *parameters, void *user_data)
{
    WebApplication *webapp = static_cast<WebApplication*>(user_data);
    QString params(parameters);
    webapp->relaunch(params);
}

void WebApplication::loadPlugin()
{
    QFileInfo pluginPath(QString("%1/plugins/%2")
                         .arg(mDescription.basePath())
                         .arg(mDescription.pluginName()));

    if (pluginPath.absoluteFilePath().length() == 0 || !pluginPath.exists())
        return;

    mPlugin = new WebApplicationPlugin(pluginPath);
    if (!mPlugin->load()) {
        delete mPlugin;
        mPlugin = 0;
    }

    qDebug() << "Plugin" << mDescription.pluginName() << "successfully loaded";
}

void WebApplication::changeActivityFocus(bool focus)
{
    if (focus)
        mActivity.focus();
    else
        mActivity.unfocus();
}

void WebApplication::relaunch(const QString &parameters)
{
    qDebug() << __PRETTY_FUNCTION__ << "Relaunching application" << mDescription.id() << "with parameters" << parameters;

    mParameters = parameters;
    emit parametersChanged();

    mMainWindow->executeScript(QString("Mojo.relaunch();"));
}

#ifndef WITH_UNMODIFIED_QTWEBKIT

void WebApplication::createWindow(QWebNewPageRequest *request)
{
    qDebug() << __PRETTY_FUNCTION__ << "creating new window for url" << request->url();

    // child windows can never be headless ones!
    QString windowType = "card";
    WebApplicationWindow *window = new WebApplicationWindow(this, request->url(), windowType, false);

    connect(window, SIGNAL(closed()), this, SLOT(windowClosed()));

    request->setWebView(window->webView());

    window->show();

    mChildWindows.append(window);
}

#endif

void WebApplication::windowClosed()
{
    WebApplicationWindow *window = static_cast<WebApplicationWindow*>(sender());

    // if the window is marked as keep alive we don't close it
    if (window->keepAlive()) {
        qDebug() << "Not closing window cause it was configured to be kept alive";
        return;
    }

    // if it's a child window we remove it but have to take care about
    // some special conditions
    if (mChildWindows.contains(window)) {
        mChildWindows.removeOne(window);
        delete window;

        // if no child window is left close the main (headless) window too
        if (mChildWindows.count() == 0 && !mLaunchedAtBoot) {
            qDebug() << "All child windows of app" << id()
                     << "were closed so closing the main window too";

            delete mMainWindow;
            emit closed();
        }
    }
    else if (window == mMainWindow) {
        // the main window was closed so close all child windows too
        delete mMainWindow;

        qDebug() << "The main window of app " << id()
                 << "was closed, so closing all child windows too";

        foreach(WebApplicationWindow *child, mChildWindows) {
            delete child;
        }

        emit closed();
    }
}

bool WebApplication::validateResourcePath(const QString &path)
{
    return ResourcePathValidator::instance().validate(path, mPrivileged);
}

QString WebApplication::id() const
{
    return mDescription.id();
}

QString WebApplication::processId() const
{
    return mProcessId;
}

QUrl WebApplication::url() const
{
    return mDescription.entryPoint();
}

QUrl WebApplication::icon() const
{
    return mDescription.icon();
}

QString WebApplication::identifier() const
{
    return mIdentifier;
}

QString WebApplication::parameters() const
{
    return mParameters;
}

bool WebApplication::headless() const
{
    return mDescription.headless();
}

bool WebApplication::privileged() const
{
    return mPrivileged;
}

QString WebApplication::trustScope() const
{
    switch (mDescription.trustScope()) {
    case ApplicationDescription::TrustScopeSystem:
        return QString("system");
    case ApplicationDescription::TrustScopeRemote:
        return QString("remote");
    }

    return QString("unknown");
}

WebApplicationPlugin* WebApplication::plugin() const
{
    return mPlugin;
}

bool WebApplication::internetConnectivityRequired() const
{
    return mDescription.internetConnectivityRequired();
}

QStringList WebApplication::urlsAllowed() const
{
    return mDescription.urlsAllowed();
}

bool WebApplication::hasRemoteEntryPoint() const
{
    return mDescription.hasRemoteEntryPoint();
}

QString WebApplication::userAgent() const
{
    return mDescription.userAgent();
}

int WebApplication::activityId() const
{
    return mActivity.id();
}

} // namespace luna
