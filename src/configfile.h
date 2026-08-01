#ifndef __CONFIGFILE_H__
#define __CONFIGFILE_H__

#include <gtk/gtk.h>
#include <stdbool.h>

#define ListTypeInt 1


typedef struct ConfigFile
{
    //struct ConfigFile* (*open)(struct ConfigFile *cf, const char* filePath);
    bool (*save)(struct ConfigFile *cf);
    void (*free)(struct ConfigFile *cf);
    gchar* (*getString)(struct ConfigFile *cf, const char* group, const char* name, const char* def);
    void (*setString)(struct ConfigFile *cf, const char* group, const char* name, const char* val);
    int (*getInt)(struct ConfigFile *cf, const char* group, const char* name, gint def);
    void (*setInt)(struct ConfigFile *cf, const char* group, const char* name, gint val);
    void (*setList)(struct ConfigFile *cf, const char* group, const char* name, int type, void* list, gsize size);
    void* (*getList)(struct ConfigFile *cf, const char* group, const char* name, int type, gsize size);
    void (*setRect)(struct ConfigFile *cf, const char* group, const char* name, GdkRectangle* rect);
    GdkRectangle (*getRect)(struct ConfigFile *cf, const char* group, const char* name);
    void* priv;
    bool isExists; 
} ConfigFile;  

extern ConfigFile* ConfigFile_open(gchar* filePath);

extern bool rectIsEmpty(GdkRectangle* rect);  // empty when area <= 0

#endif
