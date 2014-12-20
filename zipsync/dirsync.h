//
//  dirsync.h
//  zipsync
//
//  Created by john on 18.12.14.
//  Copyright (c) 2014 john@ilikegames.ru. All rights reserved.
//

#ifndef __zipsync__dirsync__
#define __zipsync__dirsync__

#include "sync.h"

namespace zipsync{
    
    class DirSync:public Sync{
    public:
        DirSync(const string& path);
        virtual ~DirSync();
        
        static bool checkPath(const string& path);
        
        virtual int open(Sync::SyncFlags flags,bool isSource) override;
        virtual uLong getCRC(const Sync::Item& item) override;
        virtual Sync::TransferStruct getTransfer(const Sync::Item& item) override;
        virtual int updateItem(const Sync::Item& item, const TransferStruct& transfer) override;
        virtual int addItem(const TransferStruct& transfer) override;
        virtual int removeItem(const Sync::Item& item) override;
    private:
        size_t scanDir(const string& dir);
        void unlinkDir(string pth);
    };

}

#endif /* defined(__zipsync__dirsync__) */
