/* WISHWOOD: KINGDOM 3030 — PS4 homebrew (OpenOrbis). */
#include <orbis/libkernel.h>
#include <orbis/SceVideoOut.h>
#include <orbis/ScePad.h>
#include <orbis/SceUserService.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "font.h"
#include "audio.h"

#define IW 480
#define IH 270
#define SC 4
#define FBW 1920
#define FBH 1080
#define TS 20
#define MW 34
#define MH 22
#define MAXIT 120
#define MAXEN 8
#define MAXPATH 400
#define PI 3.14159265f
#define FITTING_SECS 25.0f

#define RGB(r,g,b) (0xFF000000u|((uint32_t)(r)<<16)|((uint32_t)(g)<<8)|(uint32_t)(b))
#define C_WALL   RGB(22,17,35)
#define C_WALLL  RGB(36,27,58)
#define C_FLOOR1 RGB(33,26,48)
#define C_FLOOR2 RGB(28,22,41)
#define C_DOOR   RGB(42,32,64)
#define C_BARR   RGB(74,58,40)
#define C_HUD    RGB(141,132,166)
#define C_WARM   RGB(217,161,60)
#define C_RED    RGB(201,74,58)
#define C_PALE   RGB(214,203,184)

static const char* MAPS[3][MH] = {
{
"##################################","#....#.........#........#........#",
"#....#....w....#...k....#....o...#","#....D.........#........D........#",
"#....#.....B...#........#........#","#....#####D#####........#####D####",
"#....#.........#........#........#","#..S.#....T....#...E....#...w....#",
"#....#.........#........#........#","######D##################........#",
"#.......#.......#.......#........#","#...o...#...k...#...w...#....T...#",
"#.......D.......D.......D........#","#.......#.......#.......#........#",
"####D#######D#######D#######D#####","#.......#.......#.......#........#",
"#...k...#...E...#..Po...#....w...#","#.......#.......#.......#........#",
"#.......D.......D.......D........#","#.......#.......#.......#....X...#",
"#.......#.......#.......#........#","##################################"
},{
"##################################","#........#..........#............#",
"#..o.....#....T.....#....k....X..#","#........#..........#............#",
"####D####D....D.....D.....D#######","#........#..........#............#",
"#...w....#.....B....#.....o......#","#........#..........#............#",
"#########D..........D............#","#........#..........#............#",
"#........#..........#.....E......#","#########D##########D#############",
"#........#..........#............#","#...k....#....E.....#....w.......#",
"#########D..........D............#","#########D..........D............#",
"#........#..........#............#","#........#..........#.....T......#",
"#........#..........#............#","#...o....#....S.....#............#",
"#........#..........#............#","##################################"
},{
"##################################","#......#........#.......#....X...#",
"#..o...#...w....#...k...#....T...#","#......#........#.......#........#",
"#......D........D.......D........#","#......#........#.......#........#",
"####D###........#.......####D#####","#......#........#.......#........#",
"#......#...E....#...E...#....o...#","#......#........#.......#........#",
"#......D........D.......D........#","#......#........#.......#........#",
"###D####........#.......####D#####","#......#........#.......#........#",
"#......#....B...#...w...#....S...#","#......#........#.......#........#",
"#......D........D.......D........#","#......#........#.......#........#",
"#......#........#.......#........#","#..U...#........#.......#........#",
"#......#........#.......#........#","##################################"
}};

#define FIT_X0 25.0f
#define FIT_X1 32.6f
#define FIT_Y0 0.6f
#define FIT_Y1 5.6f
#define VAULT_X 7
#define VAULT_Y 16

static const char* LORE[3] = {
"PLAYBILL I. OPENING DAY 1971. A MAN IN A MOUSE SUIT SHOOK FOUR THOUSAND HANDS. HE IS STILL SHAKING THEM.",
"PLAYBILL II. WISH ENERGY. THE TURNSTILES MEASURE IT. THE PARK EATS IT. NOBODY LEAVES WITH A FULL TANK.",
"PLAYBILL III. THE ASCENSION. ONE GUEST STAYS BEHIND EVERY SEASON. THE SUIT IS FITTED TO THEM. THE SMILE IS NOT."
};

typedef struct { int x, y; char t; int alive; } Item;
typedef struct { float x, y, cd, t; int px[MAXPATH], py[MAXPATH], plen, alive, boss; } En;

static uint32_t fb[IW * IH];
static uint32_t* vram[2];
static int vidx, vh;
static float rayD[360];
static char grid[MH][MW];
static unsigned char barr[MH][MW];
static Item items[MAXIT];
static int nItems;
static En ens[MAXEN];
static int nEns;
static int spawnX[8], spawnY[8], nSpawn;
static int zone, night, phase, state;
static float timer, msgT, invuln;
static char msg[128];
static float px, py, hp, fuel, wood, tokens, batt;
static int pb3, wearing;
static int saveX, saveY, exX, exY, stX, stY;
static int fitting, fitT;
static int stepT;

enum { S_TITLE, S_PLAY, S_DEAD, S_WIN, S_ASCEND, S_STAY, S_CHOICE, S_PAUSE };

static int totalTokens(void){ return (int)tokens + pb3; }
static void say(const char* t){ strncpy(msg, t, 127); msg[127]=0; msgT=4.0f; }

static void rect(int x,int y,int w,int h,uint32_t c){
  if(x<0){w+=x;x=0;} if(y<0){h+=y;y=0;}
  if(x+w>IW)w=IW-x; if(y+h>IH)h=IH-y;
  if(w<=0||h<=0)return;
  for(int j=y;j<y+h;j++){ uint32_t* r=fb+j*IW; for(int i=x;i<x+w;i++) r[i]=c; }
}
static void blendPx(int x,int y,uint32_t c,int a){
  if(x<0||y<0||x>=IW||y>=IH)return;
  uint32_t* p=&fb[y*IW+x]; uint32_t d=*p;
  int r=(int)((d>>16&255)+((((int)(c>>16&255)-(int)(d>>16&255))*a)>>8));
  int g=(int)((d>>8&255)+((((int)(c>>8&255)-(int)(d>>8&255))*a)>>8));
  int b=(int)((d&255)+((((int)(c&255)-(int)(d&255))*a)>>8));
  *p=0xFF000000u|((uint32_t)r<<16)|((uint32_t)g<<8)|(uint32_t)b;
}
static void disc(float cx,float cy,float r,uint32_t c){
  for(int y=(int)(cy-r);y<=(int)(cy+r);y++) for(int x=(int)(cx-r);x<=(int)(cx+r);x++){
    float dx=x-cx,dy=y-cy; if(dx*dx+dy*dy<=r*r) blendPx(x,y,c,255);
  }
}
static void text(int x,int y,const char* s,uint32_t c,int sc){
  for(;*s;s++){
    unsigned ch=(unsigned char)*s; if(ch<32||ch>95)ch=32;
    const char* g=FONT[ch-32];
    for(int r=0;r<6;r++){
      char hx[3]; hx[0]=g[r*2]; hx[1]=g[r*2+1]; hx[2]=0;
      int bits=(int)strtol(hx,0,16);
      for(int i=0;i<4;i++) if(bits&(8>>i)) rect(x+i*sc,y+r*sc,sc,sc,c);
    }
    x+=5*sc;
  }
}
static int textW(const char* s,int sc){ return (int)strlen(s)*5*sc; }

static void loadZone(int z){
  zone=z; nItems=0; nSpawn=0; nEns=0; fitting=0; fitT=0;
  memset(barr,0,sizeof(barr));
  for(int y=0;y<MH;y++) for(int x=0;x<MW;x++){
    char ch=MAPS[z][y][x];
    if(ch=='#'||ch=='D'){ grid[y][x]=ch; continue; }
    grid[y][x]='.';
    if(ch=='w'||ch=='o'||ch=='k'||ch=='T'||ch=='B'||ch=='P'||ch=='U'){
      if(nItems<MAXIT){ items[nItems].x=x; items[nItems].y=y; items[nItems].t=ch; items[nItems].alive=1; nItems++; }
    } else if(ch=='E'){ if(nSpawn<8){ spawnX[nSpawn]=x; spawnY[nSpawn]=y; nSpawn++; } }
    else if(ch=='S'){ stX=x; stY=y; }
    else if(ch=='X'){ exX=x; exY=y; }
  }
  if(z==2){
    barr[VAULT_Y][VAULT_X]=3;
    if(wearing) for(int i=0;i<nItems;i++) if(items[i].t=='U') items[i].alive=0;
  }
  px=(float)stX; py=(float)stY; saveX=stX; saveY=stY;
  phase=0; timer=52.0f; night=1;
  say(z==0?"NIGHT ONE. THE GATES BEHIND YOU HAVE ALREADY CLOSED.":
      z==1?"THE SMALL WORLD HALL. THE DOLLS ARE ALL FACING YOU.":
            "THE CASTLE. SOMETHING ABOVE YOU IS BEING FITTED.");
  sfx(V_SQUARE, 330.0f, 0.05f, 0.9994f);
}

static int solid(float tx,float ty){
  int x=(int)floorf(tx), y=(int)floorf(ty);
  if(x<0||y<0||x>=MW||y>=MH) return 1;
  if(grid[y][x]=='#') return 1;
  if(grid[y][x]=='D'&&barr[y][x]) return 1;
  return 0;
}

static int prevIdx[MW*MH];
static int queue[MW*MH];
static int bfs(int sx,int sy,int tx,int ty,int* ox,int* oy){
  for(int i=0;i<MW*MH;i++) prevIdx[i]=-1;
  int head=0, tail=0, start=sy*MW+sx, goal=ty*MW+tx;
  if(start<0||goal<0||start>=MW*MH||goal>=MW*MH) return 0;
  queue[tail++]=start; prevIdx[start]=start;
  while(head<tail){
    int cur=queue[head++];
    if(cur==goal){
      int n=0, c=cur;
      while(c!=start&&n<MAXPATH){ ox[n]=c%MW; oy[n]=c/MW; n++; c=prevIdx[c]; }
      for(int i=0;i<n/2;i++){ int t=ox[i];ox[i]=ox[n-1-i];ox[n-1-i]=t; t=oy[i];oy[i]=oy[n-1-i];oy[n-1-i]=t; }
      return n;
    }
    int x=cur%MW, y=cur/MW;
    const int dx[4]={1,-1,0,0}, dy[4]={0,0,1,-1};
    for(int i=0;i<4;i++){
      int nx=x+dx[i], ny=y+dy[i];
      if(nx<0||ny<0||nx>=MW||ny>=MH) continue;
      if(grid[ny][nx]=='#') continue;
      int k=ny*MW+nx;
      if(prevIdx[k]!=-1) continue;
      prevIdx[k]=cur; queue[tail++]=k;
    }
  }
  return 0;
}

static void spawnEnemies(void){
  nEns=0; int want=night+1;
  if(want>MAXEN) want=MAXEN;
  for(int i=0;i<want;i++){
    int sx=1, sy=1, ok=0;
    if(i<nSpawn){ sx=spawnX[i]; sy=spawnY[i]; ok=1; }
    else {
      int bd=-1;
      for(int k=0;k<160;k++){
        int x=1+rand()%(MW-2), y=1+rand()%(MH-2);
        if(grid[y][x]!='.') continue;
        int d=abs(x-(int)px)+abs(y-(int)py);
        if(d>bd){ bd=d; sx=x; sy=y; ok=1; }
      }
    }
    if(!ok){ sx=1; sy=1; }
    ens[i].x=(float)sx; ens[i].y=(float)sy;
    ens[i].cd=0; ens[i].t=0; ens[i].plen=0; ens[i].alive=1; ens[i].boss=0;
  }
  nEns=want;
}

static void spawnBoss(void){
  if(nEns>=MAXEN) return;
  float bx=26.0f, by=2.0f, bd=-1.0f;
  for(int y=1;y<5;y++) for(int x=26;x<32;x++){
    if(grid[y][x]!='.') continue;
    float d=fabsf(x-px)+fabsf(y-py);
    if(d>bd){ bd=d; bx=(float)x; by=(float)y; }
  }
  En* n=&ens[nEns++];
  n->x=bx; n->y=by; n->cd=0; n->t=0; n->plen=0; n->alive=1; n->boss=1;
  sfx(V_NOISE, 120.0f, 0.30f, 0.9992f);
}

static void damage(void){
  if(invuln>0) return;
  hp-=1; invuln=1.1f;
  sfx(V_NOISE, 90.0f, 0.34f, 0.9982f);
  sfx(V_SQUARE, 62.0f, 0.16f, 0.9993f);
  if(hp<=0){
    hp=0; state=S_DEAD;
    sfx(V_SINE, 110.0f, 0.30f, 0.99985f);
    say(wearing?"THE SUIT STAYS ON. IT HAS ALREADY FITTED ITSELF TO YOU."
               :"THE SUIT FITS. YOU ARE NOT IN IT.");
  }
}

static void pickup(void){
  for(int i=0;i<nItems;i++){
    if(!items[i].alive) continue;
    if(fabsf(items[i].x-px)<0.7f&&fabsf(items[i].y-py)<0.7f){
      items[i].alive=0;
      switch(items[i].t){
        case 'w': wood+=1; sfx(V_SQUARE, 520.0f, 0.10f, 0.9988f); say("CHAIR LEG. GOOD ENOUGH."); break;
        case 'o': fuel+=35; if(fuel>100)fuel=100; sfx(V_SINE, 700.0f, 0.12f, 0.9986f); say("LAMP OIL."); break;
        case 'k': hp+=1; if(hp>3)hp=3; sfx(V_SINE, 440.0f, 0.14f, 0.9988f); say("SCRAP METAL, CHEWED FLAT. YOU FEEL STITCHED."); break;
        case 'T': if(tokens<2){ tokens+=1; sfx(V_SQUARE, 880.0f, 0.13f, 0.9988f); say(LORE[(int)tokens-1]); } break;
        case 'B': batt+=1; sfx(V_SQUARE, 620.0f, 0.11f, 0.9988f); say("BATTERIES. FOR SOMETHING THAT TALKS."); break;
      }
    }
  }
}

static void wearSuit(void){
  if(wearing){ say("ALREADY ON. IT WILL NOT COME OFF NOW."); return; }
  wearing=1;
  for(int i=0;i<nItems;i++) if(items[i].t=='U') items[i].alive=0;
  sfx(V_SINE, 130.0f, 0.24f, 0.99985f);
  sfx(V_SQUARE, 196.0f, 0.12f, 0.9997f);
  sfx(V_NOISE, 400.0f, 0.10f, 0.9990f);
  say("THE FIFTY-FOURTH SUIT. IT IS WARM AND STILL WET AT THE COLLAR. THEY STOP RECOGNISING YOU.");
}

static void useExit(void){
  if(wearing && phase!=1){
    state=S_ASCEND; audDrone=0;
    sfx(V_SINE, 392.0f, 0.18f, 0.99988f);
    sfx(V_SINE, 523.0f, 0.14f, 0.99990f);
    return;
  }
  if(wearing && phase==1){
    say("THE GATE ONLY OPENS FOR STAFF IN DAYLIGHT. IT IS NOT DAYLIGHT.");
    return;
  }
  if(totalTokens()>=3){
    if(zone<2) state=S_PAUSE;
    else { state=S_CHOICE; sfx(V_SQUARE, 494.0f, 0.10f, 0.9997f); }
  } else say("SEALED. YOU NEED THREE PLAYBILLS.");
}

static void talkOrUse(void){
  int ax=(int)floorf(px+0.5f), ay=(int)floorf(py+0.5f);
  const int dx[4]={1,-1,0,0}, dy[4]={0,0,1,-1};
  for(int i=0;i<4;i++){
    int nx=ax+dx[i], ny=ay+dy[i];
    if(nx<0||ny<0||nx>=MW||ny>=MH) continue;
    char g=grid[ny][nx];
    if(g=='D'){
      if(barr[ny][nx]){ barr[ny][nx]=0; wood+=1; sfx(V_NOISE, 300.0f, 0.12f, 0.9988f); say("YOU TEAR THE BARRICADE DOWN."); return; }
      if(wood>0){ wood-=1; barr[ny][nx]=3; sfx(V_NOISE, 180.0f, 0.20f, 0.9986f); sfx(V_SQUARE, 140.0f, 0.10f, 0.9990f); say("BARRICADED. THREE HITS OF WOOD."); return; }
      say("NO WOOD."); return;
    }
    if(g=='S'){ saveX=nx; saveY=ny; sfx(V_SQUARE, 1000.0f, 0.10f, 0.9990f); say("TURNSTILE CLICKED. THE PARK REMEMBERS YOU HERE."); return; }
    if(g=='U'){ wearSuit(); return; }
    if(g=='P'){
      if(pb3){ say("THE WAX MAN IS QUIET NOW. HE GOT WHAT HE WANTED."); return; }
      if(batt>=3.0f){
        batt-=3.0f; pb3=1;
        sfx(V_SQUARE, 196.0f, 0.14f, 0.9994f);
        sfx(V_SQUARE, 261.6f, 0.12f, 0.9994f);
        say("HE SNAPS THE BATTERIES IN. THEN HE RAPS IT AT YOU: THE THIRD PLAYBILL, WORD FOR WORD.");
        return;
      }
      say("THE WAX MAN TAPS A DEAD MICROPHONE. HE NEEDS THREE BATTERIES. YOU HAVE ONE... OR NONE.");
      return;
    }
    if(g=='X'){ useExit(); return; }
  }
  say("NOTHING HERE.");
}

static void update(float dt){
  if(state!=S_PLAY) return;
  float mx=0,my=0;
  if(pad.leftStick.x<-40||(pad.buttons&SCE_PAD_BUTTON_LEFT)) mx-=1;
  if(pad.leftStick.x> 40||(pad.buttons&SCE_PAD_BUTTON_RIGHT)) mx+=1;
  if(pad.leftStick.y<-40||(pad.buttons&SCE_PAD_BUTTON_UP)) my-=1;
  if(pad.leftStick.y> 40||(pad.buttons&SCE_PAD_BUTTON_DOWN)) my+=1;
  if(mx!=0&&my!=0){ mx*=0.707f; my*=0.707f; }
  float sp=3.2f*dt;
  if(mx!=0){ float nx=px+mx*sp; if(!solid(nx+(mx>0?0.3f:-0.3f),py)) px=nx; }
  if(my!=0){ float ny=py+my*sp; if(!solid(px,ny+(my>0?0.3f:-0.3f))) py=ny; }

  if(mx!=0||my!=0){
    stepT--;
    if(stepT<=0){ stepT=18; sfx(V_NOISE, wearing?160.0f:220.0f, 0.035f, 0.9975f); }
  } else stepT=0;

  if(invuln>0) invuln-=dt;
  pickup();
  fuel-= (phase==1?5.2f:3.4f) * (wearing?1.6f:1.0f) * dt;
  if(fuel<0) fuel=0;

  audDrone = fitting ? 2 : (phase==1 ? 1 : 0);

  if(zone==2 && fitting==0 && px>FIT_X0 && px<FIT_X1 && py>FIT_Y0 && py<FIT_Y1){
    fitting=1; fitT=(int)(FITTING_SECS*30.0f);
    for(int y=0;y<MH;y++) for(int x=0;x<MW;x++) if(grid[y][x]=='D') barr[y][x]=8;
    spawnBoss();
    sfx(V_NOISE, 70.0f, 0.30f, 0.99990f);
    say("THE DOORS SHUT THEMSELVES. SOMETHING IS BEING FITTED UP HERE WITH YOU.");
  }
  if(fitting==1){
    fitT--;
    if(fitT<=0){
      fitting=2;
      for(int y=0;y<MH;y++) for(int x=0;x<MW;x++) if(grid[y][x]=='D') barr[y][x]=0;
      sfx(V_SQUARE, 392.0f, 0.16f, 0.99980f);
      say("THE DOORS LET GO. THE SUIT IS STILL WARM.");
    }
  }

  if(phase==2) return;
  timer-=dt;
  if(timer<=0){
    if(phase==0){ phase=1; timer=38; spawnEnemies(); say("LIGHTS DOWN. SOMETHING IN THE WALKWAYS STARTS WALKING."); sfx(V_SINE, 82.0f, 0.22f, 0.99985f); }
    else if(night<3){ night++; phase=0; timer=52; nEns=0; say("MORNING. MORE OF THEM NOW."); sfx(V_SQUARE, 262.0f, 0.10f, 0.9995f); }
    else { phase=2; nEns=0; timer=0; say("THIRD DAWN. THE EXIT IS OPEN IF YOU HAVE THE PLAYBILLS."); sfx(V_SQUARE, 392.0f, 0.14f, 0.9995f); }
  }

  for(int e=0;e<nEns;e++){
    En* n=&ens[e];
    n->t-=dt;
    if(n->t<=0){
      n->plen=bfs((int)(n->x+0.5f),(int)(n->y+0.5f),(int)(px+0.5f),(int)(py+0.5f),n->px,n->py);
      n->t= n->boss?0.28f:0.42f;
    }
    if(n->plen>0){
      int nx=n->px[0], ny=n->py[0];
      if(grid[ny][nx]=='D'&&barr[ny][nx]){
        n->cd-=dt;
        if(n->cd<=0){ n->cd= n->boss?0.34f:0.7f; if(--barr[ny][nx]<=0){ barr[ny][nx]=0; say("A BARRICADE GIVES WAY."); } }
      } else {
        float dx=nx-n->x, dy=ny-n->y, d=sqrtf(dx*dx+dy*dy);
        if(d<0.12f){ memmove(n->px,n->px+1,(n->plen-1)*sizeof(int)); memmove(n->py,n->py+1,(n->plen-1)*sizeof(int)); n->plen--; }
        else { float s=(n->boss?2.35f:1.55f)*dt; n->x+=dx/d*s; n->y+=dy/d*s; }
      }
    }
    if(fabsf(n->x-px)<0.7f&&fabsf(n->y-py)<0.7f){
      if(!wearing || n->boss) damage();
    }
  }
}

static float radius(void){
  float r=30.0f+(fuel/100.0f)*82.0f;
  if(phase==1) r*=0.86f;
  if(wearing) r*=1.10f;
  return r*(0.96f+(float)sin((double)(sceKernelGetProcessTimeLow()%1000000)/140.0)*0.04f);
}

static void render(void){
  float camX=px*TS+TS*0.5f-IW*0.5f, camY=py*TS+TS*0.5f-IH*0.5f;
  if(camX<0)camX=0; if(camY<0)camY=0;
  if(camX>(float)(MW*TS-IW))camX=(float)(MW*TS-IW);
  if(camY>(float)(MH*TS-IH))camY=(float)(MH*TS-IH);

  for(int j=0;j<IH;j++){ uint32_t* r=fb+j*IW; for(int i=0;i<IW;i++) r[i]=0xFF0B0910; }

  int x0=(int)(camX/TS), x1=x0+IW/TS+2, y0=(int)(camY/TS), y1=y0+IH/TS+2;
  for(int y=y0;y<y1;y++) for(int x=x0;x<x1;x++){
    if(x<0||y<0||x>=MW||y>=MH) continue;
    int sx=(int)(x*TS-camX), sy=(int)(y*TS-camY);
    char g=grid[y][x];
    if(g=='#'){
      rect(sx,sy,TS,TS,C_WALL); rect(sx,sy,TS,2,C_WALLL);
    } else {
      rect(sx,sy,TS,TS,((x+y)&1)?C_FLOOR1:C_FLOOR2);
      if(g=='D'){
        rect(sx+2,sy+2,TS-4,TS-4, barr[y][x]?C_BARR:C_DOOR);
        if(barr[y][x]) for(int i=0;i<barr[y][x]&&i<6;i++) rect(sx+3,sy+4+i*3,TS-6,1,RGB(169,124,70));
      }
      if(g=='S'){ rect(sx+3,sy+3,TS-6,TS-6,RGB(38,32,58)); rect(sx+3,sy+3,TS-6,1,RGB(92,77,134)); }
      if(g=='X'){
        int on=(totalTokens()>=3)||(wearing&&phase!=1);
        int asc=(wearing&&phase!=1);
        rect(sx+3,sy+3,TS-6,TS-6,on?RGB(58,42,18):RGB(30,26,46));
        rect(sx+3,sy+3,TS-6,1, asc?RGB(226,214,190):(on?RGB(217,161,60):RGB(70,60,100)));
      }
      if(g=='P'){ rect(sx+5,sy+3,10,14,RGB(120,96,60)); rect(sx+6,sy+4,8,5,RGB(200,180,140));
        if(!pb3){ rect(sx+7,sy+11,6,2,RGB(230,200,120)); } }
    }
  }
  for(int i=0;i<nItems;i++){
    if(!items[i].alive) continue;
    int sx=(int)(items[i].x*TS-camX), sy=(int)(items[i].y*TS-camY);
    if(sx<-TS||sy<-TS||sx>IW||sy>IH) continue;
    switch(items[i].t){
      case 'w': rect(sx+5,sy+8,10,4,RGB(107,84,51)); break;
      case 'o': disc((float)sx+10,(float)sy+10,4,C_WARM); break;
      case 'k': disc((float)sx+10,(float)sy+10,3.5f,RGB(122,132,148)); break;
      case 'B': rect(sx+6,sy+5,8,10,RGB(90,90,110)); rect(sx+7,sy+6,6,2,RGB(200,200,160)); break;
      case 'U':
        rect(sx+5,sy+4,10,13,RGB(148,144,158));
        disc((float)sx+6,(float)sy+4,3.4f,RGB(126,122,136));
        disc((float)sx+14,(float)sy+4,3.4f,RGB(126,122,136));
        rect(sx+7,sy+8,6,2,RGB(28,22,34));
        break;
      default: rect(sx+6,sy+5,8,10,RGB(232,220,192)); rect(sx+6,sy+10,8,1,RGB(138,122,90));
    }
  }
  for(int e=0;e<nEns;e++){
    int sx=(int)(ens[e].x*TS-camX+TS*0.5f), sy=(int)(ens[e].y*TS-camY+TS*0.5f);
    float rr= ens[e].boss?11.0f:7.5f;
    disc((float)sx,(float)sy,rr, ens[e].boss?RGB(72,52,58):RGB(59,51,72));
    for(int i=-(int)rr+2;i<=(int)rr-2;i++) blendPx(sx+i,sy+4,RGB(10,8,16),255);
    disc((float)sx-3,(float)sy-3,1.4f,RGB(200,194,214));
    disc((float)sx+3,(float)sy-3,1.4f,RGB(200,194,214));
    if(ens[e].boss){ disc((float)sx-3,(float)sy-3,2.6f,RGB(217,90,60)); disc((float)sx+3,(float)sy-3,2.6f,RGB(217,90,60)); }
  }
  int psx=(int)(px*TS-camX+TS*0.5f), psy=(int)(py*TS-camY+TS*0.5f);
  if(invuln<=0||(((int)(invuln*12))&1)==0){
    if(wearing){
      disc((float)psx,(float)psy,9.0f,RGB(150,146,160));
      disc((float)psx-5,(float)psy-6,3.8f,RGB(126,122,136));
      disc((float)psx+5,(float)psy-6,3.8f,RGB(126,122,136));
      for(int i=-5;i<=5;i++) blendPx(psx+i,psy+4,RGB(18,14,22),255);
      disc((float)psx-3,(float)psy-2,1.5f,RGB(242,238,248));
      disc((float)psx+3,(float)psy-2,1.5f,RGB(242,238,248));
    } else {
      disc((float)psx,(float)psy,6.5f,RGB(201,183,160));
      disc((float)psx-4,(float)psy-4,3,RGB(143,127,107));
      disc((float)psx+4,(float)psy-4,3,RGB(143,127,107));
    }
  }

  float R=radius();
  float wpx=px*TS+TS*0.5f, wpy=py*TS+TS*0.5f;
  for(int i=0;i<360;i++){
    float ang=(float)i*(PI/180.0f);
    float dx=cosf(ang), dy=sinf(ang);
    float d=0;
    for(;;){
      d+=2.0f;
      if(d>R) break;
      int gx=(int)((wpx+dx*d)/TS), gy=(int)((wpy+dy*d)/TS);
      if(gx<0||gy<0||gx>=MW||gy>=MH) break;
      if(grid[gy][gx]=='#') break;
    }
    rayD[i]=d;
  }
  for(int y=0;y<IH;y++){
    float dy=(float)(y-psy);
    for(int x=0;x<IW;x++){
      float dx=(float)(x-psx);
      float dist=sqrtf(dx*dx+dy*dy);
      if(dist>R){ fb[y*IW+x]=0xFF06040C; continue; }
      int b=(int)((atan2f(dy,dx)+PI)*(180.0f/PI));
      if(b<0)b=0; if(b>359)b=359;
      float k=dist/R;
      if(dist>rayD[b]) k=1.0f;
      int a=(int)(k*k*250.0f);
      if(a<=0) continue;
      uint32_t* p=&fb[y*IW+x]; uint32_t d=*p;
      int r=(int)((d>>16&255)*a/255), g=(int)((d>>8&255)*a/255), bb=(int)((d&255)*a/255);
      *p=0xFF000000u|((uint32_t)r<<16)|((uint32_t)g<<8)|(uint32_t)bb;
    }
  }

  rect(0,IH-34,IW,34,0xFF08060E);
  rect(12,IH-22,150,8,RGB(58,48,72));
  rect(12,IH-22,(int)(150.0f*fuel/100.0f),8, fuel<25?C_RED:C_WARM);
  text(12,IH-31,"LANTERN",C_HUD,1);
  for(int i=0;i<3;i++) disc((float)(190+i*16),(float)(IH-18),5,i<(int)hp?C_RED:RGB(44,36,56));
  char b[64];
  snprintf(b,64,"WOOD %d",(int)wood); text(248,IH-22,b,C_HUD,1);
  if(batt>0){ snprintf(b,64,"BATT %d",(int)batt); text(248,IH-32,b,RGB(160,150,190),1); }
  if(wearing){ text(248,IH-11,"54TH SUIT",RGB(226,214,190),1); }
  snprintf(b,64,"PLAYBILLS %d/3",totalTokens()); text(320,IH-22,b,C_HUD,1);
  if(fitting==1){ snprintf(b,64,"FITTING %ds",fitT/30); text(410,IH-22,b,C_RED,1); }
  else if(phase==2) text(430,IH-22,"DAWN",C_WARM,1);
  else { snprintf(b,64,"%s %d %ds", phase?"NIGHT":"DAY", night,(int)timer); text(430,IH-22,b,C_HUD,1); }

  if(msgT>0){ rect(0,0,IW,30,0xFF08060E); text(10,11,msg,C_PALE,1); }
}

static void present(void){
  uint32_t* dst=vram[vidx];
  for(int y=0;y<IH;y++){
    uint32_t* s=fb+y*IW;
    uint32_t* d0=dst+(y*SC)*FBW;
    for(int sy=0;sy<SC;sy++){
      uint32_t* d=d0+sy*FBW;
      for(int x=0;x<IW;x++){ uint32_t c=s[x]; int o=x*SC; d[o]=c;d[o+1]=c;d[o+2]=c;d[o+3]=c; }
    }
  }
  sceVideoOutSubmitFlip(vh,vidx,SCE_VIDEO_OUT_FLIP_MODE_VSYNC,0);
  vidx^=1;
}

int main(void){
  vh=sceVideoOutOpen(0,SCE_VIDEO_OUT_BUS_TYPE_MAIN,0,NULL);
  if(vh<0) return 1;
  SceVideoOutBufferAttribute attr;
  sceVideoOutSetBufferAttribute(&attr,SCE_VIDEO_OUT_PIXEL_FORMAT_A8R8G8B8,
    SCE_VIDEO_OUT_TILING_MODE_LINEAR,SCE_VIDEO_OUT_ASPECT_RATIO_16_9,FBW,FBH,FBW);
  size_t sz=(size_t)FBW*FBH*4;
  off_t dmem=0;
  sceKernelAllocateDirectMemory(0,sceKernelGetDirectMemorySize(),sz*2,0x200000,3,&dmem);
  void* mem=NULL;
  sceKernelMapDirectMemory(&mem,sz*2,0x33,0,dmem,0x200000);
  vram[0]=(uint32_t*)mem; vram[1]=(uint32_t*)((char*)mem+sz);
  sceVideoOutRegisterBuffers(vh,0,(void* const*)vram,2,&attr);

  audInit();
  scePadInit();
  int padHandle=scePadOpen(SCE_USER_SERVICE_USER_ID_SYSTEM,0,0,NULL);
  memset(&pad,0,sizeof(pad));

  srand((unsigned)sceKernelGetProcessTimeLow());
  state=S_TITLE; hp=3; fuel=100; wood=2; tokens=0; batt=0; pb3=0; wearing=0; msgT=0; invuln=0;
  loadZone(0);

  int prevCross=0, prevOpt=0, prevTri=0;
  for(;;){
    scePadReadState(padHandle,&pad);
    int cross=(pad.buttons&SCE_PAD_BUTTON_CROSS)?1:0;
    int opt=(pad.buttons&SCE_PAD_BUTTON_OPTIONS)?1:0;
    int tri=(pad.buttons&SCE_PAD_BUTTON_TRIANGLE)?1:0;
    int pressed=cross&&!prevCross, optP=opt&&!prevOpt, triP=tri&&!prevTri;
    prevCross=cross; prevOpt=opt; prevTri=tri;

    if(state==S_TITLE){
      if(pressed){ hp=3; fuel=100; wood=2; tokens=0; batt=0; pb3=0; wearing=0; loadZone(0); state=S_PLAY; }
    } else if(state==S_DEAD){
      if(pressed){
        px=(float)saveX; py=(float)saveY; hp=3; fuel=70; tokens=0; batt=0;
        if(wood>0)wood-=1;
        memset(barr,0,sizeof(barr)); nEns=0; fitting=0; fitT=0;
        if(zone==2) barr[VAULT_Y][VAULT_X]= wearing?0:3;
        phase=0; night=1; timer=52; state=S_PLAY; invuln=1.2f; spawnEnemies();
        say("YOU WAKE AT THE TURNSTILE. YOUR PLAYBILLS ARE GONE.");
      }
    } else if(state==S_WIN||state==S_ASCEND||state==S_STAY){
      if(pressed) state=S_TITLE;
    } else if(state==S_CHOICE){
      if(pressed){ state=S_WIN; sfx(V_SINE, 523.0f, 0.20f, 0.99980f); }
      else if(triP){ state=S_STAY; sfx(V_SINE, 174.0f, 0.22f, 0.99988f); sfx(V_SQUARE, 130.8f, 0.12f, 0.99985f); }
      else if(optP) state=S_PLAY;
    } else if(state==S_PAUSE){
      if(pressed){ if(zone<2){ loadZone(zone+1); state=S_PLAY; } else state=S_CHOICE; }
    } else if(state==S_PLAY){
      if(pressed) talkOrUse();
      if(optP){ state=S_PAUSE; say("PAUSED."); }
      if(triP&&msgT<=0) say(totalTokens()>0?LORE[totalTokens()-1]:"NO PLAYBILLS YET.");
      if(msgT>0) msgT-=0.0166f;
      update(0.0166f);
    }
    if(state!=S_PLAY&&msgT>0) msgT-=0.0166f;

    render();

    if(state==S_TITLE){
      rect(0,0,IW,IH,0xE6000000);
      text(IW/2-textW("WISHWOOD",4)/2,64,"WISHWOOD",C_PALE,4);
      text(IW/2-textW("KINGDOM 3030",2)/2,104,"KINGDOM 3030",C_WARM,2);
      text(IW/2-textW("THREE NIGHTS. THREE PLAYBILLS. ONE SMILE.",1)/2,146,
           "THREE NIGHTS. THREE PLAYBILLS. ONE SMILE.",C_HUD,1);
      text(IW/2-textW("PRESS CROSS",2)/2,196,"PRESS CROSS",C_PALE,2);
    }
    if(state==S_DEAD){
      rect(0,0,IW,IH,0xE6000000);
      text(IW/2-textW("THE SUIT TOOK YOU",3)/2,IH/2-20,"THE SUIT TOOK YOU",C_RED,3);
      text(IW/2-textW("PRESS CROSS",2)/2,IH/2+20,"PRESS CROSS",C_HUD,2);
    }
    if(state==S_CHOICE){
      rect(0,0,IW,IH,0xE6000000);
      text(IW/2-textW("THE TURNSTILE IS OPEN",2)/2,90,"THE TURNSTILE IS OPEN",C_PALE,2);
      text(IW/2-textW("CROSS TO LEAVE",2)/2,130,"CROSS TO LEAVE",C_WARM,2);
      text(IW/2-textW("TRIANGLE TO STAY",2)/2,162,"TRIANGLE TO STAY",RGB(226,214,190),2);
      text(IW/2-textW("OPTIONS TO STEP BACK",1)/2,198,"OPTIONS TO STEP BACK",C_HUD,1);
    }
    if(state==S_WIN){
      rect(0,0,IW,IH,0xEE000000);
      text(IW/2-textW("YOU LEFT.",3)/2,IH/2-30,"YOU LEFT.",C_PALE,3);
      text(IW/2-textW("YOU ARE STILL SMILING.",1)/2,IH/2+6,"YOU ARE STILL SMILING.",C_HUD,1);
      text(IW/2-textW("PRESS CROSS",2)/2,IH/2+40,"PRESS CROSS",C_HUD,2);
    }
    if(state==S_STAY){
      rect(0,0,IW,IH,0xF2000000);
      text(IW/2-textW("YOU STAYED",4)/2,58,"YOU STAYED",RGB(226,214,190),4);
      text(IW/2-textW("THEY FIT THE SUIT TO YOU AT OPENING.",1)/2,104,
           "THEY FIT THE SUIT TO YOU AT OPENING.",C_HUD,1);
      text(IW/2-textW("IT FITS PERFECTLY. THE SMILE DOES NOT.",1)/2,122,
           "IT FITS PERFECTLY. THE SMILE DOES NOT.",C_HUD,1);
      text(IW/2-textW("FIFTY-FOUR BECOMES FIFTY-FIVE.",1)/2,150,
           "FIFTY-FOUR BECOMES FIFTY-FIVE.",C_WARM,1);
      text(IW/2-textW("PRESS CROSS",2)/2,196,"PRESS CROSS",C_HUD,2);
    }
    if(state==S_ASCEND){
      rect(0,0,IW,IH,0xF2000000);
      text(IW/2-textW("ASCENDED",4)/2,58,"ASCENDED",RGB(232,224,206),4);
      text(IW/2-textW("YOU WALKED OUT WEARING THE FIFTY-FOURTH.",1)/2,104,
           "YOU WALKED OUT WEARING THE FIFTY-FOURTH.",C_HUD,1);
      text(IW/2-textW("NOBODY STOPPED YOU. NOBODY EVER DOES.",1)/2,122,
           "NOBODY STOPPED YOU. NOBODY EVER DOES.",C_HUD,1);
      text(IW/2-textW("THE PARK LETS STAFF GO. THAT IS THE WHOLE TRICK.",1)/2,140,
           "THE PARK LETS STAFF GO. THAT IS THE WHOLE TRICK.",C_HUD,1);
      text(IW/2-textW("YOU ARE NOT A GUEST ANY MORE.",1)/2,166,
           "YOU ARE NOT A GUEST ANY MORE.",RGB(217,161,60),1);
      text(IW/2-textW("PRESS CROSS",2)/2,204,"PRESS CROSS",C_HUD,2);
    }
    if(state==S_PAUSE){
      rect(0,0,IW,IH,0xF0000000);
      text(IW/2-textW("THE TURNSTILE TAKES YOU DEEPER",2)/2,IH/2-10,
           "THE TURNSTILE TAKES YOU DEEPER",C_PALE,2);
      text(IW/2-textW("PRESS CROSS",1)/2,IH/2+24,"PRESS CROSS",C_HUD,1);
    }

    present();
    sceVideoOutWaitVblank(vh);
  }
  return 0;
}

