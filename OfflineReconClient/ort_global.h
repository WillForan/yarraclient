#ifndef ORT_GLOBAL_H
#define ORT_GLOBAL_H

// Include global definitions from the RDS client
#include "../Client/rds_global.h"


#define ORT_VERSION              "0.19b3"

#define ORT_ICON QIcon(":/images/orticon_256.png")

#define ORT_SERVERFILE           "YarraServer.cfg"
#define ORT_MODEFILE             "YarraModes.cfg"
#define ORT_SERVERLISTFILE       "YarraServerList.cfg"

#define ORT_LBI_NAME             "lbi.ini"

#define ORT_TASK_DIR             "queue"
#define ORT_TASK_EXTENSION       ".task"
#define ORT_LOCK_EXTENSION       ".lock"
#define ORT_TASK_EXTENSION_PRIO  "_prio"
#define ORT_TASK_EXTENSION_NIGHT "_night"
#define ORT_INVALID              "X-INVALID-X"

#define ORT_MINSIZEMB            2.0

#define ORT_SCANSHOW_DEF         50
#define ORT_SCANSHOW_MANMULT     10

#define ORT_RAID_MAXPARSECOUNT   3000

#define ORT_CONNECT_TIMEOUT      5000

#define ORT_DIR_QUEUE            "recoqueue"

#endif // ORT_GLOBAL_H
