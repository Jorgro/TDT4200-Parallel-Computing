

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb/stb_image_write.h"

//#include <windows.h>
//#include <magick_wand.h>

typedef struct{
    unsigned char r;
    unsigned char g;
    unsigned char b;
    unsigned char a;
} pixel;

int main(int argc, char** argv)
{
    stbi_set_flip_vertically_on_load(true);
	stbi_flip_vertically_on_write(true);

	int width;
	int height;
	int channels;
    unsigned char* char_pixels_1 = stbi_load(argv[1], &width, &height, &channels, STBI_rgb_alpha);
    unsigned char* char_pixels_2 = stbi_load(argv[2], &width, &height, &channels, STBI_rgb_alpha);

    pixel* pixels_1 = (pixel*) char_pixels_1;
    pixel* pixels_2 = (pixel*) char_pixels_2; 

    printf("height:%d, width: %d\n", height, width);
    if (pixels_1 == NULL || pixels_2 == NULL)
    {
        exit(1);
    }

    pixel* pixels_out = malloc(height * width * sizeof(pixel));
    
    for (int i = 0; i < height*width; i++) {
        pixels_out[i].r = (pixels_1[i].r + pixels_2[i].r)/2;
        pixels_out[i].g = (pixels_1[i].g + pixels_2[i].g)/2;
        pixels_out[i].b = (pixels_1[i].b + pixels_2[i].b)/2;
        pixels_out[i].a = 255;
    }

    stbi_write_png("output.png", width, height, STBI_rgb_alpha, pixels_out, sizeof(pixel) * width);

    free(pixels_out);
    free(pixels_1);
    free(pixels_2);

    return 0;
}
