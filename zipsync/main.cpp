//
//  main.cpp
//  zipsync
//
//  Created by john on 18.12.14.
//  Copyright (c) 2014 john@ilikegames.ru. All rights reserved.
//

#include <iostream>
#include <getopt.h>
#include <vector>
#include "dirsync.h"
#include "zipsync.h"
#include "syncler.h"

using namespace std;
using namespace zipsync;

string in;
string out;
Sync::SyncFlags flags={
    true,   // bool compareDate;
    false,  // bool compareCRC;
    true,   // bool ignoreHiddens;
    false,  // bool verbose;
    false,  // bool addEmptyDirs;
    false,   // bool store;
    false,   // bool rootDirectory;
    false   //bool modifiedCrc;
};

int usage(){
    cout << "zipsync v1.0\n usage:\n" <<
    "zipsync [-options] src dst\n\n" <<
    "parameters:\n"
    "\tsrc\t\t- source directory or zip\n" <<
    "\tdst\t\t- destination directory or zip\n\n" <<
    "options:\n" <<
    "\t-c,--crc\t\t- compare crc32\n" <<
    "\t-h,--help\t\t- this help message\n" <<
    "\t-p,--hiddens\t\t- include hidden files\n" <<
    "\t-r,--root\t\t- add root directory to archive\n" <<
    "\t-s,--store\t\t- do not compress files in zip\n" <<
    "\t-t,--notime\t\t- do not compare mtime\n" <<
    "\t-v,--verbose\t\t- verbose output\n" <<
    "\t-x,--modcrc\t\t- check crc of mtime-differs files\n";
    return 1;
}

Sync* syncForPath(const string& path){
    if (ZipSync::checkPath(path)){
        return new ZipSync(path);
    }
    if (DirSync::checkPath(path)){
        return new DirSync(path);
    }
    cerr << path << " is not a directory or achive\n";
    return NULL;
}

int parseParams(int argc, const char ** argv){
    struct option long_options[] =
    {
        {"help", no_argument,       0, 'h'},
        {"verbose", no_argument,       0, 'v'},
        {"notime", no_argument,       0, 't'},
        {"crc", no_argument,       0, 'c'},
        {"hiddens", no_argument,       0, 'p'},
        {"empty", no_argument,       0, 'e'},
        {"store", no_argument,       0, 's'},
        {"root", no_argument,       0, 'r'},
        {"modcrc", no_argument,       0, 'x'},
        {0, 0, 0, 0}
    };
    int option_index = 0;
    int c=0;
    while(1){
        c=getopt_long(argc, (char * const *)argv, "hvtcpesrx",long_options, &option_index);
        if (c==-1)
            break;
        switch (c){
            case 'h':
                return usage();
            case 'v':
                flags.verbose=true;
                break;
            case 't':
                flags.compareDate=false;
                break;
            case 'c':
                flags.compareCRC=true;
                break;
            case 'p':
                flags.ignoreHiddens=false;
                break;
            case 'e':
                flags.addEmptyDirs=true;
                break;
            case 's':
                flags.store=true;
                break;
            case 'r':
                flags.rootDirectory=true;
                break;
            case 'x':
                flags.modifiedCrc=true;
                break;
            case '?':
                return 2;
            default:
                abort();
        }
    }
    vector<string> args;
    while (optind < argc)
        args.push_back(argv[optind++]);
    if (args.size()!=2){
        cerr << args.size() << " arguments given. 2 needed: src dst\n";
        return usage();
    }
    in=args[0];
    out=args[1];
    return 0;
}

int main(int argc, const char * argv[]) {
    int res=parseParams(argc,argv);
    if (res)
        return res;
    unique_ptr<Sync> sin(syncForPath(in));
    unique_ptr<Sync> sout(syncForPath(out));
    if (!sin || !sout){
        return 3;
    }
    if (sin->equals(sout.get())){
        cerr << "destination and source are equal\n";
        return 4;
    }
    return Syncler(flags).run(sin.get(),sout.get());
}
