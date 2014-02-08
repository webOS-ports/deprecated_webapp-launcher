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

import QtQuick 2.0
import QtWebKit 3.0
import QtWebKit.experimental 1.0
import "extensionmanager.js" as ExtensionManager
import LunaNext 0.1
import Connman 0.2
import "."

Flickable {
   id: webViewContainer

   anchors.fill: parent

   property int numRestarts: 0
   property int maxRestarts: 3

   NetworkManager {
       id: networkManager

       property string oldState: "unknown"

       onStateChanged: {
           // When we are online again reload the web view in order to start the application
           // which is still visible to the user
           if (oldState !== networkManager.state && networkManager.state === "online")
               webView.reload();
       }
   }

    Rectangle {
        id: offlinePanel

        color: "white"
        visible: webApp.internetConnectivityRequired && networkManager.state !== "online"
        anchors.fill: parent

        z: 10

        Text {
            anchors.centerIn: parent
            color: "black"
            text: "Internet connectivity is required but not available"
            font.pixelSize: 20
            font.family: "Prelude"
        }
    }

    WebView {
        id: webView
        objectName: "webView"

        anchors.fill: parent

        url: webAppUrl

        experimental.preferences.navigatorQtObjectEnabled: true
        experimental.preferences.localStorageEnabled: true
        experimental.preferences.offlineWebApplicationCacheEnabled: true
        experimental.preferences.webGLEnabled: true
        experimental.preferences.developerExtrasEnabled: true
        experimental.preferences.universalAccessFromFileURLsAllowed: true
        experimental.preferences.fileAccessFromFileURLsAllowed: true

        experimental.preferences.standardFontFamily: "Prelude"
        experimental.preferences.fixedFontFamily: "Courier new"
        experimental.preferences.serifFontFamily: "Times New Roman"
        experimental.preferences.cursiveFontFamily: "Prelude"

        experimental.userAgent: webApp.userAgent

        onNavigationRequested: {
            var action = WebView.AcceptRequest;
            var url = request.url.toString();

            if (webApp.urlsAllowed && webApp.urlsAllowed.length !== 0) {
                action = WebView.IgnoreRequest;
                for (var i = 0; i < webApp.urlsAllowed.length; ++i) {
                    var pattern = webApp.urlsAllowed[i];
                    if (url.match(pattern)) {
                        action = WebView.AcceptRequest;
                        break;
                    }
                }
            }

            request.action = action;

            // If we're not handling the URL forward it to be opened within the system
            // default web browser in a safe environment
            if (request.action === WebView.IgnoreRequest) {
                Qt.openUrlExternally(url);
                return;
            }
        }

        Component.onCompleted: {
            // Only when we have a system application we enable the webOS API and the
            // PalmServiceBridge to avoid remote applications accessing unwanted system
            // internals
            if (webApp.trustScope === "system") {
                if (experimental.hasOwnProperty('userScriptsInjectAtStart') &&
                    experimental.hasOwnProperty('userScriptsForAllFrames')) {
                    experimental.userScripts = webAppWindow.userScripts;
                    experimental.userScriptsInjectAtStart = true;
                    experimental.userScriptsForAllFrames = true;
                }

                if (experimental.preferences.hasOwnProperty("palmServiceBridgeEnabled"))
                    experimental.preferences.palmServiceBridgeEnabled = true;

                if (experimental.preferences.hasOwnProperty("privileged"))
                    experimental.preferences.privileged = webApp.privileged;
            }

            if (experimental.preferences.hasOwnProperty("logsPageMessagesToSystemConsole"))
                experimental.preferences.logsPageMessagesToSystemConsole = true;

            if (experimental.preferences.hasOwnProperty("suppressIncrementalRendering"))
                experimental.preferences.suppressIncrementalRendering = true;
        }

        experimental.onMessageReceived: {
            ExtensionManager.messageHandler(message);
        }

        Connections {
            target: webAppWindow

            onJavaScriptExecNeeded: {
                webView.experimental.evaluateJavaScript(script);
            }

            onExtensionWantsToBeAdded: {
                ExtensionManager.addExtension(name, object);
            }
        }

        Connections {
            target: webView.experimental
            onProcessDidCrash: {
                if (numRestarts < maxRestarts) {
                    console.log("ERROR: The web process has crashed. Restart it ...");
                    webView.url = webAppUrl;
                    webView.reload();
                    numRestarts += 1;
                }
                else {
                    console.log("CRITICAL: restarted application " + numRestarts
                                + " times. Closing it now");
                    Qt.quit();
                }
            }
        }
    }
}
