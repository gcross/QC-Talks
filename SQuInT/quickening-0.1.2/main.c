#include <CoreServices/CoreServices.h>
#include <Quicktime/Quicktime.h>
#include <glob.h>
#include <unistd.h>
#include "adt/commando.h"
#include "adt/progress.h"
#include "adt/sorted_intlist.h"
// #include "SpriteUtilities.h"

static sorted_intlist when_to_pause;
/* default to sorenson codec, version 3. */
static CodecType codecType = FOUR_CHAR_CODE('SVQ3'); 
/* or kJPEGCodecType (has jpeg artifacts),
   or kPNGCodecType; (67MB)
   kCinepakCodecType (has color issues),
   kAnimationCodecType */
static const char *output_filename = "default.mov";
static unsigned int pauseMsec = 10;
static boolean wantNoControl = FALSE;
static boolean wantVerbose = FALSE;
static unsigned int sizeX = 640;
static unsigned int sizeY = 480;

static void set_800(const char *arg __attribute__((unused)), 
                    void *dummy __attribute__((unused))) {
  sizeX=800; sizeY=600;
}
static void set_1024(const char *arg __attribute__((unused)), 
                     void *dummy __attribute__((unused))) {
  sizeX=1024; sizeY=768;
}

static struct commandos commando[] =
  {
  {"display this help and exit", 
   "help", 'h',   no_argument, (commando_cb)commando_usage, commando },
  {"output debugging information", 
   "verbose", 'v',   no_argument, commando_boolean, &wantVerbose },
  {"output filename", 
   "output", 'o', required_argument, commando_strdup, &output_filename },
  {"codec type (SVQ3, cvid, SVQ1, smc)", 
   "codec", 'c', required_argument, commando_fourchar, &codecType },
  {"pause time in milliseconds (4000) (may be replaced by mouse clicking)", 
   "pause", 'p', required_argument, commando_int, &pauseMsec },
  {"instruct the player not to show the controls",
   "uncontrolled", 'u', no_argument, commando_boolean, &wantNoControl },
  {"source images are 800x600",
   "800x600", '8', no_argument, set_800, NULL },
  {"source images are 1024x768",
   "1024x768", '1', no_argument, set_1024, NULL },
  {NULL, NULL, '\0', 0, NULL, NULL}
  };


#define ckerr(expr) do { OSErr ckerr_err; if((ckerr_err = expr)) { \
printf("%s failed on line %d: %d\n", #expr, __LINE__, ckerr_err);  \
 exit(1); }} while(0) 

static OSErr Filename2FSSpec(const char * filename, FSSpec *fsp, bool allowNonExistent) {
  static FSSpec cwddirspec;
  static char cwdbuf[512];
  StringPtr fullfilename;
  OSErr merr;

  if(cwdbuf[0] == '\0') {
    FSCatalogInfo fsci; 
    FSRef cwddirref;
    getcwd(cwdbuf,512);
    merr = FSPathMakeRef(cwdbuf, &cwddirref, NULL);
    if(merr != noErr && merr != fnfErr) {
      printf("failed to make path reference for cwd %s: %d\n", cwdbuf, merr);
      return merr;
    }
    ckerr(FSGetCatalogInfo(&cwddirref, kFSCatInfoNodeID, &fsci, 
                           NULL, &cwddirspec, NULL));
    cwddirspec.parID = fsci.nodeID;
  }
  fullfilename = malloc(strlen(filename) + 3 + strlen(cwdbuf) + 1);
  sprintf(fullfilename + 1 , "%s", filename);
  fullfilename[0] = strlen(fullfilename+1);
  
  merr = FSMakeFSSpec(cwddirspec.vRefNum, cwddirspec.parID, fullfilename, fsp);
  if ( ! ( merr == noErr || (allowNonExistent && merr == fnfErr))) {
    printf("failed to make fsspec for %d %ld %s: %d\n", 
           cwddirspec.vRefNum, cwddirspec.parID, fullfilename, merr);
    return merr;
  }
  
  free(fullfilename);
  return noErr;
}


static OSErr DrawThePicture(FSSpec *imgFile, GWorldPtr pMovieGWorld, 
                     const Rect *rect) {
  GraphicsImportComponent gi;
  OSErr e;
  
  ckerr(GetGraphicsImporterForFile(imgFile, &gi));
  EraseRect (rect);	
  ckerr(GraphicsImportSetGWorld ( gi, pMovieGWorld, NULL ));
  e= GraphicsImportDraw(gi);
 
  CloseComponent(gi);
  return noErr;
}

#define initial_sprite_duration 5
static int Now = 0;

static int calc_sampletime(char *filenames[], long lImageNumber, 
                    long images, boolean *forceKey) {
  int sampletime;

  if(lImageNumber >= images-1) {
    sampletime = pauseMsec * 3 / 5;
    si_insert(when_to_pause, Now); // begin the last range 
    *forceKey = TRUE;
  } else {
    int thisSeq, nextSeq;
    int thisTime, nextTime, thisTime_msec, nextTime_msec;
    int thisFrame, nextFrame;
    sscanf(filenames[lImageNumber], "slithy%3d-%4d-%d.%d.png", 
           &thisSeq, &thisFrame, &thisTime, &thisTime_msec);
    sscanf(filenames[lImageNumber+1], "slithy%3d-%4d-%d.%d.png", 
           &nextSeq, &nextFrame, &nextTime, &nextTime_msec);
    if(nextSeq > thisSeq) {
      sampletime = pauseMsec * 3 / 5;
      /* doesn't help? if(thisSeq==0 || Now == 0) {
        sampletime += 100;
        } */
      sampletime += 100;
      si_insert(when_to_pause, Now);
      *forceKey = TRUE;
    } else {
      sampletime = (int)((nextTime * 1000 + nextTime_msec) - 
                         (thisTime * 1000 + thisTime_msec)) * 3 / 5;
      *forceKey = FALSE;
    }
  }

  Now += sampletime;
  if(wantVerbose) printf("\n%d %s\n", Now, (*forceKey) ? "True" : "False");
  return(sampletime);
}
  
static OSErr 
add_video_samples_to_media (Media media, short sTrackWidth, 
                            short sTrackHeight) {
  Rect rect;
  glob_t g;
  long lImageNumber;
  GWorldPtr pMovieGWorld = NULL;
  PixMapHandle pixMapHandle = NULL;
  Ptr pCompressedData = NULL;
  CGrafPtr pSavedPort = NULL;
  GDHandle hSavedDevice = NULL;
  Handle hCompressedData = NULL;
  long lMaxCompressionSize = 0L;
  ImageDescriptionHandle hImageDescription = NULL;
  

  /*  Create a new offscreen graphics world that will hold
      the movie's drawing surface.  draw_image() copies the
      image of IceFlow to this surface with varying amounts
      of transparency.
      =======================================================  */
  
  MacSetRect (&rect, 0, 0, sTrackWidth, sTrackHeight);
  
  ckerr(NewGWorld(&pMovieGWorld, /* the new GWorld. */
                    24,            /* bits/pixel   */
                    &rect,         /* desired size */
                    NULL, NULL, (GWorldFlags) 0));

  /*  Retrieve the pixel map associated with that graphics
      world and lock the pixel map in memory.
      GetMaxCompressionSize() and CompressImage() only
      operate on pixel maps, not graphics worlds.
      =========================================================  */
    
  pixMapHandle = GetGWorldPixMap (pMovieGWorld);
  if (pixMapHandle == NULL) { 
    printf ("GetGWorldPixMap failed\n"); 
    goto bail; 
  }
  LockPixels (pixMapHandle);


  /*  Get the maximum number of bytes required to hold an
      image having the specified characteristics compressed
      using the specified compressor.
      ====================================================  */

  ckerr(GetMaxCompressionSize(pixMapHandle, &rect, 0, 
                              codecNormalQuality,
                              kPNGCodecType, /* possibly wrong */
                              (CompressorComponent) anyCodec,
                              &lMaxCompressionSize 
                              ));
  
  /*  Allocate a buffer to hold the compressed image data by
      creating a new handle.
      =======================================================  */
    
  hCompressedData = NewHandle (lMaxCompressionSize);
  if (hCompressedData == NULL) { 
    printf ("NewHandle(%ld) failed\n", lMaxCompressionSize*2); 
    goto bail; 
  }


  /*  Lock the handle and then dereference it to obtain a
      pointer to the data buffer because CompressImage()
      wants us to pass it a pointer, not a handle.
      ========================================================  */

  HLockHi (hCompressedData);
  pCompressedData = *hCompressedData;


  /*  Create an image description object in memory of
      minimum size to pass to CompressImage().
      CompressImage() will resize the memory as necessary so
      create it small here.
      =========================================================  */
    
  hImageDescription = (ImageDescriptionHandle) NewHandle (4);
  if (hImageDescription == NULL) { 
    printf ("NewHandle(4) failed\n"); 
    goto bail; 
  }

  /*  Save the current GWorld and set the offscreen GWorld
      as current.
      =======================================================  */
    
  GetGWorld (&pSavedPort, &hSavedDevice);
  SetGWorld (pMovieGWorld, NULL);

  ImageSequence sequenceID;
  ckerr(CompressSequenceBegin(&sequenceID, pixMapHandle, 
                              NULL, /* the compressor can allocate prev.  */
                              &rect, /* the image rectangle.  */
                              NULL, /* prev rect, doesn't matter */
                              0, /* color depth, let it guess */
                              codecType,
                              anyCodec,
                              codecNormalQuality, /*spatial */
                              codecNormalQuality, /* temporal */
                              10, /* ten between key */
                              NULL, 
                              codecFlagUpdatePrevious,
                              hImageDescription));
  
  /*  Each cycle of this loop draws one image into the
      GWorld, compresses it, then adds it to the movie.
      ============================================  */
    
  glob("slithy*.png", 0, NULL, &g);
  if(g.gl_matchc == 0) {
    fprintf(stderr, "didn't find any files named 'slithy*.png'\n");
    exit(EXIT_FAILURE);
  }
  progress_label("encoding");
  progress_n_of(0, g.gl_matchc);
  for(lImageNumber=0;lImageNumber<g.gl_matchc;lImageNumber++) {
    FSSpec imgFile;
    int sampletime;
    UInt8 similarity;
    long lDataLength;
    OSErr err;
    boolean forceKey = FALSE;

    ckerr(Filename2FSSpec(g.gl_pathv[lImageNumber], &imgFile, false));

    progress_n_of(lImageNumber+1, g.gl_matchc);

    assert(g.gl_pathv);
    sampletime = calc_sampletime(g.gl_pathv, lImageNumber, 
                                 g.gl_matchc, &forceKey);

    if((err = DrawThePicture(&imgFile, pMovieGWorld, &rect)) != noErr) {
      fprintf(stderr, "Failed to draw '%s': %d\n", g.gl_pathv[lImageNumber], err);
      exit(EXIT_FAILURE);
    }

    /* Compress the pixel map that has just been drawn on.
       Also resize and fill in the image description.
       Resulting image size can be discovered by consulting
       the image description field dataSize.
       =============================================  */
      
    ckerr(CompressSequenceFrame(sequenceID, pixMapHandle,
                                &rect, 
                                (codecFlagUpdatePrevious &
                                 ((forceKey) ? 
                                  codecFlagForceKeyFrame : 0)),
                                pCompressedData, &lDataLength, 
                                &similarity, NULL));
    
    /*    Add the compressed image to the movie.
          ======================================  */
    // printf("%d %ld\n", similarity, lDataLength);
    
    /* (**hImageDescription).dataSize,* num bytes to be copied into media. */
    if((err = AddMediaSample(media,   /* the media to add the image to.     */
                         hCompressedData,/* the compressed image to add.  */
                         0, /* byte offs into data to begin readg */
                         lDataLength, /* num bytes to be copied into media. */
                         sampletime, /* duration of the frame (media time) */
                         (SampleDescriptionHandle) hImageDescription, 
                         /* image desc cast to   */
                         /*   a sample description since both  */
                         /*   structures start with same fields*/
                         1, /* num samples in the data buffer.    */
                         similarity ? mediaSampleNotSync : 0, /* flags */
                         NULL /* ptr to receive media time in which */
                         /*   the image was added.             */
                         ) ) != noErr ) {
      fprintf(stderr, "Failed to AddMediaSample '%s': %d\n", g.gl_pathv[lImageNumber], err);
      exit(EXIT_FAILURE);
    }

  }

  si_insert(when_to_pause, Now);

 bail:
  
  SetGWorld (pSavedPort, hSavedDevice);

  if (hImageDescription != NULL) DisposeHandle ((Handle) hImageDescription);
  if (hCompressedData   != NULL) DisposeHandle (hCompressedData);
  if (pMovieGWorld      != NULL) DisposeGWorld (pMovieGWorld);
    
  return (noErr);
}

static TimeValue try_adding_frames(Movie myMovie) {
  Track track = NewMovieTrack(myMovie, FixRatio(sizeX,1), 
                              FixRatio(sizeY,1), kNoVolume);
  Media media = NewTrackMedia(track, VideoMediaType, 
                              600, NULL, NULL);
  ckerr(GetMoviesError());

  ckerr(BeginMediaEdits(media));
  /* most work done here: */
  add_video_samples_to_media(media, sizeX, sizeY);
  ckerr(EndMediaEdits(media));

  if(GetMediaDuration(media) > 0) {
    ckerr( InsertMediaIntoTrack(track, 0, // track start time
                                0,       // media start time
                                GetMediaDuration(media),
                                FixRatio(1,1)));          // normal speed
  } else {
    printf("empty movie.  find yerself some pngs.\n");
    exit(1);
  }
  return(GetMediaDuration(media));
}

/* copied from SpriteUtilities in the sample code */
static OSErr AddSpriteSampleToMedia( Media theMedia, 
                                     QTAtomContainer sample, 
                                     TimeValue duration, 
                                     Boolean isKeyFrame,
                                     TimeValue *sampleTime ) {
  OSErr					err = noErr;
  SampleDescriptionHandle sampleDesc = nil;
  
  sampleDesc = (SampleDescriptionHandle) 
    NewHandleClear( sizeof(SpriteDescription ) );
  
  ckerr( AddMediaSample( theMedia, (Handle) sample, 0, 
                         GetHandleSize( sample ),
                         duration, sampleDesc, 1,
                         (short)(isKeyFrame ? 0 : mediaSampleNotSync), 
                         sampleTime ) );
  
  if ( sampleDesc )	DisposeHandle( (Handle) sampleDesc );
  return err;
}

static void createPauseSelection(Media spritemedia, 
                          unsigned int startTime,
                          unsigned int stopTime) {
  QTAtom actionAtom, actionAtomTwo;
  QTAtom newQTEventAtom;
  long action = kActionMovieSetSelection;
  QTAtomContainer pauseSample;
  unsigned int duration;

  if(wantVerbose) {
    fprintf(stderr, "adding a selection for %d-%d\n", startTime, stopTime);
  }
  if(startTime < initial_sprite_duration){
    duration = stopTime-initial_sprite_duration;
  } else {
    duration = stopTime-startTime;
  }
  
  ckerr(QTNewAtomContainer(&pauseSample));
  /* AddQTEventAtom */
  ckerr( QTInsertChild( pauseSample, kParentAtomIsContainer, 
                        kQTEventFrameLoaded, 
                        1, 1, 0, nil, &newQTEventAtom ) );
  /* AddActionAtom */
  ckerr( QTInsertChild( pauseSample, newQTEventAtom, kAction, 
                        0, 0, 0, nil, &actionAtom ) );
  ckerr( QTInsertChild( pauseSample, actionAtom, kWhichAction, 
                        1, 1, sizeof(action), &action, nil ) );
  unsigned int startTimeN = htonl(startTime);
  ckerr( QTInsertChild( pauseSample, actionAtom, kActionParameter, 
                        0, 1 /* parameterIndex */, 
                        sizeof(unsigned int), &startTimeN, 
                        nil ) );
  unsigned int stopTimeN = htonl(stopTime);
  ckerr( QTInsertChild( pauseSample, actionAtom, kActionParameter, 
                        0, 2 /* parameterIndex */, 
                        sizeof(unsigned int), &stopTimeN, 
                        nil ) );
  /* another action */
  ckerr( QTInsertChild( pauseSample, newQTEventAtom, kAction, 
                        0, 0, 0, nil, &actionAtomTwo ) );
  action = kActionMoviePlaySelection;
  ckerr( QTInsertChild( pauseSample, actionAtomTwo, kWhichAction, 
                        1, 1, sizeof(action), &action, nil ) );
  Boolean true_val = true;
  ckerr( QTInsertChild( pauseSample, actionAtomTwo, kActionParameter, 0, 
                        1 /* parameterIndex */, 
                        sizeof(true_val), &true_val, nil ) );
  
  ckerr( AddSpriteSampleToMedia(spritemedia, pauseSample, 
                                duration,
                                false /* be a key frame */, 
                                NULL));
  QTDisposeAtomContainer(pauseSample);
}

boolean pausemaker(int when, void *aspritemedia) {
  Media spritemedia = *(Media *) aspritemedia;
  static int last_when;
  if(when > 0) {
    createPauseSelection(spritemedia, last_when, when);
    last_when = when;
  } else {
    // then was: createPauseSelection(spritemedia, 0, 10);
    //            last_when = 10;
    //   which got stuck on the first frame.
    // tried and failed, walking backward through movie: 
    //  last_when=0;
    // last_when=initial_sprite_duration;
    last_when=0;
  }
  return TRUE;
}


static void createPlaySample(Media spritemedia) {
  QTAtomContainer playSample;
  QTAtom frameLoaded;
  FILE *f;
  char *buffer = malloc(65530);
  int len;
  
  ckerr(QTNewAtomContainer(&playSample));
  
  ckerr(QTInsertChild(playSample, kParentAtomIsContainer, 
                      kQTEventFrameLoaded, 1, 1, 0, NULL, &frameLoaded));
  {
    QTAtom actionAtom;
    long action = kActionSetFocus;
    ckerr(QTInsertChild(playSample, frameLoaded, 
                        kAction, 1, 1, 0, NULL, &actionAtom));
    ckerr(QTInsertChild(playSample, actionAtom, 
                        kWhichAction, 1, 1, 
                        sizeof(action), &action, nil));
    {
      QTAtom paramAtom;
      ckerr(QTInsertChild(playSample, actionAtom, 
                          kActionParameter, 1, 1, 
                          0, NULL, &paramAtom));
      {
        QTAtom hreftargetAtom;
        long targettrack = 2;
        long spriteid = 1;
        ckerr(QTInsertChild(playSample, paramAtom, 
                            kQTParseTextHREFTarget, 1, 1, 
                            0, NULL, &hreftargetAtom));
        ckerr(QTInsertChild(playSample, hreftargetAtom, 
                            kTargetTrackID, 1, 1, 
                            sizeof(targettrack), &targettrack, nil));
        ckerr(QTInsertChild(playSample, hreftargetAtom, 
                            kTargetSpriteID, 1, 1, 
                            sizeof(spriteid), &spriteid, nil));
      }
    }
  }
  
  if( 1 ) {
    f = fopen("keynote-sprt.dat", "r");
    if(f == NULL) { printf("argh. keynote-sprt.dat %s\n", strerror(errno)); exit(0);}
    len = fread(buffer, 1, 65500, f);
    ckerr(QTInsertChild(playSample, kParentAtomIsContainer, kSpriteAtomType, 
                        1 /* must be */, 1, len-20, buffer+20, nil));
    fclose(f);
  } else {
    /* haven't figured out how to synthesize a working version of the keynote sprite. */
    QTAtom spriteAtom;
    ckerr(QTInsertChild(playSample, kParentAtomIsContainer, kSpriteAtomType, 
                        1 /* must be */, 1, 0, nil, &spriteAtom));
    {
      QTAtom keypressEventAtom;
      
      {
        MatrixRecord	matrix;
		
        SetIdentityMatrix( &matrix );
        matrix.matrix[2][0] = ((long)0 << 16);
        matrix.matrix[2][1] = ((long)0 << 16);
		
        // EndianUtils_MatrixRecord_NtoB( &matrix );
        
        ckerr( QTInsertChild( playSample, spriteAtom, kSpritePropertyMatrix, 1, 0, sizeof(MatrixRecord), &matrix, nil ) );
      }
      {
        short isVisible = 0;
        ckerr( QTInsertChild( playSample, spriteAtom, kSpritePropertyVisible, 1, 0, sizeof(isVisible), &isVisible, nil ) );
      }
      {
		short tempLayer = htons(1);
        ckerr( QTInsertChild( playSample, spriteAtom, kSpritePropertyLayer, 1, 0, sizeof(tempLayer), &tempLayer, nil ) );
      }
      {
        short imageIndex = 1;
        ckerr( QTInsertChild( playSample, spriteAtom, kSpritePropertyImageIndex, 1, 0, sizeof(imageIndex), &imageIndex, nil ) );
      }

      ckerr(QTInsertChild(playSample, spriteAtom, kSpriteImageNameAtomType, 
                          1, 1, strlen("\pPlay Button"), strdup("\pPlay Button"), nil));
      /* going to ignore the keypress bit for now, and focus only on the (more reliable) click */
      ckerr(QTInsertChild(playSample, spriteAtom, kTrackModifierObjectQTEventSend, 
                          kQTEventMouseClickEndTriggerButton, 1, 0, nil, &keypressEventAtom));
      {
        QTAtom actionAtom;
        long action = kActionCase;
        ckerr(QTInsertChild(playSample, keypressEventAtom, kAction, 0,0,0,nil, &actionAtom));
        ckerr( QTInsertChild( playSample, actionAtom, kWhichAction, 
                              1, 1, sizeof(action), &action, nil ) );
        {
          QTAtom caseAtom;
          ckerr( QTInsertChild( playSample, actionAtom, kActionParameter, 
                                0, 1, 0, nil, &caseAtom ) );
          {
            QTAtom condAtom;
            ckerr( QTInsertChild( playSample, caseAtom, kConditionalAtomType, 
                                  0, 1, 0, nil, &condAtom ) );
            {
              QTAtom exprAtom;
              QTAtom actListAtom;
              ckerr( QTInsertChild( playSample, condAtom, kExpressionContainerAtomType, 
                                    0, 1, 0, nil, &exprAtom ) );
              {
                QTAtom neAtom,op1Atom,op2Atom;
                ckerr( QTInsertChild( playSample, exprAtom, kOperatorAtomType, 
                                      '!=  ', 1, 0, nil, &neAtom ) );
                ckerr( QTInsertChild( playSample, neAtom, kOperandAtomType, 
                                      0, 1, 0, nil, &op1Atom ) );
                ckerr( QTInsertChild( playSample, op1Atom, kOperandMovieTime, 
                                      0, 1, 0, nil, nil) );

                ckerr( QTInsertChild( playSample, neAtom, kOperandAtomType, 
                                      0, 2, 0, nil, &op2Atom ) );
                ckerr( QTInsertChild( playSample, op2Atom, kOperandMovieDuration, 
                                      0, 1, 0, nil, nil) );
              }
              ckerr( QTInsertChild( playSample, condAtom, kQTEventListReceived, 
                                    0, 1, 0, nil, &actListAtom ) );
              {
                QTAtom goAction;
                ckerr( QTInsertChild( playSample, actListAtom, kAction, 
                                      0, 1, 0, nil, &goAction ) );
                {
                  long go_action = kActionMovieSetRate;
                  Fixed go_rate = Long2Fix(1);
                  ckerr( QTInsertChild( playSample, goAction, kWhichAction, 
                                        0, 1, sizeof(go_action), &go_action, nil ) );
                  ckerr( QTInsertChild( playSample, goAction, kActionParameter, 
                                        0, 1, sizeof(go_rate), &go_rate, nil ) );
                }
              }
            }
          }
        }
      }
    }
    
  }
  
  ckerr( AddSpriteSampleToMedia(spritemedia, playSample, 
                                initial_sprite_duration /* has to be nonzero.. */, 
                                true /* be a key frame */, 
                                NULL));
  QTDisposeAtomContainer(playSample);
  free(buffer);
}

static void createTrackProperties(Media spritemedia) {
  QTAtomContainer myTrackProperties;
  Boolean hasActions = true;
  //  Boolean canEdit = true;
  Boolean false_val = false;
  // short focusFlags = 1;
  
  RGBColor myBackgroundColor;
  
  // add a background color to the sprite track
  myBackgroundColor.red = EndianU16_NtoB(0x8000);
  myBackgroundColor.green = EndianU16_NtoB(0);
  myBackgroundColor.blue = EndianU16_NtoB(0xffff);
  
  QTNewAtomContainer(&myTrackProperties);
  QTInsertChild(myTrackProperties, 0, kSpriteTrackPropertyBackgroundColor, 
                1, 1, sizeof(RGBColor), &myBackgroundColor, NULL);
  
  QTInsertChild(myTrackProperties, 0, kSpriteTrackPropertyHasActions, 
                1, 1, sizeof(hasActions), &hasActions, NULL);
  /* don't know what these do. */
  ckerr(QTInsertChild(myTrackProperties, 0, kSpriteTrackPropertyVisible, 
                      1, 1, sizeof(false_val), &false_val, nil));
  /* end dunno, to some extent */
  
  /* 
  QTInsertChild(myTrackProperties, 0, kTrackFocusCanEditFlag, 
                1, 1, sizeof(canEdit), &canEdit, NULL);
  QTInsertChild(myTrackProperties, 0, kTrackDefaultFocusFlags, 
                1, 1, sizeof(focusFlags), &focusFlags, NULL);
  
  unsigned int dat[] = { 0x2c, 'targ', 1, 1, 0,
                         0x18, 'spid', 1, 1, 0, 1 };
  ckerr(QTInsertChild(myTrackProperties, 0, 'kref', 
                      1, 1, sizeof(dat), dat, nil));
  */
  
  ckerr(SetMediaPropertyAtom(spritemedia, myTrackProperties));
  QTDisposeAtomContainer(myTrackProperties);
}

static TimeValue try_thieving_sprite(Movie myMovie) {

  Track spritetrack = NewMovieTrack(myMovie, FixRatio(sizeX,1), 
                                    FixRatio(sizeY,1), kNoVolume);
  ckerr(GetMoviesError());

  Media spritemedia = NewTrackMedia(spritetrack, SpriteMediaType, 
                                    600, NULL, NULL);
  ckerr(GetMoviesError());

  /* work */
  BeginMediaEdits(spritemedia);
  createPlaySample(spritemedia); /* must go first */
  si_print(when_to_pause, stderr);
  si_iterate(when_to_pause, pausemaker, &spritemedia);
  EndMediaEdits(spritemedia);

  ckerr( InsertMediaIntoTrack(spritetrack, 
                              0 /* when to insert */, 0, 
                              GetMediaDuration(spritemedia), 
                              FixRatio(1,1)) );

  createTrackProperties(spritemedia);

  {
    UserData ud = GetMovieUserData(myMovie);
    Handle yes = NewHandle(1);
    **yes = '\1';
    AddUserData(ud, yes, 'SelO');
  }
  
  return(GetMediaDuration(spritemedia));
}
  
int main (int argc, char **argv) {
  FSSpec myFile;
  short sResId;
  Movie myMovie;
  short myResRefNum;

  commando_parse(argc, argv, commando);

  if( 1 ) {
    FILE *f = fopen("keynote-sprt.dat", "r");
    if(f == NULL) { 
      printf("argh. couldn't fopen 'keynote-sprt.dat': %s\n", strerror(errno)); 
      printf("it's in the source tarball, copy it to pwd so I can find it.\n");
      exit(EXIT_FAILURE);
    }
    fclose(f);
  } 

  when_to_pause = si_new();

  Filename2FSSpec(output_filename, &myFile, true);
    
  ckerr( EnterMovies() );

  ckerr( CreateMovieFile(&myFile, FOUR_CHAR_CODE('TVOD'), smCurrentScript, 
                         createMovieFileDeleteCurFile | 
                         createMovieFileDontCreateResFile,
                         &myResRefNum, &myMovie) );

  // misguided. try_adding_sprite(myMovie);
  TimeValue pic_duration = try_adding_frames(myMovie);
  TimeValue sprite_duration = try_thieving_sprite(myMovie);
  if(sprite_duration != pic_duration) { 
    fprintf(stderr, "bug! duration of video track (%u units) mismatched duration of sprite (control) track (%u units) \n", 
            (unsigned int)pic_duration, (unsigned int)sprite_duration);
    exit(EXIT_FAILURE);
  }

  if(wantNoControl) {
    OSType myType=FOUR_CHAR_CODE('none');
    myType = EndianU32_NtoB(myType);
    SetUserDataItem(GetMovieUserData(myMovie), &myType, sizeof(myType),
                    kUserDataMovieControllerType,1);
  }

  sResId = movieInDataForkResID;
  ckerr(AddMovieResource(myMovie, myResRefNum, &sResId, output_filename));
  CloseMovieFile(myResRefNum); 
  DisposeMovie(myMovie);
  return 0;
}

