#define __STDC_WANT_LIB_EXT1_ 1

#include "serum-decode.h"
#include "serum-version.h"
#include <chrono>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <miniz/miniz.h>

#if defined(__APPLE__)
#include <TargetConditionals.h>
#endif

#if not defined(__STDC_LIB_EXT1__)

// trivial implementation of the secure string functions if not directly upported by the compiler
// these do not perform all security checks and can be improved for sure
int strcpy_s(char* dest, int destsz, const char* src)
{
    if ((dest == NULL) || (src == NULL)) return 1;
    if (strlen(src) >= destsz) return 1;
    strcpy(dest, src);
    return 0;
}

int strcat_s(char* dest, int destsz, const char* src) {
    if ((dest == NULL) || (src == NULL)) return 1;
    if (strlen(dest)+strlen(src) >= destsz) return 1;
    strcat(dest, src);
    return 0;
}

#endif


#pragma warning(disable: 4996)

const int pathbuflen=4096;

const int IDENTIFY_SAME_FRAME = -2;
const int IDENTIFY_NO_FRAME = -1;

// header
char rname[64];
UINT32 fwidth, fheight;
UINT16 nframes;
UINT32 nocolors, nccolors;
UINT32 ncompmasks, nmovmasks;
UINT32 nsprites;
UINT16 nbackgrounds;
// data
UINT32* hashcodes = NULL;
UINT8* shapecompmode = NULL;
UINT8* compmaskID = NULL;
UINT8* movrctID = NULL;
UINT8* compmasks = NULL;
UINT8* movrcts = NULL;
UINT8* cpal = NULL;
UINT8* cframes = NULL;
UINT8* dynamasks = NULL;
UINT8* dyna4cols = NULL;
UINT8* framesprites = NULL;
UINT8* spritedescriptionso = NULL;
UINT8* spritedescriptionsc = NULL;
UINT8* activeframes = NULL;
UINT8* colorrotations = NULL;
UINT16* spritedetareas = NULL;
UINT32* spritedetdwords = NULL;
UINT16* spritedetdwordpos = NULL;
UINT32* triggerIDs = NULL;
UINT16* framespriteBB = NULL;
UINT8* backgroundframes = NULL;
UINT16* backgroundIDs = NULL;
UINT16* backgroundBB = NULL;

// variables
bool cromloaded = false; // is there a crom loaded?
UINT16 lastfound = 0; // last frame ID identified
UINT8* lastframe = NULL; // last frame content identified
UINT32 lastframe_found = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
UINT32 lastframe_full_crc = 0;
UINT8* lastpalette = NULL; // last palette identified
UINT8* lastrotations = NULL; // last colour rotations identified
UINT8 lastsprite[MAX_SPRITE_TO_DETECT]; // last sprites identified
UINT lastnsprites; // last amount of sprites detected
UINT16 lastfrx[MAX_SPRITE_TO_DETECT], lastfry[MAX_SPRITE_TO_DETECT]; // last position in the frame of the sprite
UINT16 lastspx[MAX_SPRITE_TO_DETECT], lastspy[MAX_SPRITE_TO_DETECT]; // last top left of the sprite to display
UINT16 lastwid[MAX_SPRITE_TO_DETECT], lasthei[MAX_SPRITE_TO_DETECT]; // last dimensions of the sprite to display
UINT32 lasttriggerID = 0xFFFFFFFF; // last trigger ID found
bool isrotation = true; // are there rotations to send
bool crc32_ready = false; // is the crc32 table filled?
UINT32 crc32_table[256]; // initial table
bool* framechecked = NULL; // are these frames checked?
UINT16 ignoreUnknownFramesTimeout = 0;
UINT8 maxFramesToSkip = 0;
UINT8 framesSkippedCounter = 0;
UINT8 standardPalette[PALETTE_SIZE];
UINT8 standardPaletteLength = 0;
UINT32 colorshifts[MAX_COLOR_ROTATIONS]; // how many color we shifted
UINT32 colorshiftinittime[MAX_COLOR_ROTATIONS]; // when was the tick for this rotation
bool enabled = true; // is colorization enabled?

void Serum_free(void)
{
    if (hashcodes)
    {
        free(hashcodes);
        hashcodes = NULL;
    }
    if (shapecompmode)
    {
        free(shapecompmode);
        shapecompmode = NULL;
    }
    if (compmaskID)
    {
        free(compmaskID);
        compmaskID = NULL;
    }
    if (movrctID)
    {
        free(movrctID);
        movrctID = NULL;
    }
    if (compmasks)
    {
        free(compmasks);
        compmasks = NULL;
    }
    if (movrcts)
    {
        free(movrcts);
        movrcts = NULL;
    }
    if (cpal)
    {
        free(cpal);
        cpal = NULL;
    }
    if (cframes)
    {
        free(cframes);
        cframes = NULL;
    }
    if (dynamasks)
    {
        free(dynamasks);
        dynamasks = NULL;
    }
    if (dyna4cols)
    {
        free(dyna4cols);
        dyna4cols = NULL;
    }
    if (framesprites)
    {
        free(framesprites);
        framesprites = NULL;
    }
    if (framespriteBB)
    {
        free(framespriteBB);
        framespriteBB = NULL;
    }
    if (spritedescriptionso)
    {
        free(spritedescriptionso);
        spritedescriptionso = NULL;
    }
    if (spritedescriptionsc)
    {
        free(spritedescriptionsc);
        spritedescriptionsc = NULL;
    }
    if (spritedetdwords)
    {
        free(spritedetdwords);
        spritedetdwords = NULL;
    }
    if (spritedetdwordpos)
    {
        free(spritedetdwordpos);
        spritedetdwordpos = NULL;
    }
    if (spritedetareas)
    {
        free(spritedetareas);
        spritedetareas = NULL;
    }
    if (activeframes)
    {
        free(activeframes);
        activeframes = NULL;
    }
    if (colorrotations)
    {
        free(colorrotations);
        colorrotations = NULL;
    }
    if (lastframe)
    {
        free(lastframe);
        lastframe = NULL;
    }
    if (lastpalette)
    {
        free(lastpalette);
        lastpalette = NULL;
    }
    if (lastrotations)
    {
        free(lastrotations);
        lastrotations = NULL;
    }
    if (framechecked)
    {
        free(framechecked);
        framechecked = NULL;
    }
    if (triggerIDs)
    {
        free(triggerIDs);
        triggerIDs = NULL;
    }
    if (backgroundframes)
    {
        free(backgroundframes);
        backgroundframes = NULL;
    }
    if (backgroundIDs)
    {
        free(backgroundIDs);
        backgroundIDs = NULL;
    }
    if (backgroundBB)
    {
        free(backgroundBB);
        backgroundBB = NULL;
    }
    cromloaded = false;
}

SERUM_API const char* Serum_GetVersion()
{
    return SERUM_VERSION;
}

SERUM_API const char* Serum_GetMinorVersion()
{
    return SERUM_MINOR_VERSION;
}

void CRC32encode(void) // initiating the CRC table, must be called at startup
{
    for (int i = 0; i < 256; i++)
    {
        UINT32 ch = i;
        UINT32 crc = 0;
        for (int j = 0; j < 8; j++)
        {
            UINT32 b = (ch ^ crc) & 1;
            crc >>= 1;
            if (b != 0) crc = crc ^ 0xEDB88320;
            ch >>= 1;
        }
        crc32_table[i] = crc;
    }
    crc32_ready = true;
}

UINT32 crc32_fast(UINT8* s, UINT n, UINT8 ShapeMode) // computing a buffer CRC32, "CRC32encode()" must have been called before the first use
{

    UINT32 crc = 0xFFFFFFFF;
    for (int i = 0; i < (int)n; i++)
    {
        UINT8 val = s[i];
        if ((ShapeMode == 1) && (val > 1))  val = 1;
        crc = (crc >> 8) ^ crc32_table[(val ^ crc) & 0xFF];
    }
    return ~crc;
}

UINT32 crc32_fast_mask(UINT8* source, UINT8* mask, UINT n, UINT8 ShapeMode) // computing a buffer CRC32 on the non-masked area, "CRC32encode()" must have been called before the first use
// take into account if we are in shape mode
{
    UINT32 crc = 0xFFFFFFFF;
    for (UINT i = 0; i < n; i++)
    {
        if (mask[i] == 0)
        {
            UINT8 val = source[i];
            if ((ShapeMode == 1) && (val > 1)) val = 1;
            crc = (crc >> 8) ^ crc32_table[(val ^ crc) & 0xFF];
        }
    }
    return ~crc;
}

bool unzip_crz(const char* const filename, const char* const extractpath, char* cromname, int cromsize) {

    bool ok = true;
    mz_zip_archive zip_archive;
    memset(&zip_archive, 0, sizeof(zip_archive));

    if (!mz_zip_reader_init_file(&zip_archive, filename, 0)) {
        return false;
    }

    int num_files = mz_zip_reader_get_num_files(&zip_archive);

    if (num_files == 0 || !mz_zip_reader_get_filename(&zip_archive, 0, cromname, cromsize)) {
        mz_zip_reader_end(&zip_archive);
        return false;
    }

    for (int i = 0; i < num_files; i++) {
        mz_zip_archive_file_stat file_stat;
        if (!mz_zip_reader_file_stat(&zip_archive, i, &file_stat))
            continue;


        char dstPath[pathbuflen];
        if (strcpy_s(dstPath, pathbuflen, extractpath)) goto fail;
        if (strcat_s(dstPath, pathbuflen, file_stat.m_filename)) goto fail;

        mz_zip_reader_extract_file_to_file(&zip_archive, file_stat.m_filename, dstPath, 0);
    }

    goto nofail;
fail:
    ok = false;
nofail:

    mz_zip_reader_end(&zip_archive);

    return ok;
}

void Reset_ColorRotations(void)
{
    memset(colorshifts, 0, MAX_COLOR_ROTATIONS * sizeof(UINT32));
    colorshiftinittime[0] = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    for (int ti = 1; ti < MAX_COLOR_ROTATIONS; ti++) colorshiftinittime[ti] = colorshiftinittime[0];
}

SERUM_API bool Serum_LoadFile(const char* const filename, int* pwidth, int* pheight, unsigned int* pnocolors, unsigned int* pntriggers)
{
    char pathbuf[pathbuflen];

    if (!crc32_ready) CRC32encode();

    // check if we're using an uncompressed cROM file
    const char* ext;
    bool uncompressedCROM = false;
    if ((ext = strrchr(filename, '.')) != NULL) {
        if (strcmp(ext, ".cROM") == 0) {
            uncompressedCROM = true;
            if (strcpy_s(pathbuf, pathbuflen, filename)) return false;
        }
    }

    // extract file if it is compressed
    if (!uncompressedCROM) {
       char* tmpdir = getenv("TMPDIR");
       if (tmpdir) {
           if (strcpy_s(pathbuf, pathbuflen, tmpdir)) return false;
           if (strcat_s(pathbuf, pathbuflen, "/")) return false;
       }
       else if (strcpy_s(pathbuf, pathbuflen, filename)) return false;

       char cromname[pathbuflen];
       if (!unzip_crz(filename, pathbuf, cromname, pathbuflen)) return false;
       if (strcat_s(pathbuf, pathbuflen, cromname)) return false;
    }

    // Open cRom
    FILE* pfile;
    pfile = fopen(pathbuf, "rb");
    if (!pfile)
    {
        enabled = false;
        return false;
    }

    // read the header to know how much memory is needed
    fread(rname, 1, 64, pfile);
    UINT32 sizeheader;
    fread(&sizeheader, 4, 1, pfile);
    fread(&fwidth, 4, 1, pfile);
    fread(&fheight, 4, 1, pfile);
    // The serum file stored the number of frames as UINT32, but in fact, the number of frames will never exceed the size of UINT16 (65535)
    UINT32 nframes32;
    fread(&nframes32, 4, 1, pfile);
    nframes = (UINT16) nframes32;
    fread(&nocolors, 4, 1, pfile);
    fread(&nccolors, 4, 1, pfile);
    if ((fwidth == 0) || (fheight == 0) || (nframes == 0) || (nocolors == 0) || (nccolors == 0))
    {
        // incorrect file format
        fclose(pfile);
        enabled = false;
        return false;
    }
    fread(&ncompmasks, 4, 1, pfile);
    fread(&nmovmasks, 4, 1, pfile);
    fread(&nsprites, 4, 1, pfile);
    if (sizeheader >= 13 * sizeof(UINT)) fread(&nbackgrounds, 2, 1, pfile); else nbackgrounds = 0;
    // allocate memory for the serum format
    hashcodes = (UINT32*)malloc(sizeof(UINT32) * nframes);
    shapecompmode = (UINT8*)malloc(nframes);
    compmaskID = (UINT8*)malloc(nframes);
    movrctID = (UINT8*)malloc(nframes);
    compmasks = (UINT8*)malloc(ncompmasks * fwidth * fheight);
    movrcts = (UINT8*)malloc(nmovmasks * 4);
    cpal = (UINT8*)malloc(nframes * 3 * nccolors);
    cframes = (UINT8*)malloc(nframes * fwidth * fheight);
    dynamasks = (UINT8*)malloc(nframes * fwidth * fheight);
    dyna4cols = (UINT8*)malloc(nframes * MAX_DYNA_4COLS_PER_FRAME * nocolors);
    framesprites = (UINT8*)malloc(nframes * MAX_SPRITES_PER_FRAME);
    framespriteBB = (UINT16*)malloc(nframes * MAX_SPRITES_PER_FRAME * 4 * sizeof(UINT16));
    spritedescriptionso = (UINT8*)malloc(nsprites * MAX_SPRITE_SIZE * MAX_SPRITE_SIZE);
    spritedescriptionsc = (UINT8*)malloc(nsprites * MAX_SPRITE_SIZE * MAX_SPRITE_SIZE);
    activeframes = (UINT8*)malloc(nframes);
    colorrotations = (UINT8*)malloc(nframes * 3 * MAX_COLOR_ROTATIONS);
    spritedetdwords = (UINT32*)malloc(nsprites * sizeof(UINT32) * MAX_SPRITE_DETECT_AREAS);
    spritedetdwordpos = (UINT16*)malloc(nsprites * sizeof(UINT16) * MAX_SPRITE_DETECT_AREAS);
    spritedetareas = (UINT16*)malloc(nsprites * sizeof(UINT16) * MAX_SPRITE_DETECT_AREAS * 4);
    triggerIDs = (UINT32*)malloc(nframes * sizeof(UINT32));
    backgroundframes = (UINT8*)malloc(nbackgrounds * fwidth * fheight);
    backgroundIDs = (UINT16*)malloc(nframes * sizeof(UINT16));
    backgroundBB = (UINT16*)malloc(nframes * 4 * sizeof(UINT16));
    if (!hashcodes || !shapecompmode || !compmaskID || !movrctID || !cpal || !cframes || !dynamasks || !dyna4cols || !framesprites || !framespriteBB || !activeframes || !colorrotations || !triggerIDs ||
        (!compmasks && ncompmasks > 0) ||
        (!movrcts && nmovmasks > 0) ||
        ((nsprites > 0) && (!spritedescriptionso || !spritedescriptionsc || !spritedetdwords || !spritedetdwordpos || !spritedetareas)) ||
        ((nbackgrounds > 0) && (!backgroundframes || !backgroundIDs || !backgroundBB)))
    {
        Serum_free();
        fclose(pfile);
        enabled = false;
        return false;
    }
    // read the cRom file
    fread(hashcodes, sizeof(UINT32), nframes, pfile);
    fread(shapecompmode, 1, nframes, pfile);
    fread(compmaskID, 1, nframes, pfile);
    fread(movrctID, 1, nframes, pfile);
    fread(compmasks, 1, ncompmasks * fwidth * fheight, pfile);
    fread(movrcts, 1, nmovmasks * fwidth * fheight, pfile);
    fread(cpal, 1, nframes * 3 * nccolors, pfile);
    fread(cframes, 1, nframes * fwidth * fheight, pfile);
    fread(dynamasks, 1, nframes * fwidth * fheight, pfile);
    fread(dyna4cols, 1, nframes * MAX_DYNA_4COLS_PER_FRAME * nocolors, pfile);
    fread(framesprites, 1, nframes * MAX_SPRITES_PER_FRAME, pfile);
    for (int ti = 0; ti < (int)nsprites * MAX_SPRITE_SIZE * MAX_SPRITE_SIZE; ti++)
    {
        fread(&spritedescriptionsc[ti], 1, 1, pfile);
        fread(&spritedescriptionso[ti], 1, 1, pfile);
    }
    fread(activeframes, 1, nframes, pfile);
    fread(colorrotations, 1, nframes * 3 * MAX_COLOR_ROTATIONS, pfile);
    fread(spritedetdwords, sizeof(UINT32), nsprites * MAX_SPRITE_DETECT_AREAS, pfile);
    fread(spritedetdwordpos, sizeof(UINT16), nsprites * MAX_SPRITE_DETECT_AREAS, pfile);
    fread(spritedetareas, sizeof(UINT16), nsprites * 4 * MAX_SPRITE_DETECT_AREAS, pfile);
    if (sizeheader >= 11 * sizeof(UINT)) fread(triggerIDs, sizeof(UINT32), nframes, pfile);
    else memset(triggerIDs, 0xFF, sizeof(UINT32) * nframes);
    if (sizeheader >= 12 * sizeof(UINT)) fread(framespriteBB, sizeof(UINT16), nframes * MAX_SPRITES_PER_FRAME * 4, pfile);
    else
    {
        for (UINT tj = 0; tj < nframes; tj++)
        {
            for (UINT ti = 0; ti < MAX_SPRITES_PER_FRAME; ti++)
            {
                framespriteBB[tj * MAX_SPRITES_PER_FRAME * 4 + ti * 4] = 0;
                framespriteBB[tj * MAX_SPRITES_PER_FRAME * 4 + ti * 4 + 1] = 0;
                framespriteBB[tj * MAX_SPRITES_PER_FRAME * 4 + ti * 4 + 2] = fwidth - 1;
                framespriteBB[tj * MAX_SPRITES_PER_FRAME * 4 + ti * 4 + 3] = fheight - 1;
            }
        }
    }
    if (sizeheader >= 13 * sizeof(UINT))
    {
        fread(backgroundframes, fwidth * fheight, nbackgrounds, pfile);
        fread(backgroundIDs, sizeof(UINT16), nframes, pfile);
        fread(backgroundBB, 4 * sizeof(UINT16), nframes, pfile);
    }
    else memset(backgroundIDs, 0xFF, nframes * sizeof(UINT16));
    fclose(pfile);
    if (pntriggers)
    {
        *pntriggers = 0;
        for (UINT ti = 0; ti < nframes; ti++)
        {
            if (triggerIDs[ti] != 0xFFFFFFFF) (*pntriggers)++;
        }
    }
    // allocate memory for previous detected frame
    lastframe = (UINT8*)malloc(fwidth * fheight);
    lastpalette = (UINT8*)malloc(nccolors * 3);
    lastrotations = (UINT8*)malloc(3 * MAX_COLOR_ROTATIONS);
    framechecked = (bool*)malloc(sizeof(bool) * nframes);
    if (!lastframe || !lastpalette || !lastrotations || !framechecked)
    {
        Serum_free();
        enabled = false;
        return false;
    }
    memset(lastframe, 0, fwidth * fheight);
    memset(lastpalette, 0, nccolors * 3);
    memset(lastrotations, 255, 3 * MAX_COLOR_ROTATIONS);
    memset(lastsprite, 255, MAX_SPRITE_TO_DETECT);
    *pwidth = fwidth;
    *pheight = fheight;
    if (pnocolors)
    {
        *pnocolors = nocolors;
    }
    Reset_ColorRotations();
    cromloaded = true;

    if (!uncompressedCROM) {
        // remove temporary file that had been extracted from compressed CRZ file
        remove(pathbuf);
    }

    enabled = true;
    return true;
}

SERUM_API bool Serum_Load(const char* const altcolorpath, const char* const romname, int* pwidth, int* pheight, unsigned int* pnocolors, unsigned int* pntriggers)
{
    char pathbuf[pathbuflen];
    if (strcpy_s(pathbuf, pathbuflen, altcolorpath) ||
        ((pathbuf[strlen(pathbuf) - 1] != '\\') && (pathbuf[strlen(pathbuf) - 1] != '/')  && strcat_s(pathbuf, pathbuflen, "/")) ||
        strcat_s(pathbuf, pathbuflen, romname) ||
        strcat_s(pathbuf, pathbuflen, "/") ||
        strcat_s(pathbuf, pathbuflen, romname) ||
        strcat_s(pathbuf, pathbuflen, ".cRZ"))
    {
        enabled = false;
        return false;
    }

    return Serum_LoadFile(pathbuf, pwidth, pheight, pnocolors, pntriggers);
}

SERUM_API void Serum_Dispose(void)
{
    Serum_free();
}

int32_t Identify_Frame(UINT8* frame)
{
    // Usually the first frame has the ID 0, but lastfound is also initialized with 0. So we ned a helper to be able to detect frame 0 as new.
    static bool first_match = true;

    if (!cromloaded) return IDENTIFY_NO_FRAME;
    UINT8* pmask;
    memset(framechecked, false, nframes);
    UINT16 tj = lastfound; // we start from the frame we last found
    UINT pixels = fwidth * fheight;
    do
    {
        if (!framechecked[tj])
        {
            // calculate the hashcode for the generated frame with the mask and shapemode of the current crom frame
            UINT8 mask = compmaskID[tj];
            UINT8 Shape = shapecompmode[tj];
            UINT32 Hashc;
            if (mask < 255)
            {
                pmask = &compmasks[mask * pixels];
                Hashc = crc32_fast_mask(frame, pmask, pixels, Shape);
            }
            else Hashc = crc32_fast(frame, pixels, Shape);
            // now we can compare with all the crom frames that share these same mask and shapemode
            UINT16 ti = tj;
            do
            {
                if (!framechecked[ti])
                {
                    if ((compmaskID[ti] == mask) && (shapecompmode[ti] == Shape))
                    {
                        if (Hashc == hashcodes[ti])
                        {
                            if (first_match || ti != lastfound || mask < 255) {
                                Reset_ColorRotations();
                                lastfound = ti;
                                lastframe_full_crc = crc32_fast(frame, pixels, 0);
                                first_match = false;
                                return ti; // we found the frame, we return it
                            }

                            UINT32 full_crc = crc32_fast(frame, pixels, 0);
                            if (full_crc != lastframe_full_crc) {
                                lastframe_full_crc = full_crc;
                                return ti; // we found the same frame with shape as before, but the full frame is different
                            }

                            return IDENTIFY_SAME_FRAME; // we found the frame, but it is the same full frame as before (no mask)
                        }
                        framechecked[ti] = true;
                    }
                }
                if (++ti >= nframes) ti = 0;
            } while (ti != tj);
        }
        if (++tj >= nframes) tj = 0;
    } while (tj != lastfound);

    return IDENTIFY_NO_FRAME; // we found no frame
}

bool Check_Sprites(UINT8* Frame, int quelleframe, UINT8* pquelsprites, UINT8* nspr, UINT16* pfrx, UINT16* pfry, UINT16* pspx, UINT16* pspy, UINT16* pwid, UINT16* phei)
{
    UINT8 ti = 0;
    UINT32 mdword;
    *nspr = 0;
    while ((ti < MAX_SPRITES_PER_FRAME) && (framesprites[quelleframe * MAX_SPRITES_PER_FRAME + ti] < 255))
    {
        UINT8 qspr = framesprites[quelleframe * MAX_SPRITES_PER_FRAME + ti];
        short minxBB = (short)framespriteBB[(quelleframe * MAX_SPRITES_PER_FRAME + ti) * 4];
        short minyBB = (short)framespriteBB[(quelleframe * MAX_SPRITES_PER_FRAME + ti) * 4 + 1];
        short maxxBB = (short)framespriteBB[(quelleframe * MAX_SPRITES_PER_FRAME + ti) * 4 + 2];
        short maxyBB = (short)framespriteBB[(quelleframe * MAX_SPRITES_PER_FRAME + ti) * 4 + 3];
        for (UINT32 tm = 0; tm < MAX_SPRITE_DETECT_AREAS; tm++)
        {
            if (spritedetareas[qspr * MAX_SPRITE_DETECT_AREAS * 4 + tm * 4] == 0xffff) continue;
            // we look for the sprite in the frame sent
            for (short ty = minyBB; ty <= maxyBB; ty++)
            {
                mdword = (UINT32)(Frame[ty * fwidth + minxBB] << 8) | (UINT32)(Frame[ty * fwidth + minxBB + 1] << 16) | (UINT32)(Frame[ty * fwidth + minxBB + 2] << 24);
                for (short tx = minxBB; tx <= maxxBB - 3; tx++)
                {
                    UINT tj = ty * fwidth + tx;
                    mdword = (mdword >> 8) | (UINT32)(Frame[tj + 3] << 24);
                    // we look for the magic dword first
                    UINT16 sddp = spritedetdwordpos[qspr * MAX_SPRITE_DETECT_AREAS + tm];
                    if (mdword == spritedetdwords[qspr * MAX_SPRITE_DETECT_AREAS + tm])
                    {
                        short frax = (short)tx;
                        short fray = (short)ty;
                        short sprx = (short)(sddp % MAX_SPRITE_SIZE);
                        short spry = (short)(sddp / MAX_SPRITE_SIZE);
                        short detx = (short)spritedetareas[qspr * MAX_SPRITE_DETECT_AREAS * 4 + tm * 4];
                        short dety = (short)spritedetareas[qspr * MAX_SPRITE_DETECT_AREAS * 4 + tm * 4 + 1];
                        short detw = (short)spritedetareas[qspr * MAX_SPRITE_DETECT_AREAS * 4 + tm * 4 + 2];
                        short deth = (short)spritedetareas[qspr * MAX_SPRITE_DETECT_AREAS * 4 + tm * 4 + 3];
                        if ((frax - minxBB < sprx - detx) || (fray - minyBB < spry - dety)) continue; // if the detection area is outside the frame, continue
                        int offsx = frax - sprx + detx;
                        int offsy = fray - spry + dety;
                        if ((offsx + detw > (int)maxxBB) || (offsy + deth > (int)maxyBB)) continue;
                        // we can now check if the sprite is there
                        bool notthere = false;
                        for (UINT16 tk = 0; tk < deth; tk++)
                        {
                            for (UINT16 tl = 0; tl < detw; tl++)
                            {
                                UINT8 val = spritedescriptionso[qspr * MAX_SPRITE_SIZE * MAX_SPRITE_SIZE + (tk + dety) * MAX_SPRITE_SIZE + tl + detx];
                                if (val == 255) continue;
                                if (val != Frame[(tk + offsy) * fwidth + tl + offsx])
                                {
                                    notthere = true;
                                    break;
                                }
                            }
                            if (notthere == true) break;
                        }
                        if (!notthere)
                        {
                            pquelsprites[*nspr] = qspr;
                            if (frax - minxBB < sprx)
                            {
                                pspx[*nspr] = (UINT16)(sprx - (frax - minxBB));
                                pfrx[*nspr] = (UINT16)minxBB;
                                pwid[*nspr] = MIN((UINT16)(maxxBB - minxBB + 1), (UINT16)(MAX_SPRITE_SIZE - pspx[*nspr]));
                            }
                            else
                            {
                                pspx[*nspr] = 0;
                                pfrx[*nspr] = (UINT16)(frax - sprx);
                                pwid[*nspr] = MIN((UINT16)(maxxBB + 1 - pfrx[*nspr]), (UINT16)(MAX_SPRITE_SIZE - pfrx[*nspr]));
                            }
                            if (fray - minyBB < spry)
                            {
                                pspy[*nspr] = (UINT16)(spry - (fray - minyBB));
                                pfry[*nspr] = (UINT16)minyBB;
                                phei[*nspr] = MIN((UINT16)(maxyBB - minyBB + 1), (UINT16)(MAX_SPRITE_SIZE - pspy[*nspr]));
                            }
                            else
                            {
                                pspy[*nspr] = 0;
                                pfry[*nspr] = (UINT16)(fray - spry);
                                phei[*nspr] = MIN((UINT16)(maxyBB + 1 - pfry[*nspr]), (UINT16)(MAX_SPRITE_SIZE - pfry[*nspr]));
                            }
                            // we check the identical sprites as there may be duplicate due to the multi detection zones
                            bool identicalfound = false;
                            for (UINT8 tk = 0; tk < *nspr; tk++)
                            {
                                if ((pquelsprites[*nspr] == pquelsprites[tk]) && (pfrx[*nspr] == pfrx[tk]) && (pfry[*nspr] == pfry[tk]) &&
                                    (pwid[*nspr] == pwid[tk]) && (phei[*nspr] == phei[tk]))
                                    identicalfound = true;
                            }
                            if (!identicalfound) {
                                (*nspr)++;
                                if (*nspr == MAX_SPRITE_TO_DETECT) return true;
                            }
                        }
                    }
                }
            }
        }
        ti++;
    }
    if (*nspr > 0) return true;
    return false;
}

void Colorize_Frame(UINT8* frame, int IDfound)
{
    UINT16 tj,ti;
    // Generate the colorized version of a frame once identified in the crom frames
    for (tj = 0; tj < fheight; tj++)
    {
        for (ti = 0; ti < fwidth; ti++)
        {
            UINT16 tk = tj * fwidth + ti;

            if ((backgroundIDs[IDfound] < nbackgrounds) && (frame[tk] == 0) && (ti >= backgroundBB[IDfound * 4]) &&
                (tj >= backgroundBB[IDfound * 4 + 1]) && (ti <= backgroundBB[IDfound * 4 + 2]) && (tj <= backgroundBB[IDfound * 4 + 3]))
                frame[tk] = backgroundframes[backgroundIDs[IDfound] * fwidth * fheight + tk];
            else
            {
                UINT8 dynacouche = dynamasks[IDfound * fwidth * fheight + tk];
                if (dynacouche == 255)
                    frame[tk] = cframes[IDfound * fwidth * fheight + tk];
                else
                    frame[tk] = dyna4cols[IDfound * MAX_DYNA_4COLS_PER_FRAME * nocolors + dynacouche * nocolors + frame[tk]];
            }
        }
    }
}

void Colorize_Sprite(UINT8* frame, UINT8 nosprite, UINT16 frx, UINT16 fry, UINT16 spx, UINT16 spy, UINT16 wid, UINT16 hei)
{
    for (UINT16 tj = 0; tj < hei; tj++)
    {
        for (UINT16 ti = 0; ti < wid; ti++)
        {
            if (spritedescriptionso[(nosprite * MAX_SPRITE_SIZE + tj + spy) * MAX_SPRITE_SIZE + ti + spx] < 255)
            {
                frame[(fry + tj) * fwidth + frx + ti] = spritedescriptionsc[(nosprite * MAX_SPRITE_SIZE + tj + spy) * MAX_SPRITE_SIZE + ti + spx];
            }
        }
    }
}

void Copy_Frame_Palette(int nofr, UINT8* dpal)
{
    memcpy(dpal, &cpal[nofr * 64 * 3], 64 * 3);
}

SERUM_API void Serum_SetIgnoreUnknownFramesTimeout(UINT16 milliseconds)
{
    ignoreUnknownFramesTimeout = milliseconds;
}

SERUM_API void Serum_SetMaximumUnknownFramesToSkip(UINT8 maximum)
{
    maxFramesToSkip = maximum;
}

SERUM_API void Serum_SetStandardPalette(const UINT8* palette, const int bitDepth)
{
    int palette_length = (1 << bitDepth)*3;
    assert(palette_length < PALETTE_SIZE);

    if (palette_length <= PALETTE_SIZE) {
        memcpy(standardPalette, palette, palette_length);
        standardPaletteLength = palette_length;
    }
}

SERUM_API bool Serum_ColorizeWithMetadata(UINT8* frame, int width, int height, UINT8* palette, UINT8* rotations, UINT32* triggerID, UINT32* hashcode, int32_t* frameID)
{
    if (triggerID)
    {
        *triggerID = 0xFFFFFFFF;
    }

    *hashcode = 0xFFFFFFFF;

    if (!enabled) {
        // apply standard palette
        memcpy(palette, standardPalette, standardPaletteLength);
        return true;
    }

    // Let's first identify the incoming frame among the ones we have in the crom
    *frameID = Identify_Frame(frame);
    UINT8 nosprite[MAX_SPRITE_TO_DETECT], nspr;
    UINT16 frx[MAX_SPRITE_TO_DETECT], fry[MAX_SPRITE_TO_DETECT], spx[MAX_SPRITE_TO_DETECT], spy[MAX_SPRITE_TO_DETECT], wid[MAX_SPRITE_TO_DETECT], hei[MAX_SPRITE_TO_DETECT];
    memset(nosprite, 255, MAX_SPRITE_TO_DETECT);

    if (*frameID != IDENTIFY_NO_FRAME)
    {
        lastframe_found = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        if (maxFramesToSkip)
        {
            framesSkippedCounter = 0;
        }

        if (activeframes[lastfound] != 0 &&
            (Check_Sprites(frame, lastfound, nosprite, &nspr, frx, fry, spx, spy, wid, hei) || (*frameID >= 0)))
        {
            Colorize_Frame(frame, lastfound);
            Copy_Frame_Palette(lastfound, palette);
            UINT ti = 0;
            while (ti < nspr)
            {
                Colorize_Sprite(frame, nosprite[ti], frx[ti], fry[ti], spx[ti], spy[ti], wid[ti], hei[ti]);
                lastsprite[ti] = nosprite[ti];
                lastnsprites = nspr;
                lastfrx[ti] = frx[ti];
                lastfry[ti] = fry[ti];
                lastspx[ti] = spx[ti];
                lastspy[ti] = spy[ti];
                lastwid[ti] = wid[ti];
                lasthei[ti] = hei[ti];
                ti++;
            }
            memcpy(lastframe, frame, fwidth * fheight);
            memcpy(lastpalette, palette, 64 * 3);
            for (UINT ti = 0; ti < MAX_COLOR_ROTATIONS * 3; ti++)
            {
                lastrotations[ti] = rotations[ti] = colorrotations[lastfound * 3 * MAX_COLOR_ROTATIONS + ti];
            }
            if (triggerID && (triggerIDs[lastfound] != lasttriggerID))
            {
                lasttriggerID = *triggerID = triggerIDs[lastfound];
            }
            *hashcode = hashcodes[lastfound];
            return true; // new frame, return true
        }
    }

    UINT32 now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    if ((ignoreUnknownFramesTimeout && (now - lastframe_found) >= ignoreUnknownFramesTimeout) ||
        (maxFramesToSkip && (*frameID == IDENTIFY_NO_FRAME) && (++framesSkippedCounter >= maxFramesToSkip)))
    {
        // apply standard palette
        memcpy(palette, standardPalette, standardPaletteLength);
        // disable render features like rotations
        for (UINT ti = 0; ti < MAX_COLOR_ROTATIONS * 3; ti++)
        {
            lastrotations[ti] = rotations[ti] = 255;
        }
        for (UINT ti = 0; ti < lastnsprites; ti++)
        {
           lastsprite[ti] = nosprite[ti];
        }

        return true; // new but not colorized frame, return true
    }

    // values for the renderer
    memcpy(frame, lastframe, fwidth * fheight);
    memcpy(palette, lastpalette, PALETTE_SIZE);
    memcpy(rotations, lastrotations, 3 * MAX_COLOR_ROTATIONS);
    nspr = lastnsprites;
    for (UINT ti = 0; ti < lastnsprites; ti++)
    {
        nosprite[ti] = lastsprite[ti];
        frx[ti] = lastfrx[ti];
        fry[ti] = lastfry[ti];
        spx[ti] = lastspx[ti];
        spy[ti] = lastspy[ti];
        wid[ti] = lastwid[ti];
        hei[ti] = lasthei[ti];
    }

    return false; // no new frame, return false, client has to update rotations!
}

SERUM_API bool Serum_Colorize(UINT8* frame, int width, int height, UINT8* palette, UINT8* rotations, UINT32 *triggerID)
{
    UINT32 hashcode;
    int32_t frameID;
    return Serum_ColorizeWithMetadata(frame, width, height, palette, rotations, triggerID, &hashcode, &frameID);
}

SERUM_API bool Serum_ApplyRotations(UINT8* palette, UINT8* rotations)
{
    bool isrotation = false;
    UINT32 now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    for (int ti = 0; ti < MAX_COLOR_ROTATIONS; ti++)
    {
        if (rotations[ti * 3] == 255) continue;
        UINT32 elapsed = now - colorshiftinittime[ti];
        if (elapsed >= (long long)(rotations[ti * 3 + 2] * 10))
        {
            colorshifts[ti]++;
            colorshifts[ti] %= rotations[ti * 3 + 1];
            colorshiftinittime[ti] = now;
            isrotation = true;
            UINT8 palsave[3 * 64];
            memcpy(palsave, &palette[rotations[ti * 3] * 3], (size_t)rotations[ti * 3 + 1] * 3);
            for (int tj = 0; tj < rotations[ti * 3 + 1]; tj++)
            {
                UINT32 shift = (tj + colorshifts[ti]) % rotations[ti * 3 + 1];
                palette[(rotations[ti * 3] + tj) * 3] = palsave[shift * 3];
                palette[(rotations[ti * 3] + tj) * 3 + 1] = palsave[shift * 3 + 1];
                palette[(rotations[ti * 3] + tj) * 3 + 2] = palsave[shift * 3 + 2];
            }
        }
    }
    return isrotation;
}

SERUM_API bool Serum_ColorizeWithMetadataOrApplyRotations(UINT8* frame, int width, int height, UINT8* palette, UINT8* rotations, UINT32* triggerID, UINT32* hashcode, int32_t* frameID)
{
    bool new_frame = Serum_ColorizeWithMetadata(frame, width, height, palette, rotations, triggerID, hashcode, frameID);
    if (!new_frame) {
        return Serum_ApplyRotations(palette, rotations);
    }
    return new_frame;
}

SERUM_API bool Serum_ColorizeOrApplyRotations(UINT8* frame, int width, int height, UINT8* palette, UINT32 *triggerID)
{
    if (frame)
    {
        static UINT8 rotations[24] = { 255 };
        UINT32 hashcode;
        int frameID;
        bool new_frame = Serum_ColorizeWithMetadataOrApplyRotations(frame, width, height, palette, rotations, triggerID, &hashcode, &frameID);
        if (new_frame)
        {
            memset(rotations, 255, 24);
        }
        return new_frame;
    }
    else if (enabled)
    {
        Copy_Frame_Palette(lastfound, palette);
        return Serum_ApplyRotations(palette, lastrotations);
    }
    return false;
}

SERUM_API void Serum_DisableColorization()
{
    enabled = false;
}

SERUM_API void Serum_EnableColorization()
{
    enabled = true;
}
