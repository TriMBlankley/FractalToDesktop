#include <stdio.h>

// Sobel kernels
int sobel_x[3][3] = {
    {-1, 0, 1},
    {-2, 0, 2},
    {-1, 0, 1}
};

int sobel_y[3][3] = {
    {-1, -2, -1},
    {0,   0,  0},
    {1,   2,  1}
};
// TO-DO apply kernel to the image
// TO-DO Get all edge pixels
