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
#include <QStringList>
#include <glib.h>

#include "webapplauncher.h"

#define VERSION "0.1"
#define XDG_RUNTIME_DIR_DEFAULT "/tmp/luna-session"

static gchar *option_appinfo = NULL;
static gchar *option_url = NULL;
static gchar *option_parameters = NULL;
static gchar *option_window_type = NULL;
static gboolean option_debug = FALSE;
static gboolean option_version = FALSE;

static GOptionEntry options[] = {
    { "appinfo", 'a', 0, G_OPTION_ARG_STRING, &option_appinfo,
        "Application manifest of the application to start" },
    { "url", 'u', 0, G_OPTION_ARG_STRING, &option_url,
        "URL of the entrypoint for the application" },
    { "parameters", 'p', 0, G_OPTION_ARG_STRING, &option_parameters,
        "Parameters for the application in JSON format" },
    { "window-type", 'w', 0, G_OPTION_ARG_STRING, &option_window_type,
        "Window type used for the application window (supported are "
        "\"card\" and \"launcher\"; will default to \"card\")" },
    { "debug", 'd', 0, G_OPTION_ARG_NONE, &option_debug,
        "Enable debugging modus. This will start the webkit inspector "
        "on http://localhost:1122/" },
    { "version", 'v', 0, G_OPTION_ARG_NONE, &option_version,
        "Show version information and exit" },
    { NULL },
};

int main(int argc, char **argv)
{
    GError *error = NULL;
    GOptionContext *context;

    if (qgetenv("DISPLAY").isEmpty()) {
        setenv("EGL_PLATFORM", "wayland", 0);
        setenv("QT_QPA_PLATFORM", "wayland", 0);
        setenv("XDG_RUNTIME_DIR", XDG_RUNTIME_DIR_DEFAULT, 0);
        setenv("QT_IM_MODULE", "Maliit", 0);
        setenv("QT_WAYLAND_DISABLE_WINDOWDECORATION", "1", 1);
    }

    luna::WebAppLauncher webAppLauncher(argc, argv);

    context = g_option_context_new(NULL);
    g_option_context_add_main_entries(context, options, NULL);

    if (!g_option_context_parse(context, &argc, &argv, &error)) {
        if (error) {
            g_printerr("%s\n", error->message);
            g_error_free(error);
        }
        else
            g_printerr("An unknown error occurred\n");
        exit(1);
    }

    g_option_context_free(context);

    if (option_version) {
        g_message("webapp-launcher %s", VERSION);
        goto cleanup;
    }
    if (option_debug)
        setenv("QTWEBKIT_INSPECTOR_SERVER", "11222", 0);
    if (option_appinfo)
        webAppLauncher.setAppDesc(QString(option_appinfo));
    if (option_url)
        webAppLauncher.setUrl(QString(option_url));
    if (option_parameters)
        webAppLauncher.setParameters(QString(option_parameters));
    if (option_window_type)
        webAppLauncher.setWindowType(QString(option_window_type));

    if (!webAppLauncher.initialize()) {
        qWarning("Failed to initialize application!");
        goto cleanup;
    }

    webAppLauncher.exec();

cleanup:
    g_free(option_appinfo);
    g_free(option_url);
    g_free(option_parameters);
    g_free(option_window_type);

    return 0;
}
