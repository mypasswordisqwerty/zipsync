//
//  zipsync.cpp
//  zipsync
//
//  Created by john on 18.12.14.
//  Copyright (c) 2014 john@ilikegames.ru. All rights reserved.
//

#include "zipsync.h"
#include <sys/stat.h>
#include <errno.h>
#include <iostream>
#include <sstream>

using namespace std;

namespace zipsync {

    bool ZipSync::checkPath(const string &path){
        struct stat buf;
        int res=stat(path.c_str(),&buf);
        switch (res){
            case 0:{
                if ((buf.st_mode & S_IFREG)==0){
                    break;
                }
                struct zip* zf=zip_open(path.c_str(), 0, NULL);
                if (zf){
                    zip_close(zf);
                    return true;
                }
                break;
            }
            case -1:{
                size_t pos=path.find_last_of('.');
                if (pos!=string::npos && path.substr(pos)==".zip"){
                    return true;
                }
                break;
            }
            default:
                break;
        }
        return false;
    }
    
    
    ZipSync::~ZipSync(){
        if (zip){
            for (zip_int64_t x:deleted){
                zip_delete(zip, x);
            }
            zip_close(zip);
        }
    }
    
    void ZipSync::addEntry(struct zip_stat& st){
        string nm=st.name;
        Sync::Item i(Sync::Item::ZIP);
        i.directory= (nm[nm.length()-1] == '/');
        i.empty=true;
        stringstream ss(nm);
        string s;
        vector<string> drs;
        while(std::getline(ss, s, '/')){
            if (s.length())
                drs.push_back(s);
        }
        i.name=drs.back();
        drs.pop_back();
        string path="";
        for (const string& x:drs){
            string prev=path;
            path+=x+"/";
            if (dirs.count(path))
                continue;
            if (dirs.count(path)==0){
                Sync::Item d(Sync::Item::ZIP,x,prev);
                d.directory=true;
                d.empty=false;
                d.data=NULL;
                dirs[prev][x]=d;
            }
        }
        FileDesc fd={zip,st.index};
        files[st.index]=fd;
        i.data=&files[st.index];
        i.path=path;
        if (!i.directory){
            i.date=st.mtime;
            i.size=st.size;
        }
        dirs[path][i.name]=i;
    }
    
    int ZipSync::open(Sync::SyncFlags flags,bool isSource){
        Sync::open(flags, isSource);
        int zf=0;
        if (!isSource) {
            zf=ZIP_CREATE;
            string file=path.substr(path.find_last_of('/')+1);
            string dir=path.substr(0,path.length()-file.length());
            int res=mkDirs(dir);
            if (res)
                return res;
        }
        int err=0;
        zip=zip_open(path.c_str(), zf | ZIP_CHECKCONS, &err);
        if (!zip){
            cerr << "Can't open zip " << path << ": " << err << endl;
            return err;
        }
        zip_int64_t sz=zip_get_num_entries(zip, 0);
        files.resize(sz);
        for (zip_int64_t i=0;i<sz;i++){
            struct zip_stat st;
            zip_stat_index(zip, i, 0, &st);
            bool add=true;
            if (st.size==0){
                string nm=st.name;
                if (nm[nm.length()-1]=='/' && isSource && !flags.addEmptyDirs){
                    add=false;
                }
            }
            if (add){
                addEntry(st);
            }
        }
        fileInterface.zip=zip;
        return 0;
    }
    
    uLong ZipSync::getCRC(const Sync::Item& item){
        FileDesc* desc=(FileDesc*)item.data;
        struct zip_stat st;
        zip_stat_index(zip, desc->index, 0, &st);
        return st.crc;
    }
    
    Sync::TransferStruct ZipSync::getTransfer(const Sync::Item& item){
        TransferStruct ts(&item);
        ts.fileInterface=&fileInterface;
        return ts;
    }
    
    
    struct zip_source* ZipSync::getSource(const Sync::TransferStruct& tr){
        if (tr.item->type==Sync::Item::ZIP){
            //zip to zip
            FileDesc* desc=(FileDesc*)tr.item->data;
            return zip_source_zip(zip, desc->zip, desc->index, 0, 0, tr.item->size);
        }
        if (tr.pathInterface.length()){
            return zip_source_file(zip, tr.pathInterface.c_str(), 0, tr.item->size);
        }else{
            //TODO: add file/stream interfaces
            cerr << "Unsupported transfer interface" << endl;
            return NULL;
        }
    }

    
    int ZipSync::updateItem(const Sync::Item& item, const Sync::TransferStruct& tr){
        FileDesc* desc=(FileDesc*)item.data;
        zip_source* s=getSource(tr);
        if (!s){
            return -1;
        }
        int res=zip_replace(zip, desc->index, s);
        if (res){
            zip_source_free(s);
            cerr << "Error updating file: " << zip_strerror(zip) << endl;
        }
        return res;
    }
    
    
    int ZipSync::addItem(const Sync::TransferStruct& tr){
        zip_int16_t idx=-1;
        string fn=tr.item->path+tr.item->name;
        if (tr.item->directory){
            if (!tr.item->empty)
                return 0;
            
            idx=zip_dir_add(zip, fn.c_str(), 0);
            if (idx<0){
                cerr << "Error adding directory: "<< zip_strerror(zip) << endl;
                return -1;
            }
            //zip_set_file_compression(zip, idx, ZIP_CM_STORE, 0);
            return 0;
        }
        zip_source* s=NULL;

#if REPLACE_DELETED==1
        
        if (deleted.size()>0){
            //replace deleted with new file
            zip_int16_t ridx=*deleted.begin();
            do{
                int res=zip_rename(zip, ridx, fn.c_str());
                if (res)
                    break;
                s=getSource(tr);
                if (!s)
                    break;
                res=zip_replace(zip, ridx, s);
                if (res)
                    break;
                idx=ridx;
                deleted.erase(idx);
            }while(0);
        }else{
            
#endif
            
            s=getSource(tr);
            if (s){
                idx=zip_add(zip, fn.c_str(), s);
            }

#if REPLACE_DELETED==1
        }
#endif
        if (idx<0){
            zip_source_free(s);
            cerr << "Error adding file: " << zip_strerror(zip) << endl;
        }
        zip_set_file_compression(zip, idx, flags.store?ZIP_CM_STORE:ZIP_CM_DEFAULT, 0);
        return 0;
    }
    
    
    
    int ZipSync::removeItem(const Sync::Item& item){
        FileDesc* desc=(FileDesc*)item.data;
        if (!desc){
            map<string,Sync::Item>& ins=dirs[item.path+item.name+"/"];
            for (auto it=ins.begin(); it!=ins.end(); ++it){
                removeItem(it->second);
                it->second.removed=true;
            }
            return 0;
        }
        
#if REPLACE_DELETED==1
        
        if (item.directory){
            zip_delete(zip, desc->index);
        }
        deleted.insert(desc->index);

#else

        zip_delete(zip, desc->index);

#endif
        return 0;
    }

    
#pragma mark ZipFileInterface
    
    void* ZipSync::ZipFileInterface::open(Sync::Item const * item){
        FileDesc* desc=(FileDesc*)item->data;
        struct zip_file* res=zip_fopen_index(zip, desc->index, 0);
        if (!res){
            cerr << "Zip open failed: " << zip_strerror(zip) << endl;
        }
        return res;
    }
    
    size_t ZipSync::ZipFileInterface::read(void* file,unsigned char* buf,size_t len){
        return zip_fread((struct zip_file *)file, buf, len);
    }
    
    void ZipSync::ZipFileInterface::close(void* file){
        zip_fclose((struct zip_file *)file);
    }
    
    


}