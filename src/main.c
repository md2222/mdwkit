/*
 * Copyright (C) 2020 MD2222 
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <gtk/gtk.h>
#include <webkit2/webkit2.h>
#include <gdk/gdkkeysyms.h> // GDK_KEY_F11
#include <sys/dir.h>
#include <dirent.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include "configfile.h"

//using namespace std;

// gcc -DDEBUG your_program.c ...
#ifdef DEBUG
#define debug(fmt, ...) fprintf(stderr, "%s:%d: " fmt, __FILE__, __LINE__, ##__VA_ARGS__)
#else
#define debug(fmt, ...) do {} while (0) // No-op in release builds
#endif


typedef void (*msgResultFunc)(const char*);

#define MAXDIR  128

GtkWidget *winWeb;
gchar* appDir;
const gchar* userDataDir;
gchar* extDir;  // web_extensions_directory
gchar* pluginDir;
gchar* configPath = 0;
const gchar* currUrl = 0;

typedef struct 
{
    int x;
    int y;
    int w;
    int h;
    int max;
    int full;
    //void clear() { x = y = w = h = max = full = 0; };
    //bool isEmpty() {  return w * h <= 0;  };
} WinRect;

WinRect webWinRect = { .x=0, .y=0, .w=0, .h=0, .max=0, .full=0 };


void saveConf();

//----------------------------------------------------------------------------------------------------------------------

gint messageBox(GtkWidget *parent, const char* text, const char* caption, uint type)
{
   GtkWidget *dialog ;

   if (type & GTK_BUTTONS_YES_NO)
       dialog = gtk_message_dialog_new(GTK_WINDOW(parent), GTK_DIALOG_MODAL, GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO, "%s", text);
   else
       dialog = gtk_message_dialog_new(GTK_WINDOW(parent), GTK_DIALOG_MODAL, GTK_MESSAGE_WARNING, GTK_BUTTONS_OK, "%s", text);


   gtk_window_set_title(GTK_WINDOW(dialog), caption);
   gint result = gtk_dialog_run(GTK_DIALOG(dialog));
   gtk_widget_destroy( GTK_WIDGET(dialog) );

   return result;
}
//----------------------------------------------------------------------------------------------------------------------


gboolean isPointOnScreen(int x, int y) 
{
    GdkScreen *screen = gdk_screen_get_default();
    if (!screen) return FALSE;

    int max_w = gdk_screen_get_width(screen);
    int max_h = gdk_screen_get_height(screen);

    return (x >= 0 && x < max_w && y >= 0 && y < max_h);
}


void setWindowRect(GtkWindow *window, WinRect* rect)
{
    if (isPointOnScreen(rect->x, rect->y))
        gtk_window_move(window, rect->x, rect->y);
    else
        gtk_window_set_position(window, GTK_WIN_POS_CENTER);
        
    if (rect->w * rect->h > 0)
        gtk_window_set_default_size(window, rect->w, rect->h);
}


void getWindowRect(GtkWindow *window, WinRect* rect)
{
    gtk_window_get_position(window, &rect->x, &rect->y);
    gtk_window_get_size(window, &rect->w, &rect->h);
}


void loadConfig()
{
    debug("loadConfig:  %s\n", configPath);

    webWinRect = (WinRect){ .x=0, .y=0, .w=0, .h=0, .max=0, .full=0 };
    webWinRect.w = 1280;  webWinRect.h = 900;

    ConfigFile* cf = ConfigFile_open(configPath);
    if (!cf)
        printf("Config file open error\n");
    else
    {
        int* list = (int*)cf->getList(cf, "private", "mainWinRect", ListTypeInt, 6);
        if (list)
        {
            webWinRect.x = list[0]; webWinRect.y = list[1]; webWinRect.w = list[2]; webWinRect.h = list[3];
            webWinRect.max = list[4]; webWinRect.full = list[5];
        }
        
        cf->free(cf);
        free(cf);
    }
}


void saveConf()
{
    debug("saveConf:  %s\n", configPath);

    ConfigFile* cf = ConfigFile_open(configPath);
    if (!cf)
        printf("Config file open error\n");
    else
    {
        if (webWinRect.w * webWinRect.h > 0)
            cf->setList(cf, "private", "mainWinRect", ListTypeInt, &webWinRect, 6);

        cf->save(cf);
        cf->free(cf);
        free(cf);
    }
}


static gboolean onCloseWin(GtkWidget *widget, GdkEvent *event, gpointer data)
{
    if ( !(webWinRect.max || webWinRect.full) )
        getWindowRect(GTK_WINDOW(winWeb), &webWinRect);
    debug("onCloseWin:  getWindowRect = %d, %d, %d, %d, %d, %d\n", webWinRect.x, webWinRect.y, webWinRect.w, webWinRect.h, webWinRect.max, webWinRect.full);

    webkit_web_view_try_close(WEBKIT_WEB_VIEW(data));
    return TRUE;
}


void onCloseWebViewCb(WebKitWebView *web_view, gpointer data)
{
    debug("onCloseWebViewCb:  \n");

    saveConf();
    gtk_main_quit();
}


static void initialize_web_extensions (WebKitWebContext *context, gpointer user_data)
{
    printf("initialize_web_extensions...\n");
    webkit_web_context_set_web_extensions_directory(context, extDir);  //WEB_EXTENSIONS_DIRECTORY);
    webkit_web_context_set_web_extensions_initialization_user_data(context, g_variant_new_string(pluginDir));

    //webkit_web_context_get_plugins(context, NULL, (GAsyncReadyCallback)onGetPluginsCb, NULL);
}


static gboolean onMessageReceived(WebKitWebContext *context, WebKitUserMessage *message, gpointer user_data)
{
    printf("onMessageReceived:    %s\n", webkit_user_message_get_name(message));
    return TRUE;
}


static void onSizeAllocate(GtkWidget *widget, GtkAllocation *allocation)
{
    if ( !(webWinRect.max || webWinRect.full) )
    {
        gtk_window_get_size(GTK_WINDOW (widget), &webWinRect.w, &webWinRect.h);
        //debug("onSizeAllocate:   %d   %d\n", webWinRect.w, webWinRect.h);
    }
}


static gboolean onWindowStateEvent(GtkWidget *widget, GdkEventWindowState *event)
{
    gboolean res = GDK_EVENT_PROPAGATE;

    webWinRect.max = (event->new_window_state & GDK_WINDOW_STATE_MAXIMIZED) ? 1 : 0;
    webWinRect.full = (event->new_window_state & GDK_WINDOW_STATE_FULLSCREEN) ? 1 : 0;

    return res;
}


static gboolean onDecidePolicy(WebKitWebView *wv, WebKitPolicyDecision *decision, WebKitPolicyDecisionType type, void* data)
{
    debug("onDecidePolicy:  %d\n", (int)type);

    if (type == WEBKIT_POLICY_DECISION_TYPE_NAVIGATION_ACTION || type == WEBKIT_POLICY_DECISION_TYPE_NEW_WINDOW_ACTION)
    {
        WebKitNavigationAction* action = webkit_navigation_policy_decision_get_navigation_action(WEBKIT_NAVIGATION_POLICY_DECISION(decision));
        WebKitURIRequest* request = webkit_navigation_action_get_request(action);
        const char* uri = webkit_uri_request_get_uri(request);
        debug( "onDecidePolicy:  uri=%s\n", uri);

        if (strncmp(uri, "file:", 5) && strncmp(uri, "data:", 5))
        {
            debug("onDecidePolicy:  not local uri!\n");

            webkit_policy_decision_ignore(decision);
            
            gchar *text = g_strconcat("HTTP blocked:    ", uri, NULL);
            g_free(text);

            return TRUE;
        }
    }

    return FALSE; 
}


gboolean onWindowKeyPress(GtkWidget *widget, GdkEventKey *event, gpointer user_data)
{
    if (event->keyval == GDK_KEY_F11)
    {
        GtkWindow *window = GTK_WINDOW(widget);
        GdkWindowState state = gdk_window_get_state(gtk_widget_get_window(widget));
        
        if (state & GDK_WINDOW_STATE_FULLSCREEN)
            gtk_window_unfullscreen(window); 
        else
            gtk_window_fullscreen(window);  
        
        return TRUE; 
    }
    
    return FALSE; 
}


int main (int argc, char **argv)
{
    printf("mdwkit 0.2.2    20.06.2025\n");
    debug("DEBUG VERSION\n");

    if (argc <= 1)
    {
        printf("Syntax:  mdwkit <html_file>\n");
        return 1;
    }

    //printf("__STDC_VERSION__ is %ld\n", __STDC_VERSION__);
    printf("WebKit version: %u.%u.%u\n", webkit_get_major_version(), webkit_get_minor_version(), webkit_get_micro_version());

    gtk_init (&argc, &argv);
    
    char appPath[PATH_MAX + 1]; 
    ssize_t len = readlink("/proc/self/exe", appPath, sizeof(appPath) - 1);

    if (len != -1)
        appPath[len] = '\0'; 
    else
    {
        perror("Error getting program path\n");
        return 1;
    }

    // extensions in app dir
    appDir = g_path_get_dirname(appPath);
    printf("appDir=%s\n", appDir);
    gchar *appBaseName = g_path_get_basename(argv[0]);

    extDir = g_strdup(appDir);  // web_extensions_directory
    printf("extDir=%s\n", extDir);

    pluginDir = g_strconcat(appDir, "/plugins", NULL);
    printf("pluginDir=%s\n", pluginDir);

    gchar* htmlPath = g_strdup(argv[1]);
    gchar* htmlDir = g_path_get_dirname(htmlPath);
    gchar* htmlName = g_path_get_basename(htmlPath);
    printf("htmlPath=%s\n", htmlPath);

    chdir(htmlDir);

    gchar *htmlBaseName = NULL;
    gchar *p = g_strrstr(htmlName, ".");
    if (p)
        htmlBaseName = g_strndup(htmlName, p - htmlName);
    else
        htmlBaseName = g_strdup(htmlName);
    //debug("htmlBaseName=%s\n", htmlBaseName);

    g_setenv("DOCUMENT_ROOT", htmlDir, 1);

    const gchar* homeDir = g_get_home_dir();
    g_setenv("HOME_DIR", homeDir, 1);

    // config in html dir
    configPath = g_strconcat(htmlDir, "/", htmlBaseName, ".conf", NULL);
    printf("htmlConfigPath=%s\n", configPath);

    g_free(appBaseName);

    gchar* favIconPath = g_strconcat(htmlDir, "/favicon.png", NULL);

    loadConfig();


    WebKitWebContext *context = webkit_web_context_new_ephemeral();
    webkit_web_context_set_cache_model(context, WEBKIT_CACHE_MODEL_DOCUMENT_VIEWER);
    g_signal_connect (context, "initialize-web-extensions", G_CALLBACK(initialize_web_extensions), NULL);
    g_signal_connect (context, "user-message-received", G_CALLBACK(onMessageReceived), NULL);

    winWeb = gtk_window_new(GTK_WINDOW_TOPLEVEL);

    gtk_window_set_title(GTK_WINDOW(winWeb), htmlBaseName);
    gtk_window_set_default_icon_from_file(favIconPath, NULL);
                                       
    g_signal_connect(winWeb, "window-state-event", G_CALLBACK(onWindowStateEvent), NULL);
    g_signal_connect(G_OBJECT(winWeb), "key-press-event", G_CALLBACK(onWindowKeyPress), NULL);

    setWindowRect(GTK_WINDOW(winWeb), &webWinRect);

    WebKitWebView *web = WEBKIT_WEB_VIEW(webkit_web_view_new_with_context(context));

    WebKitSettings *settings = webkit_settings_new();
    //webkit_settings_get_enable_private_browsing(settings);
    webkit_settings_set_hardware_acceleration_policy(settings, WEBKIT_HARDWARE_ACCELERATION_POLICY_ALWAYS);
    webkit_settings_set_allow_file_access_from_file_urls(settings, TRUE);
    
    webkit_settings_set_enable_write_console_messages_to_stdout(settings, TRUE);
    webkit_settings_set_enable_offline_web_application_cache(settings, FALSE);
    webkit_settings_set_enable_page_cache(settings, FALSE);
    webkit_settings_set_default_charset(settings, "utf-8");

    webkit_web_view_set_settings(web, settings);

    g_signal_connect(web, "close", G_CALLBACK(onCloseWebViewCb), NULL);
    g_signal_connect(web, "decide-policy", G_CALLBACK(onDecidePolicy), NULL);
    g_signal_connect(winWeb, "delete-event", G_CALLBACK(onCloseWin), web);
    
    GtkWidget *scrollWin = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(scrollWin), GTK_WIDGET(web));
    gtk_container_add(GTK_CONTAINER(winWeb), scrollWin);

    gtk_widget_grab_focus(GTK_WIDGET(web));
    gtk_widget_show_all(winWeb);

    if (webWinRect.w > 0 && webWinRect.h > 0)
        gtk_window_resize(GTK_WINDOW(winWeb), webWinRect.w, webWinRect.h);

    if (webWinRect.full)
        gtk_window_fullscreen(GTK_WINDOW(winWeb));
    else if (webWinRect.max)
        gtk_window_maximize(GTK_WINDOW(winWeb));


    gchar* extFilePath = g_strconcat(extDir, "/mdwkitext.so", NULL);
    if (!g_file_test(extFilePath, G_FILE_TEST_EXISTS))
    {
        gchar *text = g_strdup_printf ("Web Extension file not found.\n%s", extFilePath);
        messageBox(winWeb, text, "mdwkit", 0);
        g_free(text);
        return 1;
    }
    
    if (!g_file_test(htmlPath, G_FILE_TEST_EXISTS))
    {
        gchar *text = g_strdup_printf ("HTML file not found.\n%s", htmlPath);
        messageBox(winWeb, text, "mdwkit", 0);
        g_free(text);
        return 1;
    }

    gchar* uri = g_strconcat("file://", htmlPath, NULL);

    webkit_web_view_load_uri(web, uri);
    g_free(uri);
    
end:
    g_free(htmlPath);
    g_free(htmlDir);
    g_free(htmlName);

    gtk_main ();

    return 0;    
}
