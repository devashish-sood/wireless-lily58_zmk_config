/* Uses the exact firmware renderer to produce a side-by-side 1-bit PGM. */
#include "render.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc,char **argv) {
    struct walkies_state s={.battery=87,.wpm=argc>2?(unsigned)atoi(argv[2]):25,.connected=true,.paired=true,
                           .moving=argc<=2 || atoi(argv[2])>0,.step=argc>1?(unsigned)atoi(argv[1]):0};
    uint8_t left[WALKIES_BYTES],right[WALKIES_BYTES];
    s.rest_frame=s.step%2;
    if (!s.moving) s.step=0;
    walkies_render(left,&s,true);
    s.battery=92;
    walkies_render(right,&s,false);
    printf("P5\n148 160\n255\n");
    for (unsigned y=0;y<160;y++) for (unsigned x=0;x<148;x++) {
        unsigned char pixel=210;
        if (x<68) pixel=walkies_pixel(left,x,y)?0:255;
        else if (x>=80) pixel=walkies_pixel(right,x-80,y)?0:255;
        fwrite(&pixel,1,1,stdout);
    }
    return 0;
}
