//
//  zipsync.h
//  zipsync
//
//  Created by john on 18.12.14.
//  Copyright (c) 2014 john@ilikegames.ru. All rights reserved.
//

#ifndef __zipsync__zipsync__
#define __zipsync__zipsync__

#include "sync.h"
#include <zip.h>
#include <map>
#include <vector>
#include <set>

using namespace std;

#define REPLACE_DELETED 0

namespace zipsync{

    class ZipSync:public Sync{
    public:
        ZipSync(const string& path):Sync(path){}
        virtual ~ZipSync();
        
        static bool checkPath(const string& path);
        
        virtual int open(Sync::SyncFlags flags,bool isSource) override;
        virtual uLong getCRC(const Sync::Item& item) override;
        virtual Sync::TransferStruct getTransfer(const Sync::Item& item) override;
        virtual int updateItem(const Sync::Item& item, const Sync::TransferStruct& transfer) override;
        virtual int addItem(const Sync::TransferStruct& transfer) override;
        virtual int removeItem(const Sync::Item& item) override;
        
    private:
        struct ZipFileInterface:public Sync::TransferStruct::FileInterface{
            virtual void* open(Sync::Item const * item) override;
            virtual size_t read(void* file,unsigned char* buf,size_t len) override;
            virtual void close(void* file) override;
            struct zip* zip;
        };
        struct FileDesc{
            struct zip* zip;
            zip_uint64_t index;
        };

        struct zip_source* getSource(const Sync::TransferStruct& transfer);
        void addEntry(struct zip_stat& st);

        ZipFileInterface fileInterface;
        set<zip_int64_t> deleted;
        struct zip* zip;
        vector<FileDesc> files;
    };

}


#endif /* defined(__zipsync__zipsync__) */
