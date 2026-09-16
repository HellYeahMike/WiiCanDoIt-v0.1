#include <gccore.h>
#include <wiiuse/wpad.h>
#include <ogc/es.h>
#include <ogc/isfs.h>

#include <fat.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <malloc.h>
#include <sys/stat.h>


/* =========================================================
   CONFIG
   ========================================================= */

#define SAVE_DIRECTORY      "sd:/WiiCanDoIt"
#define SAVE_FILE           "sd:/WiiCanDoIt/save.dat"

#define SAVE_MAGIC          0x57434449
#define SAVE_VERSION        2

#define ACHIEVEMENT_COUNT   27

#define NSMBW_SAVE_FILENAME "wiimj2d.sav"

#define NSMBW_SAVE_SLOT_COUNT 3
#define NSMBW_WORLD_COUNT     10
#define NSMBW_STAGE_COUNT     42
#define NSMBW_PLAYER_COUNT    4

/*
   wiimj2d.sav uses two CRC-protected copies of each of its
   three save slots. These offsets were verified against a
   real retail NTSC-U save file.
*/
#define NSMBW_SAVE_FILE_SIZE          0x3FA0
#define NSMBW_SAVE_SLOT_SIZE          0x0980
#define NSMBW_PRIMARY_SLOT_OFFSET     0x06A0
#define NSMBW_BACKUP_SLOT_OFFSET      0x2320

#define NSMBW_SLOT_REVISION_OFFSET    0x0000
#define NSMBW_SLOT_COMPLETION_OFFSET  0x0002
#define NSMBW_SLOT_LIVES_OFFSET       0x0022
#define NSMBW_SLOT_WORLD_OFFSET       0x0032
#define NSMBW_SLOT_STAGE_OFFSET       0x006C
#define NSMBW_SLOT_CRC_OFFSET         0x097C


/* =========================================================
   NSMBW SAVE FLAGS
   ========================================================= */

/*
   These values come directly from the NSMBW save definition.

   COURSE_COMPLETION_e
*/

#define NSMBW_COIN1_COLLECTED        (1 << 0)
#define NSMBW_COIN2_COLLECTED        (1 << 1)
#define NSMBW_COIN3_COLLECTED        (1 << 2)

#define NSMBW_COIN_MASK              \
    (NSMBW_COIN1_COLLECTED |        \
     NSMBW_COIN2_COLLECTED |        \
     NSMBW_COIN3_COLLECTED)

#define NSMBW_GOAL_NORMAL            (1 << 4)
#define NSMBW_GOAL_SECRET            (1 << 5)

#define NSMBW_GOAL_MASK              \
    (NSMBW_GOAL_NORMAL |             \
     NSMBW_GOAL_SECRET)


/*
   GAME_COMPLETION_e
*/

#define NSMBW_SAVE_EMPTY             (1 << 0)
#define NSMBW_FINAL_BOSS_BEATEN      (1 << 1)
#define NSMBW_GOAL_ALL               (1 << 2)
#define NSMBW_COIN_ALL               (1 << 3)
#define NSMBW_COIN_ALL_SPECIAL       (1 << 4)
#define NSMBW_GAME_COMPLETED         (1 << 5)


/*
   WORLD_COMPLETION_e
*/

#define NSMBW_WORLD_UNLOCKED         (1 << 0)


/* =========================================================
   NSMBW PLAYER IDS
   ========================================================= */

#define NSMBW_PLAYER_MARIO           0
#define NSMBW_PLAYER_LUIGI           1
#define NSMBW_PLAYER_YELLOW_TOAD     2
#define NSMBW_PLAYER_BLUE_TOAD       3


/* =========================================================
   NSMBW WORLD IDS
   ========================================================= */

#define NSMBW_WORLD_1                0
#define NSMBW_WORLD_2                1
#define NSMBW_WORLD_3                2
#define NSMBW_WORLD_4                3
#define NSMBW_WORLD_5                4
#define NSMBW_WORLD_6                5
#define NSMBW_WORLD_7                6
#define NSMBW_WORLD_8                7
#define NSMBW_WORLD_9                8


/*
   The stage indices used by mStageCompletion. Only the
   progression stages needed by the achievement checks are
   named here.
*/
#define NSMBW_STAGE_1                0
#define NSMBW_STAGE_8                7
#define NSMBW_STAGE_CASTLE           23
#define NSMBW_STAGE_AIRSHIP          37


/* =========================================================
   NSMBW TITLE IDS
   ========================================================= */

#define NSMBW_SMNE01 0x00010000534D4E45ULL
#define NSMBW_SMNP01 0x00010000534D4E50ULL
#define NSMBW_SMNJ01 0x00010000534D4E4AULL
#define NSMBW_SMNW01 0x00010000534D4E57ULL
#define NSMBW_SMNK01 0x00010000534D4E4BULL


/* =========================================================
   GLOBALS
   ========================================================= */

static int selectedOption = 0;

static int sdReady = 0;
static int isfsReady = 0;


/* =========================================================
   ACHIEVEMENT DATA
   ========================================================= */

typedef struct
{
    int id;
    const char *name;
    const char *description;
    int unlocked;
} Achievement;


static Achievement achievements[ACHIEVEMENT_COUNT] =
{
    {1,  "The Beggining",
        "Complete World 1", 0},

    {2,  "Grass... More Grass...",
        "Get all the starcoins in World 1", 0},

    {3,  "To hot!",
        "Complete World 2", 0},

    {4,  "Also to hot to walk...",
        "Get all the starcoins in World 2", 0},

    {5,  "Nevermind, to cold!",
        "Complete World 3", 0},

    {6,  "Merry Christmass!",
        "Get all the starcoins in World 3", 0},

    {7,  "Yay Vacations!",
        "Complete World 4", 0},

    {8,  "I almost drawned...",
        "Get all the starcoins in World 4", 0},

    {9,  "We're not lost anymore!",
        "Complete World 5", 0},

    {10, "Didn't we seen grass before?",
        "Get all the starcoins in World 5", 0},

    {11, "Climber",
        "Complete World 6", 0},

    {12, "How am I even here?",
        "Get all the starcoins in World 6", 0},

    {13, "The Paradise!",
        "Complete World 7", 0},

    {14, "Oh My God...",
        "Get all the starcoins in World 7", 0},

    {15, "The Hell...",
        "Complete World 8", 0},

    {16, "HELL YEAH!",
        "Get all the starcoins in World 8", 0},

    {17, "This must be a dream",
        "Complete World 9", 0},

    {18, "How am I even breathing?!",
        "Get all the starcoins in World 9", 0},

    {19, "Congratulations!",
        "Beat the Game", 0},

    {20, "Mama mia!",
        "Got all the starcoins in the Game", 0},

    {21, "Oh Yeah!",
        "Get 99 lives with Mario", 0},

    {22, "It's me! Luigi!",
        "Get 99 lives with Luigi", 0},

    {23, "What about this guy?",
        "Get 99 lives with Yellow Toad", 0},

    {24, "And this one as well?",
        "Get 99 lives with Blue Toad", 0},

    {25, "Unenployed...",
        "Beat all the 3 save files without copying them", 0},

    {26, "Thank you everyone for playing my game!",
        "Get all the Achievements", 0},

    {27, "Test",
        "Create a save file in New Super Mario Bros. Wii", 0}
};


/* =========================================================
   PERSISTENT SAVE DATA
   ========================================================= */

typedef struct
{
    uint32_t magic;
    uint32_t version;

    /*
       Game library membership.

       0 = never recognized
       1 = recognized at least once
    */
    uint8_t nsmbwInLibrary;

    /*
       Region/title ID that was detected.
    */
    uint64_t nsmbwTitleID;

    /*
       Permanent achievement states.
    */
    uint8_t nsmbwUnlocked[ACHIEVEMENT_COUNT];

} AchievementSave;


static AchievementSave saveData;


/* =========================================================
   NSMBW SAVE ANALYSIS DATA
   ========================================================= */

/*
   This structure represents the information that WiiCanDoIt
   needs from one NSMBW save slot.

   IMPORTANT:

   This is NOT the raw Nintendo save structure.

   It is our own normalized representation.
*/
typedef struct
{
    int valid;

    uint8_t gameCompletion;

    uint8_t playerLife[NSMBW_PLAYER_COUNT];

    uint8_t worldCompletion[NSMBW_WORLD_COUNT];

    uint32_t stageCompletion[
        NSMBW_WORLD_COUNT
    ][NSMBW_STAGE_COUNT];

} NSMBWSaveSlot;


/*
   All three NSMBW save slots.
*/
typedef struct
{
    int valid;

    NSMBWSaveSlot slot[NSMBW_SAVE_SLOT_COUNT];

} NSMBWSaveData;


/* =========================================================
   SD SAVE DIRECTORY
   ========================================================= */

static int EnsureSaveDirectory(void)
{
    struct stat st;

    if (stat(SAVE_DIRECTORY, &st) == 0)
    {
        if ((st.st_mode & S_IFDIR) != 0)
            return 1;

        return 0;
    }

    if (mkdir(SAVE_DIRECTORY, 0777) == 0)
        return 1;

    if (stat(SAVE_DIRECTORY, &st) == 0)
    {
        if ((st.st_mode & S_IFDIR) != 0)
            return 1;
    }

    return 0;
}


/* =========================================================
   SAVE SYSTEM
   ========================================================= */

static void ResetSaveData(void)
{
    memset(&saveData, 0, sizeof(AchievementSave));

    saveData.magic = SAVE_MAGIC;
    saveData.version = SAVE_VERSION;
}


static void ApplySaveToAchievements(void)
{
    int i;

    for (i = 0; i < ACHIEVEMENT_COUNT; i++)
    {
        achievements[i].unlocked =
            saveData.nsmbwUnlocked[i] ? 1 : 0;
    }
}


static void ApplyAchievementsToSave(void)
{
    int i;

    for (i = 0; i < ACHIEVEMENT_COUNT; i++)
    {
        saveData.nsmbwUnlocked[i] =
            achievements[i].unlocked ? 1 : 0;
    }
}


static int SaveAchievements(void)
{
    FILE *file;

    if (!sdReady)
        return 0;

    if (!EnsureSaveDirectory())
        return 0;

    ApplyAchievementsToSave();

    file = fopen(SAVE_FILE, "wb");

    if (!file)
        return 0;

    if (fwrite(
            &saveData,
            sizeof(AchievementSave),
            1,
            file) != 1)
    {
        fclose(file);
        return 0;
    }

    fclose(file);

    return 1;
}


static int LoadAchievements(void)
{
    FILE *file;

    if (!sdReady)
    {
        ResetSaveData();
        return 0;
    }

    file = fopen(SAVE_FILE, "rb");

    if (!file)
    {
        ResetSaveData();
        SaveAchievements();
        ApplySaveToAchievements();

        return 0;
    }

    if (fread(
            &saveData,
            sizeof(AchievementSave),
            1,
            file) != 1)
    {
        fclose(file);

        ResetSaveData();
        SaveAchievements();
        ApplySaveToAchievements();

        return 0;
    }

    fclose(file);

    if (saveData.magic != SAVE_MAGIC ||
        saveData.version != SAVE_VERSION)
    {
        ResetSaveData();
        SaveAchievements();
        ApplySaveToAchievements();

        return 0;
    }

    ApplySaveToAchievements();

    return 1;
}


/* =========================================================
   NSMBW TITLE DETECTION
   ========================================================= */

static int IsNSMBWTitleID(uint64_t titleID)
{
    switch (titleID)
    {
        case NSMBW_SMNE01:
        case NSMBW_SMNP01:
        case NSMBW_SMNJ01:
        case NSMBW_SMNW01:
        case NSMBW_SMNK01:
            return 1;

        default:
            return 0;
    }
}


static const char *GetNSMBWRegion(uint64_t titleID)
{
    switch (titleID)
    {
        case NSMBW_SMNE01:
            return "USA";

        case NSMBW_SMNP01:
            return "Europe";

        case NSMBW_SMNJ01:
            return "Japan";

        case NSMBW_SMNW01:
            return "Taiwan";

        case NSMBW_SMNK01:
            return "Korea";

        default:
            return "Unknown";
    }
}


/* =========================================================
   FIND NSMBW TITLE
   ========================================================= */

static int FindNSMBWTitle(uint64_t *foundTitleID)
{
    u32 titleCount = 0;
    u64 *titles;
    u32 i;

    if (!isfsReady)
        return 0;

    if (ES_GetNumTitles(&titleCount) < 0)
        return 0;

    if (titleCount == 0)
        return 0;

    titles = memalign(
        32,
        titleCount * sizeof(u64) + 32
    );

    if (!titles)
        return 0;

    if (ES_GetTitles(
            titles,
            titleCount) < 0)
    {
        free(titles);
        return 0;
    }

    for (i = 0; i < titleCount; i++)
    {
        if (IsNSMBWTitleID(titles[i]))
        {
            if (foundTitleID)
                *foundTitleID = titles[i];

            free(titles);

            return 1;
        }
    }

    free(titles);

    return 0;
}


/* =========================================================
   NSMBW DATA DIRECTORY
   ========================================================= */

static int GetNSMBWDataDirectory(
    uint64_t titleID,
    char *dataPath,
    size_t dataPathSize)
{
    char tempPath[ISFS_MAXPATH];

    memset(
        tempPath,
        0,
        sizeof(tempPath)
    );

    if (ES_GetDataDir(
            titleID,
            tempPath) < 0)
    {
        return 0;
    }

    strncpy(
        dataPath,
        tempPath,
        dataPathSize - 1
    );

    dataPath[dataPathSize - 1] = '\0';

    return 1;
}


/* =========================================================
   NSMBW SAVE DETECTION
   ========================================================= */

static int HasNSMBWSave(uint64_t titleID)
{
    char dataPath[ISFS_MAXPATH];
    char savePath[ISFS_MAXPATH];

    s32 fd;

    if (!isfsReady)
        return 0;

    if (!GetNSMBWDataDirectory(
            titleID,
            dataPath,
            sizeof(dataPath)))
    {
        return 0;
    }

    snprintf(
        savePath,
        sizeof(savePath),
        "%s/%s",
        dataPath,
        NSMBW_SAVE_FILENAME
    );

    fd = ISFS_Open(
        savePath,
        ISFS_OPEN_READ
    );

    if (fd < 0)
        return 0;

    ISFS_Close(fd);

    return 1;
}


/* =========================================================
   NSMBW SAVE FILE READING
   ========================================================= */

/*
   Reads the entire NSMBW save file into memory.

   This is intentionally separated from parsing.

   That means:

       filesystem
             |
             v
       raw save bytes
             |
             v
       parser
             |
             v
       NSMBWSaveData
*/
static int ReadNSMBWSaveFile(
    uint64_t titleID,
    uint8_t **buffer,
    u32 *fileSize)
{
    char dataPath[ISFS_MAXPATH];
    char savePath[ISFS_MAXPATH];

    s32 fd;
    s32 size;
    s32 readResult;

    uint8_t *data;

    if (!buffer || !fileSize)
        return 0;

    *buffer = NULL;
    *fileSize = 0;

    if (!isfsReady)
        return 0;

    if (!GetNSMBWDataDirectory(
            titleID,
            dataPath,
            sizeof(dataPath)))
    {
        return 0;
    }

    snprintf(
        savePath,
        sizeof(savePath),
        "%s/%s",
        dataPath,
        NSMBW_SAVE_FILENAME
    );

    fd = ISFS_Open(
        savePath,
        ISFS_OPEN_READ
    );

    if (fd < 0)
        return 0;

    size = ISFS_Seek(
        fd,
        0,
        SEEK_END
    );

    if (size <= 0)
    {
        ISFS_Close(fd);
        return 0;
    }

    if (ISFS_Seek(
            fd,
            0,
            SEEK_SET) < 0)
    {
        ISFS_Close(fd);
        return 0;
    }

    data = malloc(size);

    if (!data)
    {
        ISFS_Close(fd);
        return 0;
    }

    readResult = ISFS_Read(
        fd,
        data,
        size
    );

    ISFS_Close(fd);

    if (readResult != size)
    {
        free(data);
        return 0;
    }

    *buffer = data;
    *fileSize = (u32)size;

    return 1;
}


/* =========================================================
   NSMBW SAVE INTEGRITY
   ========================================================= */

static uint32_t ReadBigEndianU32(const uint8_t *data)
{
    return ((uint32_t)data[0] << 24) |
           ((uint32_t)data[1] << 16) |
           ((uint32_t)data[2] << 8) |
           (uint32_t)data[3];
}


/*
   Nintendo's OSCalcCRC32 uses the standard reflected CRC-32
   polynomial. A slot checksum covers its bytes from 0x0000 up
   to (but not including) the checksum at 0x097C.
*/
static uint32_t CalculateNSMBWCRC32(
    const uint8_t *data,
    u32 length)
{
    uint32_t crc = 0xFFFFFFFF;
    u32 index;

    for (index = 0; index < length; index++)
    {
        int bit;

        crc ^= data[index];

        for (bit = 0; bit < 8; bit++)
        {
            if (crc & 1)
                crc = (crc >> 1) ^ 0xEDB88320;
            else
                crc >>= 1;
        }
    }

    return ~crc;
}


static int IsNSMBWSlotValid(
    const uint8_t *data,
    u32 fileSize,
    u32 slotOffset)
{
    uint32_t storedCRC;
    uint32_t calculatedCRC;

    if (slotOffset > fileSize ||
        NSMBW_SAVE_SLOT_SIZE > fileSize - slotOffset)
    {
        return 0;
    }

    /*
       Revision 14.0 is the retail NSMBW save revision. This
       prevents arbitrary CRC-valid data from being parsed as a
       save slot.
    */
    if (data[slotOffset + NSMBW_SLOT_REVISION_OFFSET] != 14 ||
        data[slotOffset + NSMBW_SLOT_REVISION_OFFSET + 1] != 0)
    {
        return 0;
    }

    storedCRC = ReadBigEndianU32(
        data + slotOffset + NSMBW_SLOT_CRC_OFFSET);

    calculatedCRC = CalculateNSMBWCRC32(
        data + slotOffset,
        NSMBW_SLOT_CRC_OFFSET);

    return storedCRC == calculatedCRC;
}


/* =========================================================
   NSMBW SAVE PARSER
   ========================================================= */

/*
   IMPORTANT:

   This function is deliberately not guessing raw offsets.

   The actual dMj2dGame_c definition tells us the semantic
   fields, but the serialized save-file layout still needs to
   be mapped exactly.

   We therefore keep this parser isolated.

   Once the verified offsets/layout are established, this is
   the ONLY function that needs to change.

   Everything above and below it can remain intact.
*/
static int ParseNSMBWSave(
    const uint8_t *data,
    u32 fileSize,
    NSMBWSaveData *result)
{
    int slotIndex;
    int validSlotCount = 0;

    if (!data || !result ||
        fileSize < NSMBW_SAVE_FILE_SIZE)
    {
        return 0;
    }

    memset(result, 0, sizeof(NSMBWSaveData));

    for (slotIndex = 0;
         slotIndex < NSMBW_SAVE_SLOT_COUNT;
         slotIndex++)
    {
        u32 primaryOffset = NSMBW_PRIMARY_SLOT_OFFSET +
                            slotIndex * NSMBW_SAVE_SLOT_SIZE;
        u32 backupOffset = NSMBW_BACKUP_SLOT_OFFSET +
                           slotIndex * NSMBW_SAVE_SLOT_SIZE;
        u32 slotOffset = 0;
        int world;
        int stage;
        int player;

        /*
           The game keeps a second copy of every slot. Prefer
           the primary copy, but recover from its backup if its
           CRC is invalid (for example after an interrupted save).
        */
        if (IsNSMBWSlotValid(data, fileSize, primaryOffset))
            slotOffset = primaryOffset;
        else if (IsNSMBWSlotValid(data, fileSize, backupOffset))
            slotOffset = backupOffset;
        else
            continue;

        result->slot[slotIndex].valid = 1;
        result->slot[slotIndex].gameCompletion =
            data[slotOffset + NSMBW_SLOT_COMPLETION_OFFSET];

        for (player = 0;
             player < NSMBW_PLAYER_COUNT;
             player++)
        {
            result->slot[slotIndex].playerLife[player] =
                data[slotOffset + NSMBW_SLOT_LIVES_OFFSET + player];
        }

        for (world = 0;
             world < NSMBW_WORLD_COUNT;
             world++)
        {
            result->slot[slotIndex].worldCompletion[world] =
                data[slotOffset + NSMBW_SLOT_WORLD_OFFSET + world];

            for (stage = 0;
                 stage < NSMBW_STAGE_COUNT;
                 stage++)
            {
                u32 stageOffset = slotOffset +
                    NSMBW_SLOT_STAGE_OFFSET +
                    ((world * NSMBW_STAGE_COUNT + stage) * 4);

                result->slot[slotIndex].stageCompletion[world][stage] =
                    ReadBigEndianU32(data + stageOffset);
            }
        }

        validSlotCount++;
    }

    result->valid = validSlotCount > 0;

    return result->valid;
}


/* =========================================================
   NSMBW SAVE ANALYSIS
   ========================================================= */

static int AnalyzeNSMBWSave(
    uint64_t titleID,
    NSMBWSaveData *save)
{
    uint8_t *rawData = NULL;
    u32 fileSize = 0;

    int result;

    if (!save)
        return 0;

    memset(
        save,
        0,
        sizeof(NSMBWSaveData)
    );

    if (!ReadNSMBWSaveFile(
            titleID,
            &rawData,
            &fileSize))
    {
        return 0;
    }

    result = ParseNSMBWSave(
        rawData,
        fileSize,
        save
    );

    free(rawData);

    return result;
}


/* =========================================================
   ACHIEVEMENT UPDATE
   ========================================================= */

static int IsWorldCompleted(
    const NSMBWSaveSlot *slot,
    int world)
{
    int stage;

    if (!slot || !slot->valid)
        return 0;

    /* World 9 is complete only once all eight bonus stages are clear. */
    if (world == NSMBW_WORLD_9)
    {
        for (stage = NSMBW_STAGE_1;
             stage <= NSMBW_STAGE_8;
             stage++)
        {
            if (!(slot->stageCompletion[world][stage] &
                  NSMBW_GOAL_MASK))
            {
                return 0;
            }
        }

        return 1;
    }

    /* Worlds 4 and 6 finish at an airship; the rest finish at a castle. */
    if (world == NSMBW_WORLD_4 ||
        world == NSMBW_WORLD_6)
    {
        return (slot->stageCompletion[world][NSMBW_STAGE_AIRSHIP] &
                NSMBW_GOAL_MASK) != 0;
    }

    return (slot->stageCompletion[world][NSMBW_STAGE_CASTLE] &
            NSMBW_GOAL_MASK) != 0;
}


static int HasAllWorldStarCoins(
    const NSMBWSaveSlot *slot,
    int world)
{
    /*
       The retail game has 231 Star Coins. These are the exact
       per-world totals, including World 8's Airship and Bowser
       Castle as separate courses.
    */
    static const int requiredCoins[9] =
    {
        24, 24, 24, 27, 24, 27, 27, 30, 24
    };
    int stage;
    int collected = 0;

    if (!slot || !slot->valid ||
        world < NSMBW_WORLD_1 ||
        world > NSMBW_WORLD_9)
    {
        return 0;
    }

    for (stage = 0; stage < NSMBW_STAGE_COUNT; stage++)
    {
        uint32_t flags = slot->stageCompletion[world][stage];

        if (flags & NSMBW_COIN1_COLLECTED)
            collected++;
        if (flags & NSMBW_COIN2_COLLECTED)
            collected++;
        if (flags & NSMBW_COIN3_COLLECTED)
            collected++;
    }

    return collected >= requiredCoins[world];
}


static int HasAllOtherAchievements(void)
{
    int index;

    for (index = 0; index < ACHIEVEMENT_COUNT; index++)
    {
        /* Achievement #26 checks every achievement except itself. */
        if (index != 25 && !achievements[index].unlocked)
            return 0;
    }

    return 1;
}


static void UnlockAchievement(int index, int *changed)
{
    if (!achievements[index].unlocked)
    {
        achievements[index].unlocked = 1;

        if (changed)
            *changed = 1;
    }
}


static void UpdateNSMBWAchievements(uint64_t titleID)
{
    int hasSave = HasNSMBWSave(titleID);
    int changed = 0;
    NSMBWSaveData save;

    /* Achievement #27: creating a save is permanent. */
    if (hasSave)
        UnlockAchievement(26, &changed);

    if (hasSave && AnalyzeNSMBWSave(titleID, &save))
    {
        int slotIndex;
        int allSlotsBeaten = 1;

        for (slotIndex = 0;
             slotIndex < NSMBW_SAVE_SLOT_COUNT;
             slotIndex++)
        {
            NSMBWSaveSlot *slot = &save.slot[slotIndex];

            if (!slot->valid ||
                (slot->gameCompletion & NSMBW_SAVE_EMPTY))
            {
                allSlotsBeaten = 0;
                continue;
            }

            {
                int world;

                for (world = NSMBW_WORLD_1;
                     world <= NSMBW_WORLD_9;
                     world++)
                {
                    /* #1, #3, ... #17: complete a world. */
                    if (IsWorldCompleted(slot, world))
                        UnlockAchievement(world * 2, &changed);

                    /* #2, #4, ... #18: collect a world's coins. */
                    if (HasAllWorldStarCoins(slot, world))
                        UnlockAchievement(world * 2 + 1, &changed);
                }
            }

            /* Achievement #19: Beat the final boss. */
            if (slot->gameCompletion & NSMBW_FINAL_BOSS_BEATEN)
                UnlockAchievement(18, &changed);

            /* Achievement #20: every regular and special coin. */
            if ((slot->gameCompletion &
                 (NSMBW_COIN_ALL | NSMBW_COIN_ALL_SPECIAL)) ==
                (NSMBW_COIN_ALL | NSMBW_COIN_ALL_SPECIAL))
            {
                UnlockAchievement(19, &changed);
            }

            if (slot->playerLife[NSMBW_PLAYER_MARIO] >= 99)
                UnlockAchievement(20, &changed);
            if (slot->playerLife[NSMBW_PLAYER_LUIGI] >= 99)
                UnlockAchievement(21, &changed);
            if (slot->playerLife[NSMBW_PLAYER_YELLOW_TOAD] >= 99)
                UnlockAchievement(22, &changed);
            if (slot->playerLife[NSMBW_PLAYER_BLUE_TOAD] >= 99)
                UnlockAchievement(23, &changed);

            if (!(slot->gameCompletion & NSMBW_FINAL_BOSS_BEATEN))
                allSlotsBeaten = 0;
        }

        if (allSlotsBeaten)
            UnlockAchievement(24, &changed);
    }

    /* Achievement #26 is the meta-achievement. */
    if (HasAllOtherAchievements())
        UnlockAchievement(25, &changed);

    if (changed)
        SaveAchievements();
}


/* =========================================================
   GAME LIBRARY UPDATE
   ========================================================= */

static int UpdateGameLibrary(void)
{
    uint64_t titleID = 0;

    if (!FindNSMBWTitle(&titleID))
        return 0;

    if (!saveData.nsmbwInLibrary ||
        saveData.nsmbwTitleID != titleID)
    {
        saveData.nsmbwInLibrary = 1;
        saveData.nsmbwTitleID = titleID;

        SaveAchievements();
    }

    return 1;
}


/* =========================================================
   ACHIEVEMENT COUNT
   ========================================================= */

static int GetUnlockedAchievementCount(void)
{
    int i;
    int count = 0;

    for (i = 0; i < ACHIEVEMENT_COUNT; i++)
    {
        if (achievements[i].unlocked)
            count++;
    }

    return count;
}


/* =========================================================
   SCREEN HELPERS
   ========================================================= */

static void ClearScreen(void)
{
    printf("\x1b[2J");
    printf("\x1b[H");
}


static void WaitForButton(void)
{
    while (1)
    {
        WPAD_ScanPads();

        if (WPAD_ButtonsDown(0))
            return;

        VIDEO_WaitVSync();
    }
}


/* =========================================================
   ACHIEVEMENT DETAILS
   ========================================================= */

static void ShowAchievementDetails(int index)
{
    while (1)
    {
        WPAD_ScanPads();

        ClearScreen();

        printf("\n");
        printf("========================================\n");
        printf("           Achievement %d/%d\n",
               index + 1,
               ACHIEVEMENT_COUNT);
        printf("========================================\n\n");

        printf("%s\n\n",
               achievements[index].name);

        printf("%s\n\n",
               achievements[index].description);

        if (achievements[index].unlocked)
            printf("[ UNLOCKED ]\n");
        else
            printf("[ LOCKED ]\n");

        printf("\n");
        printf("B: Back\n");

        if (WPAD_ButtonsDown(0) &
            WPAD_BUTTON_B)
        {
            return;
        }

        VIDEO_WaitVSync();
    }
}


/* =========================================================
   ACHIEVEMENT LIST
   ========================================================= */

static void ShowAchievements(void)
{
    int index = 0;
    int firstVisible = 0;
    const int visibleTaskCount = 10;

    while (1)
    {
        WPAD_ScanPads();

        if (WPAD_ButtonsDown(0) &
            WPAD_BUTTON_B)
        {
            return;
        }

        if (WPAD_ButtonsDown(0) &
            WPAD_BUTTON_UP)
        {
            index--;

            if (index < 0)
                index = ACHIEVEMENT_COUNT - 1;
        }

        if (WPAD_ButtonsDown(0) &
            WPAD_BUTTON_DOWN)
        {
            index++;

            if (index >= ACHIEVEMENT_COUNT)
                index = 0;
        }

        if (WPAD_ButtonsDown(0) &
            WPAD_BUTTON_A)
        {
            ShowAchievementDetails(index);
        }

        /* Keep the selected task inside the visible checklist. */
        if (index < firstVisible)
            firstVisible = index;

        if (index >= firstVisible + visibleTaskCount)
            firstVisible = index - visibleTaskCount + 1;

        ClearScreen();

        printf("\n");
        printf("========================================\n");
        printf("       New Super Mario Bros. Wii\n");
        printf("             Task List\n");
        printf("========================================\n\n");

        printf("Completed: %d/%d\n\n",
               GetUnlockedAchievementCount(),
               ACHIEVEMENT_COUNT);

        {
            int row;

            for (row = 0;
                 row < visibleTaskCount &&
                 firstVisible + row < ACHIEVEMENT_COUNT;
                 row++)
            {
                int taskIndex = firstVisible + row;

                printf("%c %s %2d. %s\n",
                       taskIndex == index ? '>' : ' ',
                       achievements[taskIndex].unlocked
                           ? "[X]"
                           : "[ ]",
                       achievements[taskIndex].id,
                       achievements[taskIndex].name);
            }
        }

        printf("\n");
        printf("UP/DOWN: Select\n");
        printf("A: Task details    B: Back\n");

        VIDEO_WaitVSync();
    }
}


/* =========================================================
   GAME LIBRARY
   ========================================================= */

static void DrawGameLibrary(void)
{
    while (1)
    {
        WPAD_ScanPads();

        ClearScreen();

        printf("\n");
        printf("========================================\n");
        printf("              Games\n");
        printf("========================================\n\n");

        if (saveData.nsmbwInLibrary)
        {
            printf("> New Super Mario Bros. Wii\n");

            printf("  Region: %s\n",
                   GetNSMBWRegion(
                       saveData.nsmbwTitleID));

            printf("\n");
            printf("A: Open\n");
            printf("B: Back\n");

            if (WPAD_ButtonsDown(0) &
                WPAD_BUTTON_A)
            {
                UpdateNSMBWAchievements(
                    saveData.nsmbwTitleID);

                ShowAchievements();
            }
        }
        else
        {
            printf("No games recognized yet.\n\n");

            printf("Play a supported game first,\n");
            printf("then return here and scan again.\n");

            printf("\n");
            printf("B: Back\n");
        }

        if (WPAD_ButtonsDown(0) &
            WPAD_BUTTON_B)
        {
            return;
        }

        VIDEO_WaitVSync();
    }
}


/* =========================================================
   PROFILE
   ========================================================= */

static void ShowProfile(void)
{
    while (1)
    {
        WPAD_ScanPads();

        ClearScreen();

        printf("\n");
        printf("========================================\n");
        printf("               Profile\n");
        printf("========================================\n\n");

        printf("HellYeahMike\n\n");

        printf("Achievements unlocked:\n");
        printf("%d/%d\n\n",
               GetUnlockedAchievementCount(),
               ACHIEVEMENT_COUNT);

        printf("Games recognized:\n");

        if (saveData.nsmbwInLibrary)
            printf("- New Super Mario Bros. Wii\n");
        else
            printf("- None\n");

        printf("\n");
        printf("B: Back\n");

        if (WPAD_ButtonsDown(0) &
            WPAD_BUTTON_B)
        {
            return;
        }

        VIDEO_WaitVSync();
    }
}


/* =========================================================
   SETTINGS
   ========================================================= */

static void ShowSettings(void)
{
    while (1)
    {
        WPAD_ScanPads();

        ClearScreen();

        printf("\n");
        printf("========================================\n");
        printf("               Settings\n");
        printf("========================================\n\n");

        printf("WiiCanDoIt v0.1\n\n");

        printf("SD card: %s\n",
               sdReady
                   ? "Ready"
                   : "Unavailable");

        printf("IOS filesystem: %s\n",
               isfsReady
                   ? "Ready"
                   : "Unavailable");

        printf("\n");
        printf("B: Back\n");

        if (WPAD_ButtonsDown(0) &
            WPAD_BUTTON_B)
        {
            return;
        }

        VIDEO_WaitVSync();
    }
}


/* =========================================================
   MAIN MENU
   ========================================================= */

static void DrawMainMenu(void)
{
    selectedOption = 0;

    while (1)
    {
        WPAD_ScanPads();

        if (WPAD_ButtonsDown(0) &
            WPAD_BUTTON_UP)
        {
            selectedOption--;

            if (selectedOption < 0)
                selectedOption = 3;
        }

        if (WPAD_ButtonsDown(0) &
            WPAD_BUTTON_DOWN)
        {
            selectedOption++;

            if (selectedOption > 3)
                selectedOption = 0;
        }

        if (WPAD_ButtonsDown(0) &
            WPAD_BUTTON_A)
        {
            switch (selectedOption)
            {
                case 0:
                    DrawGameLibrary();
                    break;

                case 1:
                    ShowProfile();
                    break;

                case 2:
                    ShowSettings();
                    break;

                case 3:
                    return;
            }
        }

        ClearScreen();

        printf("\n");
        printf("========================================\n");
        printf("          WiiCanDoIt v0.1\n");
        printf("========================================\n\n");

        printf("%s Games\n",
               selectedOption == 0
                   ? ">"
                   : " ");

        printf("%s Profile\n",
               selectedOption == 1
                   ? ">"
                   : " ");

        printf("%s Settings\n",
               selectedOption == 2
                   ? ">"
                   : " ");

        printf("%s Exit\n",
               selectedOption == 3
                   ? ">"
                   : " ");

        printf("\n");

        printf("Achievements: %d/%d\n",
               GetUnlockedAchievementCount(),
               ACHIEVEMENT_COUNT);

        printf("\n");
        printf("UP/DOWN: Select\n");
        printf("A: Confirm\n");

        VIDEO_WaitVSync();
    }
}


/* =========================================================
   MAIN
   ========================================================= */

int main(int argc, char **argv)
{
    GXRModeObj *rmode;
    void *framebuffer;

    uint64_t detectedTitleID = 0;


    /* -----------------------------------------------------
       Hardware initialization
       ----------------------------------------------------- */

    VIDEO_Init();

    WPAD_Init();

    sdReady = fatInitDefault();


    if (ISFS_Initialize() >= 0)
        isfsReady = 1;
    else
        isfsReady = 0;


    /* -----------------------------------------------------
       Video setup
       ----------------------------------------------------- */

    rmode = VIDEO_GetPreferredMode(NULL);

    framebuffer = MEM_K0_TO_K1(
        SYS_AllocateFramebuffer(rmode)
    );

    console_init(
        framebuffer,
        20,
        20,
        rmode->fbWidth,
        rmode->xfbHeight,
        rmode->fbWidth *
        VI_DISPLAY_PIX_SZ
    );

    VIDEO_Configure(rmode);
    VIDEO_SetNextFramebuffer(framebuffer);
    VIDEO_SetBlack(FALSE);
    VIDEO_Flush();
    VIDEO_WaitVSync();


    /* -----------------------------------------------------
       WiiCanDoIt save data
       ----------------------------------------------------- */

    ResetSaveData();

    if (sdReady)
        LoadAchievements();
    else
        ApplySaveToAchievements();


    /* -----------------------------------------------------
       Startup game scan
       ----------------------------------------------------- */

    if (isfsReady)
    {
        if (FindNSMBWTitle(
                &detectedTitleID))
        {
            /*
               Library membership.
            */
            if (!saveData.nsmbwInLibrary ||
                saveData.nsmbwTitleID !=
                    detectedTitleID)
            {
                saveData.nsmbwInLibrary = 1;
                saveData.nsmbwTitleID =
                    detectedTitleID;

                SaveAchievements();
            }


            /*
               Achievement #27.

               Save detection is independent from
               library membership.
            */
            UpdateNSMBWAchievements(
                detectedTitleID);
        }
    }


    /* -----------------------------------------------------
       Main application
       ----------------------------------------------------- */

    DrawMainMenu();


    /* -----------------------------------------------------
       Clean exit
       ----------------------------------------------------- */

    if (isfsReady)
        ISFS_Deinitialize();

    return 0;
}
