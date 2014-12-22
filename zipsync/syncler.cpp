//
//  syncler.cpp
//  zipsync
//
//  Created by john on 18.12.14.
//  Copyright (c) 2014 john@ilikegames.ru. All rights reserved.
//

#include "syncler.h"
#include <stdarg.h>
#include <zlib.h>
#include <iostream>
using namespace std;

#define CHECKRES(X) res=X; if (res) return res

namespace zipsync {
    
    void Syncler::verbose(const char* fmt,...){
        if (!flags.verbose)
            return;
        va_list lst;
        va_start(lst, fmt);
        vfprintf(stderr,fmt,lst);
        va_end(lst);
    }
    

    int Syncler::run(Sync* from,Sync* to){
        this->from=from;
        this->to=to;
        verbose("making file list...\n");
        if (from->open(flags,true))
            return 10;
        if (to->open(flags,false))
            return 11;
        int res=syncDir("");
        res+=to->close();
        res+=from->close();
        if (res){
            cerr << "sync failed: " << res << endl;
        }
        return res;
    }
    
    
    int Syncler::syncFile(Sync::Item& src,Sync::Item& dst){
        bool upd=true;
        verbose("checking %s%s ",src.path.c_str(),src.name.c_str());
        int res=0;
        do{
            verbose("size ");
            if (src.size!=dst.size){
                verbose("%ld -> %ld",dst.size,src.size);
                break;
            }
            bool compcrc=flags.compareCRC;
            if (flags.compareDate){
                verbose("date ");
                //crooked nail: some mtimes in zip are mtimes-1
                if (abs(src.date-dst.date)>1){
                    if (!flags.modifiedCrc){
                        verbose("%ld -> %ld",dst.date,src.date);
                        break;
                    }
                    compcrc=true;
                }
            }
            if (compcrc){
                verbose("crc ");
                uLong c1=0,c2=0;
                c1=from->getCRC(src);
                c2=to->getCRC(dst);
                if (c1!=c2){
                    verbose("%lu -> %lu",c2,c1);
                    break;
                }
            }
            upd=false;
        }while(0);
        verbose("\n");
        if (upd){
            cout << "U " << src.path << src.name << endl;
            CHECKRES(to->updateItem(dst, from->getTransfer(src)));
        }
        return 0;
    }
    
    
    int Syncler::syncDir(const string& path){
        verbose("syncing directory %s\n",path.c_str());
        map<string,Sync::Item>& fdir=from->getItems(path);
        map<string,Sync::Item>& tdir=to->getItems(path);
        int res=0;
        for (auto it=tdir.begin(); it!=tdir.end(); ++it){
            if (fdir.count(it->first)==0 || it->second.directory!=fdir[it->first].directory){
                it->second.removed=true;
                cout << "D " << it->second.path << it->second.name << endl;
                CHECKRES(to->removeItem(it->second));
            }
        }
        vector<string> morepaths;
        for (auto it=fdir.begin(); it!=fdir.end(); ++it){
            Sync::Item& src=it->second;
            if (it->second.directory){
                morepaths.push_back(src.path+src.name);
            }
            if (tdir.count(it->first)==0 || tdir[it->first].removed){
                cout << "A " << src.path << src.name << endl;
                CHECKRES(to->addItem(from->getTransfer(src)));
                continue;
            }
            if (it->second.directory)
                continue;
            CHECKRES(syncFile(src,tdir[it->first]));
        }
        for (string& x: morepaths){
            CHECKRES(syncDir(x));
        }
        return 0;
    }

}