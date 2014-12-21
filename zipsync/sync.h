//
//  sync.h
//  zipsync
//
//  Created by john on 18.12.14.
//  Copyright (c) 2014 john@ilikegames.ru. All rights reserved.
//

#ifndef __zipsync__sync__
#define __zipsync__sync__

#include <string>
#include <vector>
#include <map>
#include <istream>
#include <zlib.h>
using namespace std;

namespace zipsync{

    class Sync{
    public:
        struct SyncFlags{
            bool compareDate;
            bool compareCRC;
            bool ignoreHiddens;
            bool verbose;
            bool addEmptyDirs;
            bool store;
            bool rootDirectory;
        };
        struct Item{
            enum Type {FS, ZIP};
            Type type;
            bool directory;
            bool empty;
            bool removed;
            string path;
            string name;
            uint64_t size;
            time_t date;
            void* data;
            Item(){}
            Item(Type type,const string& name="",const string& path="");
        };
        struct TransferStruct{
            struct FileInterface{
                virtual void* open(Sync::Item const * item)=0;
                virtual size_t read(void* file,unsigned char* buf,size_t len)=0;
                virtual void close(void* file)=0;
            };
            const Sync::Item* item;
            string pathInterface;
            FileInterface* fileInterface;
            istream* streamInterface;
            TransferStruct():fileInterface(NULL),streamInterface(NULL),item(NULL){}
            TransferStruct(Sync::Item const * item):fileInterface(NULL),streamInterface(NULL),item(item){}
        };
        
        Sync(){}
        Sync(const string& path):path(path){}
        virtual ~Sync(){}
        
        bool equals(Sync* other){return path==other->path;}
        
        static int isDir(const string& path);
        int mkDirs(const string& path);
        virtual map<string,Sync::Item>& getItems(const string& dir);
        
        virtual int open(Sync::SyncFlags flags,bool isSource);
        virtual int close(){return 0;}
        virtual uLong getCRC(const Sync::Item& item)=0;
        virtual TransferStruct getTransfer(const Sync::Item& item)=0;
        virtual int updateItem(const Sync::Item& item, const TransferStruct& transfer)=0;
        virtual int addItem(const TransferStruct& transfer)=0;
        virtual int removeItem(const Sync::Item& item)=0;
        
    protected:
        bool isSource;
        string path;
        SyncFlags flags;
        map<string,Sync::Item> noItems;
        map<string,map<string,Sync::Item>> dirs;
    };

}

#endif /* defined(__zipsync__sync__) */
