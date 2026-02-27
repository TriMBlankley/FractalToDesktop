#include <stdio.h>
#include <math.h>

//  ---------------- Mandlebrot set fractal generator ---------------------------

typedef struct complex{
    float a;    // real part
    float b;    // imaginary part
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

float comSquareMag(complex x){
    return (x.a * x.a) + (x.b * x.b);
}

int mandlebrot(complex c){
    complex z = {0};    // Set all feilds to zero
    for (int i = 0; i < 10; i++){
        if (comSquareMag(z) > 4){
            return i;    // point NOT in set
        }
        z = comMultiply(z, z);
        z = comAdd(z, c);
    }
    return -1;    // point IS in set
}

float lerp(float num1, float num2, float x) {
    return x * (num2 - num1) + num1;
}

float unlerp(float num1, float num2, float y) {
    return (y - num1) / (num2 - num1);
}


int generateFractal (int imHeight, int imWidth, 
                     float minX, float maxX, 
                     float minY, float maxY){
    // header of the ppm image file
    printf("P3\n%d %d\n255\n", imWidth, imHeight);

    // Iterate over x, and y to create the size of the image
    for (int y = 0; y < imHeight; y++){
        for (int x = 0; x < imWidth; x++){
            int iterations = mandlebrot((complex) {
                lerp(minX, maxX, unlerp(0, imWidth, x)),
                lerp(minY, maxY, unlerp(imHeight, 0, y))
            });
            int red = lerp(0, 255, unlerp(-1, 1, -cos(iterations * 0.1)));
            int green = lerp(0, 255, unlerp(-1, 1, -cos(iterations * 0.11)));
            int blue = lerp(0, 255, unlerp(-1, 1, -cos(iterations * 0.13)));
            printf("%d %d %d ", red, green, blue);
        }
    }
}


// ---------------------- Convolution kernel for edge detection ------------------------

int sobel_x[3][3] = {
    {-1, 0, 1},
    {-2, 0, 2},
    {-1, 0, 1}
};

int sobel_y[3][3] = {
    {-1, -2, -1},
    {0, 0, 0},
    {1, 2, 1}
};



void simplifiedVisualization() {
    for(float imag = 2; imag > -2; imag -= 0.1){
        for(float real = -2; real < 2; real += 0.1){
            if(mandlebrot((complex) {real, imag}) == -1){
                putchar('x');
            }else {
                putchar(' ');
            }
        }
        putchar('\n');
    }
};





int main(){
    // --------------------- printing super simple image to the terminal
    // simplifiedVisualization();
    generateFractal(100, 120, -2, 2, -2, 2);
}