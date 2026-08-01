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

#include <unistd.h>
#include <string>
#include <iostream>
#include <gtk/gtk.h>
#include <sys/wait.h>
#include "cgi.h"


#define WriteError 8
#define FatalError 9


void cgi(const string& script, const string& params, const string& data, string& out, Error& err)
{
    out = "";
    int fd_p2c[2], fd_c2p[2], bytes_read;
    pid_t childpid;
    char readbuffer[80];

    if (pipe(fd_p2c) != 0 || pipe(fd_c2p) != 0)
    {
        cerr << "Failed to pipe\n";
        return;
    }

    childpid = fork();

    if (childpid < 0)
    {
        cerr << "Fork failed" << endl;
        return;
    }
    else if (childpid == 0)
    {
        if (dup2(fd_p2c[0], 0) != 0 ||
            close(fd_p2c[0]) != 0 ||
            close(fd_p2c[1]) != 0)
        {
            cerr << "Child: failed to set up standard input\n";
            //return;
            exit(FatalError);
        }
        if (dup2(fd_c2p[1], 1) != 1 ||
            close(fd_c2p[1]) != 0 ||
            close(fd_c2p[0]) != 0)
        {
            cerr << "Child: failed to set up standard output\n";
            //return;
            exit(FatalError);
        }

        char sz[100];
        sz[0] = '\0';
        sprintf(sz, "%lu", data.size());
        setenv("CONTENT_LENGTH", sz, 1);
        setenv("QUERY_STRING", params.data(), 1);
        //setenv("HOME_DIR", homeDir.data(), 1);

        execl(script.data(), script.data(), (char *) 0);
        cerr << "Failed to execute " << script << endl;
        //return;
        exit(FatalError);
    }
    else
    {
        close(fd_p2c[0]);
        close(fd_c2p[1]);

        //cout << "Writing to child: <<" << data << ">>" << endl;
        int nbytes = data.size();
        if (write(fd_p2c[1], data.data(), nbytes) != nbytes)
        {
            //cerr << "Parent: short write to child\n";
            err.code = WriteError;
            err.text = "Write to process error.";
            return;
        }
        close(fd_p2c[1]);

        int pid_child, status;

        pid_child = wait(&status); //wait until any one child process terminates

        int procErr = 0;
        if (!WIFEXITED(status))
            procErr = FatalError;

        if (procErr)
        {
            err.code = procErr;
            err.text = "CGI script process error.";
        }
        else
        {
            string errStr;

            procErr = WEXITSTATUS(status);

            while (1)
            {
                bytes_read = read(fd_c2p[0], readbuffer, sizeof(readbuffer)-1);

                if (bytes_read <= 0)
                    break;

                readbuffer[bytes_read] = '\0';
                if (procErr) break;  // enough for error
                out += readbuffer;
            }

            if (procErr)
            {
                err.code = procErr;
                //err.text = strdup(readbuffer);
                err.text = readbuffer;
            }

        }

        close(fd_c2p[0]);
        //cerr << "proc status:   " << procErr << endl;
        //cout << "From child:   " << out << endl;
    }
}
