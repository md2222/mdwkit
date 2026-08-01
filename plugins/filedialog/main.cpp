// g++ `pkg-config --cflags gtk+-3.0` -o filedialog main.cpp `pkg-config --libs gtk+-3.0`

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <sstream>
#include <gtk/gtk.h>

using namespace std;


string getParam(const string& params, const char* paramName)
{
    string value = "";

    std::string param;
    std::istringstream paramStream(params);
    while (std::getline(paramStream, param, '&'))
    {
        int p = param.find("=");
        if (p != string::npos)
        {
            string name = param.substr(0, p);
            if (name == paramName)
            {
                value = param.substr(p + 1);
                break;
            }
        }
    }   
   
    return value; 
}


int main(int argc, char **argv, char** env) 
{
    string params = "";
    char* sz = getenv("QUERY_STRING");
    if (sz)  params = sz;
    fprintf(stderr, "filedialog:    params=%s\n", params.data());
    
    string act = getParam(params, "action");
    //fprintf(stderr, "filedialog:    fileName=%s\n", fileName.data());
    
    string file = getParam(params, "file");
    if (file.empty())  file = "untitled";
    
    string folder = getParam(params, "folder");


    string resp("{  \"fileName\":\"\"  }");
    
    gtk_init(&argc, &argv);
    
    GtkWidget *dialog;
    
    if (act == "save")
    {
        dialog = gtk_file_chooser_dialog_new("Save file", NULL, GTK_FILE_CHOOSER_ACTION_SAVE,
                "Cancel", GTK_RESPONSE_CANCEL,
                "Save", GTK_RESPONSE_OK,
                NULL);  
                
        GtkFileChooser *chooser = GTK_FILE_CHOOSER(dialog);
        gtk_file_chooser_set_do_overwrite_confirmation(chooser, TRUE);
        gtk_file_chooser_set_filename(chooser, file.data());
    }  
    else
    {
        dialog = gtk_file_chooser_dialog_new("Open file", NULL, GTK_FILE_CHOOSER_ACTION_OPEN,
                "Cancel", GTK_RESPONSE_CANCEL,
                "Open", GTK_RESPONSE_OK,
                NULL);    
                    
        if (!folder.empty())
        {
            GtkFileChooser *chooser = GTK_FILE_CHOOSER(dialog);
            gtk_file_chooser_set_current_folder(chooser, folder.data());
        }
    }      
    
    gint res = gtk_dialog_run(GTK_DIALOG(dialog));
    
    if (res == GTK_RESPONSE_OK)
    {
        char *filename;
        GtkFileChooser *chooser = GTK_FILE_CHOOSER (dialog);
        filename = gtk_file_chooser_get_filename (chooser);
        
        resp.assign("{  \"fileName\":\"");
        resp.append(filename);
        resp.append("\"  }");
        
        g_free(filename);
    }

    gtk_widget_destroy (dialog);
    
    printf(resp.data());
      
    return 0;
}
