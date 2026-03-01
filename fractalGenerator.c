#include <stdio.h>
#include <math.h>
#include <stdint.h>
#include <time.h>
#include <stdbool.h>

#include "stb_image_write.h"
#include "stb_image_resize2.h"

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

int mandlebrot(complex c){
    complex z = {0};    // Set all feilds to zero
    for (int i = 0; i < 1000; i++){
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


// ---------------  Final fractal Image Generation ----------------------------

int generateFractalImage (int imHeight, int imWidth, 
                          double minX, double maxX, 
                          double minY, double maxY){
                          
    // header of the ppm image file
    printf("P3\n%d %d\n255\n", imWidth, imHeight);

    // Iterate over x, and y to create the size of the image
    for (int y = 0; y < imHeight; y++){
        for (int x = 0; x < imWidth; x++){
            double iterations = mandlebrot((complex) {
                lerp(minX, maxX, unlerp(0, imWidth, x)),
                lerp(minY, maxY, unlerp(imHeight, 0, y))
            });
            iterations = iterations * 0.25;
            int red = lerp(0, 255, unlerp(-1, 1, -cos(iterations * 0.1)));
            int green = lerp(0, 255, unlerp(-1, 1, -cos(iterations * 0.11)));
            int blue = lerp(0, 255, unlerp(-1, 1, -cos(iterations * 0.13)));
            printf("%d %d %d ", red, green, blue);
        }
    }
};


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
            }) == -1){
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
            if(simpleFractal[yOut][xOut] == 1) continue;
            if (simpleFractal[yOut - 1][xOut - 1] +
                simpleFractal[yOut - 1][xOut    ] +
                simpleFractal[yOut - 1][xOut + 1] +
                simpleFractal[yOut    ][xOut + 1] +
                simpleFractal[yOut + 1][xOut - 1] +
                simpleFractal[yOut + 1][xOut    ] +
                simpleFractal[yOut + 1][xOut + 1] +
                simpleFractal[yOut    ][xOut - 1] == 0) continue;

            fractalEdgeX[index] = xOut;
            fractalEdgeY[index] = yOut;
            index++;  
        }
    }


    // Choose the random edge case.
    for (int r = 0; r < 5; r++){
        randu64(&rng);
    }
    int randIndex = randu64(&rng) % index;

    // ---------- Simple Terminal View ---------------------------------------
    // for (int yPrint = 1; yPrint < imHeight - 1; yPrint++){
    //     for (int xPrint = 1; xPrint < imWidth - 1; xPrint++){
    //         bool isEdge = false;
    //         for (int i =0; i < index; i++){
    //             if (fractalEdgeX[i] == xPrint && fractalEdgeY[i] == yPrint){
    //                 printf("\033[7m%d\033[0m", simpleFractal[yPrint][xPrint]);
    //                 isEdge = true;
    //             } 
    //         } 
    //         if (isEdge == false){
    //             printf("%d", simpleFractal[yPrint][xPrint]);
    //         }
    //     }
    //     printf("\n");
    // }



    return (complex){lerp(minX, maxX, unlerp(0, imWidth, fractalEdgeX[randIndex])),
                     lerp(minY, maxY, unlerp(imHeight, 0, fractalEdgeY[randIndex]))
    };
 
};



int main(){
    rng.s[1] = 69420;
    rng.s[0] = time(NULL);

    double minX = -2, maxX = 2, minY = -2, maxY = 2;
    
    for (int depth = 0; depth < 5; depth ++){
        complex coordinates = coordinateFinder(60, 60, minX, maxX, minY, maxY);

        double zoomFacotr = 4;
        minX = lerp(coordinates.a, minX, 1 / zoomFacotr);
        maxX = lerp(coordinates.a, maxX, 1 / zoomFacotr);
        minY = lerp(coordinates.b, minY, 1 / zoomFacotr);
        maxY = lerp(coordinates.b, maxY, 1 / zoomFacotr);
    }
    
    printf("Fractal Address: %d, %d, %d, %d", minX, maxX, minY, maxY);
    generateFractalImage(1000, 1000, minX, maxX, minY, maxY);
}