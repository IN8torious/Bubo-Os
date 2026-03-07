// =============================================================================
// Raven AOS v1.1 — Dedicated to Landon Pankuch
// Built by IN8torious | Copyright (c) 2025 | MIT License
// "NO MAS DISADVANTAGED"
//
// apps/corvus_browser.c — CORVUS Browser
// Privacy-first, voice-navigable web browser. No ads. No tracking. Yours.
// =============================================================================

#include "corvus_browser.h"
#include "framebuffer.h"
#include "font.h"
#include "polish.h"
#include "tcpip.h"
#include "voice.h"
#include "vmm.h"
#include <stdint.h>
#include <stdbool.h>

#define MAX_TABS        8
#define URL_LEN         256
#define TITLE_LEN       64
#define CONTENT_LEN     4096

static const char* AD_DOMAINS[] = {
    "doubleclick.net","googlesyndication.com","adnxs.com","ads.yahoo.com",
    "advertising.com","outbrain.com","taboola.com","scorecardresearch.com",
    "quantserve.com","moatads.com","pubmatic.com","rubiconproject.com",
    "openx.net","criteo.com","amazon-adsystem.com","adsafeprotected.com",
    "adsrvr.org","adform.net","appnexus.com","media.net", (const char*)0
};

static uint32_t blen(const char* s) {
    uint32_t n = 0; while (s[n]) n++; return n;
}
static void bcopy(char* d, const char* s, uint32_t max) {
    uint32_t i = 0;
    while (i < max-1 && s[i]) { d[i]=s[i]; i++; }
    d[i]=0;
}
static bool bhas(const char* hay, const char* ndl) {
    uint32_t hl=blen(hay), nl=blen(ndl);
    if (nl>hl) return false;
    for (uint32_t i=0; i<=hl-nl; i++) {
        bool m=true;
        for (uint32_t j=0; j<nl; j++) {
            char a=hay[i+j], b=ndl[j];
            if (a>='A'&&a<='Z') a+=32;
            if (b>='A'&&b<='Z') b+=32;
            if (a!=b){m=false;break;}
        }
        if (m) return true;
    }
    return false;
}
static bool bpfx(const char* s, const char* p) {
    for (uint32_t i=0; p[i]; i++) if (s[i]!=p[i]) return false;
    return true;
}

typedef struct {
    char     url[URL_LEN];
    char     title[TITLE_LEN];
    char     content[CONTENT_LEN];
    uint32_t content_len;
    uint32_t scroll_y;
    bool     loading;
    bool     used;
} tab_t;

static tab_t     tabs[MAX_TABS];
static int32_t   cur_tab   = 0;
static int32_t   tab_count = 1;
static char      addr_bar[URL_LEN];
static bool      addr_focused = false;
static bool      g_open = false;
static int32_t   g_x, g_y, g_w, g_h;

static bool ad_blocked(const char* url) {
    for (uint32_t i=0; AD_DOMAINS[i]; i++)
        if (bhas(url, AD_DOMAINS[i])) return true;
    return false;
}

static void html_to_text(const char* html, char* out, uint32_t max) {
    uint32_t oi=0; bool in_tag=false;
    for (uint32_t i=0; html[i]&&oi<max-1; i++) {
        if (html[i]=='<'){in_tag=true;continue;}
        if (html[i]=='>'){in_tag=false;
            if (oi>0&&out[oi-1]!='\n') out[oi++]='\n';
            continue;}
        if (!in_tag) {
            if (html[i]=='&') {
                if (bpfx(html+i,"&amp;")) {out[oi++]='&';i+=4;}
                else if (bpfx(html+i,"&lt;")) {out[oi++]='<';i+=3;}
                else if (bpfx(html+i,"&gt;")) {out[oi++]='>';i+=3;}
                else out[oi++]=html[i];
            } else out[oi++]=html[i];
        }
    }
    out[oi]=0;
}

static void do_fetch(tab_t* t) {
    if (ad_blocked(t->url)) {
        bcopy(t->content,"[CORVUS] Ad domain blocked by CORVUS Browser.\n",CONTENT_LEN);
        t->content_len=blen(t->content); t->loading=false; return;
    }
    char host[128]={0}, path[256]="/";
    const char* u=t->url;
    if (bpfx(u,"https://")) u+=8;
    else if (bpfx(u,"http://")) u+=7;
    uint32_t hi=0;
    while (u[hi]&&u[hi]!='/'&&hi<127){host[hi]=u[hi];hi++;}
    host[hi]=0;
    if (u[hi]=='/') bcopy(path,u+hi,256);

    char raw[CONTENT_LEN];
    int32_t len=http_get(host,80,path,raw,CONTENT_LEN-1);
    if (len>0) {
        raw[len]=0;
        char* body=raw;
        for (int32_t i=0;i<len-3;i++) {
            if (raw[i]=='\r'&&raw[i+1]=='\n'&&raw[i+2]=='\r'&&raw[i+3]=='\n') {
                body=raw+i+4; break;
            }
        }
        html_to_text(body,t->content,CONTENT_LEN);
        t->content_len=blen(t->content);
        bcopy(t->title,host,TITLE_LEN);
    } else {
        bcopy(t->content,"[CORVUS] Could not connect. Check network.\n",CONTENT_LEN);
        t->content_len=blen(t->content);
    }
    t->loading=false;
}

void browser_init(int32_t x, int32_t y, int32_t w, int32_t h) {
    g_x=x; g_y=y; g_w=w; g_h=h; g_open=false;
    cur_tab=0; tab_count=1;
    for (uint32_t i=0;i<MAX_TABS;i++){
        tabs[i].used=false; tabs[i].scroll_y=0;
        tabs[i].loading=false; tabs[i].content_len=0;
        tabs[i].url[0]=0; tabs[i].title[0]=0; tabs[i].content[0]=0;
    }
    tabs[0].used=true;
    bcopy(tabs[0].url,  "corvus://home",URL_LEN);
    bcopy(tabs[0].title,"CORVUS Home",  TITLE_LEN);
    bcopy(tabs[0].content,
        "CORVUS BROWSER v1.1\n"
        "====================\n"
        "Privacy-first. Voice-navigable. Sovereign.\n\n"
        "Type a URL and press Enter to navigate.\n"
        "Voice: 'CORVUS GO TO [url]'\n\n"
        "NO ADS. NO TRACKING. NO TELEMETRY.\n"
        "NO MAS DISADVANTAGED.\n", CONTENT_LEN);
    tabs[0].content_len=blen(tabs[0].content);
    addr_bar[0]=0; addr_focused=false;
}

void browser_open(void)  { g_open=true; }
void browser_close(void) { g_open=false; }
bool browser_is_open(void){ return g_open; }

void browser_open_url(const char* url) {
    if (!url||!url[0]) return;
    tab_t* t=&tabs[cur_tab];
    bcopy(t->url,url,URL_LEN);
    bcopy(t->title,"Loading...",TITLE_LEN);
    t->loading=true; t->scroll_y=0;
    do_fetch(t);
}

void browser_handle_key(uint8_t key) {
    if (!g_open) return;
    if (key==0x1B){browser_close();return;}
    tab_t* t=&tabs[cur_tab];
    if (addr_focused) {
        uint32_t al=blen(addr_bar);
        if (key==0x08&&al>0){addr_bar[--al]=0;}
        else if (key==0x0D){browser_open_url(addr_bar);addr_focused=false;}
        else if (key>=0x20&&key<0x7F&&al<URL_LEN-1){addr_bar[al]=(char)key;addr_bar[al+1]=0;}
        return;
    }
    if (key==0x50) t->scroll_y+=3;
    if (key==0x48&&t->scroll_y>=3) t->scroll_y-=3;
    if (key==0x0D) addr_focused=true;
}

void browser_voice_cmd(uint32_t cmd_id) {
    const char* url_arg = NULL; (void)url_arg;
    if (!g_open) return;
    tab_t* t=&tabs[cur_tab];
    if (cmd_id==VCMD_FASTER) t->scroll_y+=5;
    else if (cmd_id==VCMD_BRAKE&&t->scroll_y>=5) t->scroll_y-=5;
    else if (cmd_id==VCMD_NITRO) t->scroll_y=0;
}

void browser_draw(void) {
    if (!g_open) return;
    int32_t x=g_x,y=g_y,w=g_w,h=g_h;
    polish_glass_rect(x,y,w,h,0x0D0820,240);
    polish_gradient_rect(x,y,w,28,0x8B0000,0x1A0A2A,false);
    font_draw_string(x+10,y+8,"CORVUS BROWSER",0xFFFFFF,0, true);
    fb_fill_rect((uint32_t)(x+4),(uint32_t)(y+32),(uint32_t)(w-8),24,0x1A1030);
    font_draw_string(x+8,y+38,
        addr_focused?addr_bar:tabs[cur_tab].url, 0xCCCCCC,0, true);
    font_draw_string(x+w-130,y+38,"NO ADS|PRIVATE",0x00CC66,0, true);

    int32_t tx=x+4;
    for (int32_t i=0;i<tab_count;i++){
        uint32_t tc=(i==cur_tab)?0x2A1850:0x0D0820;
        fb_fill_rect((uint32_t)tx,(uint32_t)(y+60),120,20,tc);
        font_draw_string(tx+4,y+64,
            tabs[i].title[0]?tabs[i].title:"New Tab",0xCCCCCC,0, true);
        tx+=124;
    }

    int32_t cy=y+84, ch=h-84;
    fb_fill_rect((uint32_t)x,(uint32_t)cy,(uint32_t)w,(uint32_t)ch,0x0A0818);
    tab_t* t=&tabs[cur_tab];
    if (t->loading){font_draw_string(x+20,cy+20,"Loading...",0xFF4444,0, true);return;}

    const char* c=t->content;
    int32_t ly=cy+8-(int32_t)(t->scroll_y*16);
    char lb[128]; uint32_t li=0; int32_t col=0;
    for (uint32_t i=0;c[i];i++){
        if (c[i]=='\n'||col>=100){
            lb[li]=0;
            if (ly>=cy&&ly<cy+ch) font_draw_string(x+8,ly,lb,0xDDDDDD,0, true);
            ly+=16; li=0; col=0;
            if (c[i]=='\n') continue;
        }
        if (li<127){lb[li++]=c[i];col++;}
    }
    if (li>0){lb[li]=0;if(ly>=cy&&ly<cy+ch)font_draw_string(x+8,ly,lb,0xDDDDDD,0, true);}
}
