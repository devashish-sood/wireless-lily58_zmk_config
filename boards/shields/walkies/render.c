/* SPDX-License-Identifier: MIT
 * Shared, integer-only renderer: the firmware and preview use this same code.
 */
#include "render.h"
#include "luna_frames.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* 3 x 5 bitmap alphabet, MSB-first rows. */
static const uint8_t digits[10][5] = {
    {7,5,5,5,7}, {2,6,2,2,7}, {7,1,7,4,7}, {7,1,7,1,7}, {5,5,7,1,1},
    {7,4,7,1,7}, {7,4,7,5,7}, {7,1,2,2,2}, {7,5,7,5,7}, {7,5,7,1,7},
};
static const uint8_t letters[26][5] = {
    {2,5,7,5,5}, {6,5,6,5,6}, {7,4,4,4,7}, {6,5,5,5,6}, {7,4,6,4,7},
    {7,4,6,4,4}, {7,4,5,5,7}, {5,5,7,5,5}, {7,2,2,2,7}, {1,1,1,5,7},
    {5,5,6,5,5}, {4,4,4,4,7}, {5,7,7,5,5}, {5,7,7,7,5}, {7,5,5,5,7},
    {6,5,6,4,4}, {7,5,5,7,1}, {6,5,6,5,5}, {7,4,7,1,7}, {7,2,2,2,2},
    {5,5,5,5,7}, {5,5,5,5,2}, {5,5,7,7,5}, {5,5,2,5,5}, {5,5,2,2,2},
    {7,1,2,4,7},
};

static void dot(uint8_t *im, int x, int y, bool black) {
    if (x < 0 || x >= WALKIES_WIDTH || y < 0 || y >= WALKIES_HEIGHT) return;
    unsigned bit = (unsigned)y * WALKIES_WIDTH + (unsigned)x;
    uint8_t mask = 0x80u >> (bit % 8);
    if (black) im[bit / 8] |= mask;
    else im[bit / 8] &= (uint8_t)~mask;
}

bool walkies_pixel(const uint8_t *im, unsigned x, unsigned y) {
    if (x >= WALKIES_WIDTH || y >= WALKIES_HEIGHT) return false;
    unsigned bit = y * WALKIES_WIDTH + x;
    return (im[bit / 8] & (0x80u >> (bit % 8))) != 0;
}

static void rect(uint8_t *im, int x, int y, int w, int h, bool black) {
    for (int j = 0; j < h; j++) for (int i = 0; i < w; i++) dot(im,x+i,y+j,black);
}

static void line(uint8_t *im, int x0, int y0, int x1, int y1) {
    int dx = abs(x1-x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1-y0), sy = y0 < y1 ? 1 : -1, err = dx+dy;
    for (;;) {
        dot(im,x0,y0,true);
        if (x0 == x1 && y0 == y1) break;
        int e = 2*err;
        if (e >= dy) { err += dy; x0 += sx; }
        if (e <= dx) { err += dx; y0 += sy; }
    }
}

static void box(uint8_t *im, int x, int y, int w, int h) {
    line(im,x,y,x+w-1,y); line(im,x,y+h-1,x+w-1,y+h-1);
    line(im,x,y,x,y+h-1); line(im,x+w-1,y,x+w-1,y+h-1);
}

static void text(uint8_t *im, int x, int y, const char *s, int scale) {
    static const uint8_t blank[5] = {0}, percent[5] = {5,1,2,4,5};
    static const uint8_t dash[5] = {0,0,7,0,0};
    for (; *s; s++, x += 4*scale) {
        const uint8_t *g = blank;
        if (*s >= '0' && *s <= '9') g = digits[*s-'0'];
        else if (*s >= 'A' && *s <= 'Z') g = letters[*s-'A'];
        else if (*s == '%') g = percent;
        else if (*s == '-') g = dash;
        for (int j=0;j<5;j++) for (int i=0;i<3;i++)
            if (g[j] & (4 >> i)) rect(im,x+i*scale,y+j*scale,scale,scale,true);
    }
}

static void centered(uint8_t *im, int y, const char *s, int scale) {
    int w = ((int)strlen(s)*4-1)*scale;
    text(im,(WALKIES_WIDTH-w)/2,y,s,scale);
}

static void battery(uint8_t *im, uint8_t level) {
    box(im,5,6,20,10); rect(im,25,9,2,4,true);
    unsigned n = level > 100 ? 100 : level;
    rect(im,7,8,(int)((n*16+50)/100),6,true);
    char label[8];
    snprintf(label,sizeof(label),"%u%%",n);
    text(im,68-4-((int)strlen(label)*4-1)*2,6,label,2);
    line(im,4,23,63,23);
}

static void status(uint8_t *im, const struct walkies_state *s) {
    char label[8];
    centered(im,38,"LAYER",1);
    snprintf(label,sizeof(label),"%u",s->layer);
    centered(im,49,label,3);
    if (s->usb) snprintf(label,sizeof(label),"USB");
    else if (s->profile==0) snprintf(label,sizeof(label),"BT PERS");
    else if (s->profile==1) snprintf(label,sizeof(label),"BT WORK");
    else snprintf(label,sizeof(label),"BT %u",(unsigned)s->profile+1);
    centered(im,79,label,2);
    centered(im,96,s->connected?"CONNECTED":(s->paired?"OFFLINE":"PAIRING"),1);
}

static void cloud(uint8_t *im, int x, int y) {
    line(im,x,y+5,x+2,y+3); line(im,x+2,y+3,x+5,y+3);
    line(im,x+5,y+3,x+7,y); line(im,x+7,y,x+11,y);
    line(im,x+11,y,x+14,y+4); line(im,x+14,y+4,x+17,y+4);
    line(im,x+17,y+4,x+19,y+7); line(im,x+19,y+7,x+17,y+9);
    line(im,x+17,y+9,x+2,y+9); line(im,x+2,y+9,x,y+7);
    line(im,x,y+7,x,y+5);
}

static void grass(uint8_t *im, int x, int y) {
    line(im,x,y,x-2,y-3); line(im,x,y,x,y-4); line(im,x,y,x+2,y-2);
}

/* World objects wrap beyond both screen edges, never in the visible scene. */
static int world_x(int origin, unsigned scroll) {
    return (origin + 128 - (int)(scroll % 128)) % 128 - 30;
}

static void disk(uint8_t *im, int cx, int cy, int radius, bool black) {
    for (int y=-radius;y<=radius;y++) for (int x=-radius;x<=radius;x++)
        if (x*x+y*y<=radius*radius) dot(im,cx+x,cy+y,black);
}

static void tree(uint8_t *im, int x, int y) {
    // A scalloped canopy. Outline the union, avoiding internal circle seams.
    uint8_t canopy[23][21]={0};
    const int lobes[][3]={{10,5,5},{5,10,5},{15,10,5},{7,15,5},{13,15,5}};
    for (unsigned i=0;i<sizeof(lobes)/sizeof(lobes[0]);i++)
        for (int row=0;row<23;row++) for (int col=0;col<21;col++) {
            int dx=col-lobes[i][0],dy=row-lobes[i][1];
            if (dx*dx+dy*dy<=lobes[i][2]*lobes[i][2]) canopy[row][col]=1;
        }
    for (int row=0;row<23;row++) for (int col=0;col<21;col++) {
        if (!canopy[row][col]) continue;
        bool edge=row==0||row==22||col==0||col==20||
                  !canopy[row-1][col]||!canopy[row+1][col]||
                  !canopy[row][col-1]||!canopy[row][col+1];
        dot(im,x+col-10,y+row,edge);
    }
    line(im,x-1,y+18,x-1,y+28); line(im,x+1,y+18,x+1,y+28);
    line(im,x-1,y+23,x-4,y+20); line(im,x+1,y+21,x+4,y+18);
    line(im,x-4,y+29,x+4,y+29);
}

static void flower(uint8_t *im, int x, int y) {
    line(im,x,y,x,y-4);
    dot(im,x-1,y-1,true); dot(im,x+1,y-2,true);
    dot(im,x,y-7,true); dot(im,x-2,y-5,true);
    dot(im,x+2,y-5,true); dot(im,x,y-3,true);
    dot(im,x,y-5,true);
}

static int hill_height(int x,unsigned scroll,int shift,int base,int depth) {
    int d=(x+(int)(scroll%128)+shift)%128-64;
    // Rounded, periodic hills with a continuous horizon at the tile boundary.
    return base-depth+(depth*d*d)/(64*64);
}

static void landscape(uint8_t *im, const struct walkies_state *s) {
    // The entire world repeats after 512 steps. All layers share that period.
    unsigned step=s->step%512;
    // An outlined sun, stationary above the moving landscape.
    disk(im,15,42,7,true); disk(im,15,42,6,false);
    line(im,15,31,15,33); line(im,15,51,15,53);
    line(im,4,42,6,42); line(im,24,42,26,42);
    line(im,7,34,8,35); line(im,22,49,23,50);
    line(im,7,50,8,49); line(im,22,35,23,34);
    cloud(im,world_x(75,step/4),56);
    cloud(im,world_x(9,step/4),54);

    for (int x=0;x<68;x++) {
        int far=hill_height(x,step/4,20,88,13);
        int near=hill_height(x,step/2,77,96,10);
        // Only show the distant ridge where it is above the nearer hillside.
        if (far<near) dot(im,x,far,true);
        dot(im,x,near,true);
    }
    tree(im,world_x(86,step/2),67);
    tree(im,world_x(11,step/2),68);
    // A broad walking trail: the dog stands on it, never on a floating platform.
    line(im,0,99,67,99);
    line(im,0,148,67,148);
    for (int i=0;i<4;i++) {
        int x=world_x(34+i*31,step);
        grass(im,x,97);
        if (i%2) flower(im,x+7,159);
        else grass(im,x+3,156);
        // Small moving stones stay below the dog's feet.
        line(im,x,146,x+1,146);
    }
    unsigned pose=!s->moving?0:(s->wpm<40?2:4);
    unsigned beat=step%4;
    const uint8_t *frame=luna_frames[pose+(s->moving?(beat&1):(s->rest_frame&1))];
    // Original Luna proportions and face, with a gentle four-beat gait.
    // One lifted beat per walking cycle; a longer airborne beat while running.
    int lift=s->moving?((s->wpm<40)?(beat==1):(beat==1||beat==2)):0;
    for (unsigned y=0;y<22;y++) for (unsigned x=0;x<32;x++)
        if (frame[y*4+x/8] & (0x80 >> (x%8))) dot(im,18+x,120+(int)y-lift,true);
    if (!s->connected) {
        rect(im,9,150,50,9,false);
        centered(im,152,"LINK LOST",1);
    }
}

void walkies_render(uint8_t *im, const struct walkies_state *s, bool central) {
    memset(im,0,WALKIES_BYTES);
    battery(im,s->battery);
    if (central) status(im,s);
    else landscape(im,s);
}
