#include "render.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(void) {
    struct {
        uint8_t before[32];
        uint8_t image[WALKIES_BYTES];
        uint8_t after[32];
    } guarded;
    struct walkies_state s={.battery=100,.connected=true,.paired=true};
    memset(&guarded,0xA5,sizeof(guarded));
    // Exercise all battery widths, layers, speed thresholds, and scrolling positions.
    for (unsigned i=0;i<512;i++) {
        s.battery=i%256; s.layer=i%256;
        s.wpm=i%256; s.moving=i%2; s.connected=(i%3)!=0; s.step=i;
        for (unsigned central=0;central<2;central++) {
            walkies_render(guarded.image,&s,central);
            for (int j=0;j<32;j++) {
                assert(guarded.before[j]==0xA5);
                assert(guarded.after[j]==0xA5);
            }
        }
    }
    uint8_t a[WALKIES_BYTES],b[WALKIES_BYTES];
    s=(struct walkies_state){.battery=92,.connected=true,.wpm=25,.moving=true,.step=0};
    walkies_render(a,&s,false);
    s.step=1; walkies_render(b,&s,false);
    assert(memcmp(a,b,sizeof(a))!=0); // dog and scenery animate
    s.moving=false; walkies_render(a,&s,false);
    s.wpm=120; walkies_render(b,&s,false);
    assert(memcmp(a,b,sizeof(a))==0); // no running pose when motion is disabled
    s.rest_frame=1; walkies_render(b,&s,false);
    assert(memcmp(a,b,sizeof(a))!=0); // the original idle tail wag remains animated
    for (unsigned y=0;y<120;y++) for (unsigned x=0;x<68;x++)
        assert(walkies_pixel(a,x,y)==walkies_pixel(b,x,y)); // scenery remains still
    s.moving=true; s.step=0; walkies_render(a,&s,false);
    s.step=512; walkies_render(b,&s,false);
    assert(memcmp(a,b,sizeof(a))==0); // complete scene has a seamless common period
    s.wpm=0; walkies_render(a,&s,true);
    s.wpm=255; walkies_render(b,&s,true);
    assert(memcmp(a,b,sizeof(a))==0); // typing speed is no longer visible on the left
    assert(!walkies_pixel(a,68,0));
    assert(!walkies_pixel(a,0,160));
    puts("Walkies rendering checks passed");
}
