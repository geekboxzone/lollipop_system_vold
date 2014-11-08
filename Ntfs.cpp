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

#include <cutils/log.h>
#include <cutils/properties.h>
#include <logwrap/logwrap.h>
#include "Ntfs.h"

#include "VoldUtil.h"

static char NTFS_3G_PATH[] = "/system/bin/ntfs-3g";

extern "C" int mount(const char *, const char *, const char *, unsigned long, const void *);

int Ntfs::doMount(const char *fsPath, const char *mountPoint, bool ro, int ownerUid, int ownerGid)
{
    int rc = 0;
    do {
        if (!ro) {
            int status_f;
            const char *args[3];
            args[0] = NTFS_3G_PATH;
            args[1] = fsPath;
            args[2] = mountPoint;

            rc = android_fork_execvp(ARRAY_SIZE(args), (char **)args, &status_f,
                    false, true);
            SLOGI(" %s %s %s", NTFS_3G_PATH, fsPath, mountPoint);
            if (!rc) {
                SLOGI("Mount NTFS device form %s to %s OK", fsPath, mountPoint);
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
            } else {
                SLOGE("Mount NTFS device form %s to %s failed", fsPath, mountPoint);
                return -1;
            }
        } else {
            int status;
            const char *args[5];
            args[0] = NTFS_3G_PATH;
            args[1] = fsPath;
            args[2] = mountPoint;
            args[3] = "-o";
            //args[4] = "ro,uid=1000";
            char mountData[255];
            sprintf(mountData," ro,utf8,uid=%d,gid=%d", ownerUid, ownerGid);
            args[4] = mountData;

            SLOGE("Mount NTFS device form %s ===============",args[4]);
            rc = android_fork_execvp(ARRAY_SIZE(args), (char **)args, &status,
                    false, true);

            SLOGI(" %s %s %s____%d", NTFS_3G_PATH, fsPath, mountPoint, rc);
            if (rc != 0) {
                SLOGE("Filesystem mount failed due to logwrap error");
                errno = EIO;
                return -1;
            }

            if (!WIFEXITED(status)) {
                SLOGE("Filesystem mount did not exit properly");
                errno = EIO;
                return -1;
            }

            status = WEXITSTATUS(status);
            if (status == 0) {
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
                SLOGI("Mount NTFS device form %s to %s OK.", fsPath, mountPoint);
                return 0;
            } else {
                SLOGE("%dMount NTFS device form %s to %s failed.", status, fsPath, mountPoint);
                return -1;
            }
        }
    } while (0);
    return rc;
}

int Ntfs::unMount(const char *mountPoint)
{
    int rc = 0;
    do {
        int status;
        const char *args[3];
        args[0] = "umount";
        args[1] = mountPoint;
        args[2] = NULL;

        rc = android_fork_execvp(ARRAY_SIZE(args), (char **)args, &status,
                false, true);
        if (!rc) {
            SLOGI("unMount NTFS device %s OK",mountPoint);
        } else {
            SLOGE("unMount NTFS device %s failed",mountPoint);
        }
    } while (0);

    return rc;
}

int Ntfs::format(const char *fsPath, unsigned int numSectors)
{
    return 0;
}
