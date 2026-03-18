#include <stdio.h>
#include <math.h>
#include <stdint.h>
#include <time.h>
#include <stdbool.h>
#include <unistd.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"


typedef uint64_t u64;

// ----------------- Compiler Macros:
#define ARRAY_LEN(A) (sizeof(A)/sizeof(*(A)))

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


// ----------------- Usefull functions -----------------------------------------

int absoluteVal (int num){
    if (num < 0){
        return num * -1;
    }else {
        return num;
    }
}

int clamp (int min, int max, int val) {
    if ( val < min ){
        return min;
    }else if ( val > max ){
        return max;
    }else return val;
}

typedef struct Lab {float L; float a; float b;} Lab;
typedef struct RGB {float r; float g; float b;} RGB;

Lab linear_srgb_to_oklab(RGB c) {
    float l = 0.4122214708f * c.r + 0.5363325363f * c.g + 0.0514459929f * c.b;
	float m = 0.2119034982f * c.r + 0.6806995451f * c.g + 0.1073969566f * c.b;
	float s = 0.0883024619f * c.r + 0.2817188376f * c.g + 0.6299787005f * c.b;

    float l_ = cbrtf(l);
    float m_ = cbrtf(m);
    float s_ = cbrtf(s);

    return (Lab) {
        0.2104542553f*l_ + 0.7936177850f*m_ - 0.0040720468f*s_,
        1.9779984951f*l_ - 2.4285922050f*m_ + 0.4505937099f*s_,
        0.0259040371f*l_ + 0.7827717662f*m_ - 0.8086757660f*s_,
    };
}

RGB oklab_to_linear_srgb(Lab c) {
    float l_ = c.L + 0.3963377774f * c.a + 0.2158037573f * c.b;
    float m_ = c.L - 0.1055613458f * c.a - 0.0638541728f * c.b;
    float s_ = c.L - 0.0894841775f * c.a - 1.2914855480f * c.b;

    float l = l_*l_*l_;
    float m = m_*m_*m_;
    float s = s_*s_*s_;

    return (RGB) {
		+4.0767416621f * l - 3.3077115913f * m + 0.2309699292f * s,
		-1.2684380046f * l + 2.6097574011f * m - 0.3413193965f * s,
		-0.0041960863f * l - 0.7034186147f * m + 1.7076147010f * s,
    };
}

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

Lab lerpOkLAB(Lab col1, Lab col2, double x) {
    return (Lab) { lerp(col1.L, col2.L, x),
                   lerp(col1.a, col2.a, x),
                   lerp(col1.b, col2.b, x)};
}

double unlerp(double num1, double num2, double y) {
    return (y - num1) / (num2 - num1);
}


// -------------- Find a coordintate that borders points in the set -------------------------------------------

complex coordinateFinder(int imHeight, int imWidth,
                         double minX, double maxX, 
                         double minY, double maxY,
                         int *minIteration){

    int simpleFractal[imHeight][imWidth];
    int fractalEdgeX[imWidth * imHeight];
    int fractalEdgeY[imWidth * imHeight];


    for (int y = 0; y < imHeight; y++){
        for (int x = 0; x < imWidth; x++){
            int currentIteration = (mandlebrot((complex) {
                lerp(minX, maxX, unlerp(0, imWidth, x)),
                lerp(minY, maxY, unlerp(imHeight, 0, y))
            }, 100));
            if (currentIteration == -1){
                simpleFractal[y][x] = 1;
            }else {
                simpleFractal[y][x] = 0;
                if (currentIteration < *minIteration){
                    *minIteration = currentIteration;
                }
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

    if (index == 0) {
        return (complex){lerp(minX,maxX,0.5), lerp(minY,maxY,0.5)};
    }
    
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

                    image_data[pixel_index + 0] += lerp(0, 255, unlerp(-1, 1, -cos(iterations * 0.10))) / 9;
                    image_data[pixel_index + 1] += lerp(0, 255, unlerp(-1, 1, -cos(iterations * 0.11))) / 9;
                    image_data[pixel_index + 2] += lerp(0, 255, unlerp(-1, 1, -cos(iterations * 0.13))) / 9;

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
        printf("%s file created successfully!\n", fileName);
    } else {
        printf("Error creating PNG file\n");
    }
    
    // Free the allocated memory
    free(image_data);
    
    return 0;
}


int generateFractalImagePreDefinedColorPallete (const char *fileName,
    int imHeight, int imWidth, 
    double minX, double maxX, 
    double minY, double maxY, 
    RGB baseCol, RGB col1, RGB col2,
    RGB col3, RGB col4, RGB col5, 
    int iterationPerBand, int minIteration, int offset) {// TODO Use the min iteration

    

    RGB offsetColorGrad[] = {col1, col2, col3, col4, col5};
    Lab gradTop = {0, 0, 0};
    Lab gradBottom = {0, 0, 0};
    
   
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
                    }, 2000) - minIteration;


                    // Define the top and bottom of the current color band
                    int c = iterations / iterationPerBand;

                    gradTop = lerpOkLAB(
                        linear_srgb_to_oklab(baseCol), 
                        linear_srgb_to_oklab(
                            offsetColorGrad[(c + offset) % ARRAY_LEN(offsetColorGrad)]
                        ),
                        (float) c / (c + 1)
                    );
                    gradBottom = lerpOkLAB(
                        linear_srgb_to_oklab(baseCol), 
                        linear_srgb_to_oklab(
                            offsetColorGrad[(c + offset + 1) % ARRAY_LEN(offsetColorGrad)]
                        ),
                        (float) ( c + 1) / (c + 2)
                    );

                    float x = ((int) iterations % iterationPerBand) / (float) iterationPerBand;


                    RGB currentColor = oklab_to_linear_srgb(
                        lerpOkLAB(gradTop, gradBottom, x)
                    );

                    image_data[pixel_index + 0] += currentColor.r / 9;
                    image_data[pixel_index + 1] += currentColor.g / 9;
                    image_data[pixel_index + 2] += currentColor.b / 9;

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
        printf("%s file created successfully!\n", fileName);
    } else {
        printf("Error creating PNG file\n");
    }
    
    // Free the allocated memory
    free(image_data);
    
    return 0;
}


int edgeResponceCalc (const char * fileName){
    int channels = 3, imWidth = 0, imHeight = 0;
    printf("%d, %d", imWidth, imHeight);

    unsigned char *imData = stbi_load(fileName, &imWidth, &imHeight, &channels, 0);

    printf("%d, %d", imWidth, imHeight);

    int greyImage[imHeight][imWidth];


    for(int x=0; x < imWidth; x++){
        for(int y=0; y < imHeight; y++){
            int pixel_index = (y * imWidth + x) * channels;
            unsigned int colour = 0.299*imData[pixel_index + 0] + 
                                  0.587*imData[pixel_index + 1] + 
                                  0.184*imData[pixel_index + 2];
            greyImage[y][x] = colour;
            
        }
    }

    int pixelEdgeResponce = 0;
    #define quadrantSplit 3
    int edgeResponce[quadrantSplit][quadrantSplit] = {0};

    for(int x = 1; x < imWidth - 1; x++){
        for(int y = 1; y < imHeight - 1; y++){

            pixelEdgeResponce = (
                absoluteVal(
                    (greyImage[y - 1][x - 1] * - 1) +
                    (greyImage[y    ][x - 1] * - 2) +
                    (greyImage[y + 1][x - 1] * - 1) +
                    (greyImage[y + 1][x    ] *   0) +
                    (greyImage[y - 1][x + 1] *   1) +
                    (greyImage[y    ][x + 1] *   2) +
                    (greyImage[y + 1][x + 1] *   1) +
                    (greyImage[y - 1][x    ] *   0)
                ) +

                absoluteVal(
                    (greyImage[y - 1][x - 1] * - 1) +
                    (greyImage[y - 1][x    ] * - 2) +
                    (greyImage[y - 1][x + 1] * - 1) +
                    (greyImage[y    ][x + 1] *   0) +
                    (greyImage[y + 1][x - 1] *   1) +
                    (greyImage[y + 1][x    ] *   2) +
                    (greyImage[y + 1][x + 1] *   1) +
                    (greyImage[y    ][x - 1] *   0)
                )
            ) / 2;

            // Assign based off the quadrant
            edgeResponce[x / (imWidth / quadrantSplit)]
                        [y / (imHeight / quadrantSplit)] += pixelEdgeResponce;
        }
    }

    // Find minimum quadrant edge responce 
    edgeResponce[0][0] -= 5000; // make the center more likely to reject a sample
    int minQuadrantEdge = edgeResponce[0][0];
    for (int c = 0; c < quadrantSplit; c++){
        for (int v = 0; v< quadrantSplit; v++){
            if ( edgeResponce[c][v] < minQuadrantEdge ){
                minQuadrantEdge = edgeResponce[c][v];
            }
        }
    }

    stbi_image_free(imData);
    return minQuadrantEdge;
}


void okLABGradientGen (RGB col1, RGB col2, RGB col3,
                       RGB col4, RGB col5, RGB col6, int iterationPerBand) {

    RGB offsetColorGrad[] = {col2, col3, col4, col5, col6};


    printf("P3\n200 2000\n255\n");

    for (int i = 0; i < 20; i++){
        for (int j = 0; j < 200; j++){
            printf("255 255 255 ");
        } 
    }printf("\n");

    RGB gradTop = {0, 0, 0};
    RGB gradBottom = {0, 0, 0};
    int offset = randu64(&rng) % 5;

    for (int c = 0; c < 6; c++){
        if (c == 0) gradTop = col1;
        else gradTop = offsetColorGrad[
            (c + offset) % ARRAY_LEN(offsetColorGrad)
        ];
        gradBottom = offsetColorGrad[
            (c + 1 + offset) % ARRAY_LEN(offsetColorGrad)
        ];

        Lab gradTopLab = linear_srgb_to_oklab(gradTop);
        Lab gradBottomLab = linear_srgb_to_oklab(gradBottom);

        for (int i = 0; i < iterationPerBand; i++){
            float x = i / (float) iterationPerBand;
            RGB currentColor = oklab_to_linear_srgb((Lab){
                lerp(gradTopLab.L, gradBottomLab.L, x),
                lerp(gradTopLab.a, gradBottomLab.a, x),
                lerp(gradTopLab.b, gradBottomLab.b, x)
            });
            for (int j = 0; j < 200; j++){
                printf("%d %d %d ", (int) currentColor.r, 
                                    (int) currentColor.g, 
                                    (int) currentColor.b);
            } 
        }
        printf("\n");
    }
}


int main(){
    // Initialize the random number generator
    rng.s[1] = 69420;
    rng.s[0] = time(NULL);
    
    for (int r = 0; r < 5; r++) randu64(&rng);

    //int 0x
    // Initialize Colors:
    RGB lightBaseCol = {255, 252, 215};
    RGB lightCol1 = {104, 0, 150};
    RGB lightCol2 = {0, 128, 133};
    RGB lightCol3 = {52, 134, 0};
    RGB lightCol4 = {168, 138, 0};
    RGB lightCol5 = {170, 50, 50};


    RGB darkBaseCol = {28, 25, 45};
    RGB darkCol1 = {217, 136, 255};
    RGB darkCol2 = {136, 251, 255};
    RGB darkCol3 = {198, 255, 167};
    RGB darkCol4 = {255, 233, 139};
    RGB darkCol5 = {255, 139, 139};


    // okLABGradientGen (lightCol1, lightCol2, lightCol3,
    //                   lightCol4, lightCol5, lightCol6, 300);
    // return 0;

    

    int sampleWidth = 100;
    int tHeight = 1504;
    int tWidth = 2256;
    // int tHeight = 300;
    // int tWidth = 450;

    int minIteration;
    bool foundFractal = false;
    int edgeResponceCutoff = 50000;
    double minX = -2, maxX = 2, minY = -2, maxY = 2;

    while (foundFractal == false){
    // for (int i = 0; i < 100; i++){
        minX = -2, maxX = 2, minY = -2, maxY = 2;

        for (int depth = 0; depth < 10; depth ++){
            minIteration = 100;
            complex coordinates = coordinateFinder(60, 60, minX, maxX, minY, maxY, &minIteration);

            double zoomFactor = 4;
            minX = lerp(coordinates.a, minX, 1 / zoomFactor);
            maxX = lerp(coordinates.a, maxX, 1 / zoomFactor);
            minY = lerp(coordinates.b, minY, 1 / zoomFactor);
            maxY = lerp(coordinates.b, maxY, 1 / zoomFactor);
        }

        generateFractalImage("sample.png", sampleWidth, sampleWidth, minX, maxX, minY, maxY, 0.25);

        int edgeResponce = abs(edgeResponceCalc("sample.png"));
        if (absoluteVal(edgeResponce) > 50000){
            foundFractal = true;
        }
        // foundFractal = true;

        // if (absoluteVal(edgeResponce) > edgeResponceCutoff){
        //     char str[100];
        //     sprintf(str, "%d-fractal.png", edgeResponce);
        //     rename("sample.png", str);
        // } else unlink("sample.png"); 
        unlink("sample.png"); 
    }
    //Math to fix the aspect ratio when not a square
    minY = lerp(minY, maxY, (1 - ((double)tHeight / tWidth)) / 2 );
    maxY = lerp(maxY, minY, (1 - ((double)tHeight / tWidth)) / 2 );

    printf("Fractal Address: %lf, %lf, %lf, %lf\n, minimum iterations: %d \n", minX, maxX, minY, maxY, minIteration);
    // generateFractalImage("darkFractal.png", tHeight, tWidth, minX, maxX, minY, maxY, 0.1);
    // generateFractalImage("lightFractal.png", tHeight, tWidth, minX, maxX, minY, maxY, 0.25);

    int offset = randu64(&rng) % 5;
    generateFractalImagePreDefinedColorPallete("lightFractal.png", 
        tHeight, tWidth, minX, maxX, minY, maxY,
        lightBaseCol, lightCol1, lightCol2, lightCol3, lightCol4, lightCol5,
        70, minIteration, offset);
    generateFractalImagePreDefinedColorPallete("darkFractal.png", 
        tHeight, tWidth, minX, maxX, minY, maxY,
        darkBaseCol, darkCol1, darkCol2, darkCol3, darkCol4, darkCol5,
        70, minIteration, offset);

    
    return 0;
}