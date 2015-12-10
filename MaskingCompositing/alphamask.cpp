/*Author: Ashfaq Sherwa
  E-mail: asherwa@clemson.edu
  Programming Assignment # 3 for CpSc 6040
  Submitted On: 09/29/2015

  Implementation of alphamask.cpp

  This program:-
  ->reads, writes, displays image using OIIO and OpenGL concepts.
  ->handles input from keyboard.
  ->converts rgb pixel to hsv format.
  ->performs masking of the image using different methods like -
    binary, greyscale, petro vlahos and spill supression. It also
    performs masking depending on h,s and v values.
*/


#include"OpenImageIO/imageio.h"
OIIO_NAMESPACE_USING

#ifdef __APPLE__
#  include <GLUT/glut.h>
#else
#include<GL/glut.h>
#endif

#include<iostream>
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

static rgbaPixel **image, **maskedImage;
static vector<float> pixels;
int width, height, channels;
const char * outputImage;
float k = 1.0; // to be used for petro vlahos algorithm. 

// read the image
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

// performs all kinds of required masking depending on the option selected by user
void performMasking(int choice) {

  maskedImage = new rgbaPixel * [height];
  maskedImage[0] = new rgbaPixel[width*height];

  for(int i = 1; i < height; ++i)
    maskedImage[i] = maskedImage[i-1] + width;

  double h, s, v;
  int itr = 0;

  for(int row = 0; row < height; ++row) {
    for(int col = 0; col < width; ++col) {
      
      RGBtoHSV(image[row][col].r, image[row][col].g, image[row][col].b, h, s, v);

      switch(choice) {
        case 1: {
          // binary alphamasking 
          if(h > 127 and h < 143) maskedImage[row][col].a = 0.0;
          else maskedImage[row][col].a = 1.0;
          break;
        }
        case 2: {
          // greyscale alphamasking
          if(h > 127 and h < 143) maskedImage[row][col].a = 0.0;
          else if (h <= 127 && h > 80) maskedImage[row][col].a = (127 - h)/47;
          else if (h >= 143 && h < 190) maskedImage[row][col].a = (h - 143) / 47;
          else maskedImage[row][col].a = 1.0;
          break;
        }
        case 3:
        case 5: {
          // alphamasking using h,s and v values
          if((h > 89 and h < 151) and (s > 0.01 and s < 1.00001) and (v > 0.01 and v < 1.00001))
            maskedImage[row][col].a = 0.0;
          else maskedImage[row][col].a = 1.0;  
          break;
        }
        case 4:
        case 6: {
          // petro vlahos alphamasking
          if(image[row][col].g > k*image[row][col].b)
            maskedImage[row][col].a = 0.0;
          else maskedImage[row][col].a = 1.0; 
          break;
        }
      }

      // pre-multiplying each color channel
      maskedImage[row][col].r = maskedImage[row][col].a*image[row][col].r;
      
      //supressing green spill if -ss or both -pv and -ss flags are given
      if(choice == 5 or choice == 6) {
        maskedImage[row][col].g = maskedImage[row][col].a*(std::min(image[row][col].r, std::min(image[row][col].g, image[row][col].b)));
      }
      else 
        maskedImage[row][col].g = maskedImage[row][col].a*image[row][col].g;

      maskedImage[row][col].b = maskedImage[row][col].a*image[row][col].b;
 
    }
  }
}

// writes the image when user presses W/w
void writeImage() {
  
  pixels.resize(width*height*4);
 
  // flipping the image for writing
  int itr = 0;
  for( int i = height-1; i >= 0 ; i--) {
    for(int j = 0; j < width ; j++) {
      pixels[itr++] = maskedImage[i][j].r;
      pixels[itr++] = maskedImage[i][j].g;
      pixels[itr++] = maskedImage[i][j].b;
      pixels[itr++] = maskedImage[i][j].a;
    }
  }

  ImageOutput *out = ImageOutput::create(outputImage);

  ImageSpec spec (width, height, 4, TypeDesc::FLOAT);

  out->open(outputImage , spec);
  if(out->write_image(TypeDesc::FLOAT, &pixels[0]))
    cout<<"Image written successfully."<<endl;
  else 
    cout<<"Something went wrong while writing the image. "<<geterror()<<endl;
  out->close();

}

// displays the masked image
void displayImage() {
  glClear(GL_COLOR_BUFFER_BIT);
  glViewport(0, 0, width, height);
  glRasterPos2i(0,0);
  glDrawPixels(width, height, GL_RGBA,GL_FLOAT, maskedImage[0]);
  glFlush();
}

void handleKey(unsigned char key, int x, int y) {
  switch(key) {
    case 'w':
    case 'W':
      if(outputImage) writeImage();
      else cout<<"Output file not specified."<<endl;
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

void upDownArrow(int key, int x, int y) {
  switch(key) {
    case GLUT_KEY_UP : {
      k += 0.05;
      performMasking(4); // alphamasking with petro vlahos algorithm
      glutPostRedisplay();
      break;
    }
    case GLUT_KEY_DOWN : {
      k -= 0.05;
      performMasking(4); // alphamasking with petro vlahos algorithm
      glutPostRedisplay();
      break;
    }
  }
}

int main(int argc , char ** argv) {
  int option = 0;
  if(argc < 2) {
    cout<<"You need to specify atleast one valid filename for alphamasking."<<endl;
    return -1;  
  }


// decide which alphamasking method to be use depending on the parameters passed by user from command-line.
  if(argc == 2) {
    // no -pv or -ss flag given by user
    cout<<"Press 1 for binary alphamasking."<<endl; 
    cout<<"Press 2 for greyscale alphamasking."<<endl;
    cout<<"Press 3 for alphamasking using h,s and v values."<<endl;
    cin>>option;
    readImage(argv[1]);
  }
  else if(argc == 3) {
    string s1 = argv[1];
    if(s1 == "-pv") { // only petro vlahos and no writing
      option = 4;
      readImage(argv[2]);
    }
    else if(s1 == "-ss") { // only spill supression and no writing
      option = 5;
      readImage(argv[2]);
    }
    else { // binary/greyscale/hsv masking and writing
      cout<<"Press 1 for binary alphamasking."<<endl; 
      cout<<"Press 2 for greyscale alphamasking."<<endl;
      cout<<"Press 3 for alphamasking using h,s and v values."<<endl;
      cin>>option;
      readImage(argv[1]);
      outputImage = argv[2];
      cout<<"Press W to write the alphamasked image."<<endl;
    } 
  }
  else if(argc == 4) {
    string s2 = argv[1];
    string s3 = argv[2];
    if(s2 == "-pv" and s3 == "-ss") { // both petro vlahos and spill supression with no writing
      option = 6;
      readImage(argv[3]);
    }
    else if(s2 == "-pv" and s3 != "-ss") { // only petro vlahos and writing
      option = 4;
      readImage(argv[2]);
      outputImage = argv[3];
      cout<<"Press W to write the alphamasked image."<<endl;
    }
    else if(s2 == "-ss" and s3 != "-pv") { // only spill supression and writing
      option = 5;
      readImage(argv[2]);
      outputImage = argv[3];
      cout<<"Press W to write the alphamasked image."<<endl;
    }   
  }
  else if(argc == 5) { // both petro vlahos and spill supression along with writing
    option = 6;
    readImage(argv[3]);
    outputImage = argv[4];
    cout<<"Press W to write the alphamasked image."<<endl;
  }

  performMasking(option);

  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_SINGLE | GLUT_RGBA);
  glutInitWindowSize(width, height);
  glutCreateWindow("Alphamasked Image Viewer");

  glutDisplayFunc(displayImage);	 
  glutKeyboardFunc(handleKey);
  glutSpecialFunc(upDownArrow);	 

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluOrtho2D(0, width, 0, height);
  glClearColor(1,1,1,1);

  glutMainLoop();

  return 0;
};
        
