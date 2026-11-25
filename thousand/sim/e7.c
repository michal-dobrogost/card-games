/*
 * e7.c  -  bid euchre with a 28-card deck (A K Q J 10 9 8), 7 cards each, 7 tricks.
 * card = suit*7 + rank ; ranks 0=8 1=9 2=10 3=J 4=Q 5=K 6=A ; suits 0 clubs 1 spades 2 hearts 3 diamonds
 * strain 0-3 trump suit, 4 = high (no trump, A high), 5 = low (no trump, 8 high)
 *
 * modes:
 *   e7 table DEALS SEED                 -> deal masks + perfect-play tricks for every seat x strain
 *   e7 sp DEALS SEED STRAIN MINFILTER   -> maker (seat 3) plays none / salt / pepper / salt&pepper with hidden info
 *        STRAIN = -1 (maker picks a suit) or 4 (high);  MINFILTER = min trumps (suit) or min aces (high)
 *   e7 brute DEALS SEED                 -> print one deal and its perfect-play tricks (for checking)
 *
 * Optional last argument for "sp": SAMPLES = hidden layouts tried per decision (default 12).
 * Build:  gcc -O2 -o e7 e7.c        (use run_parallel.py to run on every core)
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#define NC 28
#define NR 7
#define NT 7
static int strain, esuit[NC], pw[NC];
static void setup(int s){
  strain=s;
  for(int c=0;c<NC;c++){
    int su=c/NR, r=c%NR; esuit[c]=su; pw[c]=(s==5)?(NR-1-r):r;
    if(s<4){
      if(r==3&&su==s) pw[c]=120;
      else if(r==3&&su==(s^1)){esuit[c]=s;pw[c]=119;}
      else if(su==s) pw[c]=100+r;
    }
  }
}
static int istr(int c){return strain<4 && esuit[c]==strain;}
static int beats(int a,int b,int led){
  int ta=istr(a),tb=istr(b);
  if(ta&&tb) return pw[a]>pw[b];
  if(ta) return 1;
  if(tb) return 0;
  if(esuit[a]!=led) return 0;
  if(esuit[b]!=led) return 1;
  return pw[a]>pw[b];
}
static int keepval(int c){ if(istr(c)) return 100+pw[c]; return (pw[c]==NR-1)?50:pw[c]; }
static int teamA(int p){return p==1||p==3;}

/* ---------- exact solver ---------- */
static uint32_t hands[4], live; static int alone=0;
#define TTSZ (1<<22)
static uint64_t *ttk; static int8_t *ttv; static uint64_t salt;
static uint64_t mix(uint64_t x){x^=x>>33;x*=0xff51afd7ed558ccdULL;x^=x>>33;x*=0xc4ceb9fe1a85ec53ULL;x^=x>>33;return x;}
static int can(int leader,int need,int left);
static int redundant(int c,uint32_t h){
  int best=-1,bp=1<<30;
  for(int d=0;d<NC;d++) if((live>>d&1)&&d!=c&&esuit[d]==esuit[c]&&pw[d]>pw[c]&&pw[d]<bp){bp=pw[d];best=d;}
  return best>=0 && (h>>best&1);
}
static int trick(int leader,int pos,int led,int win,int wc,int need,int left){
  if(pos==4) return can(win,need-(teamA(win)?1:0),left-1);
  int p=(leader+pos)&3;
  if(alone && p==1) return trick(leader,pos+1,led,win,wc,need,left);
  uint32_t h=hands[p],legal=0;
  if(pos>0) for(int c=0;c<NC;c++) if((h>>c&1)&&esuit[c]==led) legal|=1u<<c;
  if(!legal) legal=h;
  int maxim=teamA(p);
  for(int c=NC-1;c>=0;c--) if(legal>>c&1){
    if(redundant(c,legal)) continue;
    hands[p]&=~(1u<<c);
    int nl=led,nw=win,nwc=wc;
    if(pos==0){nl=esuit[c];nw=p;nwc=c;} else if(beats(c,wc,led)){nw=p;nwc=c;}
    int v=trick(leader,pos+1,nl,nw,nwc,need,left);
    hands[p]|=1u<<c;
    if(maxim&&v) return 1;
    if(!maxim&&!v) return 0;
  }
  return maxim?0:1;
}
static int can(int leader,int need,int left){
  if(need<=0) return 1;
  if(need>left) return 0;
  uint64_t key=mix(((uint64_t)hands[0]|((uint64_t)hands[1]<<28))^salt)^mix(((uint64_t)hands[2]|((uint64_t)hands[3]<<28))+leader*7+need*131+salt*3);
  uint32_t i=key&(TTSZ-1);
  if(ttk[i]==key) return ttv[i];
  uint32_t sv=live; live=hands[0]|hands[1]|hands[2]|hands[3];
  int v=trick(leader,0,0,0,0,need,left);
  live=sv; ttk[i]=key; ttv[i]=v; return v;
}
static int maxtricks(uint32_t H[4],int s,int al){
  setup(s); alone=al; salt=mix(s*1000003ULL+12345+al*777+(uint64_t)H[0]*31+H[1]);
  int k=0;
  for(int t=1;t<=NT;t++){ memcpy(hands,H,16); if(al) hands[1]=0; if(can(3,t,NT)) k=t; else break; }
  alone=0; return k;
}
static uint32_t trickcards;
static int ddval(int leader,int pos,int led,int win,int wc,int left){
  uint32_t sv=live; live=hands[0]|hands[1]|hands[2]|hands[3]|trickcards;
  int k=0;
  for(int n=1;n<=left;n++){ if(trick(leader,pos,led,win,wc,n,left)) k=n; else break; }
  live=sv; return k;
}
/* ---------- random ---------- */
static uint64_t rs=88172645463325252ULL, rs2=1234567ULL;
static uint64_t rnd(){rs^=rs<<13;rs^=rs>>7;rs^=rs<<17;return rs;}
static uint32_t r32(){rs2^=rs2<<13;rs2^=rs2>>7;rs2^=rs2<<17;return (uint32_t)rs2;}
static void deal(uint32_t H[4]){
  int d[NC]; for(int i=0;i<NC;i++)d[i]=i;
  for(int i=NC-1;i>0;i--){int j=rnd()%(i+1);int t=d[i];d[i]=d[j];d[j]=t;}
  memset(H,0,16); for(int i=0;i<NC;i++) H[i/NT]|=1u<<d[i];
}
static int choose(uint32_t h){
  int bs=0,bv=-1;
  for(int s=0;s<4;s++){ setup(s); int v=0;
    for(int c=0;c<NC;c++) if(h>>c&1){
      if(istr(c)) v+= pw[c]>=119?40:(pw[c]==100+NR-1?25:15)+pw[c]-100;
      else if(c%NR==NR-1) v+=10; }
    if(v>bv){bv=v;bs=s;} }
  return bs;
}
/* ---------- card-passing agreements ---------- */
enum { ROLE_PARTNER=0, ROLE_MAKER=1, ROLE_DEFENDER=2 };
static int conv_give(int role,uint32_t h){
  int best=-1,bv=-1000000;
  if(role==ROLE_PARTNER){              /* highest trump; else a top card; else best */
    for(int c=0;c<NC;c++) if(h>>c&1){
      int v = istr(c)? 1000+pw[c] : (pw[c]==NR-1? 500-c : keepval(c));
      if(v>bv){bv=v;best=c;}
    }
    return best;
  }
  int cnt[4]={0}; for(int c=0;c<NC;c++) if((h>>c&1)&&!istr(c)) cnt[esuit[c]]++;
  if(role==ROLE_MAKER){
    if(strain>=4){ for(int c=0;c<NC;c++) if(h>>c&1){ int v=-keepval(c)*10-esuit[c]; if(v>bv){bv=v;best=c;} } return best; }
    for(int c=0;c<NC;c++) if(h>>c&1){
      int v;
      if(istr(c)) v=-1000-pw[c];
      else if(cnt[esuit[c]]==1) v=1000-pw[c]*10-esuit[c];
      else v=0-pw[c]*10-esuit[c];
      if(v>bv){bv=v;best=c;}
    }
    return best;
  }
  /* defenders consolidate: strong hand (2+ trumps, or 2+ top-two cards in no-trump) passes its worst card */
  int st=0; for(int c=0;c<NC;c++) if((h>>c&1)&&(strain>=4 ? pw[c]>=NR-2 : istr(c))) st++;
  for(int c=0;c<NC;c++) if(h>>c&1){ int v=(st>=2)?-keepval(c):keepval(c); if(v>bv){bv=v;best=c;} }
  return best;
}
/* ---------- knowledge & sampling ---------- */
static uint32_t kin[4][4], played[4]; static int voids[4];
typedef struct { int active, partner, gave, got, prole; } Exch;
static Exch ex[4];
static int consistent(int D,uint32_t w[4]){
  if(!ex[D].active) return 1;
  int P=ex[D].partner;
  if(alone && P==1) return 1;
  uint32_t pre=(w[P]|played[P]); pre&=~(1u<<ex[D].gave); pre|=1u<<ex[D].got;
  return conv_give(ex[D].prole,pre)==ex[D].got;
}
static int NS=12;
static int sample(uint32_t act[4],int D,uint32_t out[4]){
  uint32_t base[4],pool=0; int cap[4];
  for(int q=0;q<4;q++){ if(q==D){base[q]=act[q];cap[q]=0;} else {pool|=act[q];base[q]=0;cap[q]=__builtin_popcount(act[q]);} }
  for(int q=0;q<4;q++) if(q!=D){ uint32_t k=kin[D][q]&pool; base[q]|=k; pool&=~k; cap[q]-=__builtin_popcount(k); }
  int cards[NC],n=0; for(int c=0;c<NC;c++) if(pool>>c&1) cards[n++]=c;
  int hf=0; uint32_t fb[4];
  for(int tries=0;tries<400;tries++){
    uint32_t o[4]; int cp[4]; memcpy(o,base,16); memcpy(cp,cap,sizeof cap);
    for(int i=n-1;i>0;i--){int j=r32()%(i+1);int t=cards[i];cards[i]=cards[j];cards[j]=t;}
    int ok=1, relax=(tries>=350);
    for(int i=0;i<n&&ok;i++){ int c=cards[i]; int el[4],ne=0;
      for(int q=0;q<4;q++) if(cp[q]>0 && (relax || !(voids[q]>>esuit[c]&1))) el[ne++]=q;
      if(!ne){ok=0;break;} int q=el[r32()%ne]; o[q]|=1u<<c; cp[q]--; }
    if(!ok) continue;
    if(!hf){memcpy(fb,o,16);hf=1;}
    if(consistent(D,o)){ memcpy(out,o,16); return 1; }
  }
  if(hf){ memcpy(out,fb,16); return 1; }
  return 0;
}
static void do_exchange(uint32_t act[4],int a,int ra,int b,int rb){
  int ga=conv_give(ra,act[a]), gb=conv_give(rb,act[b]);
  act[a]=(act[a]&~(1u<<ga))|(1u<<gb); act[b]=(act[b]&~(1u<<gb))|(1u<<ga);
  kin[a][b]|=1u<<ga; kin[b][a]|=1u<<gb;
  ex[a]=(Exch){1,b,ga,gb,rb}; ex[b]=(Exch){1,a,gb,ga,ra};
}
/* play one hand with hidden information; seat 3 = maker, leads */
static int playhand(uint32_t H0[4],int tr,int saltc,int al){
  setup(tr); salt=mix(tr*31337ULL+11+al*7777ULL);
  uint32_t act[4]; memcpy(act,H0,16);
  memset(kin,0,sizeof kin); memset(voids,0,sizeof voids); memset(played,0,sizeof played); memset(ex,0,sizeof ex);
  if(saltc){ do_exchange(act,3,ROLE_MAKER,1,ROLE_PARTNER); do_exchange(act,0,ROLE_DEFENDER,2,ROLE_DEFENDER); }
  alone=al; if(al){ act[1]=0; for(int q=0;q<4;q++) kin[q][1]=0; }
  int leader=3,tricksA=0;
  for(int tk=0;tk<NT;tk++){
    int led=0,win=0,wc=0; trickcards=0;
    for(int pos=0;pos<4;pos++){
      int p=(leader+pos)&3;
      if(alone && p==1) continue;
      uint32_t h=act[p],legal=0;
      if(pos>0) for(int c=0;c<NC;c++) if((h>>c&1)&&esuit[c]==led) legal|=1u<<c;
      if(!legal) legal=h;
      int choice=-1;
      if(__builtin_popcount(legal)==1) choice=__builtin_ctz(legal);
      else {
        double sc[NC]={0};
        for(int s=0;s<NS;s++){
          uint32_t w[4]; if(!sample(act,p,w)) memcpy(w,act,16);
          for(int c=0;c<NC;c++) if(legal>>c&1){
            memcpy(hands,w,16); hands[p]&=~(1u<<c);
            int nl=led,nw=win,nwc=wc;
            if(pos==0){nl=esuit[c];nw=p;nwc=c;} else if(beats(c,wc,led)){nw=p;nwc=c;}
            trickcards|=1u<<c;
            int v=ddval(leader,pos+1,nl,nw,nwc,NT-tk);
            trickcards&=~(1u<<c);
            sc[c]+= teamA(p)? v : -v;
          }
        }
        double best=-1e9;
        for(int c=0;c<NC;c++) if(legal>>c&1){ double v=sc[c]-1e-6*keepval(c); if(v>best){best=v;choice=c;} }
      }
      if(pos>0 && esuit[choice]!=led) voids[p]|=1<<led;
      act[p]&=~(1u<<choice); played[p]|=1u<<choice; trickcards|=1u<<choice;
      for(int q=0;q<4;q++) for(int z=0;z<4;z++) kin[q][z]&=~(1u<<choice);
      if(pos==0){led=esuit[choice];win=p;wc=choice;} else if(beats(choice,wc,led)){win=p;wc=choice;}
    }
    if(teamA(win)) tricksA++;
    leader=win;
  }
  alone=0; return tricksA;
}
int main(int argc,char**argv){
  ttk=calloc(TTSZ,8); ttv=calloc(TTSZ,1);
  if(argc<4){ fprintf(stderr,"usage: e7 table DEALS SEED | e7 sp DEALS SEED STRAIN MIN [SAMPLES] | e7 brute 1 SEED\n"); return 1; }
  int N=atoi(argv[2]); rs=strtoull(argv[3],0,10)*2654435761ULL+1; rs2=rs*3+1;
  if(!strcmp(argv[1],"table")){
    printf("deal,h0,h1,h2,h3");
    for(int s=0;s<4;s++) for(int x=0;x<6;x++) printf(",seat%d_%s",s,x<4?(const char*[]){"clubs","spades","hearts","diamonds"}[x]:(x==4?"high":"low"));
    printf("\n");
    for(int n=0;n<N;n++){
      uint32_t H[4]; deal(H);
      printf("%d,%u,%u,%u,%u",n,H[0],H[1],H[2],H[3]);
      for(int s=0;s<4;s++){ uint32_t R[4]; for(int j=0;j<4;j++) R[j]=H[(s+j+1)&3];
        for(int x=0;x<6;x++) printf(",%d",maxtricks(R,x,0)); }
      printf("\n"); fflush(stdout);
    }
    return 0;
  }
  if(!strcmp(argv[1],"sp")){
    int fstr=atoi(argv[4]), fmin=atoi(argv[5]); if(argc>6) NS=atoi(argv[6]);
    printf("deal,trump,maker_trumps,maker_bowers,maker_side_aces,maker_aces,maker_kings,partner_trumps,dd0,none,salt,pepper,saltpepper\n");
    for(int n=0;n<N;n++){
      uint32_t H[4]; deal(H);
      int tr = fstr<0 ? choose(H[3]) : fstr; setup(tr);
      int mt=0,mb=0,msa=0,ma=0,mk=0,pt=0;
      for(int c=0;c<NC;c++){
        if(H[3]>>c&1){
          if(istr(c)){ mt++; if(pw[c]>=119) mb++; }
          else if(c%NR==NR-1) msa++;
          if(c%NR==NR-1) ma++;
          if(c%NR==NR-2) mk++;
        }
        if((H[1]>>c&1)&&istr(c)) pt++;
      }
      if((fstr<0 ? mt : ma) < fmin){ n--; continue; }
      int dd0=maxtricks(H,tr,0); setup(tr);
      int r0=playhand(H,tr,0,0), r1=playhand(H,tr,1,0), r2=playhand(H,tr,0,1), r3=playhand(H,tr,1,1);
      printf("%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n",n,tr,mt,mb,msa,ma,mk,pt,dd0,r0,r1,r2,r3); fflush(stdout);
    }
    return 0;
  }
  if(!strcmp(argv[1],"brute")){
    uint32_t H[4]; deal(H);
    printf("%u %u %u %u\n",H[0],H[1],H[2],H[3]);
    for(int x=0;x<6;x++) printf("%d ",maxtricks(H,x,0));
    printf("\n");
    return 0;
  }
  return 1;
}
