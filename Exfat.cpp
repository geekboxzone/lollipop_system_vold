/*
 * Copyright (C) 2010 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/mount.h>

#include <linux/kdev_t.h>
#include <sys/wait.h>
#include <linux/fs.h>
#include <sys/ioctl.h>

#define LOG_TAG "Vold"
#include <logwrap/logwrap.h>
#include <cutils/log.h>
#include <cutils/properties.h>
#include "VoldUtil.h"
#include "Exfat.h"

static char EXFAT_PATH[] = "/system/bin/exfat";
static char MKFS_EXFAT_PATH[] = "/system/bin/mkexfat";

//extern "C" int logwrap(int argc, const char **argv, int background);
extern "C" int mount(const char *, const char *, const char *, unsigned long, const void *);

int Exfat::doMount(const char *fsPath, const char *mountPoint, bool ro, int ownerUid) {
	int rc = 0;
    do {
		if(!ro){
			int status_f;
        	const char *args[5];
			char option[255];
			sprintf(option,"uid=%d,gid=%d,fmask=%o,dmask=%o",ownerUid,ownerUid,0002,0002);
        	args[0] = EXFAT_PATH;
        	args[1] = fsPath;
        	args[2] = mountPoint;
			args[3] = "-o";
			args[4] = option; //"uid=1000,gid=1000,fmask=0002,dmask=0002";//options umask,dmask,fmask,uid,gid,ro,noatime   see parse_options in libexfat for details
		
        	rc = android_fork_execvp(ARRAY_SIZE(args), (char **)args, &status_f,
                    false, true);
		    SLOGI(" %s %s %s_%s", EXFAT_PATH, fsPath, mountPoint,args[4]);
			
			if(!rc){
				if (WIFEXITED(status_f)) {
					if (WEXITSTATUS(status_f)) {
						SLOGE("exfat fail  WEXITSTATUS %d",WEXITSTATUS(status_f));
						return -1;
					}
				}
				SLOGI("Mount Exfat device form %s to %s OK", fsPath, mountPoint);
        		char *lost_path;
        		asprintf(&lost_path, "%s/LOST.DIR", mountPoint);
        		if (access(lost_path, F_OK)) {
					
	            	/*
					* Create a LOST.DIR in the root so we have somewhere to put
					* lost cluster chains (fsck_msdos doesn't currently do this)
					*/
            		if (mkdir(lost_path, 0755)) {
                		SLOGE("Unable to create LOST.DIR (%s)", strerror(errno));
            		}
        		}
        		free(lost_path);
            	return 0;
			}else{
				SLOGE("Mount Exfat device form %s to %s failed",fsPath,mountPoint);
				return -1;
			}
		}else{
			int status;
			const char *args[5];
        	args[0] = EXFAT_PATH;
        	args[1] = fsPath;
        	args[2] = mountPoint;
        	args[3] = "-o";
        	args[4] = "ro,uid=1000";
			
			rc = android_fork_execvp(ARRAY_SIZE(args), (char **)args, &status,
                    false, true);
			if(!rc){
				SLOGI("Mount Exfat device form %s to %s OK.(Read-only)", fsPath, mountPoint);
            	return 0;
			}else{
				SLOGE("Mount Exfat device form %s to %s failed.(Read-only)", fsPath, mountPoint);
				return -1;
			}
		}
	}	while (0);
    return rc;
}

int Exfat::unMount(const char *mountPoint) {
	int rc = 0;
    do {
		int status;
        const char *args[3];
        args[0] = "umount";
        args[1] = mountPoint;
        args[2] = NULL;

        rc = android_fork_execvp(ARRAY_SIZE(args), (char **)args, &status,
                false, true);
		if(!rc){
			SLOGI("unMount Exfat device %s OK", mountPoint);
            return 0;
		}else{
			SLOGE("unMount Exfat device %s failed", mountPoint);
			return -1;
		}
    } while (0);
    return rc;
}

int Exfat::format(const char *fsPath, unsigned int numSectors) {
    const char *args[3];
    int rc;
	int status;
	
    args[0] = MKFS_EXFAT_PATH;
    args[1] = fsPath;
    args[2] = NULL;
	
	rc = android_fork_execvp(ARRAY_SIZE(args), (char **)args, &status, false,
            true);
	if(!rc){
		SLOGI("Format Exfat device %s OK", fsPath);
        return 0;
	}else{
		SLOGE("Format Exfat device %s failed", fsPath);
		return -1;
	}
}
