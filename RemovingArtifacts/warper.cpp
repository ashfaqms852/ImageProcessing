/*Author: Ashfaq Sherwa
  E-mail: asherwa@clemson.edu
  Programming Assignment # 6 for CpSc 6040
  Submitted On: 11/12/2015

  Implementation of warper.cpp

  This program:-
  ->reads, writes, displays image using OIIO and OpenGL concepts.
  ->handles input from keyboard.
  Depending on the input from user it performs-
  ->rotation
  ->translation
  ->shearing
  ->scaling and
  ->twirling
  This program also repairs the minification and magnification artifacts using
  super-sampling and bilinear interpolation.
*/

#include"OpenImageIO/imageio.h"
OIIO_NAMESPACE_USING

#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include<GL/glut.h> 
#endif

#include "Matrix.h"

#include<iostream> 
#include<string.h>
#include<math.h>

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

static rgbaPixel **image, **transformedImage;
static vector<float> pixels;
int width, height, channels, outputWidth, outputHeight;
const char * inputImage, * outputImage;
bool twirl = false, bilinear = false, jittered = false;
float pX, pY, cX, cY, s;
int centerX, centerY, totalSamples = 16;
double dstX, dstY, r, angle, minDim;

// initializes 2D array of type rgbaPixel
void initRGBAPixel2DArray(rgbaPixel ** &p, int w, int h) {
  p = new rgbaPixel * [h];
  p[0] = new rgbaPixel[w*h];
  
  for(int i = 1; i < h; ++i)
    p[i] = p[i-1] + w;
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

  ImageSpec spec(outputWidth, outputHeight, channels, TypeDesc::FLOAT);
  
  out->open(filename , spec);
  pixels.resize(outputWidth*outputHeight*channels);
  int iterator = 0;

  for(int row = outputHeight-1; row >= 0; --row) {
    for(int col = 0; col < outputWidth; ++ col) {
      pixels[iterator++] = transformedImage[row][col].r;
      pixels[iterator++] = transformedImage[row][col].g;
      pixels[iterator++] = transformedImage[row][col].b;
      if(channels == 4)
        pixels[iterator++] = transformedImage[row][col].a;
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
  glViewport(0, 0, outputWidth, outputHeight);
  glDrawPixels(outputWidth, outputHeight, GL_RGBA,GL_FLOAT, transformedImage[0]);
  glutSwapBuffers();
  glFlush();
}


void toLowerCase(char * c) {

  if(c != NULL) {
    for(int i = 0; c[i] != '\0'; ++i) {
      if(c[i] >= 'A' && c[i] <= 'Z')
        c[i] += ('a' - 'A');
    }
  }
}


void copyResultToM(Matrix3x3 &M, Matrix3x3 &result) {
 
 for(int row = 0; row < 3; ++row) {
    for(int col = 0; col < 3; ++col) {
      M[row][col] = result[row][col];
    }
  }

}

// performs rotation of image using theta provided by user
void performRotation(Matrix3x3 &M , float theta) {
  
  Matrix3x3 rotation(1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0);
  double angleInRad, cosTheta, sinTheta;

  angleInRad = PI * theta/180.0;
  cosTheta = cos(angleInRad);
  sinTheta = sin(angleInRad);

  // setting up rotation matrix
  rotation[0][0] = cosTheta;
  rotation[0][1] = -sinTheta;
  rotation[1][0] = sinTheta;
  rotation[1][1] = cosTheta;

  Matrix3x3 result = rotation*M;

  copyResultToM(M, result);

}

// performs scaling of image by using sX and sY provided by user
void performScaling(Matrix3x3 &M, float sX, float sY) {

  Matrix3x3 scale(1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0);

  if(sX == 0.0 || sY == 0.0) {
    cout<<"Error: Scale factor cannot be 0."<<endl;
    return;
  }

  // setting up scaling matrix
  scale[0][0] = sX;
  scale[1][1] = sY;
 
  Matrix3x3 result = scale*M;

  copyResultToM(M, result);

}

// performs translation of image by using dX and dY provided by user
void performTranslation(Matrix3x3 &M, float dX, float dY) {

  Matrix3x3 translate(1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0);

  // setting up translation matrix
  translate[0][2] = dX;
  translate[1][2] = dY;
 
  Matrix3x3 result = translate*M;

  copyResultToM(M, result);

}

// performs shearing on image by using hX and hY provided by user
void performShearing(Matrix3x3 &M, float hX, float hY) {

  Matrix3x3 shear(1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0);

  // setting up shearing matrix
  shear[0][1] = hX;
  shear[1][2] = hY;
 
  Matrix3x3 result = shear*M;

  copyResultToM(M, result);

}

// performs perspective warp on image using pX and pY provided by user
void performPerspective(Matrix3x3 &M, float pX, float pY) {

  Matrix3x3 perspective(1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0);

  // setting up perspective matrix
  perspective[2][0] = pX;
  perspective[2][1] = pY;

  Matrix3x3 result = perspective*M;

  copyResultToM(M, result);
}

// (x,y) <=> (u,v)
void mapPixelsToTransformedImage(float &u, float &v, int &row, int &col) {
  if(u >= 0 && u < width && v >= 0 && v < height) {
    if(round(v) > height-1) v = height-1;
    else v = round(v);
    if(round(u) > width-1) u = width-1;
    else u = round(u);
    transformedImage[row][col] = image[(int)v][(int)u];
  }
  else {
    transformedImage[row][col].r = 0.0;
    transformedImage[row][col].g = 0.0;
    transformedImage[row][col].b = 0.0;
  }
}

// removes magnification artifacts using bilinear interpolation
void removeMagnificationArtifacts(float &u, float &v, int &row, int &col) { 
  int j,k;
  float a, b;

  j = floor(u);
  k = floor(v);

  // using horizontal and vertical offsets in u and v space 
  a = u-j;
  b = v-k;

  Vector2d v1(1-b, b);
  Vector2d v2(1-a, a);

  if(j >=0 && j < width-1 && k >= 0 && k < height-1) {
    for(int i = 0; i < 3; ++i) {

      // taking four nearest neighbors 
      Matrix2x2 fourNeighbors(image[k][j][i], image[k][j+1][i], image[k+1][j][i], image[k+1][j+1][i]);
      transformedImage[row][col][i] = v1*fourNeighbors*v2;
    }
  }
  else {
    transformedImage[row][col].r = 0.0;
    transformedImage[row][col].g = 0.0;
    transformedImage[row][col].b = 0.0;
  }
}

void performAffineTransform(Matrix3x3 &M) {

  // four corners of the original image
  Vector3d inputLeftBottom(0.0, 0.0, 1.0);
  Vector3d inputLeftTop(0.0, float(height)-1, 1.0);
  Vector3d inputRightBottom(float(width)-1, 0.0, 1.0);
  Vector3d inputRightTop(float(width)-1, float(height)-1.0, 1.0);

  // mapping four corners of transformed image
  Vector3d outputLeftBottom = M * inputLeftBottom;
  Vector3d outputLeftTop = M * inputLeftTop;
  Vector3d outputRightBottom = M * inputRightBottom;
  Vector3d outputRightTop = M * inputRightTop;

  // normalizing w component
  if(outputLeftBottom.z != 1) {
    outputLeftBottom.x /= outputLeftBottom.z;
    outputLeftBottom.y /= outputLeftBottom.z; 
  }
  if(outputLeftTop.z != 1) {
    outputLeftTop.x /= outputLeftTop.z;
    outputLeftTop.y /= outputLeftTop.z;
  }
  if(outputRightBottom.z != 1) {
    outputRightBottom.x /= outputRightBottom.z;
    outputRightBottom.y /= outputRightBottom.z;
  }
  if(outputRightTop.z != 1) {
    outputRightTop.x /= outputRightTop.z;
    outputRightTop.y /= outputRightTop.z;
  }

  // calculating width and height of transformed image
  outputWidth = std::max(outputLeftBottom.x ,
                std::max(outputLeftTop.x ,
                std::max(outputRightBottom.x, outputRightTop.x))) -
                std::min(outputLeftBottom.x,
                std::min(outputLeftTop.x ,
                std::min(outputRightBottom.x, outputRightTop.x))); 
  
  outputHeight = std::max(outputLeftBottom.y ,
                 std::max(outputLeftTop.y ,
                 std::max(outputRightBottom.y, outputRightTop.y))) -
                 std::min(outputLeftBottom.y,
                 std::min(outputLeftTop.y,
                 std::min(outputRightBottom.y, outputRightTop.y)));

  initRGBAPixel2DArray(transformedImage, outputWidth, outputHeight);

  // getting x co-ordinate of origin of transformed image
  float x = std::min(outputLeftBottom.x,

            std::min(outputLeftTop.x,
            std::min(outputRightBottom.x, outputRightTop.x)));

  // getting y co-ordinate of origin of transformed image
  float y = std::min(outputLeftBottom.y,
            std::min(outputLeftTop.y,
            std::min(outputRightBottom.y, outputRightTop.y)));

  Vector3d origin(x, y, 0.0);
  Matrix3x3 inverseOfM = M.inv();

  for(int row = 0; row < outputHeight; ++row) {
    for(int col = 0; col < outputWidth; ++col) {
      Vector3d outputPixel(col , row, 1.0);
  
      // account for change in origin in transformation
      outputPixel = outputPixel + origin;

      Vector3d inputPixel = inverseOfM * outputPixel;

      float u = inputPixel[0]/inputPixel[2];
      float v = inputPixel[1]/inputPixel[2];

      if(!bilinear) mapPixelsToTransformedImage(u, v, row, col); // just perform the transformation
      else removeMagnificationArtifacts(u, v, row, col); // perform transformation and remove magnification artificats

    }
  }
}


void removeBothArtifacts(Matrix3x3 &M) {

  // four corners of the original image
  Vector3d inputLeftBottom(0.0, 0.0, 1.0);
  Vector3d inputLeftTop(0.0, float(height)-1, 1.0);
  Vector3d inputRightBottom(float(width)-1, 0.0, 1.0);
  Vector3d inputRightTop(float(width)-1, float(height)-1.0, 1.0);

  // mapping four corners of transformed image
  Vector3d outputLeftBottom = M * inputLeftBottom;
  Vector3d outputLeftTop = M * inputLeftTop;
  Vector3d outputRightBottom = M * inputRightBottom;
  Vector3d outputRightTop = M * inputRightTop;

  // normalizing w component
  if(outputLeftBottom.z != 1) {
    outputLeftBottom.x /= outputLeftBottom.z;
    outputLeftBottom.y /= outputLeftBottom.z; 
  }
  if(outputLeftTop.z != 1) {
    outputLeftTop.x /= outputLeftTop.z;
    outputLeftTop.y /= outputLeftTop.z;
  }
  if(outputRightBottom.z != 1) {
    outputRightBottom.x /= outputRightBottom.z;
    outputRightBottom.y /= outputRightBottom.z;
  }
  if(outputRightTop.z != 1) {
    outputRightTop.x /= outputRightTop.z;
    outputRightTop.y /= outputRightTop.z;
  }

  // calculating width and height of transformed image
  outputWidth = std::max(outputLeftBottom.x ,
                std::max(outputLeftTop.x ,
                std::max(outputRightBottom.x, outputRightTop.x))) -
                std::min(outputLeftBottom.x,
                std::min(outputLeftTop.x ,
                std::min(outputRightBottom.x, outputRightTop.x))); 
  
  outputHeight = std::max(outputLeftBottom.y ,
                 std::max(outputLeftTop.y ,
                 std::max(outputRightBottom.y, outputRightTop.y))) -
                 std::min(outputLeftBottom.y,
                 std::min(outputLeftTop.y,
                 std::min(outputRightBottom.y, outputRightTop.y)));

  initRGBAPixel2DArray(transformedImage, outputWidth, outputHeight);

  // getting x co-ordinate of origin of transformed image
  float x = std::min(outputLeftBottom.x,
            std::min(outputLeftTop.x,
            std::min(outputRightBottom.x, outputRightTop.x)));

  // getting y co-ordinate of origin of transformed image
  float y = std::min(outputLeftBottom.y,
            std::min(outputLeftTop.y,
            std::min(outputRightBottom.y, outputRightTop.y)));

  Vector3d origin(x, y, 0.0);
  Matrix3x3 inverseOfM = M.inv();

  float *vecOfX, *vecOfY, subOfX, subOfY, a, b, rgb[3] = {};
  int cntOfX = 0, cntOfY = 0, j, k;
  bool carryOn = false;

  vecOfX = new float[totalSamples];
  vecOfY = new float[totalSamples];
  for(int row = 0; row < outputHeight; ++row) {
    for(int col = 0; col < outputWidth; ++col) {

      cntOfX = 0;
      cntOfY = 0;
      rgb[0] = 0.0;
      rgb[1] = 0.0;
      rgb[2] = 0.0;

      for(int i = 0; i < totalSamples; ++i) {
        do {
          carryOn = false;
          float ry = ((float)rand()/(RAND_MAX));
          for(int j = 0; j < cntOfY; ++j) {
            if(vecOfY[j] == ry) carryOn = true;
          }
          if(!carryOn) vecOfY[cntOfY++] = ry - 0.5;
        } while(carryOn);
        do {
          carryOn = false;
          float rx = ((float)rand()/(RAND_MAX));
          for(int j = 0; j < cntOfX; ++j) {
            if(vecOfX[j] == rx) carryOn = true;
          }
          if(!carryOn) vecOfX[cntOfX++] = rx - 0.5;
        } while(carryOn);
      }

      for(int i = 0; i < totalSamples; ++i) {
        subOfY = row - vecOfY[i];
        subOfX = col - vecOfX[i];

        Vector3d outputPixel(subOfX , subOfY, 1.0);
  
        // account for change in origin in transformation
        outputPixel = outputPixel + origin;
         
        float u, v;
        if(!twirl) { 
          Vector3d inputPixel = inverseOfM * outputPixel;

          u = inputPixel[0]/inputPixel[2];
          v = inputPixel[1]/inputPixel[2];
        }
        else {
          dstX = subOfX - centerX; 
          dstY = subOfY - centerY;
          r = sqrt(dstX*dstX + dstY*dstY);
          angle = atan2(dstY, dstX);
          u = r * cos(angle + s * (r - minDim)/minDim) + centerX;
          v = r * sin(angle + s * (r - minDim)/minDim) + centerY;
        } 
        j = floor(u);
        k = floor(v);
 
        a = u - j;
        b = v - k;

        Vector2d v1(1-b, b);
        Vector2d v2(1-a, a);

        if(j >= 0 && j < width-1 && k >= 0 && k < height-1) {
          for(int m = 0; m < 3; ++m) {
            Matrix2x2 fourNeighbors(image[k][j][m], image[k][j+1][m], image[k+1][j][m], image[k+1][j+1][m]);
            rgb[m] += v1*fourNeighbors*v2;
          }
        }
      }
      transformedImage[row][col].r = rgb[0]/totalSamples;
      transformedImage[row][col].g = rgb[1]/totalSamples;
      transformedImage[row][col].b = rgb[2]/totalSamples; 

    }
  }
}


// performs twirling of image using s, cX and cY provided by user
void performTwirling() {

  // dimensions of transformed image would be same as original image
  Vector3d outputLeftBottom;
  outputLeftBottom = (0.0, 0.0, 1.0);
  Vector3d outputLeftTop(0.0, float(height)-1, 1.0);
  Vector3d outputRightBottom(float(width)-1, 0.0, 1.0);
  Vector3d outputRightTop(float(width)-1, float(height)-1.0, 1.0);

  outputWidth = width;
  outputHeight = height;

  initRGBAPixel2DArray(transformedImage, outputWidth, outputHeight);

  // getting x co-ordinate of origin of transformed image
  float x = std::min(outputLeftBottom.x,
            std::min(outputLeftTop.x,
            std::min(outputRightBottom.x, outputRightTop.x)));

  // getting y co-ordinate of origin of transformed image
  float y = std::min(outputLeftBottom.y,
            std::min(outputLeftTop.y,
            std::min(outputRightBottom.y, outputRightTop.y)));

  Vector3d origin(x, y, 0.0);

  centerX = (int)(cX * outputWidth);
  centerY = (int)(cY * outputHeight);
  minDim = std::min(outputWidth, outputHeight);

  for(int row = 0; row < outputHeight; ++row) {
    for(int col = 0; col < outputWidth; ++col) {
      dstX = col - centerX;
      dstY = row - centerY;

      // calculating distance between pixel and center co-ordinate
      r = sqrt(dstX*dstX + dstY*dstY);

      // calculating angle of rotation between pixel and center co-ordinate
      angle = atan2(dstY , dstX);

      Vector3d outputPixel(col , row, 1.0);

      // account for change in origin in transformation
      outputPixel = outputPixel + origin;

      float u = r * cos(angle + s * (r - minDim)/minDim) + centerX;
      float v = r * sin(angle + s * (r - minDim)/minDim) + centerY;

      if(!bilinear) mapPixelsToTransformedImage(u, v, row, col); // just perform the transformation
      else removeMagnificationArtifacts(u, v, row, col); // perform transformation and remove magnification artificats

    }
  }
}


void displayAffTransfUsage() {
  cout<<"Valid Commands: 'r' -> Rotating, 's' -> Scaling, 't' -> Translating"<<endl;
  cout<<"                'h' -> Shear, 'n' -> Twirl, 'p' -> Perspective and 'd' -> Done."<<endl;
  cout<<"You may use multiple commands except for 'n'."<<endl;
  cout<<"Enter 'd' command to indicate that you are done."<<endl;
}

void processInput(Matrix3x3 &M) {
  char command[1024];
  bool done;
  float theta, sX, sY, dX, dY, hX, hY;
  
  M.identity();

  displayAffTransfUsage();

  for(done = false; !done;) {
    cout<<"> ";
    cin>>command;
    toLowerCase(command);
    
    if(strcmp(command, "d") == 0) done = true;
    else if(strlen(command) != 1) displayAffTransfUsage();
    else {
      switch(command[0]) {
        case 'r':
          cout<<"Rotating: Please enter the theta value: "<<endl;
          cin >> theta;
          performRotation(M , theta);
          break;
        case 's':
          cout<<"Scaling: Please enter the scaling factors sX and sY:"<<endl;
          cin>>sX>>sY;
          performScaling(M, sX, sY);
          break;
        case 't':
          cout<<"Translating: Please enter the translating factors dX and dY:"<<endl;
          cin>>dX>>dY;
          performTranslation(M, dX, dY);
          break;
        case 'h':
          cout<<"Shearing: Please enter the shearing factors hX and hY:"<<endl;
          cin>>hX>>hY;
          performShearing(M, hX, hY);
          break;
        case 'n':
          done = true;
          twirl = true;
          cout<<"Twirl: Please enter the twirl factors strength, cX and cY:"<<endl;
          cin>>s>>cX>>cY;
          performTwirling();
          break;
        case 'p':
          cout<<"Perspective: Please enter the perspective factors pX and pY:"<<endl;
          cin>>pX>>pY;
          performPerspective(M, pX, pY);
          break;
        case 'd':
          done = true;
          break;
        default:
          displayAffTransfUsage();
          break; 
      }
    } 
  }
}

void handleKey(unsigned char key, int x, int y) {
  switch(key) {
    case 'w':
    case 'W':
      writeImage(outputImage);
      break;
    case 'r':
    case 'R':
      {
        jittered = !jittered; // toggle between transformed and repaired image
        Matrix3x3 M1(1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0);
        M1.identity();
        if(jittered) {
          if(!twirl) {
            performPerspective(M1, pX, pY);
            removeBothArtifacts(M1);
          }
          else {
            performTwirling();
            removeBothArtifacts(M1);
          }
        }
        else {
          bilinear = false;
          if(!twirl) {
            performPerspective(M1, pX, pY);
            performAffineTransform(M1);
          }
          else performTwirling();
        }
        glRasterPos2i(0,0);
        glDrawPixels(outputWidth, outputHeight, GL_RGBA, GL_FLOAT, transformedImage[0]);
        glutSwapBuffers();
        break;
      }
    case 'b':
    case 'B':
      {
        // toggle between transformed and repaired image
        bilinear = !bilinear; 
        Matrix3x3 M1(1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0);
        M1.identity();
        if(!twirl) {
          performPerspective(M1, pX, pY);
          performAffineTransform(M1);
        }
        else
        performTwirling();
        glRasterPos2i(0,0);
        glDrawPixels(outputWidth, outputHeight, GL_RGBA, GL_FLOAT, transformedImage[0]);
        glutSwapBuffers();
        break;
      }
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
  cout<<"./warper input.img [output.img]"<<endl;
}

int main(int argc , char * argv[]) {

  if(argc < 2) {
    displayError();
    return -1;
  }
  
  inputImage = argv[1];
  readImage(inputImage);

  if(argc == 3) {
    outputImage = argv[2];
    cout<<"Press W to write the displayed image."<<endl;
  }

  Matrix3x3 M(1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0);

  processInput(M);

  if(!twirl) performAffineTransform(M);

  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
  glutInitWindowSize(outputWidth, outputHeight);
  glutCreateWindow("Warping");

  glClearColor(0.0, 0.0, 0.0, 0.0);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluOrtho2D(0, outputWidth, 0, outputHeight);

  glutDisplayFunc(displayImage);	 
  glutKeyboardFunc(handleKey);	 

  glutMainLoop();
 
  return 0;

}
