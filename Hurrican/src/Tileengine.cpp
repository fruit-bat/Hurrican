// Datei : Tileengine.cpp

// --------------------------------------------------------------------------------------
//
// 2D Tile-Engine f�r Hurrican
// bestehend aus einem Vordergrund-Layer in verschiedenen Helligkeitsstufen
// und einem Overlay-Layer
//
// (c) 2002 J�rg M. Winterstein
//
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// Includes
// --------------------------------------------------------------------------------------

#include <stdio.h>
#include "Tileengine.h"
#include "DX8Texture.h"
#include "DX8Graphics.h"
#include "DX8Sprite.h"
#include "DX8Input.h"
#include "DX8Sound.h"
#include "Console.h"
#include "Gameplay.h"
#include "Gegner_Helper.h"
#include "Globals.h"
#include "HUD.h"
#include "Partikelsystem.h"
#include "Player.h"
#include "Projectiles.h"
#include "Logdatei.h"
#include "Timer.h"
#include "Main.h"

#ifdef USE_UNRARLIB
#include "unrarlib.h"
#endif

#define SKL(X) ((X) - POS_COORD_OFFSET)
#define SKO(X) ((X) - POS_COORD_OFFSET)
#define SKR(X) ((X) + TILESIZE_X - POS_COORD_OFFSET)
#define SKU(Y) ((Y) + TILESIZE_Y - POS_COORD_OFFSET)

#define TKL(X) (((X) + POS_COORD_OFFSET) * (1.0f / TILESETSIZE_X))
#define TKR(X) (((X) - POS_COORD_OFFSET) * (1.0f / TILESETSIZE_X))
#define TKO(Y) (((Y) + POS_COORD_OFFSET) * (1.0f / TILESETSIZE_Y))
#define TKU(Y) (((Y) - POS_COORD_OFFSET) * (1.0f / TILESETSIZE_Y))

// --------------------------------------------------------------------------------------
// externe Variablen
// --------------------------------------------------------------------------------------

extern Logdatei				Protokoll;
extern DirectInputClass		DirectInput;
extern TimerClass			Timer;
extern GegnerListClass		Gegner;
extern HUDClass				HUD;
extern PartikelsystemClass  PartikelSystem;
extern ProjectileListClass  Projectiles;

extern DirectGraphicsSprite	*pGegnerGrafix[MAX_GEGNERGFX];		// Grafiken  der Gegner

extern float				WackelMaximum;
extern float				WackelValue;

D3DCOLOR		Col1, Col2, Col3;								// Farben f�r Wasser/Lava etc
bool			DrawDragon;										// F�r den Drachen im Turm Level
float			ShadowAlpha;

// --------------------------------------------------------------------------------------
// Klassendefunktionen
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// TileEngine Klasse
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// Konstruktor
// --------------------------------------------------------------------------------------

TileEngineClass::TileEngineClass(void)
{
    bDrawShadow = false;

    XOffset = 0;
    YOffset = 0;

    NewXOffset = -1;
    NewYOffset = -1;

    RenderPosX = 0;
    RenderPosY = 0;
    RenderPosXTo = 0;
    RenderPosYTo = 0;

    LEVELSIZE_X = 128;
    LEVELSIZE_Y = 96;

    // Level scrollt von alleine
    ScrollSpeedX   = 0.0f;
    ScrollSpeedY   = 0.0f;

    // Speed f�r "Scrollto" Funktion
    SpeedX		   = 0.0f;
    SpeedY		   = 0.0f;
    CloudMovement  = 0.0f;
    TileAnimCount  = 0.0f;
    TileAnimPhase  = 0;
    LoadedTilesets = 0;

    //DKS - Added this:
    memset(Tiles, 0, sizeof(Tiles));

    for(int i=0; i<MAX_LEVELSIZE_X; i++)
        for(int j=0; j<MAX_LEVELSIZE_Y; j++)
        {
            //DKS - Disabled this in favor of memsetting entire array above the loops:
            //memset (&Tiles[i][j], 0, sizeof (Tiles[i][j]));

            //DKS - Was already commented out in original source:
//		 for (int k = 0; k < 3; k++)
//			 TileAt(i, j).Color[k] = 0xFFFFFFFF;

            Tiles[i][j].Red   = 255;
            Tiles[i][j].Green = 255;
            Tiles[i][j].Blue  = 255;
            Tiles[i][j].Alpha = 255;

            Tiles[i][j].move_v1 =
                Tiles[i][j].move_v2 =
                Tiles[i][j].move_v3 =
                Tiles[i][j].move_v4 = false;
        }

    for(int i=0; i<MAX_TILESETS; i++)
        //DKS - Adapted to new TexturesystemClass
        TileGfx[i].itsTexIdx = -1;

    //DKS - Moved these to the new TileEngineClass::LoadSprites() function (see note there)
    //// Wasserfall Textur laden
    //Wasserfall[0].LoadImage("wasserfall.png",  60,  240, 60,  240, 1, 1);
    //Wasserfall[1].LoadImage("wasserfall2.png", 640, 480, 640, 480, 1, 1);
    //LiquidGfx[0].LoadImage("water.png", 128, 128, 128, 128, 1, 1);
    //LiquidGfx[1].LoadImage("water2.png", 128, 128, 128, 128, 1, 1);

    // Texturkoordinaten f�r das Wasser vorberechnen
    float fx, fy;

    for (int i = 0; i < 9; i++)
    {
        fx = 128.0f / 8.0f * i / 128.0f;
        fy = 128.0f / 8.0f * i / 128.0f;

        WasserU[i] = fx;
        WasserV[i] = fy;
    }

    //DKS - Moved these to the new TileEngineClass::LoadSprites() function (see note there)
    //// GameOver Schriftzug laden
    //GameOver.LoadImage("gameover.png", 400, 90, 400, 90, 1, 1);
    //// Shatten f�r das Alien Level laden
    //Shadow.LoadImage ("shadow.png", 512, 512, 512, 512, 1, 1);

    WasserfallOffset = 0.0f;

    Zustand = ZUSTAND_SCROLLBAR;

    // Tile Ausschnitte vorberechnen
    //
    for (int i = 0; i < 144; i++)
    {
        TileRects[i].top	= (i/12) * TILESIZE_X;
        TileRects[i].left	= (i%12) * TILESIZE_Y;
        TileRects[i].right  = TileRects[i].left + TILESIZE_X;
        TileRects[i].bottom = TileRects[i].top  + TILESIZE_Y;
    }

    /* DKS - Replaced both SinList2 and WaterList lookup tables with new class
       WaterSinTableClass. See its comments in Tileengine.h for more info.    */
#if 0
    int i = 0;
    float w = 0.0f;
    while (i < 4000)
    {
        /* SinList  [i] = 0.0f; */  //DKS - Disabled this and all eliminated all uses of it, 
                                    // as it was only ever filled with zeroes and had no effect.
        //DKS - This is to ensure libm should explicitly be called here and not the lookup table:
        //SinList2 [i] = float (sin(w)) * 5.0f;
        //WaterList[i] = float (sin(w)) * 2.5f;
        SinList2 [i] = sinf(w) * 5.0f;
        WaterList[i] = sinf(w) * 2.5f;
        w += PI / TILESIZE_X;
        i++;
    }

    //SinPos   = 0.0f;      //DKS - unused; disabled
    SinPos2  = 0.0f;
    WaterPos = 0.0f;
#endif //0

    //DKS - Lightmap code in original game was never used and all related code has now been disabled:
    //// LightMaps laden
    //lightmaps[LIGHTMAP_BLITZ].Load("lightmap_blitz.bmp");
    //lightmaps[LIGHTMAP_EXPLOSION].Load("lightmap_explosion.bmp");
    //lightmaps[LIGHTMAP_GOLEMSHOT].Load("lightmap_golem.bmp");
    //lightmaps[LIGHTMAP_LILA].Load("lightmap_lila.bmp");

    pDragonHack = NULL;
}

// --------------------------------------------------------------------------------------
// Destruktor
// --------------------------------------------------------------------------------------

TileEngineClass::~TileEngineClass(void)
{
    if (pDragonHack != NULL)
        delete pDragonHack;
}

//DKS - Added initialization function that will load the sprites.
//      This was necessary since making TileEngineClass a global
//      static in Main.cpp instead of a dyanmically-allocated pointer.
//      The class constructor therefore should never load sprites by
//      itself, since graphics system should be initialized first.
//      All textures here will be automatically freed by Textures::Exit(),
//      so no need to worry about the sprite destructors being called in
//      any specific order (i.e. after graphics system has been shutdown.
void TileEngineClass::LoadSprites()
{
    // Wasserfall Textur laden
    Wasserfall[0].LoadImage("wasserfall.png",  60,  240, 60,  240, 1, 1);
    Wasserfall[1].LoadImage("wasserfall2.png", 640, 480, 640, 480, 1, 1);

    LiquidGfx[0].LoadImage("water.png", 128, 128, 128, 128, 1, 1);
    LiquidGfx[1].LoadImage("water2.png", 128, 128, 128, 128, 1, 1);

    // GameOver Schriftzug laden
    GameOver.LoadImage("gameover.png", 400, 90, 400, 90, 1, 1);

    // Shatten f�r das Alien Level laden
    Shadow.LoadImage ("shadow.png", 512, 512, 512, 512, 1, 1);
}

// --------------------------------------------------------------------------------------
// Schneiden sich zwei Linien?
// --------------------------------------------------------------------------------------

bool TileEngineClass::LineHitsLine(const Vector2D p,
                                   const Vector2D u,
                                   const Vector2D q,
                                   const Vector2D v,
                                   Vector2D &pHit)
{
    // Die Determinante D berechnen
    const float D = u.y * v.x - u.x * v.y;

    // Wenn D null ist, sind die Linien parallel.
    if(D < 0.0001f && D > -0.0001f) return false;

    // Determinante Ds berechnen
    const float Ds = (q.y - p.y) * v.x - (q.x - p.x) * v.y;

    // s = Ds / D berechnen und pr�fen, ob s in den Grenzen liegt
    const float s = Ds / D;
    if (s < 0.0f ||
            s > 1.0f)
        return false;

    // Jetzt berechnen wir Dt und t.
    const float Dt = (q.y - p.y) * u.x - (q.x - p.x) * u.y;
    const float t = Dt / D;
    if (t < 0.0f ||
            t > 1.0f)
        return false;

    // Die Linien schneiden sich!
    // Wir tragen den Schnittpunkt ein.
    pHit.x = p.x + s * u.x;
    pHit.y = p.y + s * u.y;

    return true;
}

// --------------------------------------------------------------------------------------
// Neues, leeres Level der Gr�sse xSize/ySize erstellen
// --------------------------------------------------------------------------------------

void TileEngineClass::InitNewLevel(int xSize, int ySize)
{
    LEVELSIZE_X = xSize;
    LEVELSIZE_Y = ySize;

    ScrollSpeedX = 0.0f;
    ScrollSpeedY = 0.0f;

    memset(&Tiles, 0, sizeof(Tiles));

    /* DKS - Replaced both SinList2 and WaterList lookup tables with new class
       WaterSinTableClass. See its comments in Tileengine.h for more info.    */
    WaterSinTable.ResetPosition();
#if 0
    //SinPos   = 0.0f;
    SinPos2  = 0.0f;
    WaterPos = 0.0f;
#endif //0

    DrawDragon = true;

    XOffset = 0.0f;
    YOffset = 0.0f;

    NewXOffset = -1;
    NewYOffset = -1;
}

// --------------------------------------------------------------------------------------
// Level freigeben
// --------------------------------------------------------------------------------------

void TileEngineClass::ClearLevel()
{
    // Zuerst alle Gegner-Texturen freigeben , damit
    // nur die geladen werden, die auch ben�tigt werden
    // Objekte, die immer ben�tigt werden, wie Extraleben, Diamanten etc.
    // werden nicht released
    for (int i = 4; i < MAX_GEGNERGFX; i++)
        if (i != PUNISHER &&
                pGegnerGrafix[i] != NULL)						// Ist eine Textur geladen ?
        {
            delete(pGegnerGrafix[i]);						// dann diese l�schen
            pGegnerGrafix[i] = NULL;						// und auf NULL setzen
        }

    for(int i=0; i<MAX_TILESETS; i++)
    {
        //DKS - Adapted to new TexturesystemClass
#if 0
#if defined(PLATFORM_DIRECTX)
        SafeRelease(TileGfx[i].itsTexture);
#elif defined(PLATFORM_SDL)
        if (TileGfx[i].itsTexture)
        {
            glDeleteTextures( 1, &TileGfx[i].itsTexture );
            TileGfx[i].itsTexture = 0;
        }
#endif
#endif //0
        Textures.UnloadTexture( TileGfx[i].itsTexIdx );
        TileGfx[i].itsTexIdx = -1;
    }

    Projectiles.ClearAll();
    PartikelSystem.ClearAll();

    XOffset = 0.0f;
    YOffset = 0.0f;

    //DKS - Added:
    //SoundManager.StopAllSongs(false);
    SoundManager.StopSongs();
    SoundManager.StopSounds();

    //DKS - Added unloading of Punisher.it music (it is now loaded on-demand in Gegner_Helper.cpp)
    SoundManager.UnloadSong(MUSIC_PUNISHER);
    //DKS - Added unloading of flugsack.it music (it is now loaded on-demand in Gegner_Helper.cpp)
    SoundManager.UnloadSong(MUSIC_FLUGSACK);
    //DKS - Might as well unload stage & boss music too, although LoadSong() will free any old songs for us
    SoundManager.UnloadSong(MUSIC_STAGEMUSIC);
    SoundManager.UnloadSong(MUSIC_BOSS);
}

// --------------------------------------------------------------------------------------
// Level laden
// --------------------------------------------------------------------------------------

bool TileEngineClass::LoadLevel(char Filename[100])
{
    char			Temp[256];
    FileHeader				DateiHeader;					// Header der Level-Datei
    FILE					*Datei = NULL;					// Level-Datei
    LevelObjectStruct		LoadObject;

#if defined(USE_UNRARLIB)   //DKS - Added ifdef block
    bool			fromrar = false;
    char			*pData = NULL;                          //DKS - Added NULL init val
    unsigned long	Size = 0;                               //DKS - Added init val
#endif // USE_UNRARLIB

    MustCenterPlayer = false;

    DisplayHintNr = rand()%30;

    IsElevatorLevel = false;
    FlugsackFliesFree = true;
    g_Fahrstuhl_yPos = -1.0f;

    // Zuerst checken, ob sich das Level in einem MOD-Ordner befindet
    if (CommandLineParams.RunOwnLevelList == true)
    {
        sprintf_s(Temp, "%s/levels/%s/%s", g_storage_ext, CommandLineParams.OwnLevelList, Filename);
        if (FileExists(Temp))
            goto loadfile;
    }

    //DKS - Levels have now been moved to their own subdir, data/levels/
    // Dann checken, ob sich das File im Standard Ordner befindet
    sprintf_s(Temp, "%s/data/levels/%s", g_storage_ext,  Filename);
    if (FileExists(Temp))
        goto loadfile;

#if defined(USE_UNRARLIB)
    // Auch nicht? Dann ist es hoffentlich im RAR file
    if (urarlib_get(&pData, &Size, Filename, RARFILENAME, convertText(RARFILEPASSWORD)) == false)
    {
        Protokoll.WriteText( true, "\n-> Error loading %s from Archive !\n", Filename );
        return false;
    }
    else
        fromrar = true;
#else
    Protokoll.WriteText( true, "\n-> Error loading %s!\n", Temp );
    return false;
#endif // USE_UNRARLIB

loadfile:

#if defined(USE_UNRARLIB)   //DKS - Added ifdef block
    // Aus RAR laden? Dann m�ssen wir das ganze unter tempor�rem Namen entpacken
    if (fromrar == true)
    {
        // Zwischenspeichern
        //
        FILE *TempFile = NULL;
        fopen_s(&TempFile, TEMP_FILE_PREFIX "temp.map", "wb");	// Datei �ffnen
        fwrite (pData, Size, 1, TempFile);			// speichern
        fclose (TempFile);							// und schliessen

        sprintf_s(Temp, "%s", TEMP_FILE_PREFIX "temp.map");			// Name anpassen
        free(pData);								// und Speicher freigeben
    }
#endif // USE_UNRARLIB

    Protokoll.WriteText(false, "\n-> Loading Level <-\n\n" );

    StopStageMusicAtStart = false;

    ClearLevel();

    // Drache im Hintergrund ggf. l�schen
    if (pDragonHack != NULL)
    {
        delete pDragonHack;
        pDragonHack = NULL;
    }

    // File �ffnen
    fopen_s(&Datei, Temp, "rb");

    if(!Datei)
    {
        Protokoll.WriteText( false, " \n-> Error loading level !\n" );
        return false;
    }

    // DateiHeader auslesen
    fread(&DateiHeader, sizeof(DateiHeader), 1, Datei);

    // und Werte �bertragen
    LEVELSIZE_X		 = FixEndian(DateiHeader.SizeX);
    LEVELSIZE_Y		 = FixEndian(DateiHeader.SizeY);
    LEVELPIXELSIZE_X = (float)LEVELSIZE_X*TILESIZE_X;
    LEVELPIXELSIZE_Y = (float)LEVELSIZE_Y*TILESIZE_Y;
    LoadedTilesets   = DateiHeader.UsedTilesets;
    strcpy_s(Beschreibung, DateiHeader.Beschreibung);
    bScrollBackground = DateiHeader.ScrollBackground;

    int i;

    // Benutzte Tilesets laden
    for (i=0; i<LoadedTilesets; i++)
        TileGfx[i].LoadImage(DateiHeader.SetNames[i], 256, 256, TILESIZE_X, TILESIZE_Y, 12, 12);

    // Benutzte Hintergrundgrafiken laden

    Background.LoadImage(DateiHeader.BackgroundFile, 640, 480, 640, 480, 1, 1);
    ParallaxLayer[0].LoadImage(DateiHeader.ParallaxAFile, 640, 480, 640, 480, 1, 1);
    ParallaxLayer[1].LoadImage(DateiHeader.ParallaxBFile, 640, 480, 640, 480, 1, 1);
    CloudLayer.LoadImage(DateiHeader.CloudFile, 640, 240, 640, 240, 1, 1);
    Timelimit = (double) FixEndian(DateiHeader.Timelimit);
    DateiHeader.NumObjects = FixEndian(DateiHeader.NumObjects);

    if (Timelimit <= 0)
        Timelimit = 500;

    Player[0].PunisherActive = false;
    Player[1].PunisherActive = false;

    Player[1].xpos = 0.0f;
    Player[1].ypos = 0.0f;

    TimelimitSave = Timelimit;
    MaxSecrets  = 0;
    MaxDiamonds = 0;
    MaxOneUps	= 0;
    MaxBlocks   = 0;

    // LevelDaten laden
    InitNewLevel(LEVELSIZE_X, LEVELSIZE_Y);

    LevelTileLoadStruct LoadTile;

    for(i=0; i<LEVELSIZE_X; i++)
        for(int j=0; j<LEVELSIZE_Y; j++)
        {
            fread(&LoadTile, sizeof(LoadTile), 1, Datei);

            if(LoadTile.TileSetBack  > LoadedTilesets) LoadTile.TileSetBack  = LoadedTilesets;
            if(LoadTile.TileSetFront > LoadedTilesets) LoadTile.TileSetFront = LoadedTilesets;

            // Geladenes Leveltile �bernehmen
            //
            TileAt(i, j).Alpha			= LoadTile.Alpha;
            TileAt(i, j).BackArt		= LoadTile.BackArt;
            TileAt(i, j).Block			= FixEndian(LoadTile.Block);
            TileAt(i, j).Blue			= LoadTile.Blue;
            TileAt(i, j).FrontArt		= LoadTile.FrontArt;
            TileAt(i, j).Green			= LoadTile.Green;
            TileAt(i, j).Red			= LoadTile.Red;
            TileAt(i, j).TileSetBack	= LoadTile.TileSetBack;
            TileAt(i, j).TileSetFront	= LoadTile.TileSetFront;

            TileAt(i, j).Color [0] = D3DCOLOR_RGBA(LoadTile.Red, LoadTile.Green, LoadTile.Blue, LoadTile.Alpha);
            TileAt(i, j).Color [1] = D3DCOLOR_RGBA(LoadTile.Red, LoadTile.Green, LoadTile.Blue, LoadTile.Alpha);
            TileAt(i, j).Color [2] = D3DCOLOR_RGBA(LoadTile.Red, LoadTile.Green, LoadTile.Blue, LoadTile.Alpha);
            TileAt(i, j).Color [3] = D3DCOLOR_RGBA(LoadTile.Red, LoadTile.Green, LoadTile.Blue, LoadTile.Alpha);

            // Eine Fl�ssigkeit als Block?
            // damit man nicht immer auf alle vier m�glichen Fl�ssigkeiten checken muss,
            // sondern nur auf BLOCKWERT_LIQUID

            if (TileAt(i, j).Block & BLOCKWERT_WASSER ||
                    TileAt(i, j).Block & BLOCKWERT_SUMPF)
                TileAt(i, j).Block ^= BLOCKWERT_LIQUID;
        }

    // eventuelle Schr�gen ermitteln und Ecken f�r die Wasseranim festlegen
    // EDIT_ME wieder reinmachen und diesmal richtig machen =)
    uint32_t bl, br, bo, bu;

    for(i=1; i<LEVELSIZE_X-1; i++)
        for(int j=2; j<LEVELSIZE_Y-1; j++)
        {
            // Schr�ge links hoch
            if (  TileAt(i+0, j+0).Block & BLOCKWERT_WAND  &&
                    !(TileAt(i+1, j+0).Block & BLOCKWERT_WAND) &&
                    TileAt(i+1, j+1).Block & BLOCKWERT_WAND  &&
                    !(TileAt(i+0, j-1).Block & BLOCKWERT_WAND))
            {
                if (!(TileAt(i+1, j+0).Block & BLOCKWERT_SCHRAEGE_L))
                    TileAt(i+1, j+0).Block ^= BLOCKWERT_SCHRAEGE_L;
            }

            // Schr�ge rechts hoch
            if (  TileAt(i+0, j+0).Block & BLOCKWERT_WAND  &&
                    !(TileAt(i-1, j+0).Block & BLOCKWERT_WAND) &&
                    TileAt(i-1, j+1).Block & BLOCKWERT_WAND &&
                    !(TileAt(i+0, j-1).Block & BLOCKWERT_WAND))
            {
                if (!(TileAt(i-1, j+0).Block & BLOCKWERT_SCHRAEGE_R))
                    TileAt(i-1, j+0).Block ^= BLOCKWERT_SCHRAEGE_R;
            }

            // Wasseranim
            //
            bl = TileAt(i - 1, j + 0).Block;
            br = TileAt(i + 1, j + 0).Block;
            bo = TileAt(i + 0, j - 1).Block;
            bu = TileAt(i + 0, j + 1).Block;

            if (!(TileAt(i - 1, j - 1).Block & BLOCKWERT_WAND)       &&
                    !(TileAt(i, j - 1).Block & BLOCKWERT_WASSERFALL) &&
                    !(TileAt(i - 1, j - 1).Block & BLOCKWERT_WASSERFALL) &&
                    (bl & BLOCKWERT_LIQUID &&
                     (!(bo & BLOCKWERT_WAND))))
                TileAt(i, j).move_v1 = true;
            else
                TileAt(i, j).move_v1 = false;

            if (!(TileAt(i - 1, j + 1).Block & BLOCKWERT_WAND) &&
                    (bl & BLOCKWERT_LIQUID &&
                     bu & BLOCKWERT_LIQUID))
                TileAt(i, j).move_v3 = true;
            else
                TileAt(i, j).move_v3 = false;

            if (!(TileAt(i + 1, j - 1).Block & BLOCKWERT_WAND)       &&
                    !(TileAt(i, j - 1).Block & BLOCKWERT_WASSERFALL) &&
                    !(TileAt(i + 1, j - 1).Block & BLOCKWERT_WASSERFALL) &&
                    (br & BLOCKWERT_LIQUID &&
                     (!(bo & BLOCKWERT_WAND))))
                TileAt(i, j).move_v2 = true;
            else
                TileAt(i, j).move_v2 = false;

            if (!(TileAt(i + 1, j + 1).Block & BLOCKWERT_WAND) &&
                    (br & BLOCKWERT_LIQUID &&
                     bu & BLOCKWERT_LIQUID))
                TileAt(i, j).move_v4 = true;
            else
                TileAt(i, j).move_v4 = false;
        }


    // Objekt Daten laden und gleich Liste mit Objekten erstellen
    if (DateiHeader.NumObjects > 0)
        for(i=0; i < (int)(DateiHeader.NumObjects); i++)
        {
            fread(&LoadObject, sizeof(LoadObject), 1, Datei);				// Objekt laden

            LoadObject.ObjectID = FixEndian(LoadObject.ObjectID);
            LoadObject.XPos     = FixEndian(LoadObject.XPos);
            LoadObject.YPos     = FixEndian(LoadObject.YPos);
            LoadObject.Value1   = FixEndian(LoadObject.Value1);
            LoadObject.Value2   = FixEndian(LoadObject.Value2);

            // Startposition des Spielers
            if(LoadObject.ObjectID == 0)
            {
                if (LoadObject.Value1 == 0)									// Anf�ngliche
                    Player[LoadObject.Value2].Blickrichtung = RECHTS;		// Blickrichtung auch
                else														// aus Leveldatei
                    Player[LoadObject.Value2].Blickrichtung = LINKS;		// lesen

                Player[LoadObject.Value2].xpos = (float)(LoadObject.XPos);
                Player[LoadObject.Value2].ypos = (float)(LoadObject.YPos);
                Player[LoadObject.Value2].xposold = Player[LoadObject.Value2].xpos;
                Player[LoadObject.Value2].yposold = Player[LoadObject.Value2].ypos;
                Player[LoadObject.Value2].JumpxSave = Player[LoadObject.Value2].xpos;
                Player[LoadObject.Value2].JumpySave = Player[LoadObject.Value2].ypos;
                Player[LoadObject.Value2].CenterLevel();
                UpdateLevel ();
            }

            // Anzahl Secrets, OneUps usw z�hlen, f�r Summary-Box

            // Secret
            if (LoadObject.ObjectID == SECRET)
                MaxSecrets++;

            // Diamant
            if (LoadObject.ObjectID == DIAMANT)
                MaxDiamonds++;

            // OneUp
            if (LoadObject.ObjectID == ONEUP)
                MaxOneUps++;

            // Powerblock
            if (LoadObject.ObjectID == POWERBLOCK)
                MaxBlocks++;

            // Gegner und andere Objekte laden und ins Level setzen
            if(LoadObject.ObjectID > 0)
            {
                // Spitter laden, wenn die Spitterbombe geladen wird
                if (LoadObject.ObjectID  == SPITTERBOMBE &&
                        pGegnerGrafix[SPITTER] == NULL)
                    LoadGegnerGrafik(SPITTER);

                // Kleinen Piranha laden, wenn der Riesen Piranha geladen wird
                if (LoadObject.ObjectID  == RIESENPIRANHA &&
                        pGegnerGrafix[PIRANHA] == NULL)
                    LoadGegnerGrafik(PIRANHA);

                // M�cke laden, wenn das Nest geladen wird
                if (LoadObject.ObjectID  == NEST &&
                        pGegnerGrafix[STAHLMUECKE] == NULL)
                    LoadGegnerGrafik(STAHLMUECKE);

                // Spinnenbombe laden, wenn die Riesenspinne geladen wird
                if (LoadObject.ObjectID  == RIESENSPINNE &&
                        pGegnerGrafix[SPIDERBOMB] == NULL)
                    LoadGegnerGrafik(SPIDERBOMB);

                // Kleine Kugeln laden, wenn eine grosse geladen wird
                if (LoadObject.ObjectID  == KUGELRIESIG ||
                        LoadObject.ObjectID  == KUGELGROSS  ||
                        LoadObject.ObjectID  == KUGELMEDIUM)
                {
                    if (pGegnerGrafix[KUGELGROSS] == NULL)
                        LoadGegnerGrafik(KUGELGROSS);

                    if (pGegnerGrafix[KUGELMEDIUM] == NULL)
                        LoadGegnerGrafik(KUGELMEDIUM);

                    if (pGegnerGrafix[KUGELKLEIN] == NULL)
                        LoadGegnerGrafik(KUGELKLEIN);
                }

                // Boulder und Stelzsack laden, wenn der Fahrstuhlendboss geladen wird
                if (LoadObject.ObjectID  == FAHRSTUHLBOSS)
                {
                    if (pGegnerGrafix[BOULDER]   == NULL) LoadGegnerGrafik(BOULDER);
                    if (pGegnerGrafix[STELZSACK] == NULL) LoadGegnerGrafik(STELZSACK);
                }

                // lava Ball laden, wenn dessen Spawner geladen wird
                if (LoadObject.ObjectID  == LAVABALLSPAWNER)
                    if (pGegnerGrafix[LAVABALL]   == NULL) LoadGegnerGrafik(LAVABALL);

                // Made laden, wenn der Bratklops oder der Partikelspawner geladen wird
                if ((LoadObject.ObjectID == BRATKLOPS ||
                        LoadObject.ObjectID == PARTIKELSPAWN) &&
                        pGegnerGrafix[MADE] == NULL)
                    LoadGegnerGrafik(MADE);

                // Steine laden, wenn der Schrein geladen wird
                if (LoadObject.ObjectID == SHRINE &&
                        pGegnerGrafix[FALLINGROCK] == NULL)
                    LoadGegnerGrafik(FALLINGROCK);

                // ShootButton laden, wenn die entsprechende Plattform geladen wird
                if (LoadObject.ObjectID  == SHOOTPLATTFORM &&
                        pGegnerGrafix[SHOOTBUTTON] == NULL)
                    LoadGegnerGrafik(SHOOTBUTTON);

                // Made laden, wenn der Schwabbelsack geladen wird
                if (LoadObject.ObjectID == SCHWABBEL &&
                        pGegnerGrafix[MADE] == NULL)
                    LoadGegnerGrafik(MADE);

                // Boulder laden, wenn der MetalHead Boss geladen wird
                if (LoadObject.ObjectID    == METALHEAD &&
                        pGegnerGrafix[BOULDER] == NULL)
                    LoadGegnerGrafik(BOULDER);

                // Schleimbollen laden, wenn das Schleimmaul geladen wird
                if (LoadObject.ObjectID		    == SCHLEIMMAUL &&
                        pGegnerGrafix[SCHLEIMALIEN] == NULL)
                    LoadGegnerGrafik(SCHLEIMALIEN);

                // Mittelgro�e Spinne laden, wenn der Spinnen Ansturm geladen wird
                if (LoadObject.ObjectID		    == WUXESPINNEN &&
                        pGegnerGrafix[MITTELSPINNE] == NULL)
                    LoadGegnerGrafik(MITTELSPINNE);

                // Blauen Boulder laden, wenn der Golem geladen wird
                if (LoadObject.ObjectID		    == GOLEM &&
                        pGegnerGrafix[BOULDER] == NULL)
                    LoadGegnerGrafik(BOULDER);

                // Climbspider laden, wenn die Spinnenmaschine geladen wird
                if (LoadObject.ObjectID == SPINNENMASCHINE &&
                        pGegnerGrafix[CLIMBSPIDER] == NULL)
                    LoadGegnerGrafik(CLIMBSPIDER);

                // Drone laden, wenn die Spinnenmaschine geladen wird
                if (LoadObject.ObjectID == SPINNENMASCHINE &&
                        pGegnerGrafix[DRONE] == NULL)
                    LoadGegnerGrafik(DRONE);

                // Fette Spinne laden, wenn die Spinnen Presswurst geladen wird
                if (LoadObject.ObjectID == PRESSWURST &&
                        pGegnerGrafix[FETTESPINNE] == NULL)
                    LoadGegnerGrafik(FETTESPINNE);

                // La Fass laden, wenn der La Fass Spawner geladen wird
                if (LoadObject.ObjectID == LAFASSSPAWNER &&
                        pGegnerGrafix[LAFASS] == NULL)
                    LoadGegnerGrafik(LAFASS);

                // Minirakete laden, wenn Stachelbeere geladen wird
                if (LoadObject.ObjectID == STACHELBEERE &&
                        pGegnerGrafix[MINIROCKET] == NULL)
                {
                    LoadGegnerGrafik(MINIROCKET);
                }

                // Fette Rakete laden, wenn Riesenspinne oder Drache geladen wird
                if ((LoadObject.ObjectID == RIESENSPINNE ||
                        LoadObject.ObjectID == UFO ||
                        LoadObject.ObjectID == SKELETOR ||
                        LoadObject.ObjectID == DRACHE) &&
                        pGegnerGrafix[FETTERAKETE] == NULL)
                    LoadGegnerGrafik(FETTERAKETE);

                // Minidragon laden, wenn Drache geladen wird
                if (LoadObject.ObjectID == DRACHE &&
                        pGegnerGrafix[MINIDRAGON] == NULL)
                    LoadGegnerGrafik(MINIDRAGON);

                // Schneekoppe laden, wenn der Schneek�nig geladen wird
                if (LoadObject.ObjectID == SCHNEEKOENIG &&
                        pGegnerGrafix[SCHNEEKOPPE] == NULL)
                    LoadGegnerGrafik(SCHNEEKOPPE);

                // Ein Paar Gegner laden, wenn der BigFish geladen wird
                if (LoadObject.ObjectID == BIGFISH)
                {
                    if (pGegnerGrafix[PIRANHA] == NULL)
                        LoadGegnerGrafik(PIRANHA);

                    if (pGegnerGrafix[SWIMWALKER] == NULL)
                        LoadGegnerGrafik(SWIMWALKER);

                    if (pGegnerGrafix[KUGELKLEIN] == NULL)
                        LoadGegnerGrafik(KUGELKLEIN);

                    if (pGegnerGrafix[KUGELMEDIUM] == NULL)
                        LoadGegnerGrafik(KUGELMEDIUM);

                    if (pGegnerGrafix[KUGELGROSS] == NULL)
                        LoadGegnerGrafik(KUGELGROSS);

                    if (pGegnerGrafix[KUGELRIESIG] == NULL)
                        LoadGegnerGrafik(KUGELRIESIG);
                }

                // Kettenglied laden, wenn der Rollmops geladen wird
                if (LoadObject.ObjectID == ROLLMOPS &&
                        pGegnerGrafix[KETTENGLIED] == NULL)
                    LoadGegnerGrafik(KETTENGLIED);

                // Mutant laden, wenn die Tube geladen wird
                if (LoadObject.ObjectID == TUBE &&
                        pGegnerGrafix[MUTANT] == NULL)
                    LoadGegnerGrafik(MUTANT);

                // Skull laden, wenn der Skeletor geladen wird
                if (LoadObject.ObjectID == SKELETOR &&
                        pGegnerGrafix[SKULL] == NULL)
                    LoadGegnerGrafik(SKULL);

                // Drache wird geladen?
                if (LoadObject.ObjectID == DRACHE)
                {
                    if (LoadObject.Value2 == 0)
                        pDragonHack = new CDragonHack();
                }

                // Gegner laden, wenn er nicht schon geladen wurde
                if (pGegnerGrafix[LoadObject.ObjectID] == NULL)
                    LoadGegnerGrafik(LoadObject.ObjectID);

                // Gegner bei aktuellem Skill level �berhaupt erzeugen ?
                if (LoadObject.Skill <= Skill)
                {
                    Gegner.PushGegner(float(LoadObject.XPos),
                                        float(LoadObject.YPos),
                                        LoadObject.ObjectID,
                                        LoadObject.Value1,
                                        LoadObject.Value2,
                                        LoadObject.ChangeLight);

                    if (LoadObject.ObjectID == REITFLUGSACK &&
                            NUMPLAYERS == 2)
                        Gegner.PushGegner(float(LoadObject.XPos) + 60,
                                            float(LoadObject.YPos) + 40,
                                            LoadObject.ObjectID,
                                            LoadObject.Value1,
                                            LoadObject.Value2,
                                            LoadObject.ChangeLight);

                }
            }
        }

    // Kein Startpunkt f�r Spieler 2 gefunden? Dann von Spieler 1 kopieren
    if (Player[1].xpos == 0.0f &&
            Player[1].ypos == 0.0f)
    {
        Player[1].Blickrichtung = Player[0].Blickrichtung;
        Player[1].xpos = Player[0].xpos + 40.0f;
        Player[1].ypos = Player[0].ypos;
        Player[1].xposold = Player[1].xpos;
        Player[1].yposold = Player[1].ypos;
        Player[1].JumpxSave = Player[1].xpos;
        Player[1].JumpySave = Player[1].ypos;
    }

    fread(&DateiAppendix, sizeof(DateiAppendix), 1, Datei);

    DateiAppendix.UsedPowerblock = FixEndian(DateiAppendix.UsedPowerblock);

    bDrawShadow = DateiAppendix.Taschenlampe;
    ShadowAlpha = 255.0f;

    // Datei schliessen
    fclose(Datei);

    // Temp Datei l�schen und speicher freigeben
    DeleteFile(TEMP_FILE_PREFIX "temp.map");

    // Liquid Farben setzen
    ColR1 = GetDecValue(&DateiAppendix.Col1[0], 2);
    ColG1 = GetDecValue(&DateiAppendix.Col1[2], 2);
    ColB1 = GetDecValue(&DateiAppendix.Col1[4], 2);

    ColR2 = GetDecValue(&DateiAppendix.Col2[0], 2);
    ColG2 = GetDecValue(&DateiAppendix.Col2[2], 2);
    ColB2 = GetDecValue(&DateiAppendix.Col2[4], 2);

    Col1 = D3DCOLOR_RGBA(ColR1, ColG1, ColB1,
                         GetDecValue(&DateiAppendix.Col1[6], 2));

    Col2 = D3DCOLOR_RGBA(ColR2, ColG2, ColB2,
                         GetDecValue(&DateiAppendix.Col2[6], 2));

    ColR3 = ColR1 + ColR2 + 32;
    if (ColR3 > 255) ColR3 = 255;
    ColG3 = ColG1 + ColG2 + 32;
    if (ColG3 > 255) ColG3 = 255;
    ColB3 = ColB1 + ColB2 + 32;
    if (ColB3 > 255) ColB3 = 255;

    ColA3 = GetDecValue(&DateiAppendix.Col1[6], 2) +
            GetDecValue(&DateiAppendix.Col2[6], 2);
    if (ColA3 > 255) ColA3 = 255;

    Col3 = D3DCOLOR_RGBA(ColR3, ColG3, ColB3, ColA3);

    ComputeCoolLight();

    // Level korrekt geladen
    strcpy_s(Temp, "-> Load Level : ");
    strcat_s(Temp, Filename);
    strcat_s(Temp, " successful ! <-\n\n");
    Protokoll.WriteText( false, Temp );

    Zustand = ZUSTAND_SCROLLBAR;

    return true;
}

// --------------------------------------------------------------------------------------
// Neue Scrollspeed setzen
// --------------------------------------------------------------------------------------

void TileEngineClass::SetScrollSpeed(float xSpeed, float ySpeed)
{
    ScrollSpeedX = xSpeed;
    ScrollSpeedY = ySpeed;
}

// --------------------------------------------------------------------------------------
// Ausrechnen, welcher Levelausschnitt gerendert werden soll
// --------------------------------------------------------------------------------------

void TileEngineClass::CalcRenderRange(void)
{
    long xo, yo;

    // Ausschnittgr��e berechnen
    //
    xo = (long)(XOffset * 0.05f);
    yo = (long)(YOffset * 0.05f);

    /* CHECKME: Without the following checks the game
       in places like start of level 2 for a short
       time renders areas outside map which can cause
       crashes (random non existing textures). (stegerg) */

    if (xo < 0) xo = 0;
    if (yo < 0) yo = 0;

    RenderPosX = - 1;
    RenderPosY = - 1;

    if (RenderPosX + xo < 0)
        RenderPosX = 0;

    if (RenderPosY + yo < 0)
        RenderPosY = 0;

    RenderPosXTo = SCREENSIZE_X + 2;
    RenderPosYTo = SCREENSIZE_Y + 2;

    if (xo + RenderPosXTo > LEVELSIZE_X) RenderPosXTo = SCREENSIZE_X;
    if (yo + RenderPosYTo > LEVELSIZE_Y) RenderPosYTo = SCREENSIZE_Y;

    //DKS - Added:
    while (xo + RenderPosXTo > LEVELSIZE_X)
        --RenderPosXTo;
    while (yo + RenderPosYTo > LEVELSIZE_Y)
        --RenderPosYTo;

    // Sonstige Ausgangswerte berechnen
    xLevel = (int)xo;
    yLevel = (int)yo;

    // Offsets der Tiles berechnen (0-19)
    xTileOffs = (int)(XOffset) % TILESIZE_X;
    yTileOffs = (int)(YOffset) % TILESIZE_Y;

    // DKS - Update the new water sin table indexes before rendering any tiles:
    WaterSinTable.UpdateTableIndexes(xLevel, yLevel);
}

// --------------------------------------------------------------------------------------
// Hintergrund Parallax Layer anzeigen
// --------------------------------------------------------------------------------------

void TileEngineClass::DrawBackground(void)
{
    int		xoff;
    float	yoff;

    // Hintergrund nicht rotieren
    //
    D3DXMATRIX matView;
    D3DXMatrixIdentity	 (&matView);
#if defined(PLATFORM_DIRECTX)
    lpD3DDevice->SetTransform(D3DTS_VIEW, &matView);
#elif defined(PLATFORM_SDL)
    g_matView = matView;
#endif

//----- Hintergrund-Bild

    xoff = (int)(XOffset / 5) % 640;

    if (bScrollBackground == true)				// Hintergrundbild mitscrollen
    {
        // Linke H�lfte
        Background.SetRect(0, 0, xoff, 480);
        Background.RenderSprite(640-(float)(xoff), 0, 0xFFFFFFFF);

        // Rechte H�lfte
        Background.SetRect(xoff, 0, 640, 480);
        Background.RenderSprite(0, 0, 0xFFFFFFFF);
    }
    else										// oder statisch ?
    {
        Background.SetRect(0, 0, 640, 480);
        Background.RenderSprite(0, 0, 0xFFFFFFFF);
    }

//----- Layer ganz hinten (ausser im Flugsack Level)

    xoff = (int)(XOffset / 3) % 640;
    yoff = (float)((LEVELSIZE_Y-SCREENSIZE_Y)*TILESIZE_Y);	// Gr�sse des Levels in Pixeln (-1 Screen)
    yoff = (float)(220-150/yoff*YOffset);			// y-Offset des Layers berechnen
    yoff -= 40;

    // Linke H�lfte
    ParallaxLayer[0].SetRect(0, 0, xoff, 480);
    ParallaxLayer[0].RenderSprite(640-(float)(xoff), yoff, 0xFFFFFFFF);

    // Rechte H�lfte
    ParallaxLayer[0].SetRect(xoff, 0, 640, 480);
    ParallaxLayer[0].RenderSprite(0, yoff, 0xFFFFFFFF);


//----- vorletzter Layer

    yoff = (float)((LEVELSIZE_Y-SCREENSIZE_Y)*TILESIZE_Y);	// Gr�sse des Levels in Pixeln (-1 Screen)
    yoff = (float)(200-200/yoff*YOffset);			// y-Offset des Layers berechnen
    xoff = (int)(XOffset / 2) % 640;

    // Linke H�lfte
    ParallaxLayer[1].SetRect(0, 0, xoff, 480);
    ParallaxLayer[1].RenderSprite(640-(float)(xoff), yoff, 0xFFFFFFFF);

    // Rechte H�lfte
    ParallaxLayer[1].SetRect(xoff, 0, 640, 480);
    ParallaxLayer[1].RenderSprite(0, yoff, 0xFFFFFFFF);

//----- Im Fahrstuhl-Level noch den vertikalen Parallax-Layer anzeigen

    if (IsElevatorLevel)
    {
        yoff = float ((int)(YOffset / 1.5f) % 480);

        // Obere H�lfte
        ParallaxLayer[2].SetRect(0, 0, 640, int (yoff));
        ParallaxLayer[2].RenderSprite(float (390 - XOffset), 480-(float)(yoff), 0xFFFFFFFF);

        // Untere H�lfte
        ParallaxLayer[2].SetRect(0, int (yoff), 640, 480);
        ParallaxLayer[2].RenderSprite(float (390 - XOffset), 0, 0xFFFFFFFF);
    }

//----- Wolken Layer (Wenn Focus des Level GANZ oben, dann wird er GANZ angezeigt)

    // Wolken bewegen
    CloudMovement += SpeedFaktor;
    if (CloudMovement > 640.0f)
        CloudMovement = 0.0f;

    DirectGraphics.SetAdditiveMode();

    xoff = int(XOffset / 4 + CloudMovement) % 640;
    yoff = float((LEVELSIZE_Y-SCREENSIZE_Y)*40);	// Gr�sse des Levels in Pixeln (-1 Screen)
    yoff = float(240/yoff*YOffset);					// y-Offset des Layers berechnen

    // Linke H�lfte
    CloudLayer.SetRect(0, int(yoff), int(xoff), 240);
    CloudLayer.RenderSprite(640-(float)(xoff), 0, 0xFFFFFFFF);

    // Rechte H�lfte
    CloudLayer.SetRect(int(xoff), int(yoff), 640, 240);
    CloudLayer.RenderSprite(0, 0, 0xFFFFFFFF);

    DirectGraphics.SetColorKeyMode();

//----- Dragon Hack

    if (pDragonHack != NULL &&
            Console.Showing == false)
        pDragonHack->Run();
}

// --------------------------------------------------------------------------------------
//
// Spezial Layer
// zb der Drache im Turm Level
// --------------------------------------------------------------------------------------

//DKS - this was an empty function in the original code, disabling it:
//void TileEngineClass::DrawSpecialLayer(void)
//{
//
//}

// --------------------------------------------------------------------------------------
// Level Hintergrund anzeigen
//
// Das Level wird Tile f�r Tile durchgegangen das Vertex-Array solange mit
// Vertices gef�llt, die das selbe Tileset verwenden, bis ein anderes Tileset
// gesetzt werden muss, worauf alle Vetices im Array mit der alten Textur gerendert
// werden und alles mit der neuen Textur von vorne wieder losgeht, bis alle Tiles
// durchgenommen wurden.

// So funzt das auch bei FrontLevel und (vielleicht bald) Overlay
//
// Hier werden alle Tiles im Back-Layer gesetzt, die KEINE Wand sind,
// da die W�nde sp�ter gesetzt werden, da sie alles verdecken, was in sie reinragt
// --------------------------------------------------------------------------------------

void TileEngineClass::DrawBackLevel(void)
{
    RECT			Rect;						// Texturausschnitt im Tileset
    int				NumToRender;				// Wieviele Vertices zu rendern ?
    int				ActualTexture;				// Aktuelle Textur
    float			l,  r,  o,  u;				// Vertice Koordinaten
    float			tl, tr, to, tu;				// Textur Koordinaten
    unsigned int	Type;						// TileNR des Tiles

    // Am Anfang noch keine Textur gew�hlt
    ActualTexture = -1;

    // x und ypos am screen errechnen
    xScreen = (float)(-xTileOffs) + RenderPosX * TILESIZE_X;
    yScreen = (float)(-yTileOffs) + RenderPosY * TILESIZE_Y;

    DirectGraphics.SetColorKeyMode();

    // Noch keine Tiles zum rendern
    NumToRender = 0;

    //DKS - WaterList lookup table has been replaced with WaterSinTableClass,
    //      see comments for it in Tileengine.h
    //int off  = 0;

    for(int j = RenderPosY; j<RenderPosYTo; j++)
    {
        xScreen = (float)(-xTileOffs) + RenderPosX * TILESIZE_X;

        for(int i = RenderPosX; i<RenderPosXTo; i++)
        {
            if   (TileAt(xLevel+i, yLevel+j).BackArt > 0 &&			// �berhaupt ein Tile drin ?
                    (!(TileAt(xLevel+i, yLevel+j).Block & BLOCKWERT_WAND) ||
                     (TileAt(xLevel+i, yLevel+j).FrontArt > 0 &&
                      TileAt(xLevel+i, yLevel+j).Block & BLOCKWERT_VERDECKEN)))
            {
                // Neue Textur ?
                if (TileAt(xLevel+i, yLevel+j).TileSetBack != ActualTexture)
                {
                    // Aktuelle Textur sichern
                    ActualTexture = TileAt(xLevel+i, yLevel+j).TileSetBack;

                    // Tiles zeichnen
                    if (NumToRender > 0)
                        DirectGraphics.RendertoBuffer (D3DPT_TRIANGLELIST, NumToRender*2, &TilesToRender[0]);

                    // Neue aktuelle Textur setzen
                    DirectGraphics.SetTexture( TileGfx[ActualTexture].itsTexIdx );

                    // Und beim rendern wieder von vorne anfangen
                    NumToRender = 0;
                }

                Type = TileAt(xLevel+i, yLevel+j).BackArt - INCLUDE_ZEROTILE;

                // Animiertes Tile ?
                if (TileAt(xLevel+i, yLevel+j).Block & BLOCKWERT_ANIMIERT_BACK)
                    Type += 36*TileAnimPhase;

                // richtigen Ausschnitt f�r das aktuelle Tile setzen
                Rect = TileRects[Type];

                // Screen-Koordinaten der Vertices
                l = SKL(xScreen);						// Links
                o = SKO(yScreen);						// Oben
                r = SKR(xScreen);                		// Rechts
                u = SKU(yScreen);	               		// Unten

                // Textur-Koordinaten
                tl = TKL(Rect.left);			    	// Links
                tr = TKR(Rect.right);		    		// Rechts
                to = TKO(Rect.top);	        			// Oben
                tu = TKU(Rect.bottom);		    		// Unten

                // Vertices definieren
                v1.color = TileAt(xLevel+i, yLevel+j).Color [0];
                v2.color = TileAt(xLevel+i, yLevel+j).Color [1];
                v3.color = TileAt(xLevel+i, yLevel+j).Color [2];
                v4.color = TileAt(xLevel+i, yLevel+j).Color [3];

                v1.x		= l;				            		// Links oben
                v1.y		= o;
                v1.tu		= tl;
                v1.tv		= to;

                v2.x		= r;		            				// Rechts oben
                v2.y		= o;
                v2.tu		= tr;
                v2.tv		= to;

                v3.x		= l;			                		// Links unten
                v3.y		= u;
                v3.tu		= tl;
                v3.tv		= tu;

                v4.x		= r;                					// Rechts unten
                v4.y		= u;
                v4.tu		= tr;
                v4.tv		= tu;

                // Hintergrund des Wasser schwabbeln lassen

                //DKS - WaterList lookup table has been replaced with WaterSinTableClass,
                //      see comments for it in Tileengine.h
#if 0
                off = (int(SinPos2) + (yLevel * 2) % 40 + j*2) % 1024;

                //DKS - Fixed out of bounds access to Tiles[][] array on y here:
                //      When yLevel is 1 and j is -1 (indicating that the one-tile overdraw
                //      border is being drawn on the top screen edge), this was ending up
                //      trying to access a row higher than the top screen border which doesn't
                //      exist. Loading the Eis map (level 7) would crash on some machines.
                //if (TileAt(xLevel+i, yLevel+j-1).Block & BLOCKWERT_LIQUID)                    // Original line
                if ( yLevel+j > 0 &&    // DKS Added this check to above line
                        TileAt(xLevel+i, yLevel+j-1).Block & BLOCKWERT_LIQUID)
                {
                    if (TileAt(xLevel+i, yLevel+j).move_v1 == true) v1.x += SinList2[off];
                    if (TileAt(xLevel+i, yLevel+j).move_v2 == true) v2.x += SinList2[off];
                }

                if (TileAt(xLevel+i, yLevel+j).move_v3 == true) v3.x += SinList2[off + 2];
                if (TileAt(xLevel+i, yLevel+j).move_v4 == true) v4.x += SinList2[off + 2];
#endif //0

                if (TileAt(xLevel+i, yLevel+j).move_v1 ||
                    TileAt(xLevel+i, yLevel+j).move_v2 ||
                    TileAt(xLevel+i, yLevel+j).move_v3 ||
                    TileAt(xLevel+i, yLevel+j).move_v4)
                {
                    float x_offs[2];
                    WaterSinTable.GetNonWaterSin(j, x_offs);

                    //DKS - Fixed out of bounds access to Tiles[][] array on y here:
                    //      When yLevel is 1 and j is -1 (indicating that the one-tile overdraw
                    //      border is being drawn on the top screen edge), this was ending up
                    //      trying to access a row higher than the top screen border which doesn't
                    //      exist. Loading the Eis map (level 7) would crash on some machines.
                    //if (TileAt(xLevel+i, yLevel+j-1).Block & BLOCKWERT_LIQUID)                    // Original line
                    if ( yLevel+j > 0 &&    // DKS Added this check to above line
                            TileAt(xLevel+i, yLevel+j-1).Block & BLOCKWERT_LIQUID)
                    {
                        if (TileAt(xLevel+i, yLevel+j).move_v1 == true) v1.x += x_offs[0];
                        if (TileAt(xLevel+i, yLevel+j).move_v2 == true) v2.x += x_offs[0];
                    }

                    if (TileAt(xLevel+i, yLevel+j).move_v3 == true) v3.x += x_offs[1];
                    if (TileAt(xLevel+i, yLevel+j).move_v4 == true) v4.x += x_offs[1];
                }
                
                // Zu rendernde Vertices ins Array schreiben
                TilesToRender[NumToRender*6]   = v1;	// Jeweils 2 Dreicke als
                TilesToRender[NumToRender*6+1] = v2;	// als ein viereckiges
                TilesToRender[NumToRender*6+2] = v3;	// Tile ins Array kopieren
                TilesToRender[NumToRender*6+3] = v3;
                TilesToRender[NumToRender*6+4] = v2;
                TilesToRender[NumToRender*6+5] = v4;

                NumToRender++;				// Weiter im Vertex Array
            }
            xScreen += TILESIZE_X;		// Am Screen weiter
        }
        yScreen += TILESIZE_Y;			// Am Screen weiter
    }

    if (NumToRender > 0)
        DirectGraphics.RendertoBuffer (D3DPT_TRIANGLELIST, NumToRender*2, &TilesToRender[0]);
}

// --------------------------------------------------------------------------------------
// Level Vodergrund anzeigen
// --------------------------------------------------------------------------------------

void TileEngineClass::DrawFrontLevel(void)
{
    RECT			Rect;						// Texturausschnitt im Tileset
    int				NumToRender;				// Wieviele Vertices zu rendern ?
    int				ActualTexture;				// Aktuelle Textur
    float			l,  r,  o,  u;				// Vertice Koordinaten
    float			tl, tr, to, tu;				// Textur Koordinaten
    unsigned int	Type;						// TileNR des Tiles

    // Am Anfang noch keine Textur gew�hlt
    ActualTexture = -1;

    // x und ypos am screen errechnen
    xScreen = (float)(-xTileOffs) + RenderPosX * TILESIZE_X;
    yScreen = (float)(-yTileOffs) + RenderPosY * TILESIZE_Y;

    DirectGraphics.SetColorKeyMode();

    // Noch keine Tiles zum rendern
    NumToRender = 0;

    //DKS - WaterList lookup table has been replaced with WaterSinTableClass,
    //      see comments for it in Tileengine.h
    //int off = 0;

    for(int j=RenderPosY; j<RenderPosYTo; j++)
    {
        xScreen = (float)(-xTileOffs) + RenderPosX * TILESIZE_X;

        for(int i=RenderPosX; i<RenderPosXTo; i++)
        {
            if (TileAt(xLevel+i, yLevel+j).FrontArt > 0				   &&
                    !(TileAt(xLevel+i, yLevel+j).Block & BLOCKWERT_VERDECKEN) &&
                    !(TileAt(xLevel+i, yLevel+j).Block & BLOCKWERT_WAND))
            {
                // Neue Textur ?
                if (TileAt(xLevel+i, yLevel+j).TileSetFront != ActualTexture)
                {
                    // Aktuelle Textur sichern
                    ActualTexture = TileAt(xLevel+i, yLevel+j).TileSetFront;

                    // Tiles zeichnen
                    if (NumToRender > 0)
                        DirectGraphics.RendertoBuffer (D3DPT_TRIANGLELIST, NumToRender*2,
                                                       &TilesToRender[0]);

                    // Neue aktuelle Textur setzen
                    DirectGraphics.SetTexture( TileGfx[ActualTexture].itsTexIdx );

                    // Und beim rendern wieder von vorne anfangen
                    NumToRender = 0;
                }

                Type = TileAt(xLevel+i, yLevel+j).FrontArt - INCLUDE_ZEROTILE;

                // Animiertes Tile ?
                if (TileAt(xLevel+i, yLevel+j).Block & BLOCKWERT_ANIMIERT_FRONT)
                    Type += 36*TileAnimPhase;

                // richtigen Ausschnitt f�r das aktuelle Tile setzen
                Rect = TileRects[Type];

                // Screen-Koordinaten der Vertices
                l = SKL(xScreen);						// Links
                o = SKO(yScreen);						// Oben
                r = SKR(xScreen);   		            // Rechts
                u = SKU(yScreen);		            	// Unten

                // Textur-Koordinaten
                tl = TKL(Rect.left);			    	// Links
                tr = TKR(Rect.right);		    		// Rechts
                to = TKO(Rect.top);	        			// Oben
                tu = TKU(Rect.bottom);		    		// Unten

                // Licht setzen (pr�fen auf Overlay light, wegen hellen Kanten)
                if (TileAt(xLevel+i, yLevel+j).Block & BLOCKWERT_OVERLAY_LIGHT)
                {
                    v1.color = TileAt(xLevel+i, yLevel+j).Color[0];
                    v2.color = TileAt(xLevel+i, yLevel+j).Color[1];
                    v3.color = TileAt(xLevel+i, yLevel+j).Color[2];
                    v4.color = TileAt(xLevel+i, yLevel+j).Color[3];
                }
                else
                {
                    v1.color =
                        v2.color =
                            v3.color =
                                v4.color = D3DCOLOR_RGBA(255, 255, 255, TileAt(xLevel+i, yLevel+j).Alpha);
                }

                v1.x		= l;            						// Links oben
                v1.y		= o;
                v1.tu		= tl;
                v1.tv		= to;

                v2.x		= r;						            // Rechts oben
                v2.y		= o;
                v2.tu		= tr;
                v2.tv		= to;

                v3.x		= l;				                	// Links unten
                v3.y		= u;
                v3.tu		= tl;
                v3.tv		= tu;

                v4.x		= r;	                				// Rechts unten
                v4.y		= u;
                v4.tu		= tr;
                v4.tv		= tu;

                // Hintergrund des Wasser schwabbeln lassen

                //DKS - WaterList lookup table has been replaced with WaterSinTableClass,
                //      see comments for it in Tileengine.h
#if 0
                off = (int(SinPos2) + (yLevel * 2) % 40 + j*2) % 1024;

                //DKS - Fixed out of bounds access to Tiles[][] array on y here:
                //      When yLevel is 1 and j is -1 (indicating that the one-tile overdraw
                //      border is being drawn on the top screen edge), this was ending up
                //      trying to access a row higher than the top screen border which doesn't
                //      exist. Loading the Eis map (level 7) would crash on some machines.
                //if (TileAt(xLevel+i, yLevel+j-1).Block & BLOCKWERT_LIQUID)                    // Original line
                if ( yLevel+j > 0 &&    // DKS Added this check to above line
                        TileAt(xLevel+i, yLevel+j-1).Block & BLOCKWERT_LIQUID)
                {
                    if (TileAt(xLevel+i, yLevel+j).move_v1 == true) v1.x += SinList2[off];
                    if (TileAt(xLevel+i, yLevel+j).move_v2 == true) v2.x += SinList2[off];
                }

                if (TileAt(xLevel+i, yLevel+j).move_v3 == true) v3.x += SinList2[off + 2];
                if (TileAt(xLevel+i, yLevel+j).move_v4 == true) v4.x += SinList2[off + 2];
#endif //0

                if (TileAt(xLevel+i, yLevel+j).move_v1 ||
                    TileAt(xLevel+i, yLevel+j).move_v2 ||
                    TileAt(xLevel+i, yLevel+j).move_v3 ||
                    TileAt(xLevel+i, yLevel+j).move_v4)
                {
                    float x_offs[2];
                    WaterSinTable.GetNonWaterSin(j, x_offs);

                    //DKS - Fixed out of bounds access to Tiles[][] array on y here:
                    //      When yLevel is 1 and j is -1 (indicating that the one-tile overdraw
                    //      border is being drawn on the top screen edge), this was ending up
                    //      trying to access a row higher than the top screen border which doesn't
                    //      exist. Loading the Eis map (level 7) would crash on some machines.
                    //if (TileAt(xLevel+i, yLevel+j-1).Block & BLOCKWERT_LIQUID)                    // Original line
                    if ( yLevel+j > 0 &&    // DKS Added this check to above line
                            TileAt(xLevel+i, yLevel+j-1).Block & BLOCKWERT_LIQUID)
                    {
                        if (TileAt(xLevel+i, yLevel+j).move_v1 == true) v1.x += x_offs[0];
                        if (TileAt(xLevel+i, yLevel+j).move_v2 == true) v2.x += x_offs[0];
                    }

                    if (TileAt(xLevel+i, yLevel+j).move_v3 == true) v3.x += x_offs[1];
                    if (TileAt(xLevel+i, yLevel+j).move_v4 == true) v4.x += x_offs[1];
                }
                
                // Zu rendernde Vertices ins Array schreiben
                TilesToRender[NumToRender*6]   = v1;	// Jeweils 2 Dreicke als
                TilesToRender[NumToRender*6+1] = v2;	// als ein viereckiges
                TilesToRender[NumToRender*6+2] = v3;	// Tile ins Array kopieren
                TilesToRender[NumToRender*6+3] = v3;
                TilesToRender[NumToRender*6+4] = v2;
                TilesToRender[NumToRender*6+5] = v4;

                NumToRender++;				// Weiter im Vertex Array

            }
            xScreen += TILESIZE_X;		// Am Screen weiter
        }
        yScreen += TILESIZE_Y;			// Am Screen weiter
    }

    if (NumToRender > 0)

        DirectGraphics.RendertoBuffer (D3DPT_TRIANGLELIST, NumToRender*2,&TilesToRender[0]);
}

// --------------------------------------------------------------------------------------
// Level Ausschnitt scrollen
// --------------------------------------------------------------------------------------

void TileEngineClass::ScrollLevel(float x, float y, int neu, float sx, float sy)
{
    ScrolltoX = x;
    ScrolltoY = y;
    SpeedX	  = sx;
    SpeedY	  = sy;
    Zustand	  = neu;
    Player[0].BronsonCounter = 0.0f;
    Player[1].BronsonCounter = 0.0f;
}

// --------------------------------------------------------------------------------------
// Wandst�cke, die den Spieler vedecken, erneut zeichnen
// --------------------------------------------------------------------------------------

void TileEngineClass::DrawBackLevelOverlay (void)
{
    RECT			Rect;						// Texturausschnitt im Tileset
    int				NumToRender;				// Wieviele Vertices zu rendern ?
    int				ActualTexture;				// Aktuelle Textur
    float			l,  r,  o,  u;				// Vertice Koordinaten
    float			tl, tr, to, tu;				// Textur Koordinaten
    unsigned int	Type;						// TileNR des Tiles

    // Am Anfang noch keine Textur gew�hlt
    ActualTexture = -1;

    // x und ypos am screen errechnen
    xScreen = (float)(-xTileOffs) + RenderPosX * TILESIZE_X;
    yScreen = (float)(-yTileOffs) + RenderPosY * TILESIZE_Y;

    DirectGraphics.SetColorKeyMode();

    // Noch keine Tiles zum rendern
    NumToRender = 0;
    //int al;
    //DKS - Variable was unused in original source, disabled:
    //int off = 0;

    for(int j = RenderPosY; j<RenderPosYTo; j++)
    {
        xScreen = (float)(-xTileOffs) + RenderPosX * TILESIZE_X;

        for(int i = RenderPosX; i<RenderPosXTo; i++)
        {
            // Hintergrundtile nochmal neu setzen?
            //
            if (TileAt(xLevel+i, yLevel+j).BackArt > 0 &&
                    TileAt(xLevel+i, yLevel+j).Block & BLOCKWERT_WAND &&
                    (!(TileAt(xLevel+i, yLevel+j).FrontArt > 0 &&
                       TileAt(xLevel+i, yLevel+j).Block & BLOCKWERT_VERDECKEN)))
            {
                // Neue Textur ?
                if (TileAt(xLevel+i, yLevel+j).TileSetBack != ActualTexture)
                {
                    // Aktuelle Textur sichern
                    ActualTexture = TileAt(xLevel+i, yLevel+j).TileSetBack;

                    // Tiles zeichnen
                    if (NumToRender > 0)
                        DirectGraphics.RendertoBuffer (D3DPT_TRIANGLELIST, NumToRender*2, &TilesToRender[0]);

                    // Neue aktuelle Textur setzen
                    DirectGraphics.SetTexture( TileGfx[ActualTexture].itsTexIdx );

                    // Und beim rendern wieder von vorne anfangen
                    NumToRender = 0;
                }

                Type = TileAt(xLevel+i, yLevel+j).BackArt - INCLUDE_ZEROTILE;

                // Animiertes Tile ?
                if (TileAt(xLevel+i, yLevel+j).Block & BLOCKWERT_ANIMIERT_BACK)
                    Type += 36*TileAnimPhase;

                // richtigen Ausschnitt f�r das aktuelle Tile setzen
                Rect = TileRects[Type];

                // Screen-Koordinaten der Vertices
                l = SKL(xScreen);						// Links
                o = SKO(yScreen);						// Oben
                r = SKR(xScreen);               		// Rechts
                u = SKU(yScreen);		            	// Unten

                // Textur-Koordinaten
                tl = TKL(Rect.left);			    	// Links
                tr = TKR(Rect.right);		    		// Rechts
                to = TKO(Rect.top);	        			// Oben
                tu = TKU(Rect.bottom);		    		// Unten

                //al = TileAt(xLevel+i, yLevel+j).Alpha;

                //DKS - Variable was unused in original source, disabled:
                //off = (int(SinPos2) + (yLevel * 2) % 40 + j*2) % 1024;

                v1.color = TileAt(xLevel+i, yLevel+j).Color [0];
                v2.color = TileAt(xLevel+i, yLevel+j).Color [1];
                v3.color = TileAt(xLevel+i, yLevel+j).Color [2];
                v4.color = TileAt(xLevel+i, yLevel+j).Color [3];

                v1.x		= l;						// Links oben
                v1.y		= o;
                v1.tu		= tl;
                v1.tv		= to;

                v2.x		= r;						// Rechts oben
                v2.y		= o;
                v2.tu		= tr;
                v2.tv		= to;

                v3.x		= l;					// Links unten
                v3.y		= u;
                v3.tu		= tl;
                v3.tv		= tu;

                v4.x		= r;					// Rechts unten
                v4.y		= u;
                v4.tu		= tr;
                v4.tv		= tu;

                // Zu rendernde Vertices ins Array schreiben
                TilesToRender[NumToRender*6]   = v1;	// Jeweils 2 Dreicke als
                TilesToRender[NumToRender*6+1] = v2;	// als ein viereckiges
                TilesToRender[NumToRender*6+2] = v3;	// Tile ins Array kopieren
                TilesToRender[NumToRender*6+3] = v3;
                TilesToRender[NumToRender*6+4] = v2;
                TilesToRender[NumToRender*6+5] = v4;

                NumToRender++;				// Weiter im Vertex Array
            }
            xScreen += TILESIZE_X;		// Am Screen weiter
        }
        yScreen += TILESIZE_Y;			// Am Screen weiter
    }

    if (NumToRender > 0)
        DirectGraphics.RendertoBuffer (D3DPT_TRIANGLELIST, NumToRender*2, &TilesToRender[0]);
}

// --------------------------------------------------------------------------------------
// Die Levelst�cke zeigen, die den Spieler verdecken
// --------------------------------------------------------------------------------------

void TileEngineClass::DrawOverlayLevel(void)
{
    RECT			Rect;						// Texturausschnitt im Tileset
    int				NumToRender;				// Wieviele Vertices zu rendern ?
    int				ActualTexture;				// Aktuelle Textur
    float			l,  r,  o,  u;				// Vertice Koordinaten
    float			tl, tr, to, tu;				// Textur Koordinaten
    unsigned int	Type;						// TileNR des Tiles

    // Am Anfang noch keine Textur gew�hlt
    ActualTexture = -1;

    // x und ypos am screen errechnen
    xScreen = (float)(-xTileOffs) + RenderPosX * TILESIZE_X;
    yScreen = (float)(-yTileOffs) + RenderPosY * TILESIZE_Y;

    DirectGraphics.SetColorKeyMode();

    // Noch keine Tiles zum rendern
    NumToRender = 0;
    //int al;
    //DKS - Variable was unused in original source, disabled:
    //int off = 0;

    for(int j = RenderPosY; j<RenderPosYTo; j++)
    {
        xScreen = (float)(-xTileOffs) + RenderPosX * TILESIZE_X;

        for(int i = RenderPosX; i<RenderPosXTo; i++)
        {
            // Vordergrund Tiles setzen, um Spieler zu verdecken
            if ((TileAt(i+xLevel, j+yLevel).FrontArt > 0  &&
                    (TileAt(i+xLevel, j+yLevel).Block & BLOCKWERT_VERDECKEN ||
                     TileAt(i+xLevel, j+yLevel).Block & BLOCKWERT_WAND)) ||
                    TileAt(i+xLevel, j+yLevel).Block & BLOCKWERT_WASSERFALL ||
                    TileAt(i+xLevel, j+yLevel).Block & BLOCKWERT_MOVELINKS ||
                    TileAt(i+xLevel, j+yLevel).Block & BLOCKWERT_MOVERECHTS ||
                    TileAt(i+xLevel, j+yLevel).Block & BLOCKWERT_MOVEVERTICAL)
            {
                // Screen-Koordinaten der Vertices
                l = SKL(xScreen);						// Links
                o = SKO(yScreen);						// Oben
                r = SKR(xScreen);               		// Rechts
                u = SKU(yScreen);		            	// Unten
                /*
                				// Wasserfall
                				//
                				if (TileAt(xLevel+i, yLevel+j).Block & BLOCKWERT_WASSERFALL)
                				{
                					// Tiles zeichnen
                					if (NumToRender > 0)
                					{
                						DirectGraphics.RendertoBuffer (D3DPT_TRIANGLELIST, NumToRender*2, &TilesToRender[0]);
                						NumToRender   = 0;
                					}

                					ActualTexture = -1;

                					// Drei Schichten Wasserfall rendern =)
                					//

                					// Schicht 1
                					//
                					int xoff = (i+xLevel) % 3 * 20;
                					int yoff = (j+yLevel) % 3 * 20 + 120 - int (WasserfallOffset);

                					Wasserfall[0].SetRect (xoff, yoff, xoff+20, yoff+20);
                					Wasserfall[0].RenderSprite((float)(i*20-xTileOffs), (float)(j*20-yTileOffs), Col1);

                					// Schicht 2
                					//
                					xoff = (i+xLevel+1) % 3 * 20;
                					yoff = (j+yLevel)   % 3 * 20 + 120 - int (WasserfallOffset / 2.0f);

                					Wasserfall[0].SetRect (xoff, yoff, xoff+20, yoff+20);
                					Wasserfall[0].RenderSprite((float)(i*20-xTileOffs), (float)(j*20-yTileOffs), Col2);

                					// Glanzschicht (Schicht 3) dr�ber
                					//
                					DirectGraphics.SetAdditiveMode();
                					Wasserfall[1].SetRect ((i*20-xTileOffs)%640, (j*20-yTileOffs)%480, (i*20-xTileOffs)%640+20, (j*20-yTileOffs)%480+20);
                					Wasserfall[1].RenderSprite((float)(i*20-xTileOffs), (float)(j*20-yTileOffs), D3DCOLOR_RGBA (180, 240, 255, 60));
                					DirectGraphics.SetColorKeyMode();
                				}


                				// normales Overlay Tile
                				else
                */
                if ((TileAt(i+xLevel, j+yLevel).FrontArt > 0  &&
                        (TileAt(i+xLevel, j+yLevel).Block & BLOCKWERT_VERDECKEN ||
                         TileAt(i+xLevel, j+yLevel).Block & BLOCKWERT_MOVEVERTICAL ||
                         TileAt(i+xLevel, j+yLevel).Block & BLOCKWERT_MOVELINKS ||
                         TileAt(i+xLevel, j+yLevel).Block & BLOCKWERT_MOVERECHTS ||
                         TileAt(i+xLevel, j+yLevel).Block & BLOCKWERT_WAND)))
                {
                    // Neue Textur ?
                    if (TileAt(xLevel+i, yLevel+j).TileSetFront != ActualTexture)
                    {
                        // Aktuelle Textur sichern
                        ActualTexture = TileAt(xLevel+i, yLevel+j).TileSetFront;

                        // Tiles zeichnen
                        if (NumToRender > 0)
                            DirectGraphics.RendertoBuffer (D3DPT_TRIANGLELIST, NumToRender*2, &TilesToRender[0]);

                        // Neue aktuelle Textur setzen
                        DirectGraphics.SetTexture( TileGfx[ActualTexture].itsTexIdx );

                        // Und beim rendern wieder von vorne anfangen
                        NumToRender = 0;
                    }

                    // "normales" Overlay Tile setzen
                    Type = TileAt(xLevel+i, yLevel+j).FrontArt - INCLUDE_ZEROTILE;

                    // Animiertes Tile ?
                    if (TileAt(xLevel+i, yLevel+j).Block & BLOCKWERT_ANIMIERT_FRONT)
                        Type += 36*TileAnimPhase;

                    // richtigen Ausschnitt f�r das aktuelle Tile setzen
                    Rect = TileRects[Type];

                    // Textur-Koordinaten
                    tl = TKL(Rect.left);			    	// Links
                    tr = TKR(Rect.right);		    		// Rechts
                    to = TKO(Rect.top);	        			// Oben
                    tu = TKU(Rect.bottom);		    		// Unten

                    // bewegtes Tile vertikal
                    if (TileAt(i+xLevel, j+yLevel).Block & BLOCKWERT_MOVEVERTICAL)
                    {
                        to -= 60.0f / 256.0f * WasserfallOffset / 120.0f;
                        tu -= 60.0f / 256.0f * WasserfallOffset / 120.0f;
                    }

                    // bewegtes Tile links
                    if (TileAt(i+xLevel, j+yLevel).Block & BLOCKWERT_MOVELINKS)
                    {
                        tl += 60.0f / 256.0f * WasserfallOffset / 120.0f;
                        tr += 60.0f / 256.0f * WasserfallOffset / 120.0f;
                    }

                    // bewegtes Tile rechts
                    if (TileAt(i+xLevel, j+yLevel).Block & BLOCKWERT_MOVERECHTS)
                    {
                        tl -= 60.0f / 256.0f * WasserfallOffset / 120.0f;
                        tr -= 60.0f / 256.0f * WasserfallOffset / 120.0f;
                    }

                    //al = TileAt(xLevel+i, yLevel+j).Alpha;

                    if (TileAt(xLevel+i, yLevel+j).Block & BLOCKWERT_OVERLAY_LIGHT)
                    {
                        v1.color = TileAt(xLevel+i, yLevel+j).Color [0];
                        v2.color = TileAt(xLevel+i, yLevel+j).Color [1];
                        v3.color = TileAt(xLevel+i, yLevel+j).Color [2];
                        v4.color = TileAt(xLevel+i, yLevel+j).Color [3];
                    }
                    else
                    {
                        v1.color = v2.color = v3.color = v4.color = D3DCOLOR_RGBA (255, 255, 255, TileAt(xLevel+i, yLevel+j).Alpha);
                    }

                    //DKS - Variable was unused in original source, disabled:
                    //off = (int(SinPos2) + (yLevel * 2) % 40 + j*2) % 1024;

                    v1.x		= l;						// Links oben
                    v1.y		= o;
                    v1.tu		= tl;
                    v1.tv		= to;

                    v2.x		= r;						// Rechts oben
                    v2.y		= o;
                    v2.tu		= tr;
                    v2.tv		= to;

                    v3.x		= l;					    // Links unten
                    v3.y		= u;
                    v3.tu		= tl;
                    v3.tv		= tu;

                    v4.x		= r;					    // Rechts unten
                    v4.y		= u;
                    v4.tu		= tr;
                    v4.tv		= tu;

                    // Zu rendernde Vertices ins Array schreiben
                    TilesToRender[NumToRender*6]   = v1;	// Jeweils 2 Dreicke als
                    TilesToRender[NumToRender*6+1] = v2;	// als ein viereckiges
                    TilesToRender[NumToRender*6+2] = v3;	// Tile ins Array kopieren
                    TilesToRender[NumToRender*6+3] = v3;
                    TilesToRender[NumToRender*6+4] = v2;
                    TilesToRender[NumToRender*6+5] = v4;

                    NumToRender++;				// Weiter im Vertex Array
                }
            }

            xScreen += TILESIZE_X;		// Am Screen weiter
        }

        yScreen += TILESIZE_Y;			// Am Screen weiter
    }

    if (NumToRender > 0)
        DirectGraphics.RendertoBuffer (D3DPT_TRIANGLELIST, NumToRender*2, &TilesToRender[0]);
}

// --------------------------------------------------------------------------------------
// Die Levelst�cke zeigen, die den Spieler verdecken
// --------------------------------------------------------------------------------------

void TileEngineClass::DrawWater(void)
{
    int				NumToRender;				// Wieviele Vertices zu rendern ?
    float			l,  r,  o,  u;				// Vertice Koordinaten
    int				xo, yo;

    // x und ypos am screen errechnen
    xScreen = (float)(-xTileOffs) + RenderPosX * TILESIZE_X;
    yScreen = (float)(-yTileOffs) + RenderPosY * TILESIZE_Y;

    // Noch keine Tiles zum rendern
    NumToRender = 0;

    //DKS - WaterList lookup table has been replaced with WaterSinTableClass,
    //      see comments for it in Tileengine.h
    //int off = (int)((WaterPos) + xLevel % 32 + (yLevel * 10) % 40) % 1024;

    DirectGraphics.SetFilterMode (true);
    DirectGraphics.SetColorKeyMode();

    // billiges Wasser?
    //
    if (options_Detail < DETAIL_MEDIUM)
    {

        DirectGraphics.SetTexture( -1 );

        for(int j = RenderPosY; j<RenderPosYTo; j++)
        {
            xScreen = (float)(-xTileOffs) + RenderPosX * TILESIZE_X;

            for(int i = RenderPosX; i<RenderPosXTo; i++)
            {
                // Vordergrund Tiles setzen um Spieler zu verdecken
                if (TileAt(i+xLevel, j+yLevel).Block & BLOCKWERT_LIQUID)
                {
                    // Screen-Koordinaten der Vertices
                    l = SKL(xScreen);						// Links
                    o = SKO(yScreen);						// Oben
                    r = SKR(xScreen);   	            	// Rechts
                    u = SKU(yScreen);		            	// Unten

                    //DKS - Original code was setting texture coordinates when no texture 
                    //      is bound.. disabled the code:
                    /* DISABLED BLOCK
                    // Textur Koordinaten setzen
                    //
                    xo = (i + xLevel) % 8;
                    yo = (j + yLevel) % 8;

                    v1.tu = WasserU [xo];
                    v1.tv = WasserV [yo];
                    v2.tu = WasserU [xo+1];
                    v2.tv = WasserV [yo];
                    v3.tu = WasserU [xo];
                    v3.tv = WasserV [yo+1];
                    v4.tu = WasserU [xo+1];
                    v4.tv = WasserV [yo+1];
                    END DISABLED BLOCK */

                    v1.x = l;						// Links oben
                    v1.y = o;
                    v2.x = r;						// Rechts oben
                    v2.y = o;
                    v3.x = l;					    // Links unten
                    v3.y = u;
                    v4.x = r;					    // Rechts unten
                    v4.y = u;

                    // Farbe festlegen
                    //
                    v1.color = v2.color = v3.color = v4.color = Col1;

                    // Zu rendernde Vertices ins Array schreiben
                    TilesToRender[NumToRender*6]   = v1;	// Jeweils 2 Dreicke als
                    TilesToRender[NumToRender*6+1] = v2;	// als ein viereckiges
                    TilesToRender[NumToRender*6+2] = v3;	// Tile ins Array kopieren
                    TilesToRender[NumToRender*6+3] = v3;
                    TilesToRender[NumToRender*6+4] = v2;
                    TilesToRender[NumToRender*6+5] = v4;

                    NumToRender++;				// Weiter im Vertex Array
                }

                xScreen += TILESIZE_X;		// Am Screen weiter
            }

            yScreen += TILESIZE_Y;			// Am Screen weiter
        }

        if (NumToRender > 0)
            DirectGraphics.RendertoBuffer (D3DPT_TRIANGLELIST, NumToRender*2, &TilesToRender[0]);
    }

    // oder geiles, animiertes Wasser?
    else
    {
        // zwei Schichten Wasser rendern
        for (int schicht = 0; schicht < 2; schicht++)
        {
            // Offsets der Tiles berechnen (0-19)
            xTileOffs = (int)(XOffset) % TILESIZE_X;
            yTileOffs = (int)(YOffset) % TILESIZE_Y;

            // ypos am screen errechnen
            yScreen = (float)(-yTileOffs) + RenderPosY * TILESIZE_Y;

            for(int j = RenderPosY; j<RenderPosYTo; j++)
            {
                xScreen = (float)(-xTileOffs) + RenderPosX * TILESIZE_X;

                for(int i = RenderPosX; i<RenderPosXTo; i++)
                {
                    // Vordergrund Tiles setzen um Spieler zu verdecken
                    if (TileAt(i+xLevel, j+yLevel).Block & BLOCKWERT_LIQUID)
                    {
                        // Screen-Koordinaten der Vertices
                        l = SKL(xScreen);						// Links
                        o = SKO(yScreen);						// Oben
                        r = SKR(xScreen);   	            	// Rechts
                        u = SKU(yScreen);		            	// Unten

                        // Vertices definieren
                        v1.x = l;						// Links oben
                        v1.y = o;
                        v2.x = r;						// Rechts oben
                        v2.y = o;
                        v3.x = l;					    // Links unten
                        v3.y = u;
                        v4.x = r;					    // Rechts unten
                        v4.y = u;

                        if (schicht == 0)
                        {
                            v1.color = v2.color = v3.color = v4.color = Col1;
                        }
                        else
                        {
                            v1.color = v2.color = v3.color = v4.color = Col2;
                        }

                        // Oberfl�che des Wassers aufhellen
                        //DKS - Fixed potential out of bounds access to Tiles[][] array here
                        //      by adding check for yLevel+j-1 >= 0:
                        //if (!(TileAt(xLevel+i, yLevel+j-1).Block & BLOCKWERT_LIQUID) &&           (ORIGINAL TWO LINES)
                        //        !(TileAt(xLevel+i, yLevel+j-1).Block & BLOCKWERT_WASSERFALL))
                        if (yLevel+j-1 >= 0 &&
                                !(TileAt(xLevel+i, yLevel+j-1).Block & BLOCKWERT_LIQUID) &&         
                                !(TileAt(xLevel+i, yLevel+j-1).Block & BLOCKWERT_WASSERFALL))
                        {
                            //DKS - Fixed potential out of bounds access to Tiles[][] array here
                            //      by adding check for xLevel+i-1 >= 0:
                            //if (!(TileAt(xLevel+i-1, yLevel+j-1).Block & BLOCKWERT_LIQUID) &&
                            //        !(TileAt(xLevel+i-1, yLevel+j-1).Block & BLOCKWERT_WASSERFALL))
                            if ( xLevel+i-1 >= 0 &&
                                    !(TileAt(xLevel+i-1, yLevel+j-1).Block & BLOCKWERT_LIQUID) &&
                                    !(TileAt(xLevel+i-1, yLevel+j-1).Block & BLOCKWERT_WASSERFALL))
                                v1.color = Col3;

                            //DKS - Fixed potential out of bounds access to Tiles[][] array here
                            //      by adding check for xLevel+i+1 < LEVELSIZE_X
                            //if (!(TileAt(xLevel+i+1, yLevel+j-1).Block & BLOCKWERT_LIQUID) &&
                            //        !(TileAt(xLevel+i+1, yLevel+j-1).Block & BLOCKWERT_WASSERFALL))
                            if ( xLevel+i+1 < LEVELSIZE_X &&
                                    !(TileAt(xLevel+i+1, yLevel+j-1).Block & BLOCKWERT_LIQUID) &&
                                    !(TileAt(xLevel+i+1, yLevel+j-1).Block & BLOCKWERT_WASSERFALL))
                                v2.color = Col3;
                        }

                        xo = (i + xLevel) % 8;
                        yo = (j + yLevel) % 8;

                        // Schicht 0 == langsam, gespiegelt
                        if (schicht == 0)
                        {
                            v1.tu = WasserU [8-xo];
                            v1.tv = WasserV [8-yo];
                            v2.tu = WasserU [8-xo-1];
                            v2.tv = WasserV [8-yo];
                            v3.tu = WasserU [8-xo];
                            v3.tv = WasserV [8-yo-1];
                            v4.tu = WasserU [8-xo-1];
                            v4.tv = WasserV [8-yo-1];
                        }


                        // Schicht 0 == schnell
                        else
                        {
                            v1.tu = WasserU [xo];
                            v1.tv = WasserV [yo];
                            v2.tu = WasserU [xo+1];
                            v2.tv = WasserV [yo];
                            v3.tu = WasserU [xo];
                            v3.tv = WasserV [yo+1];
                            v4.tu = WasserU [xo+1];
                            v4.tv = WasserV [yo+1];
                        }

                        //DKS - WaterList lookup table has been replaced with WaterSinTableClass,
                        //      see comments for it in Tileengine.h
                        //if (TileAt(xLevel+i, yLevel+j).move_v1 == true)
                        //    v1.y += WaterList[off + j*10 + i * 2];
                        //if (TileAt(xLevel+i, yLevel+j).move_v2 == true)
                        //    v2.y += WaterList[off + j*10 + i * 2];
                        //if (TileAt(xLevel+i, yLevel+j).move_v3 == true)
                        //    v3.y += WaterList[off + j*10 + i * 2 + 10];
                        //if (TileAt(xLevel+i, yLevel+j).move_v4 == true)
                        //    v4.y += WaterList[off + j*10 + i * 2 + 10];
                        if (TileAt(xLevel+i, yLevel+j).move_v1 ||
                            TileAt(xLevel+i, yLevel+j).move_v2 ||
                            TileAt(xLevel+i, yLevel+j).move_v3 ||
                            TileAt(xLevel+i, yLevel+j).move_v4)
                        {
                            float y_offs[2];
                            WaterSinTable.GetWaterSin(i, j, y_offs);
                            if (TileAt(xLevel+i, yLevel+j).move_v1 == true) v1.y += y_offs[0];
                            if (TileAt(xLevel+i, yLevel+j).move_v2 == true) v2.y += y_offs[0];
                            if (TileAt(xLevel+i, yLevel+j).move_v3 == true) v3.y += y_offs[1];
                            if (TileAt(xLevel+i, yLevel+j).move_v4 == true) v4.y += y_offs[1];
                        }

                        // Zu rendernde Vertices ins Array schreiben
                        TilesToRender[NumToRender*6]   = v1;	// Jeweils 2 Dreicke als
                        TilesToRender[NumToRender*6+1] = v2;	// als ein viereckiges
                        TilesToRender[NumToRender*6+2] = v3;	// Tile ins Array kopieren
                        TilesToRender[NumToRender*6+3] = v3;
                        TilesToRender[NumToRender*6+4] = v2;
                        TilesToRender[NumToRender*6+5] = v4;

                        NumToRender++;				// Weiter im Vertex Array
                    }

                    xScreen += TILESIZE_X;		// Am Screen weiter
                }

                yScreen += TILESIZE_Y;			// Am Screen weiter
            }

            if (NumToRender > 0)
            {
                if (schicht > 0)
                {
                    DirectGraphics.SetAdditiveMode();
                    DirectGraphics.SetTexture( LiquidGfx[1].itsTexIdx );
                }
                else
                {
                    DirectGraphics.SetTexture( LiquidGfx[0].itsTexIdx );
                }

                DirectGraphics.RendertoBuffer (D3DPT_TRIANGLELIST, NumToRender*2, &TilesToRender[0]);
            }

            NumToRender = 0;
        }
    }

    // Wasserfall rendern
    xScreen = (float)(-xTileOffs) + RenderPosX * TILESIZE_X;
    yScreen = (float)(-yTileOffs) + RenderPosY * TILESIZE_Y;
    {
        for(int j = RenderPosY; j<RenderPosYTo; j++)
        {
            xScreen = (float)(-xTileOffs) + RenderPosX * TILESIZE_X;

            for(int i = RenderPosX; i<RenderPosXTo; i++)
            {
                // Ist ein Wasserfall teil?
                if (TileAt(i+xLevel, j+yLevel).Block & BLOCKWERT_WASSERFALL)
                {
                    DirectGraphics.SetColorKeyMode();
                    // Drei Schichten Wasserfall rendern =)
                    //

                    // Schicht 1
                    //
                    int xoff = (i+xLevel) % 3 * TILESIZE_X;
                    int yoff = (j+yLevel) % 3 * TILESIZE_Y + 120 - int (WasserfallOffset);

                    Wasserfall[0].SetRect (xoff, yoff, xoff+TILESIZE_X, yoff+TILESIZE_Y);
                    Wasserfall[0].RenderSprite((float)(i*TILESIZE_X-xTileOffs), (float)(j*TILESIZE_Y-yTileOffs), Col1);

                    // Schicht 2
                    //
                    xoff = (i+xLevel+1) % 3 * TILESIZE_X;
                    yoff = (j+yLevel)   % 3 * TILESIZE_Y + 120 - int (WasserfallOffset / 2.0f);

                    Wasserfall[0].SetRect (xoff, yoff, xoff+TILESIZE_X, yoff+TILESIZE_Y);
                    Wasserfall[0].RenderSprite((float)(i*TILESIZE_X-xTileOffs), (float)(j*TILESIZE_Y-yTileOffs), Col2);

                    // Glanzschicht (Schicht 3) dr�ber
                    //
                    Wasserfall[1].SetRect ((i*TILESIZE_X-xTileOffs)%640,
                                           (j*TILESIZE_Y-yTileOffs)%480,
                                           (i*TILESIZE_X-xTileOffs)%640+TILESIZE_X,
                                           (j*TILESIZE_Y-yTileOffs)%480+TILESIZE_Y);

                    Wasserfall[1].RenderSprite((float)(i*TILESIZE_X-xTileOffs),
                                               (float)(j*TILESIZE_Y-yTileOffs),
                                               D3DCOLOR_RGBA (180, 240, 255, 60));
                }

                xScreen += TILESIZE_X;		// Am Screen weiter
            }

            yScreen += TILESIZE_Y;			// Am Screen weiter
        }
    }

    DirectGraphics.SetFilterMode (false);
}

// --------------------------------------------------------------------------------------
// Grenzen checken
// --------------------------------------------------------------------------------------

void TileEngineClass::CheckBounds(void)
{
    // Grenzen des Levels checken
    if (XOffset < TILESIZE_X)
        XOffset = TILESIZE_X;

    if (YOffset < TILESIZE_Y)
        YOffset = TILESIZE_Y;

    if (XOffset > LEVELPIXELSIZE_X - 640.0f - TILESIZE_X)
        XOffset = LEVELPIXELSIZE_X - 640.0f - TILESIZE_X;

    if (YOffset > LEVELPIXELSIZE_Y - 480.0f - TILESIZE_Y)
        YOffset = LEVELPIXELSIZE_Y - 480.0f - TILESIZE_Y;
}

// --------------------------------------------------------------------------------------

void TileEngineClass::WertAngleichen(float &nachx, float &nachy, float vonx, float vony)
{
//	nachx = (float)(int)vonx;
//	nachy = (float)(int)vony;

//	return;

    float rangex, rangey;

    rangex = vonx - nachx;
    rangey = vony - nachy;

    if (rangex >  50.0f) rangex =  50.0f;
    if (rangex < -50.0f) rangex = -50.0f;
    if (rangey >  60.0f) rangey =  60.0f;
    if (rangey < -60.0f) rangey = -60.0f;

    nachx += rangex * 0.8f SYNC;

    /*
    if (NUMPLAYERS == 1 &&
    	(FlugsackFliesFree == false ||
    	 Player[0].Riding() == false)
    	&&
    	(Player[0].Aktion[AKTION_OBEN] ||
    	 Player[0].Aktion[AKTION_UNTEN]))
    {
    	int a;
    	a = 0;
    }
    else */
    if (FlugsackFliesFree == true)
        nachy += rangey * 0.75f SYNC;
}


// --------------------------------------------------------------------------------------
// Level scrollen und Tiles animieren usw
// --------------------------------------------------------------------------------------

void TileEngineClass::UpdateLevel(void)
{
    if (Console.Showing)
        return;

    // Zeit ablaufen lassen
    if (RunningTutorial == false &&
            Timelimit > 0.0f &&
            HUD.BossHUDActive == 0.0f &&
            Console.Showing == false)
    {
        if (Skill == 0)
            Timelimit -= 0.05f SYNC;
        else if (Skill == 1)
            Timelimit -= 0.075f SYNC;
        else
            Timelimit -= 0.1f SYNC;
    }

    // Zeit abgelaufen? Dann Punisher erscheinen lassen
    //
    if (Timelimit < 0.0f)
    {
        if (Player[0].PunisherActive == false &&
                HUD.BossHUDActive == 0.0f)
        {
            Gegner.PushGegner(0, 0, PUNISHER, 0, 0, false);
            Player[0].PunisherActive = true;
            PartikelSystem.ThunderAlpha = 255.0f;
            PartikelSystem.ThunderColor[0] = 0;
            PartikelSystem.ThunderColor[1] = 0;
            PartikelSystem.ThunderColor[2] = 0;
        }

        Timelimit = 0.0f;
    }

    XOffset += ScrollSpeedX * SpeedFaktor;
    YOffset += ScrollSpeedY * SpeedFaktor;

    // Tiles animieren
    TileAnimCount += SpeedFaktor;				// Counter erh�hen
    if (TileAnimCount > TILEANIM_SPEED)			// auf Maximum pr�fen
        //if (TileAnimCount > 0.5f)			// auf Maximum pr�fen
    {
        TileAnimCount = 0.0f;					// Counter wieder auf 0 setzen
        TileAnimPhase++;						// und n�chste Animphase setzen
        if (TileAnimPhase >=4)					// Animation wieer von vorne ?
            TileAnimPhase = 0;
    }

    // Sichtbaren Level-Ausschnitt scrollen
    if (Zustand == ZUSTAND_SCROLLTO ||
            Zustand == ZUSTAND_SCROLLTOLOCK)
    {
        if (XOffset < ScrolltoX)
        {
            XOffset += SpeedX SYNC;

            if (XOffset > ScrolltoX)
                XOffset = ScrolltoX;
        }

        if (XOffset > ScrolltoX)
        {
            XOffset -= SpeedX SYNC;

            if (XOffset < ScrolltoX)
                XOffset = ScrolltoX;
        }

        if (YOffset < ScrolltoY)
        {
            YOffset += SpeedY SYNC;

            if (YOffset > ScrolltoY)
                YOffset = ScrolltoY;
        }

        if (YOffset > ScrolltoY)
        {
            YOffset -= SpeedY SYNC;

            if (YOffset < ScrolltoY)
                YOffset = ScrolltoY;
        }

        if (XOffset == ScrolltoX &&
                YOffset == ScrolltoY)
        {
            if (Zustand == ZUSTAND_SCROLLTOLOCK)
                Zustand =  ZUSTAND_LOCKED;
            else
                Zustand =  ZUSTAND_SCROLLBAR;
        }
    }

    // Level-Ausschnitt zum Spieler Scrollen
    if (Zustand == ZUSTAND_SCROLLTOPLAYER)
    {
        if (NUMPLAYERS == 1)
        {
            ScrolltoX = Player[0].xpos;
            ScrolltoY = Player[0].ypos;
        }
        else
        {
            if (Player[0].Lives <= 0)
            {
                ScrolltoX = Player[1].xpos;
                ScrolltoY = Player[1].ypos;
            }
            else if (Player[1].Lives <= 0)
            {
                ScrolltoX = Player[0].xpos;
                ScrolltoY = Player[0].ypos;
            }
            else
            {
                float xdist = (float)(Player[1].xpos - Player[0].xpos);
                float ydist = (float)(Player[1].ypos - Player[0].ypos);

                ScrolltoX = (Player[0].xpos + 35) + xdist / 2.0f - 320.0f;
                ScrolltoY = (Player[0].ypos + 40) + ydist / 2.0f - 240.0f;
            }
        }

        bool isScrolling = false;
        /*
        		if (ScrolltoX - XOffset <  SCROLL_BORDER_LEFT)
        		{
        			XOffset -= SpeedX SYNC;
        			isScrolling = true;
        		}

        		if (ScrolltoX - XOffset >  SCROLL_BORDER_RIGHT)
        		{
        			XOffset += SpeedX SYNC;
        			isScrolling = true;
        		}
        */
        if (ScrolltoY - YOffset <  SCROLL_BORDER_EXTREME_TOP)
        {
            YOffset -= SpeedY SYNC;
            isScrolling = true;
        }

        if (ScrolltoY - YOffset >  SCROLL_BORDER_EXTREME_BOTTOM)
        {
            YOffset += SpeedY SYNC;
            isScrolling = true;
        }

        if (isScrolling == false)
            Zustand = ZUSTAND_SCROLLBAR;
    }

    // Scrollbar und 2-Spieler Modus? Dann Kamera entsprechend setzen
    if (Zustand == ZUSTAND_SCROLLBAR)
    {
        float newx, newy;
        float angleichx, angleichy;

        if (NUMPLAYERS == 2)
        {
            if (Player[0].Handlung == TOT)
            {
                newx = Player[1].xpos + 35;
                newy = Player[1].ypos;
            }
            else if (Player[1].Handlung == TOT)
            {
                newx = Player[0].xpos + 35;
                newy = Player[0].ypos;
            }
            else
            {
                float xdist, ydist;

                xdist = (float)(Player[1].xpos - Player[0].xpos);
                newx = (Player[0].xpos + 35) + xdist / 2.0f;

                ydist = (float)(Player[1].ypos - Player[0].ypos);
                newy = (Player[0].ypos) + ydist / 2.0f;
            }
        }
        else
        {
            newx = Player[0].xpos + 35;
            newy = Player[0].ypos;
        }

        if (newx < 0) newx = 0;
        if (newy < 0) newy = 0;

        angleichx = XOffset;
        angleichy = YOffset;

        // Nach Boss wieder auf Spieler zentrieren?
//		if (MustCenterPlayer == true)
        {
            bool CenterDone = true;

            if (newx < XOffset + 320.0f - SCROLL_BORDER_HORIZ)
            {
                CenterDone = false;
                angleichx = newx - 320.0f + SCROLL_BORDER_HORIZ;
            }
            if (newx > XOffset + 320.0f + SCROLL_BORDER_HORIZ)
            {
                CenterDone = false;
                angleichx = newx - 320.0f + SCROLL_BORDER_HORIZ;
            }
            if (newy < YOffset + 240.0f - SCROLL_BORDER_TOP)
            {
                CenterDone = false;
                angleichy = newy - 240.0f + SCROLL_BORDER_TOP;
            }
            if (newy > YOffset + 240.0f + SCROLL_BORDER_BOTTOM)
            {
                CenterDone = false;
                angleichy = newy - 240.0f - SCROLL_BORDER_BOTTOM;
            }

            WertAngleichen(XOffset, YOffset, angleichx, angleichy);

            if (CenterDone == true)
                MustCenterPlayer = false;
        }
        /*
        		// oder einfach Kanten checken?
        		else
        		{
        			if (newx < XOffset + 320.0f - SCROLL_BORDER_HORIZ)	 XOffset = newx - 320.0f + SCROLL_BORDER_HORIZ;
        			if (newx > XOffset + 320.0f + SCROLL_BORDER_HORIZ)	 XOffset = newx - 320.0f - SCROLL_BORDER_HORIZ;
        			if (newy < YOffset + 240.0f - SCROLL_BORDER_TOP)	 YOffset = newy - 240.0f + SCROLL_BORDER_TOP;
        			if (newy > YOffset + 240.0f + SCROLL_BORDER_BOTTOM) YOffset = newy - 240.0f - SCROLL_BORDER_TOP;
        		}
        		*/
    }

    // Wasserfall animieren
    WasserfallOffset += 16.0f SYNC;

    while (WasserfallOffset >= 120.0f)
        WasserfallOffset -= 120.0f;

    /* DKS - Replaced both SinList2 and WaterList lookup tables with new
             class WaterSinTableClass.  See comments in Tileengine.h */
    WaterSinTable.AdvancePosition(SpeedFaktor);
#if 0
    // Schwabbeln des Levels animieren
    //SinPos   += 3.0f SYNC;    //DKS - unused; disabled
    SinPos2  += 2.0f SYNC;
    WaterPos += 2.0f SYNC;

    //while (SinPos   >= 40.0f) SinPos   -= 40.0f;  //DKS - unused; disabled
    while (SinPos2  >= 40.0f) SinPos2  -= 40.0f;
    while (WaterPos >= 40.0f) WaterPos -= 40.0f;
#endif //0

    // Level an vorgegebene Position anpassen
    // weil z.B. der Fahrstuhl selber scrollt?
    if (NewXOffset != -1)
        XOffset = NewXOffset;

    if (NewYOffset != -1)
        YOffset = NewYOffset;
}

// --------------------------------------------------------------------------------------
// Die R�nder bei den Overlay Tiles der zerst�rbaren W�nde
// ausfransen, indem die Alphawerte der angrenzenden Tiles auf 0 gesetzt werden
// damit die Tiles nicht so abgeschnitten aussehen
// --------------------------------------------------------------------------------------

void TileEngineClass::MakeBordersLookCool(int x, int y)
{
    /*	if (TileAt(x+1, y+0).Block & BLOCKWERT_DESTRUCTIBLE)
    	{
    		TileAt(x+1, y+0).ColorOverlay[0] = D3DCOLOR_RGBA(TileAt(x+1, y+0).Red,
    														TileAt(x+1, y+0).Green,
    														TileAt(x+1, y+0).Blue,
    														0);
    		TileAt(x+1, y+0).ColorOverlay[2] = D3DCOLOR_RGBA(TileAt(x+1, y+0).Red,
    														TileAt(x+1, y+0).Green,
    														TileAt(x+1, y+0).Blue,
    														0);
    	}

    	if (TileAt(x+0, y+1).Block & BLOCKWERT_DESTRUCTIBLE)
    	{
    		TileAt(x+0, y+1).ColorOverlay[0] = D3DCOLOR_RGBA(TileAt(x+1, y+0).Red,
    													    TileAt(x+1, y+0).Green,
    													    TileAt(x+1, y+0).Blue,
    													    0);
    		TileAt(x+0, y+1).ColorOverlay[1] = D3DCOLOR_RGBA(TileAt(x+1, y+0).Red,
    											   			TileAt(x+1, y+0).Green,
    														TileAt(x+1, y+0).Blue,
    														0);
    	}

    	if (TileAt(x+0, y-1).Block & BLOCKWERT_DESTRUCTIBLE)
    	{
    		TileAt(x+0, y-1).ColorOverlay[2] = D3DCOLOR_RGBA(TileAt(x+1, y+0).Red,
    														TileAt(x+1, y+0).Green,
    														TileAt(x+1, y+0).Blue,
    														0);
    		TileAt(x+0, y-1).ColorOverlay[3] = D3DCOLOR_RGBA(TileAt(x+1, y+0).Red,
    														TileAt(x+1, y+0).Green,
    														TileAt(x+1, y+0).Blue,
    														0);
    	}*/
}

//DKS - After discovering some particles (LONGFUNKE particles emitted when spreadshot projectiles
//      hit walls) get lodged inside wall tiles and stay there (only visible when drawing of 
//      overlay tile layers is disabled), I had a look at the Block* functions below.
//      It was clear that they were sometimes accessing the Tiles[][] array out-of-bounds, and
//      I confirmed this through use of a new TileAt() function that does range-checking in
//      debug-mode. Furthermore, the code was messy, returned int type and not uint32_t,
//      and looped for no discernable reason. Also, many of parameters were marked as
//      references, when only a few in any given function were ever modified.
//      I have rewritten them, addressing all these issues. I have left the original functions
//      here as a reference, inside the '#if 0' block. New ones are after.
//      The ResolveLinks() function that was in the middle of these was never used anywhere,
//      and I've commented it out and made no replacement.
#if 0  // BEGIN COPY OF ORIGINAL VERSIONS THAT HAVE BEEN REWRITTEN
// --------------------------------------------------------------------------------------
// Zur�ckliefern, welche BlockArt sich Rechts vom �bergebenen Rect befindet
// --------------------------------------------------------------------------------------

int	TileEngineClass::BlockRechts(float &x, float &y, float  &xo, float &yo, RECT rect, bool resolve)
{
    // Nach rechts muss nicht gecheckt werden ?
    if (xo > x)
        return 0;


    // Ausserhalb des Arrays ?
    if (x < 0 ||
            x > LEVELPIXELSIZE_X ||
            y < 0 ||
            y > LEVELPIXELSIZE_Y)
        return 0;

    int Art	= 0;
    int xlevel;
    int ylevel;
    int laenge;

    //laenge = abs(int(xs SYNC))+1;
    laenge = 5;

    for (int l=0; l<laenge && Art == 0; l++)
    {
        for(int i=rect.top; i<rect.bottom; i+= TILESIZE_Y)
        {
            xlevel = (unsigned int)((x+rect.right+1) * 0.05f);
            ylevel = (unsigned int)((y+i) * 0.05f);

            if (TileAt(xlevel, ylevel).Block > 0 && !(Art & BLOCKWERT_WAND))
                Art = TileAt(xlevel, ylevel).Block;

            if (Art & BLOCKWERT_WAND)
            {
                if (resolve)
                {
                    x = (float)(xlevel*TILESIZE_X)-rect.right-1;
                    xo = x;
                }

                return Art;
            }
        }
    }

    return Art;
}

// --------------------------------------------------------------------------------------
// Zur�ckliefern, ob sich rechts eine zerst�rbare Wand befindet
// --------------------------------------------------------------------------------------

bool TileEngineClass::BlockDestroyRechts(float &x, float &y, float  &xo, float &yo, RECT rect)
{
    // Nach rechts muss nicht gecheckt werden ?
    if (xo > x)
        return 0;

    int xlevel;
    int ylevel;
    int laenge;

    if (x < 0 || y < 0 ||
            x > LEVELPIXELSIZE_X ||
            y > LEVELPIXELSIZE_Y)
        return false;

    //laenge = abs(int(xs SYNC))+1;
    laenge = 5;

    for (int l=0; l<laenge; l++)
    {
        for(int i=rect.top; i<rect.bottom; i+= TILESIZE_Y)
        {
            xlevel = (unsigned int)((x+rect.right+1) * 0.05f);
            ylevel = (unsigned int)((y+i) * 0.05f);

            if (TileAt(xlevel, ylevel).Block & BLOCKWERT_DESTRUCTIBLE)
            {
                ExplodeWall(xlevel, ylevel);
                return true;
            }
        }
    }

    return false;
}

//DKS - Never used anywhere, so I've not made a replacement version:
void TileEngineClass::ResolveLinks (float &x, float &y, float &xo, float &yo, RECT rect)
{
    float laenge;
//	float xr, yr;
    Vector2D ppos;
    Vector2D pdir;
//	Vector2D lpos;
//	Vector2D ldir;

    for (laenge = y + rect.top; laenge < y + rect.bottom; laenge += TILESIZE_Y)
    {
        ppos.x = x;
        ppos.y = laenge;
        pdir.x = x - xo;
        pdir.y = y - yo;


        D3DXVECTOR2 p1, p2;
        p1.x = ppos.x;
        p1.y = ppos.y;
        p2.x = p1.x + pdir.x;
        p2.y = p1.x + pdir.x;
        RenderLine(p1, p2, 0xFFFFFFFF);
    }
}

// --------------------------------------------------------------------------------------
// Zur�ckliefern, welche BlockArt sich Links vom �bergebenen Rect befindet
// --------------------------------------------------------------------------------------

int	TileEngineClass::BlockLinks(float &x, float &y, float &xo, float &yo, RECT rect, bool resolve)
{
    // Nach links muss nicht gecheckt werden ?
    if (xo < x ||
            x  < 0 ||
            y  < 0)
        return 0;

    // Ausserhalb des Arrays ?
    if (x < 0 ||
            x > LEVELPIXELSIZE_X ||
            y < 0 ||
            y > LEVELPIXELSIZE_Y)
        return 0;

    int Art	= 0;
    int xlevel;
    int ylevel;
    int laenge;

    //laenge = abs(int(xs SYNC))+1;
    laenge = 5;

    for (int l=0; l<laenge && Art == 0; l++)
    {
        for(int i=rect.top; i<rect.bottom; i+=TILESIZE_Y)
        {
            xlevel = (unsigned int)((x+rect.left-1) * 0.05f);
            ylevel = (unsigned int)((y+i) * 0.05f);

            if (TileAt(xlevel, ylevel).Block > 0 && !(Art & BLOCKWERT_WAND))
                Art = TileAt(xlevel, ylevel).Block;

            if (Art & BLOCKWERT_WAND)
            {
                if (resolve)
                {
                    x = (float)(xlevel*TILESIZE_X) + TILESIZE_X - rect.left;
                    xo = x;
                }
                return Art;
            }
        }
    }

    return Art;
}

// --------------------------------------------------------------------------------------
// Zur�ckliefern, ob sich links eine zerst�rbare Wand befindet
// --------------------------------------------------------------------------------------

bool TileEngineClass::BlockDestroyLinks(float &x, float &y, float &xo, float &yo, RECT rect)
{
    // Nach links muss nicht gecheckt werden ?
    if (xo < x ||
            x  < 0 ||
            y  < 0)
        return 0;

    // Ausserhalb des Arrays ?
    if (x < 0 ||
            x > LEVELPIXELSIZE_X ||
            y < 0 ||
            y > LEVELPIXELSIZE_Y)
        return 0;

    int xlevel;
    int ylevel;
    int laenge;

    //laenge = abs(int(xs SYNC))+1;
    laenge = 5;

    for (int l=0; l<laenge; l++)
    {
        for(int i=rect.top; i<rect.bottom; i+=TILESIZE_Y)
        {
            xlevel = (unsigned int)((x+rect.left-1) * 0.05f);
            ylevel = (unsigned int)((y+i) * 0.05f);

            if (TileAt(xlevel, ylevel).Block & BLOCKWERT_DESTRUCTIBLE)
            {
                ExplodeWall(xlevel, ylevel);
                return true;
            }
        }
    }

    return false;
}

// --------------------------------------------------------------------------------------
// Zur�ckliefern, welche BlockArt sich oberhalb vom �bergebenen Rect befindet
// --------------------------------------------------------------------------------------

int	TileEngineClass::BlockOben(float &x, float &y, float &xo, float &yo, RECT rect, bool resolve)
{
    // Nach oben muss nicht gecheckt werden ?
    if (yo < y ||
            x  < 0 ||
            y  < 0)
        return 0;

    // Ausserhalb des Arrays ?
    if (x < 0 ||
            x > LEVELPIXELSIZE_X ||
            y < 0 ||
            y > LEVELPIXELSIZE_Y)
        return 0;

    int Art	= 0;
    int xlevel;
    int ylevel;
    int laenge;

    //laenge = abs(int(yo - y))+1;
    laenge = 5;

    for (int l=0; l<laenge && Art == 0; l++)
    {
        for(int i=rect.left; i<rect.right; i+=TILESIZE_X)
        {
            xlevel = (unsigned int)((x+i)   * 0.05f);
            ylevel = (unsigned int)((y+rect.top-l-1) * 0.05f);

            if (TileAt(xlevel, ylevel).Block > 0 && !(Art & BLOCKWERT_WAND))
                Art = TileAt(xlevel, ylevel).Block;

            if (Art & BLOCKWERT_WAND)
            {
                if (resolve)
                {
                    y  = (float)(ylevel*TILESIZE_Y + 21 - rect.top);
                    yo = y;
                }

                return Art;
            }
        }
    }

    return Art;
}

// --------------------------------------------------------------------------------------
// Zur�ckliefern, ob sich oben eine zerst�rbare Wand befindet
// --------------------------------------------------------------------------------------

bool TileEngineClass::BlockDestroyOben(float &x, float &y, float &xo, float &yo, RECT rect)
{
    // Nach oben muss nicht gecheckt werden ?
    if (yo < y ||
            x  < 0 ||
            y  < 0)
        return 0;

    if (x < 0 || y < 0 ||
            x > LEVELPIXELSIZE_X ||
            y > LEVELPIXELSIZE_Y)
        return false;

    int xlevel;
    int ylevel;
    int laenge;

    //laenge = abs(int(yo - y))+1;
    laenge = 5;

    for (int l=0; l<laenge; l++)
    {
        for(int i=rect.left; i<rect.right; i+=TILESIZE_X)
        {
            xlevel = (unsigned int)((x+i)   * 0.05f);
            ylevel = (unsigned int)((y+rect.top-l-1) * 0.05f);

            if (TileAt(xlevel, ylevel).Block & BLOCKWERT_DESTRUCTIBLE)
            {
                ExplodeWall(xlevel, ylevel);
                return true;
            }
        }
    }

    return false;
}

// --------------------------------------------------------------------------------------
// Zur�ckliefern, welche BlockArt sich unterhalb vom �bergebenen Rect befindet
// und dabei nicht "begradigen" sprich die y-Position an das Tile angleichen
// --------------------------------------------------------------------------------------

int	TileEngineClass::BlockUntenNormal(float &x, float &y, float &xo, float &yo, RECT rect)
{
    // Nach unten muss nicht gecheckt werden ?
    if (yo > y ||
            x  < 0 ||
            y  < 0)
        return 0;

    // Ausserhalb des Arrays ?
    if (x < 0 ||
            x > LEVELPIXELSIZE_X ||
            y < 0 ||
            y > LEVELPIXELSIZE_Y)
        return 0;

    int Art	= 0;
    int xlevel;
    int ylevel;
    int laenge;

    laenge = int(y - yo)+1;
    laenge = 5;

    for (int l=0; l<laenge && Art == 0; l+=20)
    {
        for(int i=rect.left+1; i<rect.right-1; i++)
        {
            xlevel = (unsigned int)((x+i) * 0.05f);
            ylevel = (unsigned int)((y+rect.bottom+l+1) * 0.05f);

            if (TileAt(xlevel, ylevel).Block > 0 && !(Art & BLOCKWERT_WAND))
                Art = TileAt(xlevel, ylevel).Block;

            if (Art & BLOCKWERT_WAND ||
                    Art & BLOCKWERT_PLATTFORM)
                return Art;
        }
    }

    return Art;
}

// --------------------------------------------------------------------------------------
// Zur�ckliefern, welche BlockArt sich unterhalb vom �bergebenen Rect befindet
// --------------------------------------------------------------------------------------

int	TileEngineClass::BlockUnten(float &x, float &y, float &xo, float &yo, RECT rect, bool resolve)
{
    // Nach unten muss nicht gecheckt werden ?
    // Nach unten muss nicht gecheckt werden ?
    if (yo > y ||
            x  < 0 ||
            y  < 0)
        return 0;

    // Ausserhalb des Arrays ?
    if (x < 0 ||
            x > LEVELPIXELSIZE_X ||
            y < 0 ||
            y > LEVELPIXELSIZE_Y)
        return 0;


    int  Art	= 0;
    int  xlevel;
    int  ylevel;
    int  laenge;
    //bool checkagain = true;

    //laenge = int(y - yo)+1;
    laenge = 5;

    for (int l=0; l<laenge && Art == 0; l+=20)
    {
        for(int i=rect.left+1; i<rect.right-1; i++)
        {
            xlevel = (unsigned int)((x+i) * 0.05f);
            ylevel = (unsigned int)((y+rect.bottom+l+1) * 0.05f);

            if (!(Art & BLOCKWERT_WAND) &&
                    !(Art & BLOCKWERT_PLATTFORM) &&
                    TileAt(xlevel, ylevel).Block > 0)
                Art = TileAt(xlevel, ylevel).Block;

            if (Art & BLOCKWERT_WAND ||
                    Art & BLOCKWERT_PLATTFORM)
            {
//				while ((TileAt(xlevel, ylevel - 1).Block & BLOCKWERT_WAND) &&
//					    yLevel > 0)
//					ylevel--;

                if (resolve == true)
                {
                    // resolven
                    y = (float)(ylevel*TILESIZE_Y-rect.bottom);
                    yo = y;
                }

                return Art;
            }
        }
    }

    return Art;
}

// --------------------------------------------------------------------------------------
// Zur�ckliefern, welche BlockArt sich unterhalb vom �bergebenen Rect befindet
// --------------------------------------------------------------------------------------

bool TileEngineClass::BlockDestroyUnten(float &x, float &y, float &xo, float &yo, RECT rect)
{
    // Nach unten muss nicht gecheckt werden ?
    // Nach unten muss nicht gecheckt werden ?
    if (yo > y ||
            x  < 0 ||
            y  < 0)
        return 0;

    if (x < 0 || y < 0 ||
            x > LEVELPIXELSIZE_X ||
            y > LEVELPIXELSIZE_Y)
        return false;

    int  xlevel;
    int  ylevel;
    int  laenge;
    //bool checkagain = true;

    //laenge = int(y - yo)+1;
    laenge = 5;

    // so lange ausf�hren, bis kein Block mehr unter den F��en ist
    //do
    for (int l=0; l<laenge; l++)
    {
        for(int i=rect.left+1; i<rect.right-1; i+=TILESIZE_X)
        {
            xlevel = (unsigned int)((x+i) * 0.05f);
            ylevel = (unsigned int)((y+rect.bottom+l+1) * 0.05f);

            if (TileAt(xlevel, ylevel).Block & BLOCKWERT_DESTRUCTIBLE)
            {
                ExplodeWall(xlevel, ylevel);
                return true;
            }
        }
    }

    return false;
}
#endif //0
// DKS - BEGIN REWRITTEN VERSIONS OF ABOVE FUNCTIONS:
// --------------------------------------------------------------------------------------
// Zur�ckliefern, welche BlockArt sich Rechts vom �bergebenen Rect befindet
// --------------------------------------------------------------------------------------

uint32_t TileEngineClass::BlockRechts(float &x, float y, float &xo, float yo, RECT rect, bool resolve)
{
    // Nach rechts muss nicht gecheckt werden ?
    if ( xo > x )
        return 0;

    int xlev = (int)((x+rect.right+1) * (1.0f/TILESIZE_X));
    if (xlev < 0 || xlev >= LEVELSIZE_X)
        return 0;

    uint32_t block = 0;

    for(int j=rect.top; j<rect.bottom; j+=TILESIZE_Y) {
        int ylev = (int)((y+j) * (1.0f/TILESIZE_Y));

        if (ylev < 0)
            continue;
        else if (ylev >= LEVELSIZE_Y)
            return 0;

        if (TileAt(xlev, ylev).Block != 0 && !(block & BLOCKWERT_WAND))
            block = TileAt(xlev, ylev).Block;

        if (block & BLOCKWERT_WAND) {
            if (resolve) {
                x = (float)((xlev*TILESIZE_X)-rect.right-1);
                xo = x;
            }

            return block;
        }
    }

    return block;
}

// --------------------------------------------------------------------------------------
// Zur�ckliefern, ob sich rechts eine zerst�rbare Wand befindet
// --------------------------------------------------------------------------------------

bool TileEngineClass::BlockDestroyRechts(float x, float y, float xo, float yo, RECT rect)
{
    // Nach rechts muss nicht gecheckt werden ?
    if (xo > x)
        return false;

    int xlev = (int)((x+rect.right+1) * (1.0f/TILESIZE_X));
    if (xlev < 0 || xlev >= LEVELSIZE_X)
        return false;

    for(int j=rect.top; j<rect.bottom; j+= TILESIZE_Y) {
        int ylev = (int)((y+j) * (1.0f/TILESIZE_Y));

        if (ylev < 0)
            continue;
        else if (ylev >= LEVELSIZE_Y)
            return false;

        if (TileAt(xlev, ylev).Block & BLOCKWERT_DESTRUCTIBLE) {
            ExplodeWall(xlev, ylev);
            return true;
        }
    }

    return false;
}

// --------------------------------------------------------------------------------------
// Zur�ckliefern, welche BlockArt sich Links vom �bergebenen Rect befindet
// --------------------------------------------------------------------------------------

uint32_t TileEngineClass::BlockLinks(float &x, float y, float &xo, float yo, RECT rect, bool resolve)
{
    // Nach links muss nicht gecheckt werden ?
    if (xo < x)
        return 0;

    int xlev = (int)((x+rect.left-1) * (1.0f/TILESIZE_X));
    if (xlev < 0 || xlev >= LEVELSIZE_X)
        return 0;

    uint32_t block = 0;

    for(int j=rect.top; j<rect.bottom; j+=TILESIZE_Y) {
        int ylev = (int)((y+j) * (1.0f/TILESIZE_Y));

        if (ylev < 0)
            continue;
        else if (ylev >= LEVELSIZE_Y)
            return 0;

        if (TileAt(xlev, ylev).Block != 0 && !(block & BLOCKWERT_WAND))
            block = TileAt(xlev, ylev).Block;

        if (block & BLOCKWERT_WAND) {
            if (resolve) {
                x = (float)((xlev*TILESIZE_X) + TILESIZE_X - rect.left);
                xo = x;
            }
            return block;
        }
    }

    return block;
}

// --------------------------------------------------------------------------------------
// Zur�ckliefern, ob sich links eine zerst�rbare Wand befindet
// --------------------------------------------------------------------------------------

bool TileEngineClass::BlockDestroyLinks(float x, float y, float xo, float yo, RECT rect)
{
    // Nach links muss nicht gecheckt werden ?
    if (xo < x)
        return false;

    int xlev = (int)((x+rect.left-1) * (1.0f/TILESIZE_X));
    if (xlev < 0 || xlev >= LEVELSIZE_X)
        return false;

    for(int j=rect.top; j<rect.bottom; j+=TILESIZE_Y) {
        int ylev = (int)((y+j) * (1.0f/TILESIZE_Y));

        if (ylev < 0)
            continue;
        else if (ylev >= LEVELSIZE_Y)
            return false;

        if (TileAt(xlev, ylev).Block & BLOCKWERT_DESTRUCTIBLE) {
            ExplodeWall(xlev, ylev);
            return true;
        }
    }

    return false;
}

// --------------------------------------------------------------------------------------
// Zur�ckliefern, welche Blockblock sich oberhalb vom �bergebenen Rect befindet
// --------------------------------------------------------------------------------------

uint32_t TileEngineClass::BlockOben(float x, float &y, float xo, float &yo, RECT rect, bool resolve)
{
    // Nach oben muss nicht gecheckt werden ?
    if (yo < y)
        return 0;

    int ylev = (int)((y+rect.top-1) * (1.0f/TILESIZE_Y));
    if (ylev < 0 || ylev >= LEVELSIZE_Y)
        return 0;

    uint32_t block = 0;

    for(int i=rect.left; i<rect.right; i+=TILESIZE_X) {
        int xlev = (int)((x+i) * (1.0f/TILESIZE_X));
        if (xlev < 0)
            continue;
        else if (xlev >= LEVELSIZE_X)
            return 0;

        if (TileAt(xlev, ylev).Block > 0 && !(block & BLOCKWERT_WAND))
            block = TileAt(xlev, ylev).Block;

        if (block & BLOCKWERT_WAND) {
            if (resolve) {
                y  = (float)((ylev*TILESIZE_Y) + TILESIZE_Y - rect.top);
                yo = y;
            }

            return block;
        }
    }

    return block;
}

// --------------------------------------------------------------------------------------
// Zur�ckliefern, ob sich oben eine zerst�rbare Wand befindet
// --------------------------------------------------------------------------------------

bool TileEngineClass::BlockDestroyOben(float x, float y, float xo, float yo, RECT rect)
{
    // Nach oben muss nicht gecheckt werden ?
    if (yo < y)
        return false;

    int ylev = (int)((y+rect.top-1) * (1.0f/TILESIZE_Y));
    if (ylev < 0 || ylev >= LEVELSIZE_Y)
        return false;

    for(int i=rect.left; i<rect.right; i+=TILESIZE_X) {
        int xlev = (int)((x+i) * (1.0f/TILESIZE_X));

        if (xlev < 0)
            continue;
        else if (xlev >= LEVELSIZE_X)
            return false;

        if (TileAt(xlev, ylev).Block & BLOCKWERT_DESTRUCTIBLE) {
            ExplodeWall(xlev, ylev);
            return true;
        }
    }

    return false;
}

// --------------------------------------------------------------------------------------
// Zur�ckliefern, welche Blockblock sich unterhalb vom �bergebenen Rect befindet
// und dabei nicht "begradigen" sprich die y-Position an das Tile angleichen
// --------------------------------------------------------------------------------------

uint32_t TileEngineClass::BlockUntenNormal(float x, float y, float xo, float yo, RECT rect)
{
    // Nach unten muss nicht gecheckt werden ?
    if (yo > y)
        return false;

    int ylev = (int)((y+rect.bottom+1) * (1.0f/TILESIZE_Y));
    if (ylev < 0 || ylev >= LEVELSIZE_Y)
        return 0;

    uint32_t block = 0;

    // BIG TODO: see if you can make this increment by TILESIZE_X
    for(int i=rect.left; i<rect.right; i++) {
        int xlev = (int)((x+i) * (1.0f/TILESIZE_X));

        if (xlev < 0)
            continue;
        else if (xlev >= LEVELSIZE_X)
            return 0;

        //DKS - TODO: might be optimized a bit:
        if (TileAt(xlev, ylev).Block > 0 && !(block & BLOCKWERT_WAND))
            block = TileAt(xlev, ylev).Block;

        if (block & BLOCKWERT_WAND || block & BLOCKWERT_PLATTFORM)
            return block;
    }

    return block;
}

// --------------------------------------------------------------------------------------
// Zur�ckliefern, welche Blockblock sich unterhalb vom �bergebenen Rect befindet
// --------------------------------------------------------------------------------------

uint32_t TileEngineClass::BlockUnten(float x, float &y, float xo, float &yo, RECT rect, bool resolve)
{
    // Nach unten muss nicht gecheckt werden ?
    if (yo > y)
        return 0;

    int ylev = (int)((y+rect.bottom+1) * (1.0f/TILESIZE_Y));
    if (ylev < 0 || ylev >= LEVELSIZE_Y)
        return 0;

    uint32_t block = 0;

    // BIG TODO: see if you can make this increment by TILESIZE_X
    for(int i=rect.left; i<rect.right; i++) {
        int xlev = (int)((x+i) * (1.0f/TILESIZE_X));

        if (xlev < 0)
            continue;
        else if (xlev >= LEVELSIZE_X)
            return 0;

        //DKS - TODO: might be optimized a bit
        if (!(block & BLOCKWERT_WAND) && !(block & BLOCKWERT_PLATTFORM) &&
                TileAt(xlev, ylev).Block > 0)
            block = TileAt(xlev, ylev).Block;

        if (block & BLOCKWERT_WAND || block & BLOCKWERT_PLATTFORM) {
            if (resolve) {
                y = (float)(ylev*TILESIZE_Y-rect.bottom);
                yo = y;
            }

            return block;
        }
    }

    return block;
}

// --------------------------------------------------------------------------------------
// Zur�ckliefern, welche Blockblock sich unterhalb vom �bergebenen Rect befindet
// --------------------------------------------------------------------------------------

bool TileEngineClass::BlockDestroyUnten(float x, float y, float xo, float yo, RECT rect)
{
    // Nach unten muss nicht gecheckt werden ?
    if (yo > y)
        return 0;

    int ylev = (int)((y+rect.bottom+1) * (1.0f/TILESIZE_Y));
    if (ylev < 0 || ylev >= LEVELSIZE_Y)
        return false;

    for(int i=rect.left; i<rect.right; i+=TILESIZE_X) {
        int xlev = (int)((x+i) * (1.0f/TILESIZE_X));

        if (xlev < 0)
            continue;
        else if (xlev >= LEVELSIZE_X)
            return 0;

        if (TileAt(xlev, ylev).Block & BLOCKWERT_DESTRUCTIBLE) {
            ExplodeWall(xlev, ylev);
            return true;
        }
    }

    return false;
}
//DKS - END BLOCK OF NEW FUNCTIONS

// --------------------------------------------------------------------------------------
// Auf Schr�gen pr�fen
// --------------------------------------------------------------------------------------
//DKS - Rewrote this function to not access Tiles[][] array out of bounds, and to be
//      a bit more efficient and clean. It also now only take parameters that it actually
//      needs (xo/yo params were unused) and x parameter didn't need to be a reference.
//      It returns the correct type for Block data as well (uint32_t vs int).
// ORIGINAL CODE:
#if 0
int	TileEngineClass::BlockSlopes(float &x, float &y, float &xo, float &yo, RECT rect, float ySpeed, bool resolve)
{
    int Art	= 0;
    int xlevel;
    int ylevel;
    //int laenge;

    //laenge = int(y - yo)+1;
    //laenge = 5;

    for(int j = rect.bottom; j<rect.bottom+TILESIZE_Y; j++)
    {
        // Schr�ge links
        // von links anfangen mit der Block-Pr�dung
        //
        for(int i = rect.left; i<rect.right; i+=1)
        {
            xlevel = int ((x + i) * 0.05f);
            ylevel = int ((y + j - 1) * 0.05f);

            Art = TileAt(xlevel, ylevel).Block;

            if (Art & BLOCKWERT_SCHRAEGE_L)
            {
                float newy = (ylevel+1) * TILESIZE_Y - rect.bottom - (TILESIZE_Y - float (int (x + i) % TILESIZE_X)) - 1;

                {
                    if (ySpeed == 0.0f ||
                            y > newy)
                    {
                        y = newy;
                        return Art;
                    }
                }
            }
        }

        // Schr�ge rechts
        // von rechts anfangen mit der Block-Pr�dung
        //
        for(int i = rect.right; i>rect.left; i-=1)
        {
            xlevel = int ((x + i) * 0.05f);
            ylevel = int ((y + j - 1) * 0.05f);

            Art = TileAt(xlevel, ylevel).Block;


            if (Art & BLOCKWERT_SCHRAEGE_R)
            {
                float newy = (ylevel+1) * TILESIZE_Y - rect.bottom - (float (int (x + i) % TILESIZE_X)) - 1;

                {
                    if (ySpeed == 0.0f ||
                            y > newy)
                    {
                        y = newy;
                        return Art;
                    }
                }
            }
        }
    }

    return 0;
}
#endif //0
//DKS - Rewritten version of above function:
uint32_t TileEngineClass::BlockSlopes(const float x, float &y, const RECT rect, const float ySpeed)
{
    uint32_t block = 0;
    for(int j = rect.bottom; j<rect.bottom+TILESIZE_Y; j++)
    {
        int ylev = int ((y + (j - 1)) * (1.0f/TILESIZE_Y));

        if (ylev < 0)
           continue;
        else if (ylev >= LEVELSIZE_Y)
            return 0;

        // Schr�ge links
        // von links anfangen mit der Block-Pr�dung

        //DKS - TODO see if you can get this to increment faster, by tile:
        for(int i = rect.left; i<rect.right; i++) {
            int xlev = int ((x + i) * (1.0f/TILESIZE_X));

            if (xlev < 0)
                continue;
            else if (xlev >= LEVELSIZE_X)
                break;

            block = TileAt(xlev, ylev).Block;

            if (block & BLOCKWERT_SCHRAEGE_L) {
                float newy = (ylev+1) * TILESIZE_Y - rect.bottom - (TILESIZE_Y - float (int (x + i) % TILESIZE_X)) - 1;
                if (ySpeed == 0.0f || y > newy) {
                    y = newy;
                    return block;
                }
            }
        }

        // Schr�ge rechts
        // von rechts anfangen mit der Block-Pr�dung

        //DKS TODO: try to get this to decrement faster, by tile:
        for(int i = rect.right; i>rect.left; i--) {
            int xlev = int ((x + i) * (1.0f/TILESIZE_X));

            if (xlev >= LEVELSIZE_X)
                continue;
            else if (xlev < 0)
                break;

            block = TileAt(xlev, ylev).Block;

            if (block & BLOCKWERT_SCHRAEGE_R) {
                float newy = (ylev+1) * TILESIZE_Y - rect.bottom - (float (int (x + i) % TILESIZE_X)) - 1;
                if (ySpeed == 0.0f || y > newy) {
                    y = newy;
                    return block;
                }
            }
        }
    }

    return 0;
}

// --------------------------------------------------------------------------------------
// Checken ob ein Schuss eine zerst�rbare Wand getroffen hat und wenn ja, diese
// zerst�ren und true zur�ckliefern, damit der Schuss ebenfalls gel�scht wird
// --------------------------------------------------------------------------------------
//DKS - x,y parameters did not need to be references and xs,ys params were never
//      anything but 0 in the only place this function was used, so I eliminated
//      them as they had no effect when 0. Rewrote code to be clearer and also
//      not allow out-of-bounds access to Tiles[][] array. Before, it was possible
//      this happened when, say, loading the 2nd map and immediately firing the
//      blitz-beam to the left off the edge of the screen. Also, made inline.
// ORIGINAL VERSION:
#if 0
bool TileEngineClass::CheckDestroyableWalls(float &x, float &y, float xs, float ys, RECT rect)
{
    int	  xstart, ystart;
    int   xl, yl;

    xstart = int((x+xs SYNC) * 0.05f);
    ystart = int((y+ys SYNC) * 0.05f);

    // Ausserhalb vom Level?
    //
    if (x < 0 || y < 0 ||
            x > LEVELPIXELSIZE_X ||
            y > LEVELPIXELSIZE_Y)
        return false;

    xl = int(rect.right )/TILESIZE_X+2;
    yl = int(rect.bottom)/TILESIZE_Y+2;

    // Ganzes Rect des Schusses pr�fen
    for (int i=xstart-1; i<xstart + xl; i++)
        for (int j=ystart; j<ystart + yl; j++)
            if (TileAt(i, j).Block & BLOCKWERT_DESTRUCTIBLE)
            {
                ExplodeWall(i, j);
                return true;
            }

    return false;
}
#endif //0
//DKS - Rewritten version of above function:
bool TileEngineClass::CheckDestroyableWalls(float x, float y, float xs, float ys, RECT rect)
{
    int xstart = int(x * (1.0f/TILESIZE_X));
    int ystart = int(y * (1.0f/TILESIZE_Y));
    int xl = int(rect.right )/TILESIZE_X+2;
    int yl = int(rect.bottom)/TILESIZE_Y+2;

    // Ganzes Rect des Schusses pr�fen
    for (int i=xstart-1; i < xstart+xl; i++) {
        if (i < 0)
            continue;
        else if (i >= LEVELSIZE_X)
            break;

        for (int j=ystart; j < ystart+yl; j++) {
            if (j < 0)
                continue;
            else if (j >= LEVELSIZE_Y)
                break;

            if (TileAt(i, j).Block & BLOCKWERT_DESTRUCTIBLE) {
                ExplodeWall(i, j);
                return true;
            }
        }
    }

    return false;
}

// --------------------------------------------------------------------------------------
// Zur�ckliefern, welcher Farbwert sich in der Mitte des RECTs an x/y im Level befindet
// damit die Gegner entsprechend dem Licht im Editor "beleuchtet" werden
// --------------------------------------------------------------------------------------

D3DCOLOR TileEngineClass::LightValue(float x, float y, RECT rect, bool forced)
{
    D3DCOLOR		Color;
    int	 			xLevel, yLevel;
    unsigned  int	r, g, b;

    xLevel = int((x + (rect.right  - rect.left) / 2) /TILESIZE_X);		// xPosition im Level
    yLevel = int((y + (rect.bottom - rect.top)  / 2) /TILESIZE_Y);		// yPosition im Level

    //DKS - Added check for xLevel,yLevel being in bounds of levels' dimensions, also,
    //      check forced==false before blindly looking up block value:
    //if (!(TileAt(xLevel, yLevel).Block & BLOCKWERT_LIGHT) &&		// Soll das Leveltile garnicht
    //        forced == false)
    //    return 0xFFFFFFFF;										// das Licht des Objektes �ndern
    if ( (xLevel >= LEVELSIZE_X || yLevel >= LEVELSIZE_Y) ||
         (forced == false && !(TileAt(xLevel, yLevel).Block & BLOCKWERT_LIGHT)) )		// Soll das Leveltile garnicht
        return 0xFFFFFFFF;										// das Licht des Objektes �ndern

    r = TileAt(xLevel, yLevel).Red;
    g = TileAt(xLevel, yLevel).Green;
    b = TileAt(xLevel, yLevel).Blue;

    r += 48;				// Farbewerte ein wenig erh�hen, damit man selbst bei 0,0,0
    g += 48;				// noch ein wenig was sehen kann und das Sprite nicht
    b += 48;				// KOMPLETT schwarz wird ... minimum ist 48, 48, 48

    if (r > 255) r = 255;	// Grenzen �berschritten ? Dann angleichen
    if (g > 255) g = 255;
    if (b > 255) b = 255;

    Color = D3DCOLOR_RGBA(r, g, b, 255);

    return Color;
}

// --------------------------------------------------------------------------------------
// Anf�nglich verwendete Lichtberechnung herstellen
// Jedes Tile hat an allen vier Ecken die selbe Farbe -> Keine �berg�nge -> Scheisse
// --------------------------------------------------------------------------------------

//DKS - This function was never actually used in the original game and is now disabled:
#if 0
void TileEngineClass::ComputeShitLight (void)
{
    for (int i=0; i< LEVELSIZE_X; i += 1)
        for (int j=0; j<LEVELSIZE_Y; j += 1)
        {
            TileAt(i, j).Color[0] =
                TileAt(i, j).Color[1] =
                TileAt(i, j).Color[2] =
                TileAt(i, j).Color[3] = D3DCOLOR_RGBA(TileAt(i, j).Red,
                        TileAt(i, j).Green,
                        TileAt(i, j).Blue,
                        TileAt(i, j).Alpha);
        }
} // ComputeShitLight
#endif //0

// --------------------------------------------------------------------------------------
// Neue Lichtberechnung
// Jedes Tile hat an allen vier Ecken eine interpolierte Farbe
// entsprechend der umliegenden Tiles -> smoothe �berg�nge -> Geilomat!
// --------------------------------------------------------------------------------------

void TileEngineClass::ComputeCoolLight (void)
{
    // Lichter im Level interpolieren
    // Dabei werden die Leveltiles in 2er Schritten durchgegangen
    // Dann werden die 4 Ecken des aktuellen Tiles auf die Farben der Nachbarfelder gesetzt
    // Farben der Nachbarfelder werden allerdings nur mit verrechnet, wenn es sich nicht um eine massive Wand handelt.
    // In diesem Falle wird die Standard-Tilefarbe verwendet
    //
    int rn, gn, bn, r1, r2, r3, r4, g1, g2, g3, g4, b1, b2 ,b3, b4, al;

    for (int i = 1; i < LEVELSIZE_X - 1; i += 1)
        for (int j = 1; j < LEVELSIZE_Y - 1; j += 1)
        {
            al = TileAt(i, j).Alpha;

            // Ecke links oben
            //
            if ((!(TileAt(i-1, j-1).Block & BLOCKWERT_WAND)  && !(TileAt(i, j).Block & BLOCKWERT_WAND)) ||
                    (TileAt(i-1, j-1).Block & BLOCKWERT_WAND   &&   TileAt(i, j).Block & BLOCKWERT_WAND))
            {
                r1 = TileAt(i-1, j-1).Red;
                g1 = TileAt(i-1, j-1).Green;
                b1 = TileAt(i-1, j-1).Blue;
            }
            else
            {
                r1 = TileAt(i, j).Red;
                g1 = TileAt(i, j).Green;
                b1 = TileAt(i, j).Blue;
            }

            if ((!(TileAt(i+0, j-1).Block & BLOCKWERT_WAND) && !(TileAt(i, j).Block & BLOCKWERT_WAND)) ||
                    (TileAt(i+0, j-1).Block & BLOCKWERT_WAND  &&   TileAt(i, j).Block & BLOCKWERT_WAND))
            {
                r2 = TileAt(i+0, j-1).Red;
                g2 = TileAt(i+0, j-1).Green;
                b2 = TileAt(i+0, j-1).Blue;
            }
            else
            {
                r2 = TileAt(i, j).Red;
                g2 = TileAt(i, j).Green;
                b2 = TileAt(i, j).Blue;
            }

            if ((!(TileAt(i-1, j+0).Block & BLOCKWERT_WAND) && !(TileAt(i, j).Block & BLOCKWERT_WAND)) ||
                    (TileAt(i-1, j+0).Block & BLOCKWERT_WAND  &&  TileAt(i, j).Block & BLOCKWERT_WAND))
            {
                r3 = TileAt(i-1, j+0).Red;
                g3 = TileAt(i-1, j+0).Green;
                b3 = TileAt(i-1, j+0).Blue;
            }
            else
            {
                r3 = TileAt(i, j).Red;
                g3 = TileAt(i, j).Green;
                b3 = TileAt(i, j).Blue;
            }

            if ((!(TileAt(i, j).Block & BLOCKWERT_WAND) && !(TileAt(i, j).Block & BLOCKWERT_WAND)) ||
                    (TileAt(i, j).Block & BLOCKWERT_WAND  &&   TileAt(i, j).Block & BLOCKWERT_WAND))
            {
                r4 = TileAt(i, j).Red;
                g4 = TileAt(i, j).Green;
                b4 = TileAt(i, j).Blue;
            }
            else
            {
                r4 = TileAt(i, j).Red;
                g4 = TileAt(i, j).Green;
                b4 = TileAt(i, j).Blue;
            }

            rn = int ((r1 + r2 + r3 + r4) / 4.0f);
            gn = int ((g1 + g2 + g3 + g4) / 4.0f);
            bn = int ((b1 + b2 + b3 + b4) / 4.0f);


            TileAt(i, j).Color[0] = D3DCOLOR_RGBA (rn, gn, bn, al);

            // Ecke rechts oben
            //
            if ((!(TileAt(i-0, j-1).Block & BLOCKWERT_WAND) && !(TileAt(i, j).Block & BLOCKWERT_WAND)) ||
                    (TileAt(i-0, j-1).Block & BLOCKWERT_WAND  &&   TileAt(i, j).Block & BLOCKWERT_WAND))
            {
                r1 = TileAt(i-0, j-1).Red;
                g1 = TileAt(i-0, j-1).Green;
                b1 = TileAt(i-0, j-1).Blue;
            }
            else
            {
                r1 = TileAt(i, j).Red;
                g1 = TileAt(i, j).Green;
                b1 = TileAt(i, j).Blue;
            }

            if ((!(TileAt(i+1, j-1).Block & BLOCKWERT_WAND) && !(TileAt(i, j).Block & BLOCKWERT_WAND)) ||
                    (TileAt(i+1, j-1).Block & BLOCKWERT_WAND  &&   TileAt(i, j).Block & BLOCKWERT_WAND))
            {
                r2 = TileAt(i+1, j-1).Red;
                g2 = TileAt(i+1, j-1).Green;
                b2 = TileAt(i+1, j-1).Blue;
            }
            else
            {
                r2 = TileAt(i, j).Red;
                g2 = TileAt(i, j).Green;
                b2 = TileAt(i, j).Blue;
            }

            if ((!(TileAt(i+1, j+0).Block & BLOCKWERT_WAND) && !(TileAt(i, j).Block & BLOCKWERT_WAND)) ||
                    (TileAt(i+1, j+0).Block & BLOCKWERT_WAND  &&   TileAt(i, j).Block & BLOCKWERT_WAND))
            {
                r3 = TileAt(i+1, j+0).Red;
                g3 = TileAt(i+1, j+0).Green;
                b3 = TileAt(i+1, j+0).Blue;
            }
            else
            {
                r3 = TileAt(i, j).Red;
                g3 = TileAt(i, j).Green;
                b3 = TileAt(i, j).Blue;
            }

            if ((!(TileAt(i, j).Block & BLOCKWERT_WAND) && !(TileAt(i, j).Block & BLOCKWERT_WAND)) ||
                    (TileAt(i, j).Block & BLOCKWERT_WAND  &&   TileAt(i, j).Block & BLOCKWERT_WAND))
            {
                r4 = TileAt(i, j).Red;
                g4 = TileAt(i, j).Green;
                b4 = TileAt(i, j).Blue;
            }
            else
            {
                r4 = TileAt(i, j).Red;
                g4 = TileAt(i, j).Green;
                b4 = TileAt(i, j).Blue;
            }

            rn = int ((r1 + r2 + r3 + r4) / 4.0f);
            gn = int ((g1 + g2 + g3 + g4) / 4.0f);
            bn = int ((b1 + b2 + b3 + b4) / 4.0f);

            TileAt(i, j).Color[1] = D3DCOLOR_RGBA (rn, gn, bn, al);

            // Ecke links unten
            //
            if ((!(TileAt(i-1, j-0).Block & BLOCKWERT_WAND) && !(TileAt(i, j).Block & BLOCKWERT_WAND)) ||
                    (TileAt(i-1, j-0).Block & BLOCKWERT_WAND  &&   TileAt(i, j).Block & BLOCKWERT_WAND))
            {
                r1 = TileAt(i-1, j-0).Red;
                g1 = TileAt(i-1, j-0).Green;
                b1 = TileAt(i-1, j-0).Blue;
            }
            else
            {
                r1 = TileAt(i, j).Red;
                g1 = TileAt(i, j).Green;
                b1 = TileAt(i, j).Blue;
            }

            if ((!(TileAt(i-1, j+1).Block & BLOCKWERT_WAND) && !(TileAt(i, j).Block & BLOCKWERT_WAND)) ||
                    (TileAt(i-1, j+1).Block & BLOCKWERT_WAND  &&   TileAt(i, j).Block & BLOCKWERT_WAND))
            {
                r2 = TileAt(i-1, j+1).Red;
                g2 = TileAt(i-1, j+1).Green;
                b2 = TileAt(i-1, j+1).Blue;
            }
            else
            {
                r2 = TileAt(i, j).Red;
                g2 = TileAt(i, j).Green;
                b2 = TileAt(i, j).Blue;
            }

            if ((!(TileAt(i-0, j+1).Block & BLOCKWERT_WAND) && !(TileAt(i, j).Block & BLOCKWERT_WAND)) ||
                    (TileAt(i-0, j+1).Block & BLOCKWERT_WAND  &&   TileAt(i, j).Block & BLOCKWERT_WAND))
            {
                r3 = TileAt(i-0, j+1).Red;
                g3 = TileAt(i-0, j+1).Green;
                b3 = TileAt(i-0, j+1).Blue;
            }
            else
            {
                r3 = TileAt(i, j).Red;
                g3 = TileAt(i, j).Green;
                b3 = TileAt(i, j).Blue;
            }

            if ((!(TileAt(i, j).Block & BLOCKWERT_WAND) && !(TileAt(i, j).Block & BLOCKWERT_WAND)) ||
                    (TileAt(i, j).Block & BLOCKWERT_WAND  &&   TileAt(i, j).Block & BLOCKWERT_WAND))
            {
                r4 = TileAt(i, j).Red;
                g4 = TileAt(i, j).Green;
                b4 = TileAt(i, j).Blue;
            }
            else
            {
                r4 = TileAt(i, j).Red;
                g4 = TileAt(i, j).Green;
                b4 = TileAt(i, j).Blue;
            }

            rn = int ((r1 + r2 + r3 + r4) / 4.0f);
            gn = int ((g1 + g2 + g3 + g4) / 4.0f);
            bn = int ((b1 + b2 + b3 + b4) / 4.0f);

            TileAt(i, j).Color[2] = D3DCOLOR_RGBA (rn, gn, bn, al);

            // Ecke rechts unten
            //
            if ((!(TileAt(i+1, j-0).Block & BLOCKWERT_WAND) && !(TileAt(i, j).Block & BLOCKWERT_WAND)) ||
                    (TileAt(i+1, j-0).Block & BLOCKWERT_WAND  &&   TileAt(i, j).Block & BLOCKWERT_WAND))
            {
                r1 = TileAt(i+1, j-0).Red;
                g1 = TileAt(i+1, j-0).Green;
                b1 = TileAt(i+1, j-0).Blue;
            }
            else
            {
                r1 = TileAt(i, j).Red;
                g1 = TileAt(i, j).Green;
                b1 = TileAt(i, j).Blue;
            }

            if ((!(TileAt(i-0, j+1).Block & BLOCKWERT_WAND) && !(TileAt(i, j).Block & BLOCKWERT_WAND)) ||
                    (TileAt(i-0, j+1).Block & BLOCKWERT_WAND  &&   TileAt(i, j).Block & BLOCKWERT_WAND))
            {
                r2 = TileAt(i-0, j+1).Red;
                g2 = TileAt(i-0, j+1).Green;
                b2 = TileAt(i-0, j+1).Blue;
            }
            else
            {
                r2 = TileAt(i, j).Red;
                g2 = TileAt(i, j).Green;
                b2 = TileAt(i, j).Blue;
            }

            if ((!(TileAt(i+1, j+1).Block & BLOCKWERT_WAND) && !(TileAt(i, j).Block & BLOCKWERT_WAND)) ||
                    (TileAt(i+1, j+1).Block & BLOCKWERT_WAND  &&   TileAt(i, j).Block & BLOCKWERT_WAND))
            {
                r3 = TileAt(i+1, j+1).Red;
                g3 = TileAt(i+1, j+1).Green;
                b3 = TileAt(i+1, j+1).Blue;
            }
            else
            {
                r3 = TileAt(i, j).Red;
                g3 = TileAt(i, j).Green;
                b3 = TileAt(i, j).Blue;
            }

            if ((!(TileAt(i, j).Block & BLOCKWERT_WAND) && !(TileAt(i, j).Block & BLOCKWERT_WAND)) ||
                    (TileAt(i, j).Block & BLOCKWERT_WAND  &&   TileAt(i, j).Block & BLOCKWERT_WAND))
            {
                r4 = TileAt(i, j).Red;
                g4 = TileAt(i, j).Green;
                b4 = TileAt(i, j).Blue;
            }
            else
            {
                r4 = TileAt(i, j).Red;
                g4 = TileAt(i, j).Green;
                b4 = TileAt(i, j).Blue;
            }

            rn = int ((r1 + r2 + r3 + r4) / 4.0f);
            gn = int ((g1 + g2 + g3 + g4) / 4.0f);
            bn = int ((b1 + b2 + b3 + b4) / 4.0f);

            TileAt(i, j).Color[3] = D3DCOLOR_RGBA (rn, gn, bn, al);

            //DKS - Lightmap code in original game was never used and all related code has now been disabled:
            //OriginalTiles[i][j].Color[0] = TileAt(i, j).Color[0];
            //OriginalTiles[i][j].Color[1] = TileAt(i, j).Color[1];
            //OriginalTiles[i][j].Color[2] = TileAt(i, j).Color[2];
            //OriginalTiles[i][j].Color[3] = TileAt(i, j).Color[3];
        }

} // ComputeCoolLight

// --------------------------------------------------------------------------------------
// "Taschenlampen" Ausschnitt im Alien Level rendern
// --------------------------------------------------------------------------------------

void TileEngineClass::DrawShadow (void)
{
    if (bDrawShadow == false)
        return;

    float x, y;

    if (NUMPLAYERS == 1)
    {
        x = (float)(int)(Player[0].xpos + 35 - 512 - XOffset);
        y = (float)(int)(Player[0].ypos + 40 - 512 - YOffset);
    }
    else
    {
        x = (float)(int)(XOffset + 320.0f);
        y = (float)(int)(YOffset + 240.0f);
    }

    D3DCOLOR col;

    col = D3DCOLOR_RGBA(255, 255, 255, (int)ShadowAlpha);

    Shadow.RenderSprite (x, y, col);
    Shadow.RenderSprite (x + 512, y, 0, col, true);

    Shadow.RenderMirroredSprite (x, y + 512, col, false, true);
    Shadow.RenderMirroredSprite (x + 512, y + 512, col, true, true);

    col = D3DCOLOR_RGBA(0, 0, 0, (int)ShadowAlpha);

    // Seitenr�nder
    RenderRect(x - 200,  y - 200,  1420, 200,  col);
    RenderRect(x - 200,  y + 1420, 1420, 200,  col);
    RenderRect(x - 200,  y,		   200,  1024, col);
    RenderRect(x + 1024, y,		   200,  1024, col);
}

// --------------------------------------------------------------------------------------
// Wand an x/y explodieren lassen
// --------------------------------------------------------------------------------------

void TileEngineClass::ExplodeWall (int x, int y)
{
    // ausserhalb des Levels nicht testen ;)
    if (x<1 ||
            y<1 ||
            x>(int)LEVELSIZE_X-1 ||
            y>(int)LEVELSIZE_Y-1)
        return;

    // keine zerst�rbare Wand?
    //
    if (!(TileAt(x, y).Block & BLOCKWERT_DESTRUCTIBLE))
        return;

    TileAt(x, y).Block    = 0;
    TileAt(x, y).FrontArt = 0;

    for (int i = 0; i < 2; i++)
        PartikelSystem.PushPartikel((float)(x * TILESIZE_X + rand()%10), (float)(y * TILESIZE_Y + rand()%10), ROCKSPLITTERSMALL);

    for (int k=0; k<4; k++)
        PartikelSystem.PushPartikel(float(x*TILESIZE_X+8), float(y*TILESIZE_Y+8), FUNKE);

    PartikelSystem.PushPartikel((float) x*TILESIZE_X - TILESIZE_X, (float) y*TILESIZE_Y - TILESIZE_Y, SMOKEBIG);

    SoundManager.PlayWave(100, 128, 11025 + rand()%2000, SOUND_EXPLOSION1);
}

// --------------------------------------------------------------------------------------
// Wand an x/y und angrenzende W�nde explodieren lassen
// --------------------------------------------------------------------------------------

void TileEngineClass::ExplodeWalls (int x, int y)
{
    // Rundes Loch erzeugen
    //  xxx
    // xxxxx
    // xxxxx
    // xxxxx
    //  xxx

    ExplodeWall(x-1, y-2);
    ExplodeWall(x+0, y-2);
    ExplodeWall(x+1, y-2);

    ExplodeWall(x-2, y-1);
    ExplodeWall(x-1, y-1);
    ExplodeWall(x+0, y-1);
    ExplodeWall(x+1, y-1);
    ExplodeWall(x+2, y-1);

    ExplodeWall(x-2, y+0);
    ExplodeWall(x-1, y+0);
    ExplodeWall(x+0, y+0);
    ExplodeWall(x+1, y+0);
    ExplodeWall(x+2, y+0);

    ExplodeWall(x-2, y+1);
    ExplodeWall(x-1, y+1);
    ExplodeWall(x+0, y+1);
    ExplodeWall(x+1, y+1);
    ExplodeWall(x+2, y+1);

    ExplodeWall(x-1, y+2);
    ExplodeWall(x+0, y+2);
    ExplodeWall(x+1, y+2);
}

// --------------------------------------------------------------------------------------
// Alle LightMaps auf dem Screen l�schen und Ursprungszustand wieder herstellen
// --------------------------------------------------------------------------------------

//DKS - See note for function below this one for DrawLightmap() as to why this was
//      disabled.
#if 0
void TileEngineClass::ClearLightMaps(void)
{
    unsigned int xoff, yoff;

    xoff = (int)(XOffset * 0.05f);
    yoff = (int)(YOffset * 0.05f);

    for (unsigned int x = xoff; x < xoff + SCREENSIZE_X + 1; x++)
        for (unsigned int y = yoff; y < yoff + SCREENSIZE_Y + 1; y++)
            for (int i = 0; i < 4; i++)
                TileAt(x, y).Color[i] = OriginalTiles[x][y].Color[i];
}
#endif //0

// --------------------------------------------------------------------------------------
// Lightmap an x/y mit alpha zum Level dazuaddieren
// Gilt nur einen Frame lang, danach wird der Originalzustand wieder hergestellt
// --------------------------------------------------------------------------------------

//DKS - The original code had a return as line 5 of this function, making it a stub.
//      When I tried removing it, I saw why: the lightmaps colors' don't match what
//      you'd expect and they appear in places off-center from where they should be.
//      I have now disabled the function and all four calls to it elsewhere in the code.
#if 0
void TileEngineClass::DrawLightmap (int Map, float x, float y, int alpha)
{
    // keine Lightmap Effekte?
    if (options_Detail < DETAIL_HIGH)
        return;

    return; // DKS - This is the return that was in the original code (see note above)

    if (x < 0 || y < 0 ||
            x > LEVELPIXELSIZE_X ||
            y > LEVELPIXELSIZE_Y)
        return;

    int xpos, ypos;

    xpos = (int)(x * 0.05f - lightmaps[Map].xsize / 2);
    ypos = (int)(y * 0.05f - lightmaps[Map].ysize / 2);

    int xo, yo;
    int sx;

    sx = lightmaps[Map].xsize;

    if (alpha < 0)
        alpha = 0;

    // normal draufaddieren?
    if (alpha == 255)
    {
        for (int mapx = 0; mapx < lightmaps[Map].xsize - 1; mapx++)
            for (int mapy = 0; mapy < lightmaps[Map].ysize - 1; mapy++)

                if (xpos >= 0 &&
                        ypos >= 0)
                {
                    // Position des Level Tiles ausrechnen
                    xo = xpos + mapx;
                    yo = ypos + mapy;

                    if (!(TileAt(xo, yo).Block & BLOCKWERT_WAND))
                    {
                        // Farben addieren (mit OR verkn�pfen)
                        TileAt(xo, yo).Color[0] |= lightmaps[Map].map[mapx + mapy * sx];
                        TileAt(xo, yo).Color[1] |= lightmaps[Map].map[mapx + 1 + mapy * sx];
                        TileAt(xo, yo).Color[2] |= lightmaps[Map].map[mapx + (mapy + 1) * sx];
                        TileAt(xo, yo).Color[3] |= lightmaps[Map].map[mapx + 1 + (mapy + 1) * sx];
                    }
                }
    }

    // oder alpha mit einberechnen (langsamer)
    else
    {
        // alphamaske erstellen
        FarbUnion col;

        for (int mapx = 0; mapx < lightmaps[Map].xsize - 1; mapx++)
            for (int mapy = 0; mapy < lightmaps[Map].ysize - 1; mapy++)

                if (xpos >= 0 &&
                        ypos >= 0)
                {
                    // Position des Level Tiles ausrechnen
                    xo = xpos + mapx;
                    yo = ypos + mapy;

                    // Nur Levelhintergrund bzw farbiges overlay beleuchten
                    if (!(TileAt(xo, yo).Block & BLOCKWERT_WAND))
                    {
                        // links oben
                        col.color = lightmaps[Map].map[mapx + mapy * sx];
                        col.color = D3DCOLOR_RGBA(col.farbStruct.r * alpha / 255,
                                                  col.farbStruct.g * alpha / 255,
                                                  col.farbStruct.b * alpha / 255,
                                                  255);
                        TileAt(xo, yo).Color[0] |= col.color;

                        // rechts oben
                        col.color = lightmaps[Map].map[mapx + 1 + mapy * sx];
                        col.color = D3DCOLOR_RGBA(col.farbStruct.r * alpha / 255,
                                                  col.farbStruct.g * alpha / 255,
                                                  col.farbStruct.b * alpha / 255,
                                                  255);
                        TileAt(xo, yo).Color[1] |= col.color;

                        // links unten
                        col.color = lightmaps[Map].map[mapx + (mapy + 1) * sx];
                        col.color = D3DCOLOR_RGBA(col.farbStruct.r * alpha / 255,
                                                  col.farbStruct.g * alpha / 255,
                                                  col.farbStruct.b * alpha / 255,
                                                  255);
                        TileAt(xo, yo).Color[2] |= col.color;

                        // rechts unten
                        col.color = lightmaps[Map].map[mapx + 1 + (mapy + 1) * sx];
                        col.color = D3DCOLOR_RGBA(col.farbStruct.r * alpha / 255,
                                                  col.farbStruct.g * alpha / 255,
                                                  col.farbStruct.b * alpha / 255,
                                                  255);
                        TileAt(xo, yo).Color[3] |= col.color;
                    }
                }
    }
}
#endif //0
