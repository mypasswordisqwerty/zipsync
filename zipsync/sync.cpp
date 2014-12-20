//
//  sync.cpp
//  zipsync
//
//  Created by john on 18.12.14.
//
//

#include "sync.h"
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>
#include <sstream>
#include <iostream>
using namespace std;

namespace zipsync{
    
    Sync::Item::Item(Type type,const string& name,const string& path):
    type(type),
    name(name),
    path(path),
    directory(false),
    empty(false),
    removed(false),
    size(0),
    date(0),
    data(NULL){
    }

    
    int Sync::isDir(const string& path){
        struct stat buf;
        int res=stat(path.c_str(),&buf);
        if (res==0){
            return S_ISDIR(buf.st_mode) ? 0 : 1;
        }
        if (res==-1){
            return errno==ENOENT?-1:2;
        }
        return 3;
    }
    
    int Sync::open(Sync::SyncFlags flags, bool isSource){
        this->flags=flags;
        this->isSource=isSource;
        return -1;
    }
    
    map<string,Sync::Item>& Sync::getItems(const string& dir){
        string pth=dir;
        if (pth[pth.length()-1]!='/' and pth.length())
            pth+='/';
        if (dirs.count(pth)==0){
            return noItems;
        }
        return dirs[pth];
    }
    
    int Sync::mkDirs(const string &path){
        int res=isDir(path);
        if (res>=0)
            return res;
        string p="";
        string f;
        stringstream ss(path);
        while(std::getline(ss, f, '/')){
            if (p.length() && p!="/")
                p+="/";
            if (!f.length()){
                if (!p.length())
                    p="/";
                continue;
            }
            p+=f;
            if (p==".")
                continue;
            res=isDir(p);
            if (res>0)
                return res;
            if (res<0){
                res=mkdir(p.c_str(), 0755);
                if (res)
                    return res;
            }
        }
        return 0;
    }
    
    
}