#include "configfile.h"


typedef struct
{
    GKeyFile* file;
    gchar* filePath;
} This;


static bool fileSave(ConfigFile* cf)
{
    if (!cf)  return FALSE;

    This* this = (This*)cf->priv;

    if (!g_file_test(this->filePath, G_FILE_TEST_EXISTS))
    {
        GFile* f = g_file_new_for_path (this->filePath);
        printf("Config file created:  %s\n", this->filePath);
    }

    g_autoptr(GError) err = NULL;

    if (!g_key_file_save_to_file(this->file, this->filePath, &err))
    {
        if (err)
        {
            g_warning ("Error saving key file: %s\n%s\n", err->message, this->filePath);
        }
            
        return false;
    }
    else
        cf->isExists = true;

    //g_key_file_free(file);

    return true;
}


static void configFree(ConfigFile* cf)
{
    This* this = (This*)cf->priv;
    if (this->file)  g_key_file_free(this->file);
    if (this->filePath)  {  g_free(this->filePath);  this->filePath = 0;  }
}


static gchar* confGetString(ConfigFile* cf, const char* group, const char* name, const char* def)
{
    g_autoptr(GError) err = NULL;

    gchar *val = g_key_file_get_string(((This*)cf->priv)->file, group, name, &err);

    if (val == NULL || err)
    {
        g_warning ("Error finding key in key file: %s", err->message);
        val = g_strdup(def);
    }

    return val;
}


static void confSetString(ConfigFile* cf, const char* group, const char* name, const char* val)
{
    g_key_file_set_string (((This*)cf->priv)->file, group, name, val);
}


static int confGetInt(ConfigFile* cf, const char* group, const char* name, gint def)
{
    g_autoptr(GError) err = NULL;

    gint val = g_key_file_get_integer(((This*)cf->priv)->file, group, name, &err);

    if (err)
    {
        g_warning ("Error finding key in key file: %s", err->message);
        val = def;
    }

    return val;
}


static void confSetInt(ConfigFile* cf, const char* group, const char* name, gint val)
{
    g_key_file_set_integer(((This*)cf->priv)->file, group, name, val);
}


static void confSetList(ConfigFile* cf, const char* group, const char* name, int type, void* list, gsize size)
{
    if (type == ListTypeInt)
    {
        g_key_file_set_integer_list(((This*)cf->priv)->file, group, name, (gint*)list, size);
    }
}


static void* confGetList(ConfigFile* cf, const char* group, const char* name, int type, gsize size)
{
    gsize sz = 0;
    void* list = NULL;
    void* res = NULL;

    if (type == ListTypeInt)
    {
        g_autoptr(GError) err = NULL;
        
        list = g_key_file_get_integer_list(((This*)cf->priv)->file, group, name, &sz, &err);

        if (sz && sz == size)
        {
            gsize len = sizeof(gint) * sz;
            res = g_malloc(len);
            memcpy(res, list, len);
            g_free(list);
        }
        
        if (err)
        {
            g_warning ("confGetList:    error=%s", err->message);
        }
    }

    return res;
}


static void confSetRect(ConfigFile* cf, const char* group, const char* name, GdkRectangle* rect)
{
    g_key_file_set_integer_list(((This*)cf->priv)->file, group, name, (gint*)rect, 4);
}


static GdkRectangle confGetRect(ConfigFile* cf, const char* group, const char* name)
{
    GdkRectangle rect = { -1, -1, -1, -1 };
    gsize sz = 0;
    g_autoptr(GError) err = NULL;

    gint* list = g_key_file_get_integer_list(((This*)cf->priv)->file, group, name, &sz, &err);

    if (sz && sz == 4)
    {
        rect.x = list[0];  rect.y = list[1];  rect.width = list[2];  rect.height = list[3];
        g_free(list);
    }
    
    if (err)
    {
        g_warning ("confGetRect:    error=%s", err->message);
    }

    return rect;
}


bool rectIsEmpty (GdkRectangle* rect)
{
     return (rect->width <= 0 || rect->height <= 0);
}



ConfigFile* ConfigFile_open(gchar* filePath)
{
    ConfigFile* cf = malloc(sizeof(ConfigFile));
    if (!cf)  return NULL;

    *cf = (ConfigFile){ .save = fileSave, .free = configFree,
        .getString = confGetString, .setString = confSetString, .getInt = confGetInt, .setInt = confSetInt, 
        .getList = confGetList, .setList = confSetList, .setRect = confSetRect, .getRect = confGetRect,
        .priv=0, .isExists=false };

    This* this = malloc(sizeof(This));
    *this = (This){ .file=0, .filePath=0 };

    this->filePath = g_strdup(filePath);

    GKeyFile* keyFile = g_key_file_new ();
    if (!keyFile)  return NULL;

    this->file = keyFile;

    g_autoptr(GError) err = NULL;  // pointer variable with automatic cleanup

    if (!g_key_file_load_from_file(keyFile, filePath, G_KEY_FILE_NONE, &err))
    {
        if (err && !g_error_matches(err, G_FILE_ERROR, G_FILE_ERROR_NOENT))
        {
            g_warning ("Error loading config file: %s", err->message);
            //g_error_free(err);
            //return NULL;
        }
    }
    else
        cf->isExists = true;

    cf->priv = this;

    return cf;
}


//ConfigFile* (*ConfigFile_open)(gchar* filePath) = configOpen;
