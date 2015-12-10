/*Author: Ashfaq Sherwa
  E-mail: ashfaqms852@gmail.com
  Submitted On: 12/03/2015

  Implementation of histogram.cpp

  This program:-
  ->reads, writes, displays image using OIIO and OpenGL concepts.
  ->handles input from keyboard.
  ->performs Global Histogram Equalization and Local Histogram Equalization on the given image.

  Histogram equalization is a non-linear process so splitting and equalizing each channel won't produce
  correct results. So we have to convert RGB to HSV and equalize the intensity channel 'v'.
  This way we do not disturb the color balance of the image.
*/


#include"OpenImageIO/imageio.h"
OIIO_NAMESPACE_USING

#ifdef __APPLE__
#  include <GLUT/glut.h>
#else
#include<GL/glut.h>
#endif

#include<iostream>
#include<math.h>
#include<float.h>
using namespace std;

class rgbaPixel {
public:
  float r,g,b,a;

  float& operator[] (const unsigned int index) {
    if (index == 0) return r; 
    else if(index == 1) return g;
    else if (index == 2) return b;
    else if (index == 3) return a;
    else {
      cout<<"Invalid index."<<endl;
      exit(-2);
    }
  }
};

static rgbaPixel **image, **gheEqualizedImage, **paddedImage, **lheEqualizedImage;
static vector<float> pixels;
int width, height, paddingWidth, paddingHeight, channels;
int histogramForGHE[256] = {0}, cumHistogramForGHE[256], lutForGHE[256];
int windowSize, histogramForLHE[256] = {0}, **values, d[256];
const char * outputImage, * inputImage;
float * data;
bool firstPressG = false;

// initializes 2D array of type rgbaPixel
void initRGBAPixel2DArray(rgbaPixel ** &p, int w, int h) {
  p = new rgbaPixel * [h];
  p[0] = new rgbaPixel[w*h];
  
  for(int i = 1; i < h; ++i)
    p[i] = p[i-1] + w;
}

// initializes 2D array of type int
void initInteger2DArray(int ** &p, int w, int h) {
  p = new int * [h];
  p[0] = new int [w*h];
  
  for(int i = 1; i < h; ++i)
    p[i] = p[i-1] + w;
}


void RGBtoHSV(double red, double green, double blue, double & h, double & s, double & v){

  double max, min, delta;

  max = std::max(red, std::max(green, blue));
  min = std::min(red, std::min(green, blue));

  v = max;        /* value is maximum of r, g, b */

  if (max == 0) {    /* saturation and hue 0 if value is 0 */ 
    s = 0; 
    h = 0; 
  } 
  else { 
    s = (max - min)/max;           /* saturation is color purity on scale 0 - 1 */

    delta = max - min; 
    if (delta == 0)                    /* hue doesn't matter if saturation is 0 */ 
      h = 0; 
    else { 
      if(red == max) {                 /* otherwise, determine hue on scale 0 - 360 */ 
        h = (green - blue)/delta; 
      } 
      else if(green == max) {
        h = 2.0 + (blue - red)/delta; 
      } 
      else { /* (blue == max) */ 
        h = 4.0 + (red - green)/delta; 
      }
      h = h*60.0; 
      if (h < 0) 
        h = h + 360.0; 
    } 
  }
}

// Referred From - https://gist.github.com/fairlight1337/4935ae72bcbcc1ba5c72
void HSVtoRGB(double &red, double &green, double &blue, double h, double s, double v){

  double fC = v*s;
  double fHPrime = fmod(h/60.0, 6);
  double fX = fC * (1 - fabs(fmod(fHPrime,2)-1));
  double fM = v - fC;

  if(0 <= fHPrime && fHPrime < 1) {
    red = fC;
    green = fX;
    blue = 0;
  }
  else if(1 <= fHPrime && fHPrime < 2) {
    red = fX; 
    green = fC;
    blue = 0;
  }
  else if(2 <= fHPrime && fHPrime < 3) {
    red = 0; 
    green = fC;
    blue = fX;
  }
  else if(3 <= fHPrime && fHPrime < 4) {
    red = 0; 
    green = fX;
    blue = fC;
  }
  else if(4 <= fHPrime && fHPrime < 5) {
    red = fX; 
    green = 0;
    blue = fC;
  }
  else if(5 <= fHPrime && fHPrime < 6) {
    red = fC; 
    green = 0;
    blue = fX;
  }
  else {
    red = 0; 
    green = 0;
    blue = 0;
  }

  red += fM;
  green += fM;
  blue += fM;

}

// prepare look-up table for global histogram equalization
void prepareLUTForGHE() {

  double h, s, v;
  
  for(int row = 0; row < height; ++row) {
    for(int col = 0; col < width; ++col) {

      // generate histogram for 'v' channel after converting rgb to hsv      
      RGBtoHSV(image[row][col].r, image[row][col].g, image[row][col].b, h, s, v);
      histogramForGHE[int(v*255)]++;
     
    }
  }

  int mini = 255;

  cumHistogramForGHE[0] = histogramForGHE[0];

  for(int i = 0; i < 256; ++i) {
    
    // generate cumulative histogram and get the minimum non-zero value from it
    cumHistogramForGHE[i] = histogramForGHE[i] + cumHistogramForGHE[i-1];
    if(cumHistogramForGHE[i] != 0) {
      if(cumHistogramForGHE[i] < mini) {
        mini = cumHistogramForGHE[i];
      }
    }

  }

  for(int i = 0; i < 256; ++i) {

    /* prepare look-up table
       apply global histogram equalization formula and normalize the values between 0 and 255 */
  
    lutForGHE[i] = ((cumHistogramForGHE[i] - mini)*255/((width*height)-mini));

    if(lutForGHE[i] < 0) lutForGHE[i] = 0;
    if(lutForGHE[i] > 255) lutForGHE[i] = 255;
  
  }

}

void performGHEEqualization() {

  initRGBAPixel2DArray(gheEqualizedImage, width, height);

  for(int row = 0; row < height; ++row) {
    for(int col = 0; col < width; ++col) {

      /* convert rgb to hsv as we want to equalize the intensity ('v' channel) without affecting
         color information of the image.
         replace the 'v' channel by looking up from look-up table and convert back hsv to rgb */
     
      double h,s,v,r,g,b;
      RGBtoHSV(image[row][col].r, image[row][col].g, image[row][col].b, h, s, v);

      HSVtoRGB(r, g, b, h, s, (lutForGHE[(int)(v*255)]/255.0));

      // allocate new rgb values to the same pixel with their intensity equalized
      gheEqualizedImage[row][col].r = r;
      gheEqualizedImage[row][col].g = g;
      gheEqualizedImage[row][col].b = b;
     
      if(channels == 4)
        gheEqualizedImage[row][col].a = image[row][col].a;

    }
  }

}


// pad image to handle window falling outside image boundary
void padImage() {

  paddingWidth = width -1 + windowSize;
  paddingHeight = height - 1 + windowSize;

  initRGBAPixel2DArray(paddedImage, paddingWidth, paddingHeight);

  for(int row = 0; row < paddingHeight; ++row) {
    for(int col = 0; col < paddingWidth; ++col) {

      //if pixel is falling outside image boundary, set it to black
      if((row < (windowSize/2)) or (row > (height - 1 + (windowSize/2))) or (col < (windowSize/2)) or (col > (width - 1 + (windowSize/2)))) {
        paddedImage[row][col].r = 0.0;
        paddedImage[row][col].g = 0.0;
        paddedImage[row][col].b = 0.0;
        paddedImage[row][col].a = 1.0;
      }
      // else keep as it is
      else {
        paddedImage[row][col].r = data[((row-windowSize/2)*width + col-windowSize/2)*4 + 0];
        paddedImage[row][col].g = data[((row-windowSize/2)*width + col-windowSize/2)*4 + 1];
        paddedImage[row][col].b = data[((row-windowSize/2)*width + col-windowSize/2)*4 + 2];
        paddedImage[row][col].a = data[((row-windowSize/2)*width + col-windowSize/2)*4 + 3];
      }

    }
  }

}


// populate a 2d array with only 'v' channel values of each pixel in padded image
void vOfEachPixel() {

  double h, s, v;
  initInteger2DArray(values, paddingWidth, paddingHeight);

  for(int row = 0; row < paddingHeight; ++row) {
    for(int col = 0; col < paddingWidth; ++col) {
    
      RGBtoHSV(paddedImage[row][col].r, paddedImage[row][col].g, paddedImage[row][col].b, h, s, v);
      values[row][col] = round(v*255.0);

    }
  }

}

// generates local histogram of 'v' channel of (windowSize*windowSize) pixels
void generatehistogramForLHE(int &r, int &c) {

  for(int row = (r - (windowSize/2)); row < (r+(windowSize/2)+1); ++row) 
    for(int col = (c - (windowSize/2)); col < (c+(windowSize/2)+1); ++col) 
      histogramForLHE[values[row][col]]++;

}

// perform local historgram equalization for each pixel of image
void performLHEEqualization() {

  initRGBAPixel2DArray(lheEqualizedImage, width, height);
  double h, s, v, r, g, b;

  // by-passing the padding, start from top-left pixel of original image until we reach bottom-right pixel
  for(int row = (windowSize/2); row < ((windowSize/2)+height); ++row) {
    for(int col = (windowSize/2); col < ((windowSize/2) +width); ++col) {

      generatehistogramForLHE(row,col);
      int sum = 0;
      for(int i = 0; i < 256; ++i) {
        if(histogramForLHE[i] != 0) {
          sum = sum + histogramForLHE[i]; // probability density of each value in histogram
          d[i] = round(sum*255.0/(float)(windowSize*windowSize)); // cumulative distribution of each value
        }
        else d[i] = 0;
      }

      /* convert rgb to hsv as we want to equalize the intensity ('v' channel) without affecting
         color information of the image.
         replace the 'v' channel by looking up from d[] and convert back hsv to rgb */

      RGBtoHSV(image[row-(windowSize/2)][col-(windowSize/2)].r, image[row-(windowSize/2)][col-(windowSize/2)].g, image[row-(windowSize/2)][col-(windowSize/2)].b, h, s, v);

      HSVtoRGB(r, g, b, h, s, d[values[row][col]]/255.0);

      // allocate new rgb values to the same pixel with their intensity equalized
      lheEqualizedImage[row-(windowSize/2)][col-(windowSize/2)].r = r;
      lheEqualizedImage[row-(windowSize/2)][col-(windowSize/2)].g = g;
      lheEqualizedImage[row-(windowSize/2)][col-(windowSize/2)].b = b;
     
      if(channels == 4)
        lheEqualizedImage[row-(windowSize/2)][col-(windowSize/2)].a = image[row-(windowSize/2)][col-(windowSize/2)].a;

      // reset histogram and cumulative distribution arrays to zero for next pixel
      for(int j = 0; j < 256; ++j)
        histogramForLHE[j] = d[j] = 0;

    }
  }
}

// reads the image
void readImage(const char * filename) {

  ImageInput * img = ImageInput::open(filename);
  if (!img) {
    std::cout<<std::endl<<"Read Error: Could not open "<< filename <<std::endl;
    exit(-1);
  }

  ImageSpec imageSpec;

  img->open(filename , imageSpec);

  width = imageSpec.width;
  height = imageSpec.height;
  channels = imageSpec.nchannels;
  initRGBAPixel2DArray(image, width, height);

  pixels.resize(width*height*channels);

  if(!img->read_image(TypeDesc::FLOAT , &pixels[0])) {
    cout<<"Read error: Could not read from "<< filename <<endl;
    cout<< img->geterror() <<endl;
    delete img;
    exit(-1);
  }
  
  for(int row = 0; row < height; ++row) {
    for(int col = 0; col < width; ++col) {

      image[height-row-1][col].r = pixels[(row*width+col)*channels + 0];
      image[height-row-1][col].g = pixels[(row*width+col)*channels + 1];
      image[height-row-1][col].b = pixels[(row*width+col)*channels + 2];

      if(channels == 3)
        image[height-row-1][col].a = 1.0;
      else
        image[height-row-1][col].a = pixels[(row*width+col)*channels + 3];

    }
  }
  img->close();
  delete img;

}

// writes the image when user presses W/w
void writeImage(const char * filename) {

  if(filename == NULL) {
    cout<<"Filename not provided."<<endl;
    exit(-1);
  }  
  
  std::vector<unsigned char> flippedPixels(width*height*4);
  std::vector<unsigned char> tempPixels(width*height*4);

  glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, &tempPixels[0]);

  int i = 0;
  int temp = (width*height*4) - (width*4);

  while(temp >= 0) {
    for(int j = 0; j < (width*4); ++j) {
      flippedPixels.at(i++) = tempPixels.at(j+temp);
    }
    temp = temp - (width*4);
  }
  ImageOutput * out = ImageOutput::create(filename);
  if (!out) {
    std::cerr << "Could not create: " << geterror();
  }

  ImageSpec spec(width, height, 4, TypeDesc::UINT8);
  
  out->open(filename , spec);
  
  if(out->write_image(TypeDesc::UINT8, &flippedPixels[0]))
    cout<<"Image written successfully."<<endl;
  else
    cout<<"Something went wrong while writing the image. "<<geterror()<<endl;
  out->close();
  delete out;

}

// displays the image
void displayImage() {
  glClear(GL_COLOR_BUFFER_BIT);
  glRasterPos2i(0,0);
  glViewport(0, 0, width, height);  
  glDrawPixels(width, height, GL_RGBA,GL_FLOAT, image[0]);
  glutSwapBuffers();
  glFlush();
}


void handleKey(unsigned char key, int x, int y) {

  switch(key) {
    case 'w':
    case 'W':
      writeImage(outputImage);
      break;
    case 'o':
    case 'O':
      glRasterPos2i(0,0);
      glDrawPixels(width, height, GL_RGBA, GL_FLOAT, image[0]);
      glutSwapBuffers();
      break;      
    case 'g':
    case 'G':
      // do not want to repetitively apply GHE on image on repeated pressing of 'g/G'
      if(!firstPressG) {  
        prepareLUTForGHE();
        performGHEEqualization();
      }
      glRasterPos2i(0,0);
      glDrawPixels(width, height, GL_RGBA, GL_FLOAT, gheEqualizedImage[0]);
      glutSwapBuffers();
      firstPressG = true;
      break;
    case 'l':
    case 'L':
      cout<<"Please enter the window size for LHE: "<<endl;
      cin>>windowSize;
      if(windowSize%2 == 0) windowSize += 1;
      data = new float[width*height*4];
      glReadPixels(0, 0, width, height, GL_RGBA, GL_FLOAT, data);
      padImage();
      vOfEachPixel();
      performLHEEqualization();
      glRasterPos2i(0,0);
      glDrawPixels(width, height, GL_RGBA, GL_FLOAT, lheEqualizedImage[0]);
      glutSwapBuffers();
      break;    
    case 'q':
    case 'Q':
    case 27 :
      exit(0);
      break;
    default:
      return;
      
  }
}


void displayError() {
  cout<<"Usage:"<<endl;
  cout<<"./histogramForGHE input.img [output.img]"<<endl;
}


int main(int argc , char ** argv) {
  
  if(argc < 2 or argc > 3) {
    displayError();
    return -1;
  }

  inputImage = argv[1];
  readImage(inputImage);

  if(argc == 3) {
    outputImage = argv[2];
    cout<<"Press W to write the displayed image."<<endl; 
  }

  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
  glutInitWindowSize(width, height);
  glutCreateWindow("GHE/LHE");

  glClearColor(0.0, 0.0, 0.0, 0.0);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluOrtho2D(0, width, 0, height);

  glutDisplayFunc(displayImage);	 
  glutKeyboardFunc(handleKey);	 

  glutMainLoop();
 
  return 0;
}
