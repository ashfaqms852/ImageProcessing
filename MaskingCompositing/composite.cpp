/*Author: Ashfaq Sherwa
  E-mail: asherwa@clemson.edu
  Programming Assignment # 3 for CpSc 6040
  Submitted On: 09/29/2015

  Implementation of composite.cpp

  This program:-
  ->reads, writes, displays image using OIIO and OpenGL concepts.
  ->handles input from keyboard.
  ->composites an image supporting 4 channels over an image supporting 3 channels.
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

static rgbaPixel **images[2], **compositeImage;
static vector<float> pixels;
int width[2], height[2], channels[2], bg, fg;
const char * outputImage;


// reads the image
void readImage(const char * filename , int index) {

  ImageInput * img = ImageInput::open(filename);
  if (!img) {
    std::cout<<std::endl<<"Something is wrong with this file."<<std::endl;
    exit(-1);
  }

  ImageSpec imageSpec;

  img->open(filename , imageSpec);

  width[index] = imageSpec.width;
  height[index] = imageSpec.height;
  channels[index] = imageSpec.nchannels;

  images[index] = new rgbaPixel * [height[index]];
  images[index][0] = new rgbaPixel [width[index]*height[index]];

  for(int i = 1; i < height[index]; ++i) 
    images[index][i] = images[index][i-1] + width[index];

  pixels.resize(width[index]*height[index]*channels[index]);

  if(!img->read_image(TypeDesc::FLOAT , &pixels[0])) {
    cout<<"Read error. Could not read from "<< filename <<endl;
    cout<< img->geterror() <<endl;
    delete img;
    exit(-1);
  }
  
  for(int row = 0; row < height[index]; ++row) {

    for(int col = 0; col < width[index]; ++col) {

      images[index][height[index]-row-1][col].r = pixels[(row*width[index]+col)*channels[index] + 0];

      images[index][height[index]-row-1][col].g = pixels[(row*width[index]+col)*channels[index] + 1];

      images[index][height[index]-row-1][col].b = pixels[(row*width[index]+col)*channels[index] + 2];

      if(channels[index] == 3)
        images[index][height[index]-row-1][col].a = 1.0;
      else
        images[index][height[index]-row-1][col].a = pixels[(row*width[index]+col)*channels[index] + 3];

    }
  }
  img->close();
  delete img;

}


// performs A over B operation
void performComposting() {
  
  float alpha;
  
  int startX = 0, startY = 0; 

  compositeImage = new rgbaPixel * [height[bg]];
  compositeImage[0] = new rgbaPixel [width[bg]*height[bg]];

  for(int i = 1; i < height[bg]; ++i)
    compositeImage[i] = compositeImage[i-1] + width[bg];

  for(int row = 0; (startY+row) < height[bg] && row < height[fg]; ++row) {
    for(int col = 0; (startX+col) < width[bg] && col < width[fg] ; ++col) {

      alpha = images[fg][row][col].a;

      // to make adjustments and avoid segmentation default if height and width of both images are different.
      int x = startX + col;
      int y = startY + row;

      compositeImage[row][col].r = alpha*images[fg][row][col].r + ((1-alpha)*images[bg][y][x].r);

      compositeImage[row][col].g = alpha*images[fg][row][col].g + ((1-alpha)*images[bg][y][x].g);

      compositeImage[row][col].b = alpha*images[fg][row][col].b + ((1-alpha)*images[bg][y][x].b);

      compositeImage[row][col].a = alpha*images[fg][row][col].a + ((1-alpha)*images[bg][y][x].a); 

    }
  }

}


// writes the image when user presses W/w
void writeImage() {
  
  pixels.resize(width[bg]*height[bg]*4);

  // flipping the image for writing
  int itr = 0;
  for( int i = height[bg]-1; i >= 0 ; i--) {
    for(int j = 0; j < width[bg] ; j++) {
      pixels[itr++] = compositeImage[i][j].r;
      pixels[itr++] = compositeImage[i][j].g;
      pixels[itr++] = compositeImage[i][j].b;
      pixels[itr++] = compositeImage[i][j].a;
    }
  }

  ImageOutput *out = ImageOutput::create(outputImage);

  ImageSpec spec (width[bg], height[bg], 4, TypeDesc::FLOAT);

  out->open(outputImage , spec);
  if(out->write_image(TypeDesc::FLOAT, &pixels[0]))
    cout<<"Image written successfully."<<endl;
  else 
    cout<<"Something went wrong while writing the image. "<<geterror()<<endl;
  out->close();

}

void displayImage() {
  glClear(GL_COLOR_BUFFER_BIT);
  glViewport(0, 0, width[bg], height[bg]);
  glRasterPos2i(0,0);
  glDrawPixels(width[bg], height[bg], GL_RGBA,GL_FLOAT, compositeImage[0]);
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

int main(int argc , char ** argv) {

  if(argc < 3) {
    cout<<"You need to specify atleast two valid file names for composting."<<endl;
    return -1;
  }
  

  // store first image in images[0] and second image in images[1]
  readImage(argv[1] , 0);
  readImage(argv[2] , 1);

  // deciding background and foreground images depending on number of channels
  if(channels[0] == 3 and channels[1] == 4) {
    // set images[0] as background and images[1] as foreground
    bg = 0;
    fg = 1;
    performComposting(); 
  }
  else if(channels[1] == 3 and channels[0] == 4) {
    // set images[1] as background and images[0] as foreground
    bg = 1;
    fg = 0;
    performComposting(); 
  }
  else {
    cout<<"You need to specify one image with three channels and one with four channels."<<endl;
    return -1;
  }

  if(argc == 4) {
    cout<<"Press W to write the composite image."<<endl;
    outputImage = argv[3];
  }

  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_SINGLE | GLUT_RGBA);
  glutInitWindowSize(width[bg], height[bg]);
  glutCreateWindow("Composite Image Viewer");

  glutDisplayFunc(displayImage);	 
  glutKeyboardFunc(handleKey);	 

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluOrtho2D(0, width[bg], 0, height[bg]);
  glClearColor(1,1,1,1);

  glutMainLoop();

 
  return 0;
}
