#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "seamcarving.h"

void calc_energy(struct rgb_img *im, struct rgb_img **grad) {
  int height = im->height;
  int width = im->width;
  int xminus, xplus, yminus, yplus;
  int rx, ry, gx, gy, bx, by;
  int oenergy;
  uint8_t energy;
  // create the place to store energy values:
  create_img(grad, height, width);
  // loop through every pixel:
  for (int y = 0; y < height; y ++) {
    for (int x = 0; x < width; x++) {
      //setting the y/x-plus/minus
      if (y == 0) { // top row of the photo
        yplus = y + 1;
        yminus = height - 1;
      } else if (y == height - 1) { // bottom row
        yplus = 0;
        yminus = y - 1;
      } else { // middle rows
        yplus = y + 1;
        yminus = y - 1;
      }
      if (x == 0) { // left column
        xplus = x + 1;
        xminus = width - 1;
      } else if (x == width - 1) { // right column
        xplus = 0;
        xminus = x - 1;
      } else { // middle columns
        xplus = x + 1;
        xminus = x - 1;
      }
      // computing the energy value:
      rx = (int)get_pixel(im, y, xplus, 0) - (int)get_pixel(im, y, xminus, 0);
      gx = (int)get_pixel(im, y, xplus, 1) - (int)get_pixel(im, y, xminus, 1);
      bx = (int)get_pixel(im, y, xplus, 2) - (int)get_pixel(im, y, xminus, 2);
      ry = (int)get_pixel(im, yplus, x, 0) - (int)get_pixel(im, yminus, x, 0);
      gy = (int)get_pixel(im, yplus, x, 1) - (int)get_pixel(im, yminus, x, 1);
      by = (int)get_pixel(im, yplus, x, 2) - (int)get_pixel(im, yminus, x, 2);
      oenergy = sqrt((double)(rx*rx + gx*gx + bx*bx +ry*ry + gy*gy + by*by));
      energy = (uint8_t)(oenergy/10);
      // assigning the energy value to grad:
      set_pixel(*grad, y, x, energy, energy, energy);
    }
  }
}

void dynamic_seam(struct rgb_img *grad, double **best_arr) {
    int height = grad->height;
    int width = grad->width;
    *best_arr =  (double *)malloc((height*width)*sizeof(double));
    //loop through each pixel:
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            if (i == 0) {
                (*best_arr)[i*width+j] = (double)(get_pixel(grad, i, j, 0));
                continue;
            }
            if (j == 0) {
                (*best_arr)[i*width+j] = (double)get_pixel(grad, i, j, 0) + (double)(fmin((*best_arr)[(i-1)*width+j], (*best_arr)[(i-1)*width+(j+1)]));
            } else if (j == width - 1) {
                (*best_arr)[i*width+j] = (double)get_pixel(grad, i, j, 0) + (double)(fmin((*best_arr)[(i-1)*width+(j-1)], (*best_arr)[(i-1)*width+j]));
            } else {
                (*best_arr)[i*width+j] = (double)get_pixel(grad, i, j, 0) + (double)(fmin((*best_arr)[(i-1)*width+(j-1)], fmin((*best_arr)[(i-1)*width+j], (*best_arr)[(i-1)*width+(j+1)]) ));
            }
        }
    }
}

void recover_path(double *best, int height, int width, int **path) {
    *path = (int *)malloc(height * sizeof(int));
    int min_index, min, temp_min;

    for (int i = height - 1; i >= 0; i--) { // goes from bottom row to the top
        min_index = 0;
        if (i == height - 1) {
            min = best[i * width]; // first seam value of the bottom row
            for (int j = 1; j < width; j++) {
              if (min > best[i*width + j]) {
                min = best[i*width + j];
                min_index = j;
              }
            }
        } else {
          min_index = (*path)[i + 1];
          min = best[i * width + min_index];
          if (min_index == 0) {
            if (min > best[i*width + (min_index+1)]) {
              min_index += 1;
            }
          } else if (min_index == width - 1) {
            if (min > best[i*width + (min_index-1)]) {
              min_index -= 1;
            }
          } else {
            temp_min = min_index;
            if (min > best[i*width + (min_index-1)]) {
              min = best[i*width + (min_index-1)];
              temp_min = min_index - 1;
            }
            if (min > best[i*width + (min_index+1)]) {
              temp_min = min_index + 1;
            }
            min_index = temp_min;
          }
        }
        (*path)[i] = min_index;
        //printf("%d ", (*path)[i]);
    }
}

void remove_seam(struct rgb_img *src, struct rgb_img **dest, int *path) {
  // create_img(dest, src->height, (src->width) - 1); // creating an image with one less column than the original
  create_img(dest, src->height, (src->width)-1);
  uint8_t r, g, b;
  int removed; // boolean
  // loop through each pixel in the original image:
  for (int y = 0; y < src->height; y++) {
    removed = 0; // reset for every row
    for (int x = 0; x < src->width; x++) { // move across each row
      // getting pixel values from the original image:
      r = get_pixel(src, y, x, 0);
      g = get_pixel(src, y, x, 1);
      b = get_pixel(src, y, x, 2);
      if (x == path[y]) { // if the x value is the same as the one stored in the path for that row (ie the one to be removed)
        removed = 1; // dont add it, change the boolean
      } else if (removed == 1) { // if the pixel has already been 'removed'
        set_pixel(*dest, y, x-1, r, g, b); // x index -1
      } else { // if the pixel hasn't been removed yet
        set_pixel(*dest, y, x, r, g, b); // same x index
      } // if statements
    } // x loop
  } // y loop
} // function

int main(void) {
    // // testing calc_energy:
    // struct rgb_img *im;
    // create_img(&im, 4, 3);
    // set_pixel(im, 0, 0, 255,101,51);
    // set_pixel(im, 0, 1, 255,101,153);
    // set_pixel(im, 0, 2, 255,101,255);
    // set_pixel(im, 1, 0, 255,153,51);
    // set_pixel(im, 1, 1, 255,153,153);
    // set_pixel(im, 1, 2, 255,153,255);
    // set_pixel(im, 2, 0, 255,203,51);
    // set_pixel(im, 2, 1, 255,204,153);
    // set_pixel(im, 2, 2, 255,205,255);
    // set_pixel(im, 3, 0, 255,255,51);
    // set_pixel(im, 3, 1, 255,255,153);
    // set_pixel(im, 3, 2, 255,255,255);
    // struct rgb_img *grad;
    // calc_energy(im,  &grad);
    // print_grad(grad);

    /*
    struct rgb_img *im;
    read_in_img(&im, "6x5.bin");
    double *best_arr;
    struct rgb_img *grad;
    calc_energy(im,  &grad);
    dynamic_seam(grad, &best_arr);

    //testing part 2
    for (int i = 0; i < grad->height; i++) {
        for (int j = 0; j < grad->width; j++) {
            printf("%lf\n", (best_arr)[i*(grad->width)+j]);
        }
    }

    //testing part 3
    int *path_arr;
    recover_path(best_arr, grad->height, grad->width, &path_arr);
    for (int i = 0; i < grad->height; i++) {
        printf("%d ", (path_arr)[i]);
    }
    */

    // testing part 4:
    struct rgb_img *im;
    struct rgb_img *cur_im;
    struct rgb_img *grad;
    double *best;
    int *path;

    read_in_img(&im, "HJoceanSmall.bin");
    
    for(int i = 0; i < 300; i++){
        printf("i = %d\n", i);
        calc_energy(im,  &grad);
        dynamic_seam(grad, &best);
        recover_path(best, grad->height, grad->width, &path);
        remove_seam(im, &cur_im, path);

        char filename[200];
        sprintf(filename, "img%d.bin", i);
        write_img(cur_im, filename);

        destroy_image(im);
        destroy_image(grad);
        free(best);
        free(path);
        im = cur_im;
    }
    destroy_image(im);

    return 0;
}