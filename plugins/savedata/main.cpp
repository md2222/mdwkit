// g++ -o savedata main.cpp

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <sstream>
#include <unistd.h>
#include <time.h>

using namespace std;


string getParam(const string& params, const char* paramName)
{
    string value = "";
    std::string param;
    std::istringstream paramStream(params);
    
    while (std::getline(paramStream, param, '&'))
    {
        //tokens.push_back(token);
        //fprintf(stderr, "savedata:    param=%s\n", param.data());
        int p = param.find("=");
        if (p != string::npos)
        {
            string name = param.substr(0, p);
            //fprintf(stderr, "savedata:    name=%s\n", name.data());
            if (name == paramName)
            {
                value = param.substr(p + 1);
                break;
            }
        }
    }   
   
    return value; 
}


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


int main(int argc, char **argv, char** env) 
{
    char* sz = getenv("CONTENT_LENGTH");
    //long int len = strtol(sz, NULL, 10);
    int len = atoi(sz);
    char* data = (char*)malloc(len + 1);
    if (!data) 
    {  
        printf("savedata:  No data received");
        return 1;
    }
    
    fread(data, len, 1, stdin);
    fprintf(stderr, "savedata:    data=%s\n", data);
    
    string params(getenv("QUERY_STRING"));
    fprintf(stderr, "savedata:    params=%s\n", params.data());
    
    string fileName = getParam(params, "file");
    //fprintf(stderr, "savedata:    fileName=%s\n", fileName.data());
    string mode = getParam(params, "mode");
    
    string errText = "";

    if (fileName.empty())
        errText = "savedata:  File name is empty.";
    else
    {
        if (mode == "rn")
        {
            // if file exists
            if ( access(fileName.data(), 0 ) == 0 )
            {
                time_t tt = time(NULL);
                struct tm lt;
                char buf [24];

                localtime_r(&tt, &lt);
                strftime(buf, 24, "%Y%m%d-%H%M%S", &lt);
                
                FilePath fp;
                fp.set(fileName);
                string backupFileName = fp.dir + "/" + fp.name + "="+ buf + "." + fp.ext; 
                fprintf(stderr, "savedata:    backupFileName=%s\n", backupFileName.data());
                
                if ( rename(fileName.data(), backupFileName.data()) != 0 )
                    errText = "savedata:  Rename file error. \\n" + fileName;
            }
        }
        
        if (errText.empty())
        {
            FILE *file = fopen(fileName.data(), "w");
            if (!file)
                errText = "savedata:  Open file error. \\n" + fileName;
            else
            {
                if (fwrite(data, len, 1, file) < 1)
                    errText = "savedata:  Write file error. \\n" + fileName;

                fclose(file);
            }
        }
    }

    if (errText.size()) 
    {  
        printf(errText.data());
        return 3;
    }
      
    return 0;
}
