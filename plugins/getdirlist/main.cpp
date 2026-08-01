// g++ -o getdirlist main.cpp
// sudo cp getdirlist /opt/mdwkit/plugins/

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <sstream>
//#include "jute.h"
#include <sys/types.h>
#include <dirent.h>
#include <cerrno>   // errno, (ENOENT, EACCES ...)
#include <cstring>  // std::strerror
#include <pwd.h>
#include <vector>
#include <algorithm>
#include <iostream>
#include <mntent.h>

using namespace std;


string getParam(const string& params, const char* paramName)
{
    string value = "";
    std::string param;
    std::istringstream paramStream(params);
    while (std::getline(paramStream, param, '&'))
    {
        //fprintf(stderr, "getdirlist:    param=%s\n", param.data());
        int p = param.find("=");
        if (p != string::npos)
        {
            string name = param.substr(0, p);
            //fprintf(stderr, "getdirlist:    name=%s\n", name.data());
            if (name == paramName)
            {
                value = param.substr(p + 1);
                break;
            }
        }
    }   
   
    return value; 
}


std::vector<std::string> getLinuxMounts()
{
    std::vector<std::string> mounts;
    FILE* fp = setmntent("/proc/mounts", "r");
    if (!fp)
    {
        std::cerr << "Cant open /proc/mounts" << std::endl;
        return mounts;
    }

    struct mntent* mnt;
    
    while ((mnt = getmntent(fp)) != nullptr)
    {
        std::string fsType = mnt->mnt_type;
        if (fsType != "sysfs" && fsType != "proc" && fsType != "tmpfs")
        {
            mounts.push_back(mnt->mnt_dir); 
        }
    }
    
    endmntent(fp);
    return mounts;
}


// getdirlist
int main(int argc, char **argv, char** env) 
{
    int errCode = 1;
    
    const char* query = getenv("QUERY_STRING");
    string params = query ? query : "";
    fprintf(stderr, "getdirlist:  params=%s\n", params.data());

    string dirPath = getParam(params, "dir");
    fprintf(stderr, "getdirlist:  dirPath=%s\n", dirPath.data());
    
    string errText = "";
    string resp;
    resp.append("[ ");

    if (dirPath.find("/home/") != 0 && dirPath.find("/media/") != 0)
    {

        string homeDir = getenv("HOME");
        fprintf(stderr, "getdirlist:  homeDir=%s\n", homeDir.data());
        if (homeDir.empty())
        {
            printf("getdirlist:  HOME is empty.\n");
            //return 1;
        }
        else
        {
            resp.append("\"");
            resp.append(homeDir.substr(1));
            resp.append("\",");
        }
        
        std::vector<std::string> disks = getLinuxMounts();

        if (!homeDir.empty())
            disks.insert(disks.begin(), homeDir);

        for (const auto& disk : disks)
            if (disk.rfind("/media/", 0) == 0)
            {
                fprintf(stderr, "getdirlist:  disk=%s\n", disk.data());
                resp.append("\"");
                resp.append(disk.substr(1));
                resp.append("\",");
            }

        resp.pop_back();
    }
    else
    {
        DIR *dir = opendir(dirPath.data());

        if (!dir)
            errText = "getdirlist:  opendir error";
            //fprintf(stderr, "getdirlist:  opendir error\n");
        else
        {
            std::vector<std::string> list;

            struct dirent *entry = readdir(dir);

            while (entry != NULL)
            {
                if (entry->d_type == DT_DIR && entry->d_name[0] != '.')
                    list.push_back(entry->d_name);

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
    }

    resp.append(" ]");
    fprintf(stderr, "getdirlist:  resp=%s\n", resp.data());
    
    if (errText.size())
    {
        printf("%s\n", errText.data());
        return 3;
    }
    else
    {
        //printf("Content-Type: application/json; charset=utf-8\n\n");
        printf("%s\n", resp.data());
    }
      
    return 0;
}
