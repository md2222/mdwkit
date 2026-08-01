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

#include <JavaScriptCore/JSContextRef.h>
#include <JavaScriptCore/JSStringRef.h>
#include <webkit2/webkit-web-extension.h>
#include <stdio.h>
#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <sys/types.h>
#include <pwd.h>
#include "cgi.h"

using namespace std;

#ifdef DEBUG
#define debug(fmt, ...) fprintf(stderr, "%s:%d: " fmt, __FILE__, __LINE__, ##__VA_ARGS__)
#else
#define debug(fmt, ...) do {} while (0) // No-op in release builds
#endif

enum { JsErrorNone, JsErrorFuncParams, JsErrorScriptName };

string pluginDir;
string confFileName;
extern string homeDir;


static gboolean onSendRequest(WebKitWebPage *web_page, WebKitURIRequest *request, WebKitURIResponse *redirected_response, gpointer user_data)
{
    const char *request_uri;
    const char *page_uri;
    const char *method;

    page_uri = webkit_web_page_get_uri (web_page);
    request_uri = webkit_uri_request_get_uri(request);
    method = webkit_uri_request_get_http_method(request);  // = (null)

    if (strncmp(request_uri, "file:", 5) && strncmp(request_uri, "data:", 5))
    {
        g_print ("blocked:   %s\n", request_uri);
        return TRUE;
    }

    return FALSE;
}


static void onConsoleMessage(WebKitWebPage* web_page, WebKitConsoleMessage* message, gpointer user_data)
{
    debug("onConsoleMessage:  \n");
    const char *text = webkit_console_message_get_text(message);
    if (text)
        std::cout << "JS console:  " << text << std::endl;
}


void jsStrToStr(JSStringRef jsStr, string& res)
{
    res.clear();
    size_t strLen = JSStringGetMaximumUTF8CStringSize(jsStr);
    res.resize(strLen);
    JSStringGetUTF8CString(jsStr, &res[0], strLen);
    res.resize(strlen(&res[0]));
}


static JSValueRef jsFuncCb(JSContextRef ctx, JSObjectRef func, JSObjectRef obj, size_t argCount, const JSValueRef args[], JSValueRef* exc)
{
    Error err;
    string req = "";
    string data = "";
    string cgiResp = "";

    if (argCount > 0)
    {
        JSStringRef js_str = JSValueToStringCopy(ctx, args[0], NULL);
        jsStrToStr(js_str, req);
        debug("jsFuncCb:    script=%s    argCount=%lu\n", req.data(), argCount);

        if (req.empty())
        {
            err.code = JsErrorScriptName;
            err.text = "Script name not found.";
        }
        else
        {
            string script = "";
            string params = "";

            size_t p = req.find("?");
            if (p == string::npos)
                script = req;
            else
            {
                script = req.substr(0, p);
                params = req.substr(p + 1);
            }

            if (argCount > 1)
            {
                JSStringRef jsDataStr = JSValueToStringCopy(ctx, args[1], NULL);
                jsStrToStr(jsDataStr, data);
                debug("jsFuncCb:    data.size=%lu\n", data.size());
            }

            script.insert(0, pluginDir + "/");

            cgi(script, params, data, cgiResp, err);
        }
    }
    else
    {
        err.code = JsErrorFuncParams;
        err.text = "Function parameters not found.";
    }

    string resp = "{ \"errCode\":\"" + std::to_string(err.code) + "\", \"errText\":\"" + err.text + "\"  }\r\n\r\n" + cgiResp;

    debug("jsFuncCb:    err=%d: %s    resp.size=%lu\n", err.code, err.text.data(), cgiResp.size());
    JSStringRef result = JSStringCreateWithUTF8CString(resp.data());
    return JSValueMakeString(ctx, result);
}


static void onPageCreated(WebKitWebExtension *extension, WebKitWebPage *web_page, gpointer user_data)
{
    debug("onPageCreated:  \n");
    g_signal_connect_object (web_page, "send-request", G_CALLBACK(onSendRequest), NULL, (GConnectFlags)0);
    g_signal_connect_object (web_page, "console-message-sent", G_CALLBACK (onConsoleMessage), NULL, (GConnectFlags)0);
}


static void onWindowClearedCb(WebKitScriptWorld *world, WebKitWebPage *web_page, WebKitFrame *frame, gpointer user_data)
{
    JSGlobalContextRef jsContext = webkit_frame_get_javascript_context_for_script_world(frame, world);

    JSObjectRef globalObject = JSContextGetGlobalObject(jsContext);
    JSStringRef funcName = JSStringCreateWithUTF8CString("jsCoreRequest");

    JSObjectRef func = JSObjectMakeFunctionWithCallback(jsContext, funcName, jsFuncCb);
    JSObjectSetProperty(jsContext, globalObject, funcName, func, 0, NULL);
}


extern "C" G_MODULE_EXPORT void
webkit_web_extension_initialize_with_user_data (WebKitWebExtension *extension, GVariant *user_data)
{
    printf("mdwkitext 0.2.1    15.01.2025\n");
    gsize size;

    pluginDir = g_variant_get_string(user_data, &size);
    debug("ext:  pluginDir=%s\n", pluginDir.data());

    g_signal_connect(extension, "page-created", G_CALLBACK (onPageCreated), NULL);
    g_signal_connect (webkit_script_world_get_default(), "window-object-cleared", G_CALLBACK(onWindowClearedCb), NULL);
}
