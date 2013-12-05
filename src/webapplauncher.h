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

#ifndef WEBAPPMANAGER_H_
#define WEBAPPMANAGER_H_

#include <QtGlobal>
#include <glib.h>
#include <QGuiApplication>
#include <QMap>
#include <QUrl>
#include <QFile>
#include <QTextStream>
#include <QStringList>

namespace luna
{

class ApplicationDescription;
class WebApplication;

class WebAppLauncher : public QGuiApplication
{
    Q_OBJECT

public:
    WebAppLauncher(int& argc, char **argv);
    virtual ~WebAppLauncher();

    void launchApp(const QString &manifestPath, const QString &parameters);

private slots:
    void onApplicationWindowClosed();
    void onAboutToQuit();

private:
    WebApplication *mLaunchedApp;
    QStringList mAllowedHeadlessApps;

    bool validateApplication(const ApplicationDescription& desc);
};

} // namespace luna

#endif
