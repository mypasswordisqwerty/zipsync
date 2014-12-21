//
//  dirsync.cpp
//  zipsync
//
//  Created by john on 18.12.14.
//  Copyright (c) 2014 john@ilikegames.ru. All rights reserved.
//

#include "dirsync.h"
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <zlib.h>
#include <iostream>
#include <fstream>
#include <utime.h>

#define BLK_SIZE    4096

namespace zipsync {
    
    DirSync::DirSync(const string& path):Sync(path){
        if (this->path[this->path.length()-1]!='/'){
            this->path+='/';
        }
    }
    
    DirSync::~DirSync(){
    }


    bool DirSync::checkPath(const string &path){
        return isDir(path)<=0;
    }
    
    int DirSync::open(Sync::SyncFlags flags,bool isSource){
        Sync::open(flags, isSource);
        if (isSource && Sync::isDir(path)!=0){
            cerr << "source directory does not exists: " << path << endl;
            return 20;
        }
        int res=mkDirs(path);
        if (res)
            return res;
        string pth="";
        if (flags.rootDirectory) {
            size_t idx=path.find_last_of('/',path.length()-2);
            string pnm=path;
            Sync::Item it(Sync::Item::FS);
            it.directory=true;
            it.empty=false;
            if (idx!=string::npos){
                pnm=path.substr(idx+1,path.length()-idx-2);
                path=path.substr(0,idx+1);
            }else{
                path="";
            }
            it.name=pnm;
            dirs[""][pnm]=it;
            pth=pnm;
        }
        scanDir(pth);
        return 0;
    }
    
    size_t DirSync::scanDir(const string &dir){
        string pth=dir;
        if (pth[pth.length()-1]!='/' and pth.length())
            pth+='/';
        DIR* d=opendir((path+pth).c_str());
        if (!d){
            return 0;
        }
        dirent *de=NULL;
        struct stat buf;
        map<string,Sync::Item> items;
        while ((de = readdir(d)) != NULL){
            string nm=de->d_name;
            if (nm=="." || nm==".." || (nm[0]=='.' && flags.ignoreHiddens && isSource))
                continue;
            Sync::Item i(Sync::Item::FS,nm,pth);
            if (de->d_type & DT_DIR){
                i.directory=true;
                size_t sz=scanDir(pth+i.name);
                if (sz==0){
                    i.empty=true;
                }
                if (!isSource || !i.empty || flags.addEmptyDirs){
                    items[i.name]=i;
                }
            }
            if (de->d_type & DT_REG){
                i.directory=false;
                stat((path+pth+i.name).c_str(), &buf);
                i.size=buf.st_size;
                i.date=buf.st_mtime;// st_birthtimespec.tv_sec;
                items[i.name]=i;
            }
        }
        closedir(d);
        if (items.size()>0 || flags.addEmptyDirs){
            dirs[pth]=items;
        }
        return (int)items.size();
    }
    
    uLong DirSync::getCRC(const Sync::Item& item){
        string pth=path+item.path+item.name;
        ifstream s(pth.c_str(),ifstream::in | ifstream::binary);
        uLong c=crc32(0, Z_NULL, 0);
        unsigned char buf[BLK_SIZE];
        while (!s.eof()){
            s.read((char*)buf,BLK_SIZE);
            if (s.gcount()>0){
                c=crc32(c, buf, (uint32_t)s.gcount());
            }
        }
        s.close();
        return c;
    }
    
    Sync::TransferStruct DirSync::getTransfer(const Sync::Item& item){
        string pth=path+item.path+item.name;
        TransferStruct ts(&item);
        ts.pathInterface=pth;
        return ts;
    }
    
    int DirSync::updateItem(const Sync::Item& item,const Sync::TransferStruct& transfer){
        return addItem(transfer);
    }
    
    int DirSync::addItem(const Sync::TransferStruct& transfer){
        string pth=path+transfer.item->path+transfer.item->name;
        if (transfer.item->directory){
            return mkdir(pth.c_str(), 0755);
        }
        ofstream f(pth.c_str(),ofstream::out|ofstream::binary);
        if (f.fail()){
            cerr << "Can't create file " << pth << endl;
            return -1;
        }
        int res=0;
        do{
            if (transfer.pathInterface.length()){
                ifstream stream(transfer.pathInterface.c_str(),ifstream::in|ifstream::binary);
                if (stream.fail()){
                    res=-1;
                    cerr << "Can't open file " << transfer.pathInterface << endl;
                    break;
                }
                f << stream.rdbuf();
                stream.close();
            }else if (transfer.streamInterface){
                transfer.streamInterface->seekg(0);
                if (transfer.streamInterface->fail()){
                    res=-1;
                    cerr << "Failed stream " << transfer.item->path << transfer.item->name << endl;
                    break;
                }
                f << transfer.streamInterface->rdbuf();
            }else if(transfer.fileInterface){
                unsigned char buf[BLK_SIZE];
                void* in=transfer.fileInterface->open(transfer.item);
                if (!in){
                    res=-1;
                    cerr << "Failed file interface " << transfer.item->path << transfer.item->name << endl;
                    break;
                }
                size_t len=0;
                do{
                    len=transfer.fileInterface->read(in,buf,BLK_SIZE);
                    f.write((char*)buf, len);
                }while(len==BLK_SIZE);
                transfer.fileInterface->close(in);
            }else{
                res=-1;
                cerr << "Unsupported transfer interface type" << endl;
            }
        }while(0);
        size_t len=f.tellp();
        f.close();
        if (!res && transfer.item->size!=len){
            res=-1;
            cerr << "Transfer size wrong " << len << " vs " << transfer.item->size << " for " << transfer.item->path << transfer.item->name << endl;
        }
        if (res){
            return res;
        }
        utimbuf times;
        times.actime=transfer.item->date;
        times.modtime=transfer.item->date;
        utime(pth.c_str(), &times);
        return 0;
    }
    
    void DirSync::unlinkDir(string pth){
        if (pth[pth.length()-1]!='/')
            pth+='/';
        DIR* d=opendir(pth.c_str());
        dirent *de=NULL;
        while ((de = readdir(d)) != NULL){
            string nm=de->d_name;
            if (nm=="." || nm==".."){
                continue;
            }
            if (de->d_type & DT_DIR){
                unlinkDir(pth+nm);
            }else{
                unlink((pth+nm).c_str());
            }
        }
        closedir(d);
        rmdir(pth.c_str());
    }
    
    int DirSync::removeItem(const Sync::Item& item){
        string pth=path+item.path+item.name;
        if (item.directory){
            unlinkDir(pth);
        }else{
            unlink(pth.c_str());
        }
        return 0;
    }


}