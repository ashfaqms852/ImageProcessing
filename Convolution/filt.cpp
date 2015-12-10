/*Author: Ashfaq Sherwa
  E-mail: asherwa@clemson.edu
  Programming Assignment # 4 for CpSc 6040
  Submitted On: 10/15/2015

  Implementation of filt.cpp

  This program:-
  ->reads, writes, displays image using OIIO and OpenGL concepts.
  ->handles input from keyboard.
  ->performs convolution using filter and image file provided by the user.
  ->performs convolution using gabor filter if -g flag is specified at command line instead of filter file.
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
#include<string.h>
#include<fstream>
using namespace std;

#define PI 3.14159265

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

static rgbaPixel **image, **paddedImage, **convolutedImage;
static vector<float> pixels;
int width, height, channels, paddingWidth, paddingHeight, filterSize, theta, sigma, period;
float * data, **filter, posSum = 0.0, negSum = 0.0;
const char * outputImage, * inputImage, * filterFile;
bool gabor = false;


// reads the image
void readImage(const char * filename) {

  ImageInput * img = ImageInput::open(filename);
  if (!img) {
    std::cout<<std::endl<<"Something is wrong with this file."<<std::endl;
    exit(-1);
  }

  ImageSpec imageSpec;

  img->open(filename , imageSpec);

  width = imageSpec.width;
  height = imageSpec.height;
  channels = imageSpec.nchannels;

  image = new rgbaPixel * [height];
  image[0] = new rgbaPixel [width*height];

  for(int i = 1; i < height; ++i) 
    image[i] = image[i-1] + width;

  pixels.resize(width*height*channels);

  if(!img->read_image(TypeDesc::FLOAT , &pixels[0])) {
    cout<<"Read error. Could not read from "<< filename <<endl;
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
  glViewport(0, 0, width, height);
  glRasterPos2i(0,0);
  glDrawPixels(width, height, GL_RGBA,GL_FLOAT, image[0]);
  glutSwapBuffers();
  glFlush();
}

// reads the filter file provided by user at command line
void readFilter() {
  ifstream input(filterFile);
  if(!input.is_open()){
    cout<<"Error in opening filter file."<<endl;
    exit(-1);
  }
  input>>filterSize;

  filter = new float * [filterSize];
  filter[0] = new float [filterSize*filterSize];

  for(int i = 1; i < filterSize; ++i)
    filter[i] = filter[i-1] + filterSize;

  float temp;

  for(int row = 0; row < filterSize; ++row){
    for(int col = 0; col < filterSize; ++col){
      input>>temp;
      filter[row][col] = temp;

      // scale-factor calculation
      if(filter[row][col] > 0.0)
        posSum += filter[row][col];
      else  negSum += filter[row][col];
    }
  }
  input.close();
}

// boundary mechanism, adds a boundary of filterSize/2 on all four sides of the image
void padImage() {

  paddingWidth = width -1 + filterSize;
  paddingHeight = height - 1 + filterSize;

  paddedImage = new rgbaPixel * [paddingHeight];
  paddedImage[0] = new rgbaPixel [paddingHeight*paddingWidth];

  for(int i = 1; i < paddingHeight; ++i)
    paddedImage[i] = paddedImage[i-1] + paddingWidth;

  for(int row = 0; row < paddingHeight; ++row) {
    for(int col = 0; col < paddingWidth; ++col) {

      //if pixel is falling outside image boundary, set it to black
      if((row < (filterSize/2)) or (row > (height - 1 + (filterSize/2))) or (col < (filterSize/2)) or (col > (width - 1 + (filterSize/2)))) {
        paddedImage[row][col].r = 0.0;
        paddedImage[row][col].g = 0.0;
        paddedImage[row][col].b = 0.0;
        paddedImage[row][col].a = 1.0;
      }
      // else keep as it is
      else {
        paddedImage[row][col].r = data[((row-filterSize/2)*width + col-filterSize/2)*4 + 0];
        paddedImage[row][col].g = data[((row-filterSize/2)*width + col-filterSize/2)*4 + 1];
        paddedImage[row][col].b = data[((row-filterSize/2)*width + col-filterSize/2)*4 + 2];
        paddedImage[row][col].a = data[((row-filterSize/2)*width + col-filterSize/2)*4 + 3];

      }
    }
  }

}

double degToRad( const double degrees )
{
  return degrees * PI/180.0;
}

void generateGaborFilter() {

  filterSize = 2*sigma + 1;
  filter = new float * [filterSize];
  filter[0] = new float [filterSize*filterSize];

  for(int i = 1; i < filterSize; ++i)
    filter[i] = filter[i-1] + filterSize;

  float xHat, yHat;

  for(int row = 0; row < filterSize; ++row) {
    for(int col = 0; col < filterSize; ++col) {
      xHat = (col-filterSize/2)*cos(degToRad(theta)) + (row-filterSize/2)*sin(degToRad(theta));
      yHat = (row-filterSize/2)*cos(degToRad(theta)) - (col-filterSize/2)*sin(degToRad(theta));

      filter[row][col] = exp(-(pow(xHat,2) + pow(yHat,2)) / (2*pow(sigma,2))) * cos(2*PI*xHat/period);

      //scale-factor calculation
      if(filter[row][col] > 0.0)
        posSum += filter[row][col];
      else  negSum += filter[row][col];

    }
  }
} 


void performConvolution() {

  if(gabor) generateGaborFilter();
  else readFilter();

  padImage();

  float sum, scaleFactor, mini = 0.0, maxi = 0.0;

  convolutedImage = new rgbaPixel * [height];
  convolutedImage[0] = new rgbaPixel [height*width];
  for(int i = 1; i < height; ++i)
    convolutedImage[i] = convolutedImage[i-1] + width;

  negSum = abs(negSum);

  if(negSum > posSum) scaleFactor = negSum;
  else scaleFactor = posSum;

  for(int row = 0; row < height; ++row) {
    for(int col = 0; col < width; ++col) {
      for(int ch = 0; ch < 4; ++ch) {
        sum = 0.0;

        for(int n = -(filterSize/2); n <= (filterSize/2); ++n) {
          for(int m = -(filterSize/2); m <= (filterSize/2); ++m) {
            sum += filter[n+(filterSize/2)][m+(filterSize/2)]*paddedImage[row+(filterSize/2)-n][col+(filterSize/2)-m][ch];
          }
        }
        sum /= scaleFactor;
        mini = std::min(mini , sum);
        maxi = std::max(maxi , sum);
        convolutedImage[row][col][ch] = sum;
      }
    }
  }

  //normalizing, brings channel values between 0 and 1
  for(int row = 0; row < height; ++row) 
    for(int col = 0; col < width; ++col)
      for(int ch = 0; ch < 4; ++ch) 
          convolutedImage[row][col][ch] = (convolutedImage[row][col][ch] - mini) / (maxi - mini);
      
} 

void handleKey(unsigned char key, int x, int y) {
  switch(key) {
    case 'w':
    case 'W':
      writeImage(outputImage);
      break;
    case 'r':
    case 'R':
      glutPostRedisplay();
      break;
    case 'c':
    case 'C':
      data = new float[width*height*4]; //keeping separate buffer
      glReadPixels(0, 0, width, height, GL_RGBA, GL_FLOAT, data);
      performConvolution();
      glRasterPos2i(0,0);
      glDrawPixels(width, height, GL_RGBA, GL_FLOAT, convolutedImage[0]);
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

int main(int argc , char ** argv) {

  if(argc < 3) {
    cout<<"Usage: ./filt filterfile.filt inImage.img [outImage.img] OR"<<endl;
    cout<<"./filt -g theta sigma period inImage.img [outImage.img]"<<endl;
    return -1;
  }  

  for(int i = 1; i < argc; ++i) {
    if(string(argv[i]) == "-g") gabor = true;
  }

  if(!gabor) {
    filterFile = argv[1];
    inputImage = argv[2];
    if(argc > 3) outputImage = argv[3];
  }
  else {
    int counter = 0;
    for(int i = 1; i < argc; ++i) {
      if(string(argv[i]) == "-g") {
        if(sscanf(argv[i+1], "%d", &theta) != 1 or
           sscanf(argv[i+2], "%d", &sigma) != 1 or
           sscanf(argv[i+3], "%d", &period) != 1) {
          cout<<"-g flag requires three parameters - theta, sigma and period"<<endl;
          return 1;
        }
        i += 3;
      }
      else {
        if(counter == 0) inputImage = argv[i];
        else if (counter == 1) outputImage = argv[i];
        counter++;
      }
    }
  }

  if(outputImage) 
    cout<<"Press W to write the displayed image."<<endl; 

  readImage(inputImage);

  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_SINGLE | GLUT_RGBA);
  glutInitWindowSize(width, height);
  glutCreateWindow("Convolved Image Viewer");

  glutDisplayFunc(displayImage);	 
  glutKeyboardFunc(handleKey);	 

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluOrtho2D(0, width, 0, height);
  glClearColor(1,1,1,1);

  glutMainLoop();
 
  return 0;
}
