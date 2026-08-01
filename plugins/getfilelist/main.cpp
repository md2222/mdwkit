// g++ -o getfilelist main.cpp

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <sstream>
#include <sys/types.h>
#include <dirent.h>
#include <pwd.h>
#include <vector>
#include <algorithm>

using namespace std;

// getfilelist

struct FilePath
{
    string dir;
    string name;
    string ext;
    void set(const string path)
    {
        dir.clear();  name.clear();  ext.clear();
        std::size_t pos1 = path.find_last_of("/");
        if (pos1 != string::npos)  dir = path.substr(0, pos1);
        else  pos1 = 0;

        int pos2 = path.find_last_of(".");

        if (pos2 != string::npos && pos2 > pos1)
        {
             name = path.substr(pos1 + 1, pos2 - pos1 - 1);
             ext = path.substr(pos2 + 1);
        }
        else
            name = path.substr(pos1 + 1);
    }
    const string get()  {  return dir + "/" + name + "." + ext;  }
};


string getParam(const string& params, const char* paramName)
{
    string value = "";
    std::string param;
    std::istringstream paramStream(params);
    while (std::getline(paramStream, param, '&'))
    {
        //tokens.push_back(token);
        //fprintf(stderr, "getfilelist:    param=%s\n", param.data());
        int p = param.find("=");
        if (p != string::npos)
        {
            string name = param.substr(0, p);
            //fprintf(stderr, "getfilelist:    name=%s\n", name.data());
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
    string params(getenv("QUERY_STRING"));
    fprintf(stderr, "getfilelist:    params=%s\n", params.data());

    string dirPath = getParam(params, "dir");
    fprintf(stderr, "getfilelist:    dirPath=%s\n", dirPath.data());
    
    string filter = getParam(params, "filter");
    fprintf(stderr, "getfilelist:    filter=%s\n", filter.data());

    string errText = "";
    string resp;
    resp.append("[ ");

    DIR *dir = opendir(dirPath.data());

    if (dir)
    {
        string f("+");  f.append(filter);  f.append("+");

        std::vector<std::string> list;

        struct dirent *entry = readdir(dir);
        FilePath  filePath;

        while (entry != NULL)
        {
            //if ( entry->d_type == DT_REG && entry->d_name[0] != '.' && strstr(entry->d_name, ".jpg") )
            if ( entry->d_type == DT_REG && entry->d_name[0] != '.' )
            {
                bool isFilter = false;
                
                if (f == "+*+")
                    isFilter = true;
                else
                {
                    //fprintf("%s\n", entry->d_name);
                    filePath.set(entry->d_name);
                    string ext = "+" + filePath.ext + "+";
                    //fprintf("getFileList:   %s\n", ext.data());
                    if (f.find(ext) != string::npos)
                        isFilter = true;
                }
                    
                if (isFilter)
                    list.push_back(entry->d_name);
            }

            entry = readdir(dir);
        }

        closedir(dir);

        std::sort(list.begin(), list.end());
        for (std::vector<string>::iterator it = list.begin(); it != list.end(); ++it)
        {
            resp.append("\"");
            resp.append(*it);
            resp.append("\",");
        }
        resp.pop_back();
    }

    resp.append(" ]");
    
    if (errText.size())
    {
        printf(errText.data());
        return 3;
    }
    else 
        printf(resp.data());
      
    return 0;
}
