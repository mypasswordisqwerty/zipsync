//
//  syncler.h
//  zipsync
//
//  Created by john on 18.12.14.
//  Copyright (c) 2014 john@ilikegames.ru. All rights reserved.
//

#ifndef __zipsync__syncler__
#define __zipsync__syncler__

#include "sync.h"

namespace zipsync {

    class Syncler{
    public:
        Syncler(Sync::SyncFlags flags):flags(flags){};
        void verbose(const char* fmt,...);

        int run(Sync* from,Sync* to);
        
    private:
        Sync *from;
        Sync *to;
        Sync::SyncFlags flags;
        int syncFile(Sync::Item& src,Sync::Item& dst);
        int syncDir(const string& path);
    };
    
    
}



#endif /* defined(__zipsync__syncler__) */
