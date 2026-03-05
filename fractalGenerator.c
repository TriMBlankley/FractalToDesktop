#include <stdio.h>
#include <math.h>
#include <stdint.h>
#include <time.h>
#include <stdbool.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include <opencv2/opencv.hpp>

typedef uint64_t u64;

// ----------------- Random Number Generation -------------------------------------


typedef struct RNG {
    u64 s[2];
} RNG;

inline static u64 randu64(RNG *r) {
    u64 s0 = r->s[0];
    u64 s1 = r->s[1];
    u64 out = s0 + s1;

    s1 ^= s0;
    r->s[0] = ((s0 << 24) | (s0 >> 40)) ^ s1 ^ (s1 << 16);
    r->s[1] = ((s1 << 37) | (s1 >> 27));

    return out;
}

RNG rng;


//  ---------------- Mandlebrot set fractal generator ---------------------------

typedef struct complex{
    double a;    // real part
    double b;    // imaginary part
}complex; 

complex comAdd(complex x, complex y){
    return (complex) {
        (x.a + y.a), 
        (x.b + y.b)
    };
}

complex comMultiply(complex x, complex y){
    return (complex) {
        ((x.a * y.a) - (x.b * y.b)),
        ((x.b * y.a) + (x.a * y.b))
    };
}

double comSquareMag(complex x){
    return (x.a * x.a) + (x.b * x.b);
}

int mandlebrot(complex c, int iterations){
    complex z = {0};    // Set all feilds to zero
    for (int i = 0; i < iterations; i++){
        if (comSquareMag(z) > 4){
            return i;    // point NOT in set
        }
        z = comMultiply(z, z);
        z = comAdd(z, c);
    }
    return -1;    // point IS in set
}


double lerp(double num1, double num2, double x) {
    return x * (num2 - num1) + num1;
}

double unlerp(double num1, double num2, double y) {
    return (y - num1) / (num2 - num1);
}


// -------------- Find a coordintate that borders points in the set -------------------------------------------

complex coordinateFinder(int imHeight, int imWidth,
                             double minX, double maxX, 
                             double minY, double maxY){

    int simpleFractal[imHeight][imWidth];
    int fractalEdgeX[imWidth * imHeight];
    int fractalEdgeY[imWidth * imHeight];

    for (int y = 0; y < imHeight; y++){
        for (int x = 0; x < imWidth; x++){
            if(mandlebrot((complex) {
                lerp(minX, maxX, unlerp(0, imWidth, x)),
                lerp(minY, maxY, unlerp(imHeight, 0, y))
            }, 100) == -1){
                simpleFractal[y][x] = 1;
            }else {
                simpleFractal[y][x] = 0;
            }
        }
    }

    // Neighbor Detection
    int index = 0;
    for (int yOut = 1; yOut < imHeight - 1; yOut++){
        for (int xOut = 1; xOut < imWidth - 1; xOut++){
            if(simpleFractal[yOut][xOut] == 1) continue;  // breaks if the point is in the set
            if (simpleFractal[yOut - 1][xOut - 1] +
                simpleFractal[yOut - 1][xOut    ] +
                simpleFractal[yOut - 1][xOut + 1] +
                simpleFractal[yOut    ][xOut + 1] +
                simpleFractal[yOut + 1][xOut - 1] +
                simpleFractal[yOut + 1][xOut    ] +
                simpleFractal[yOut + 1][xOut + 1] +
                simpleFractal[yOut    ][xOut - 1] == 0) continue;  // breaks if the point does not neigbor a location in the set
            
            // Add the point to the fractal edge lists
            fractalEdgeX[index] = xOut;
            fractalEdgeY[index] = yOut;
            index++;  
        }
    }

    // ---------- Simple Terminal View ---------------------------------------
    for (int yPrint = 1; yPrint < imHeight - 1; yPrint++){
        for (int xPrint = 1; xPrint < imWidth - 1; xPrint++){
            bool isEdge = false;
            for (int i =0; i < index; i++){
                if (fractalEdgeX[i] == xPrint && fractalEdgeY[i] == yPrint){
                    printf("\033[7m%d\033[0m", simpleFractal[yPrint][xPrint]);
                    isEdge = true;
                } 
            } 
            if (isEdge == false){
                printf("%d", simpleFractal[yPrint][xPrint]);
            }
        }
        printf("\n");
    }


    // Initialize the random number generator
    for (int r = 0; r < 5; r++) randu64(&rng);
    
    // Choose the random edge case.
    int randIndex = randu64(&rng) % index;

    
    // Returns the address of the fracal we are planning on rendering
    return (complex){lerp(minX, maxX, unlerp(0, imWidth, fractalEdgeX[randIndex])),
                     lerp(minY, maxY, unlerp(imHeight, 0, fractalEdgeY[randIndex]))
    };
};

// ---------------  Final fractal Image Generation ----------------------------

int generateFractalImage (const char *fileName, int imHeight, int imWidth, 
                          double minX, double maxX, 
                          double minY, double maxY, double palleteSweep){
                          
    int channels = 3; // RGB image
    
    // Allocate memory for the image (3 bytes per pixel: R, G, B)
    // Data is stored as RGBRGBRGB... from left to right, top to bottom
    unsigned char* image_data = (unsigned char*)malloc(imWidth * imHeight * channels);
    
    if (!image_data) {
        return -1; // Allocation failed
    }


    // Iterate over x, and y to find the pixel at each point of the image
    for (int y = 0; y < imHeight; y++){
        for (int x = 0; x < imWidth; x++){

            int pixel_index = (y * imWidth + x) * channels;

            image_data[pixel_index + 0] = 0;
            image_data[pixel_index + 1] = 0;
            image_data[pixel_index + 2] = 0;

            // This next nested for loop is for the Anti-Aliasing.
            for (double ySample = 0; ySample < 3; ySample++){
                for (double xSample = 0; xSample < 3; xSample++){

                    double iterations = mandlebrot((complex) {
                        lerp(minX, maxX, unlerp(0, imWidth, x + xSample / 3)),
                        lerp(minY, maxY, unlerp(imHeight, 0, y + ySample / 3))
                    }, 2000);
                    iterations = iterations * palleteSweep;

                    image_data[pixel_index + 0] = image_data[pixel_index + 0] + lerp(0, 255, unlerp(-1, 1, -cos(iterations * 0.10))) / 9;
                    image_data[pixel_index + 1] = image_data[pixel_index + 1] + lerp(0, 255, unlerp(-1, 1, -cos(iterations * 0.11))) / 9;
                    image_data[pixel_index + 2] = image_data[pixel_index + 2] + lerp(0, 255, unlerp(-1, 1, -cos(iterations * 0.13))) / 9;
                }
            }
        }
    }

    // Write the PNG file
    // Parameters: filename, width, height, channels, data, stride_in_bytes
    // stride_in_bytes = width * channels (0 means let the library calculate it)
    int result = stbi_write_png(fileName, 
                                imWidth, imHeight, channels, 
                                image_data, imWidth * channels);
    
    // Check if writing was successful
    if (result) {
        printf("%s.png file created successfully!\n", fileName);
    } else {
        printf("Error creating PNG file\n");
    }
    
    // Free the allocated memory
    free(image_data);
    
    return 0;
}

// Source - https://stackoverflow.com/a/58228695
// Posted by Simson, modified by community. See post 'Timeline' for change history
// Retrieved 2026-03-04, License - CC BY-SA 4.0

void grayscale(int height, int width, RGBTRIPLE image[][]){
    for(int x=0; x<width ; x++){
        for(int y=0; y<height; y++){
            unsigned int colour = 0.299*image[x][y].rgbtRed + 0.587*image[x][y].rgbtGreen + 0.114*image[x][y].rgbtBlue;
            image[x][y].rgbtRed = colour & 0xFF;
            image[x][y].rgbtBlue = colour & 0xFF;
            image[x][y].rgbtGreen = colour & 0xFF;
         }
    }
}




int main(){
    rng.s[1] = 69420;
    rng.s[0] = time(NULL);
     
    double minX = -2, maxX = 2, minYInit = -2, maxYInit = 2;

    for (int i = 0; i < 1; i++){

        int tWidth = 100;
        int tHeight = 100;


        double minY = lerp(minYInit, maxYInit, (1 - ((double)tHeight / tWidth)) / 2 );
        double maxY = lerp(maxYInit, minYInit, (1 - ((double)tHeight / tWidth)) / 2 );
        
        for (int depth = 0; depth < 10; depth ++){
            complex coordinates = coordinateFinder(60, 60, minX, maxX, minY, maxY);

            double zoomFactor = 4;
            minX = lerp(coordinates.a, minX, 1 / zoomFactor);
            maxX = lerp(coordinates.a, maxX, 1 / zoomFactor);
            minY = lerp(coordinates.b, minY, 1 / zoomFactor);
            maxY = lerp(coordinates.b, maxY, 1 / zoomFactor);
        }

        generateFractalImage("test.png", tHeight, tWidth, minX, maxX, minY, maxY, 0.25);

        unsigned char *data = stbi_load("test.png", tWidth, tHeight, 3, 0);

        printf(data);

        char str[100];
        sprintf(str, "fractal%d.png", i);

        rename("test.png", str);
    }
    // printf("Fractal Address: %lf, %lf, %lf, %lf\n", minX, maxX, minY, maxY);
    // generateFractalImage("darkFractal.png", tHeight, tWidth, minX, maxX, minY, maxY, 0.1);
    // generateFractalImage("lightFractal.png", tHeight, tWidth, minX, maxX, minY, maxY, 0.3);
}