#include "update_profile.h"
#include <stdio.h>
#include <string.h>

int main(int argc, char **argv) {
    const char *exe=NULL,*archive=NULL,*profile_path=NULL;
    char default_archive[4096];bool derive=false;
    if(argc==2) {
        exe=argv[1];const char *slash=strrchr(exe,'/');
        if(!slash || snprintf(default_archive,sizeof(default_archive),"%.*s/../Frameworks/MoltenVK.framework/Versions/A/MoltenVK",(int)(slash-exe),exe)>=(int)sizeof(default_archive))return 2;
        archive=default_archive;
    } else if(argc==4 && !strcmp(argv[1],"--derive")) {
        derive=true;exe=argv[2];archive=argv[3];
    } else if(argc==5 && !strcmp(argv[1],"--verify")) {
        exe=argv[2];archive=argv[3];profile_path=argv[4];
    } else {
        fprintf(stderr,"usage: %s ESO | --derive ESO ARCHIVE | --verify ESO ARCHIVE PROFILE\n",argv[0]);return 2;
    }
    TesoUpdateProfile profile;
    if(!teso_update_resolve(exe,archive,&profile))return 3;
    if(profile_path) {
        char digest[65];
        if(!teso_update_verify_file(&profile,profile_path,digest))return 3;
        puts(digest);
    } else if(derive) {
        char bytes[TESO_UPDATE_PROFILE_LIMIT];size_t length=teso_update_render(&profile,bytes,sizeof(bytes));
        if(!length||fwrite(bytes,1,length,stdout)!=length)return 2;
    } else puts("COMPATIBLE_UPDATE");
    return 0;
}
