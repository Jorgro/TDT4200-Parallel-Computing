


#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <signal.h>
#include "mpi.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb/stb_image_write.h"

typedef struct pixel_struct {
	unsigned char r;
	unsigned char g;
	unsigned char b;
	unsigned char a;
} pixel;



//--------------------------------------------------------------------------------------------------
//--------------------------bilinear interpolation--------------------------------------------------
//--------------------------------------------------------------------------------------------------
void bilinear(pixel* Im, float row, float col, pixel* pix, int width, int height)
{
	int cm, cn, fm, fn;
	double alpha, beta;

	cm = (int)ceil(row);
	fm = (int)floor(row);
	cn = (int)ceil(col);
	fn = (int)floor(col);
	alpha = ceil(row) - row;
	beta = ceil(col) - col;

	pix->r = (unsigned char)(alpha*beta*Im[fm*width+fn].r
			+ (1-alpha)*beta*Im[cm*width+fn].r
			+ alpha*(1-beta)*Im[fm*width+cn].r
			+ (1-alpha)*(1-beta)*Im[cm*width+cn].r );
	pix->g = (unsigned char)(alpha*beta*Im[fm*width+fn].g
			+ (1-alpha)*beta*Im[cm*width+fn].g
			+ alpha*(1-beta)*Im[fm*width+cn].g
			+ (1-alpha)*(1-beta)*Im[cm*width+cn].g );
	pix->b = (unsigned char)(alpha*beta*Im[fm*width+fn].b
			+ (1-alpha)*beta*Im[cm*width+fn].b
			+ alpha*(1-beta)*Im[fm*width+cn].b
			+ (1-alpha)*(1-beta)*Im[cm*width+cn].b );
	pix->a = 255;
}
//---------------------------------------------------------------------------

//Helper function to locate the source of errors
void
SEGVFunction( int sig_num)
{
	printf ("\n Signal %d received\n",sig_num);
	exit(sig_num);
}

int main(int argc, char** argv)
{
	signal(SIGSEGV, SEGVFunction);
	stbi_set_flip_vertically_on_load(true);
	stbi_flip_vertically_on_write(true);

    int comm_size;
    int rank;
	const int root = 0;
	MPI_Init(NULL, NULL);
	MPI_Comm_size(MPI_COMM_WORLD, &comm_size);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	printf("Rank %d out of %d processors\n",
           rank, comm_size);

	pixel* pixels_in;

	int in_width;
	int in_height;
	int channels;


	if (rank == root) {
		pixels_in = (pixel *) stbi_load(argv[1], &in_width, &in_height, &channels, STBI_rgb_alpha);
		if (pixels_in == NULL) {
			exit(1);
		}
		printf("Image dimensions: %dx%d\n", in_width, in_height);
	}

	// Broadcast image dimensions
	MPI_Bcast(&in_width, 1, MPI_INT, root, MPI_COMM_WORLD);
	MPI_Bcast(&in_height, 1, MPI_INT, root, MPI_COMM_WORLD);

	// Allocate for all other processes than root
	if (rank != root) {
		pixels_in = (pixel*) malloc(in_width*in_height*sizeof(pixel));
	}
	MPI_Bcast(pixels_in, in_width*in_height*4, MPI_UNSIGNED_CHAR, root, MPI_COMM_WORLD);

	double scale_x = argc > 2 ? atof(argv[2]): 2;
	double scale_y = argc > 3 ? atof(argv[3]): 8;

	int out_width = in_width * scale_x;
	int out_height = in_height * scale_y;

	int local_width = in_width;
	// Divide height equally between number of processes
	int local_height = in_height / comm_size;

	int local_out_width = out_width;
	int local_out_height = out_height / comm_size;

	pixel* local_out = (pixel *) malloc(sizeof(pixel) * local_out_width * local_out_height);

	for(int i = 0; i < local_out_height; i++) {
		for(int j = 0; j < local_out_width; j++) {
			pixel new_pixel;
			// i is offsetted by the local_out_height (which is equal for all processes) times the rank of the process
			float row = (i+local_out_height*rank) * (in_height-1) / (float)out_height;
			float col = j * (in_width-1) / (float)out_width;
			bilinear(pixels_in, row, col, &new_pixel, in_width, in_height);
			local_out[i*local_out_width+j] = new_pixel;
		}
	}

	pixel* pixels_out = (pixel *) malloc(sizeof(pixel) * out_width * out_height);
	// Gather from all processes
	MPI_Gather(local_out, local_out_width*local_out_height*4, MPI_UNSIGNED_CHAR, pixels_out, local_out_width*local_out_height*4, MPI_UNSIGNED_CHAR, root,
           MPI_COMM_WORLD);
	stbi_write_png("output.png", out_width, out_height, STBI_rgb_alpha, pixels_out, sizeof(pixel) * out_width);

	printf("Operation finished.\n");

	MPI_Finalize();
	return 0;
}
