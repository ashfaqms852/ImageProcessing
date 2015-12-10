/*Author: Ashfaq Sherwa
  E-mail: asherwa@clemson.edu
  Programming Assignment # 5 for CpSc 6040
  Submitted On: 10/27/2015

  Implementation of tonemap.cpp

  This program:-
  ->reads, writes, displays image using OIIO and OpenGL concepts.
  ->handles input from keyboard.
  Depending on the flag provided by user at command line it performs
  ->simple tonemap(-g flag) on the image file provided 
    by user.
  ->tone mapping with convolution (-c flag) and
  ->tone mapping with convolution using bilateral filter (-b flag)
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

static rgbaPixel **image, **tonedImage;
static vector<float> pixels;
static float **worldLuminance, **filter, **B, **S, **paddedImage, g;
int width, height, channels, paddingWidth, paddingHeight, filterSize;
const char * outputImage, * inputImage;
float BMax = FLT_MIN, BMin = FLT_MAX, scale, sum;
bool switchToTonedImage = false, switchToBilateralFilter;

// initializes 2D array of type rgbaPixel
void initRGBAPixel2DArray(rgbaPixel ** &p, int w, int h) {
  p = new rgbaPixel * [h];
  p[0] = new rgbaPixel[w*h];
  
  for(int i = 1; i < h; ++i)
    p[i] = p[i-1] + w;
}

// initializes 2D array of type float
void initFloat2DArray(float ** &f, int w, int h) {
  f = new float * [h];
  f[0] = new float[w*h];
  
  for(int i = 1; i < h; ++i)
    f[i] = f[i-1] + w;
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
    cout<<"Write error: Filename not provided."<<endl;
    exit(-1);
  }  
  
  ImageOutput * out = ImageOutput::create(filename);
  if (!out) {
    std::cerr << "Write Error: Could not create: " << filename << geterror();
  }

  ImageSpec spec(width, height, channels, TypeDesc::FLOAT);
  
  out->open(filename , spec);
  pixels.resize(width*height*channels);
  int iterator = 0;
  for(int row = height-1; row >= 0; --row) {
    for(int col = 0; col < width; ++ col) {
      pixels[iterator++] = tonedImage[row][col].r;
      pixels[iterator++] = tonedImage[row][col].g;
      pixels[iterator++] = tonedImage[row][col].b;
      if(channels == 4)
        pixels[iterator++] = tonedImage[row][col].a;
    }
  }

  if(out->write_image(TypeDesc::FLOAT, &pixels[0]))
    cout<<"Image written successfully."<<endl;
  else
    cout<<"Write Error: Something went wrong while writing the image. "<<geterror()<<endl;
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

// performs simple tonemaping 
void simpleToneMap() {
  float lW, lD;

  initRGBAPixel2DArray(tonedImage, width, height);

  for(int row = 0; row < height; ++row) {
    for(int col = 0; col < width; ++col) {
      
      lW = (1/61.0)*(20.0 * image[row][col].r + 40.0 * image[row][col].g + image[row][col].b);

      if(lW == 0) lW = FLT_MIN; //correction to avoid caculating log of zero
      lD = exp(g*log(lW));
   
      //scaling each color channel separately using lW and lD
      tonedImage[row][col].r = lD/lW * image[row][col].r;
      tonedImage[row][col].g = lD/lW * image[row][col].g;
      tonedImage[row][col].b = lD/lW * image[row][col].b; 
      tonedImage[row][col].a = lD/lW * image[row][col].a;
    
    }
  }
}

// boundary mechanism, adds a boundary of filterSize/2 on all four sides of the image
void padImage() {

  paddingWidth = width -1 + filterSize;
  paddingHeight = height - 1 + filterSize;

  initFloat2DArray(paddedImage, paddingWidth, paddingHeight);

  for(int row = 0; row < paddingHeight; ++row) {
    for(int col = 0; col < paddingWidth; ++col) {

      //if pixel is falling outside image boundary, set it to black
      if((row < (filterSize/2)) or (row > (height - 1 + (filterSize/2))) or (col < (filterSize/2)) or (col > (width - 1 + (filterSize/2)))) 
        paddedImage[row][col] = 0.0;
       else 
        paddedImage[row][col] = log(worldLuminance[row-(filterSize/2)][col-(filterSize/2)]); // log(Lw)
      
    }
  }

}

// generates the box filter of size specified by user.
void generateBoxFilter() {
  int sigma;
  cout<<"Filter's size would be 2*sigma + 1."<<endl;
  cout<<"Enter the sigma value for Box Filter: "<<endl;
  cin>> sigma;
  filterSize = 2*sigma + 1;

  initFloat2DArray(filter, filterSize, filterSize);

  for(int row = 0; row < filterSize; ++row) {
    for(int col = 0; col < filterSize; ++col) {
      filter[row][col] = 1.0;
      scale += filter[row][col];
    }
  }
}

void convolveLogSpaceLuminance() {

  initFloat2DArray(B, width, height);

  // Step 1: B = log(lW) convolve Box Filter
  for(int row = 0; row < height; ++row) {
    for(int col = 0; col < width; ++col) {

      sum = 0.0;

      for(int i = -(filterSize/2); i <= (filterSize/2); ++i) { 
        for(int j = -(filterSize/2); j <= (filterSize/2); ++j) { 
          sum += filter[i+(filterSize/2)][j+(filterSize/2)] * paddedImage[row+(filterSize/2)-i][col+(filterSize/2)-j];
        }
      }
      sum /= scale;
      B[row][col] = sum; // low pass image
 
      // keeping tab on BMax and BMin which would help in calculating g
      if(B[row][col] > BMax)
        BMax = B[row][col];
      
      if(BMin > B[row][col])
        BMin = B[row][col];        
      
    }
  }
}

void convolveUsingBilateralFilter() {

  float dSquare, d, w;

  initFloat2DArray(B, width, height);

  for(int row = 0; row < height; ++row) {
    for(int col = 0; col < width; ++col) {

      sum = 0.0, scale = 0.0;

      for(int i = -(filterSize/2); i <= (filterSize/2); ++i) { 
        for(int j = -(filterSize/2); j <= (filterSize/2); ++j) {
          
          d = paddedImage[row+(filterSize/2)][col+(filterSize/2)] - paddedImage[row+(filterSize/2)-i][col+(filterSize/2)-j];

          dSquare = d*d;
       
          //clamping
          if(dSquare < 0.0) dSquare = 0.0;
          else if(dSquare > 1.0) dSquare = 1.0;

          w = exp(-dSquare);

          scale += w;

          sum += filter[i+(filterSize/2)][j+(filterSize/2)] * paddedImage[row+(filterSize/2)-i][col+(filterSize/2)-j]*w;
 
        }
      }

      sum /= scale;
      B[row][col] = sum; // low pass image

      // keeping tab on BMax and BMin which would help in calculating g
      if(B[row][col] > BMax) BMax = B[row][col];
 
      if(BMin > B[row][col]) BMin = B[row][col];

    }     
  }
}

void convolutedToneMap() {
  float lW, lD;

  initFloat2DArray(worldLuminance, width, height);

  for(int row = 0; row < height; ++row) {
    for(int col = 0; col < width; ++col) {

      lW = (20.0*image[row][col].r + 40.0*image[row][col].g + image[row][col].b) / 61.0;

      if(lW == 0) lW = FLT_MIN;

      worldLuminance[row][col] = lW;
    }
  }

  generateBoxFilter();
  padImage();

  if(!switchToBilateralFilter) convolveLogSpaceLuminance();
  else convolveUsingBilateralFilter();

  initFloat2DArray(S, width, height);

  //Step 2: S = log(Lw) - B
  for(int row = 0; row < height; ++row)
    for(int col = 0; col < width; ++col)
      S[row][col] = log(worldLuminance[row][col]) - B[row][col]; // high pass image

  initRGBAPixel2DArray(tonedImage, width, height);

  int contrastThresh;
  cout<<"Enter contrast threshold: "<<endl;
  cin>>contrastThresh;

  g = log(contrastThresh) / (BMax - BMin);

  for(int row = 0; row < height; ++row) {
    for(int col = 0; col < width; ++col) {
     
      //Step 3: log(Ld) = gamma * B + S
      //Step 4: Ld = exp(log(Ld))
      lD = exp(g * B[row][col] + S[row][col]);

      //Step 5: Cd = (Ld/Lw) * C
      tonedImage[row][col].r = lD/lW * image[row][col].r;
      tonedImage[row][col].g = lD/lW * image[row][col].g;
      tonedImage[row][col].b = lD/lW * image[row][col].b;
      tonedImage[row][col].a = lD/lW * image[row][col].a;


    }
  }
}


void handleKey(unsigned char key, int x, int y) {
  switch(key) {
    case 'w':
    case 'W':
      writeImage(outputImage);
      break;
    case 's':
    case 'S':
      switchToTonedImage = !switchToTonedImage;
      if(switchToTonedImage) 
        glDrawPixels(width, height, GL_RGBA, GL_FLOAT, tonedImage[0]);
      else
        glDrawPixels(width, height, GL_RGBA, GL_FLOAT, image[0]);
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
  cout<<"./tonemap input.img -g gamma_value [output.img] OR"<<endl;
  cout<<"./tonemap input.img [-c or -b] [output.img]"<<endl;
}

int main(int argc , char ** argv) {
  if(argc < 3) {
    displayError();
    return -1;
  }

  inputImage = argv[1];
  readImage(inputImage);

  if(argc == 3) {
    string s1 = argv[2];
    if(s1 != "-g" and s1 != "-c" and s1 != "-b") {
      displayError();
      return -1;    
    }
    else if(s1 == "-g") {
      displayError();
      return -1;    
    }
    else if(s1 == "-c") {
      switchToBilateralFilter = false;
      convolutedToneMap();
    }
    else if(s1 == "-b") {
      switchToBilateralFilter = true;
      convolutedToneMap();
    }
  }
  else if(argc == 4) {
    string s2 = argv[2];
    if(s2 != "-g" and s2 != "-c" and s2 != "-b") {
      displayError();
      return -1;    
    }
    else if(s2 == "-g") { 
      g = atof(argv[3]);
      simpleToneMap();
    }
    else if(s2 == "-c") {
      outputImage = argv[3];
      switchToBilateralFilter = false;
      convolutedToneMap();      
    } 
    else if(s2 == "-b") {
      outputImage = argv[3];
      switchToBilateralFilter = true;
      convolutedToneMap();      
    } 
  }
  else if(argc == 5) {
    string s3 = argv[2];
    if(s3 != "-g") {
      displayError();
      return -1;    
    }
    else {
      g = atof(argv[3]);
      outputImage = argv[4];
      simpleToneMap();
    }
  }
  else {
    displayError();
    return -1;    
  }

  if(outputImage) 
    cout<<"Press W to write the displayed image."<<endl; 

  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
  glutInitWindowSize(width, height);
  glutCreateWindow("Tone Mapping");

  glClearColor(0.0, 0.0, 0.0, 0.0);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluOrtho2D(0, width, 0, height);

  glutDisplayFunc(displayImage);	 
  glutKeyboardFunc(handleKey);	 

  glutMainLoop();
 
  return 0;
}
