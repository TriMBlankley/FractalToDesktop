#include <stdio.h>
#include <math.h>

typedef struct complex{
    float a;
    float b;
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
    complex z = {0};  // Set all feilds to zero
    for (int i = 0; i < 100; i++){
        if (comSquareMag(z) > 4){
            return i;
        }
        z = comMultiply(z, z);
        z = comAdd(z, c);
    }
    return -1;
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


int main(){
    // --------------------- printing super simple image to the terminal
    // for(float imag = 2; imag > -2; imag -= 0.1){
    //     for(float real = -2; real < 2; real += 0.1){
    //         if(mandlebrot((complex) {real, imag}) == -1){
    //             putchar('x');
    //         }else {
    //             putchar(' ');
    //         }
    //     }
    //     putchar('\n');
    // }
    generateFractal(10000, 10000, -2, 2, -2, 2);
}