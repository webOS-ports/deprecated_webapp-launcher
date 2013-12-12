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

#ifndef WEBAPPLICATIONWINDOW_H
#define WEBAPPLICATIONWINDOW_H

#include <QObject>
#include <QQmlEngine>
#include <QQuickWindow>
#include <QTimer>

#include <QtWebKit/private/qquickwebview_p.h>
#ifndef WITH_UNMODIFIED_QTWEBKIT
#include <QtWebKit/private/qwebnewpagerequest_p.h>
#endif
#include <QtWebKit/private/qwebloadrequest_p.h>

#include <scriptexecutor.h>

namespace luna
{

class WebAppBasePlugin;
class WebApplication;

class WebApplicationWindow : public ScriptExecutor
{
    Q_OBJECT
    Q_PROPERTY(WebApplication *application READ application)

public:
    explicit WebApplicationWindow(WebApplication *application, const QUrl& url, const QString& windowType,
                                  bool headless = false, QObject *parent = 0);
    ~WebApplicationWindow();

    WebApplication *application() const;

    void stagePreparing();
    void stageReady();

    void show();
    void hide();

    bool headless() const;
    bool keepAlive() const;
    QQuickWebView *webView() const;

    void setKeepAlive(bool alive);

    void executeScript(const QString &script);

signals:
    void javaScriptExecNeeded(const QString &script);
    void pluginWantsToBeAdded(const QString &name, QObject *object);
    void closed();

protected:
    bool eventFilter(QObject *object, QEvent *event);

private slots:
#ifndef WITH_UNMODIFIED_QTWEBKIT
    void onCreateNewPage(QWebNewPageRequest *request);
    void onSyncMessageReceived(const QVariantMap& message, QString& response);
#endif
    void onClosed();
    void onLoadingChanged(QWebLoadRequest *request);
    void onShowWindowTimeout();

private:
    WebApplication *mApplication;
    QMap<QString, WebAppBasePlugin*> mPlugins;
    QQmlEngine mEngine;
    QObject *mRootItem;
    QQuickWindow *mWindow;
    bool mHeadless;
    QQuickWebView *mWebView;
    QUrl mUrl;
    QString mWindowType;
    bool mKeepAlive;
    bool mStagePreparing;
    bool mStageReady;
    QTimer mShowWindowTimer;

    void createAndSetup();
    void createPlugins();
    void createAndInitializePlugin(WebAppBasePlugin *plugin);
    void setWindowProperty(const QString &name, const QVariant &value);
    void setupPage();
};

} // namespace luna

#endif // WEBAPPLICATIONWINDOW_H
