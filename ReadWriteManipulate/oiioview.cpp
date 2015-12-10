/* Author: Ashfaq Sherwa
   E-mail: asherwa@clemson.edu
   Programming Assignment # 1 for CpSc 6040
   Submitted On: 09/08/2015

   Implementation of overview.h

   This program reads, writes, displays and manipulates image using OIIO and OpenGL concepts.
   It also handles input from keyboard and mouse.
*/

#include "oiioview.h"

Image image;

Image::Image() : 
  pixels(),
  w(0),
  h(0),
  channels(0),
  writeImage(false),
  outputFileName("")
{}

Image::~Image() {
  delete [] pixels;
}


bool fileExists(const std::string& filename) {

/*Checks whether the filename specified by user exists.
It takes name of the file to be checked as input and return a bool value.*/

  std::ifstream ifile(filename.c_str());
  return ifile;
}


int width;
int height;
std::vector<std::string> filenames; //Stores all the filenames specified by user at commandline.
int currentFile = 0;
std::vector<unsigned char> copiedPixels;//Stores the manipulated image.



void Image::manipulateImage(char c) {

/*Does the horizontal and vertical flip on the image.
Also diplays only red, green or blue channel in the image as single channel.
It takes a char as an input to check which kind of manipulation is to be performed on the image.
*/

  if(c != 'f' or c!= 'i') {
    copiedPixels.clear();
    if(readImage(filenames[currentFile])) {
      width = getWidth();
      height = getHeight();
      glutReshapeWindow(width , height);
    }
  } 
 
  int size = width*height*4;
  copiedPixels.reserve(size);

  for (int i = 0; i<size; ++i) copiedPixels.push_back(pixels[i]);

  switch(c) {
    case 'f':
    {
      //horizontally flipping the image.
      int itr = 0, offset = 0;
      int temp = (width*4) - 1;
      while (temp <= (height*width*4)) {
        offset = temp;
        for (int i = 0; i<width; i++ ) {
          for (int j = 0; j<4; j++) {
            copiedPixels.at(itr++) = pixels[offset-3+j]; 
          }
          offset = offset-4;
        }    
        temp = temp + (width*4);  
      } 
      break;  
    }

    case 'i':
      {
        //vertically flipping the image.
        int itr = 0;
        int temp = size - (width*4);
        while(temp >=0) {
          for(int j = 0; j<(width*4); ++j) copiedPixels.at(itr++) = pixels[j+temp];
          temp = temp - (width*4);
        }
        break;
      }

    case 'r':
      //displaying only red channel of the image.
      for(int i = 0; i < size-4; i=i+4) {
        //setting g and b channel to zero.
        copiedPixels.at(i+1) = 0;
        copiedPixels.at(i+2) = 0;
      }
      break;

    case 'g':
      //displaying only green channel of the image.
      for(int i = 0; i < size-4; i=i+4) {
        //setting r and b channel to zero.
        copiedPixels.at(i) = 0;
        copiedPixels.at(i+2) = 0;
      }
      break;

    case 'b':
      //displaying only blue channel of the image.
      for(int i = 0; i < size-4; i=i+4) {
        //setting r and g channel to zero.
        copiedPixels.at(i) = 0;
        copiedPixels.at(i+1) = 0;
      }
      break;
  }
   
  delete [] pixels;
  pixels = new unsigned char[size];

  for (int k = 0; k<size; ++k) pixels[k] = copiedPixels.at(k);

  glutPostRedisplay();
  
}



bool Image::readImage(std::string filename) {

/* Reads the image specified by user as second parameter on command line.
   It takes the name of the file as input and returns bool as output. */

  ImageInput *image = ImageInput::open (filename);
  if (!image) {
    std::cout<<std::endl<<"Something is wrong with this file."<<std::endl;
    std::cout<<std::endl<<OpenImageIO::geterror()<<std::endl;
    return false;
  }

  const ImageSpec &imageSpec = image->spec();
  w = imageSpec.width;
  h = imageSpec.height;
  channels = imageSpec.nchannels;
  pixels = new unsigned char[w*h*channels];

  image->read_image(TypeDesc::UINT8, pixels);


  if(channels == 3) {
    //if the image has only three channels then add an alpha channel
    unsigned char* tempPixels = new unsigned char[w*h*4];
    int j = 0;
    for(int i = 0; i < (w*h*4 - 4); i=i+4) {
      tempPixels[i] = pixels[j];
      tempPixels[i+1] = pixels[j+1];
      tempPixels[i+2] = pixels[j+2];
      tempPixels[i+3] = 255;
      j = j+3;
  }
    delete [] pixels;
    pixels = tempPixels;
  }

  image->close();
  delete image;
  
  return true;
}



bool Image::writeImg() {

// This function writes an image and returns a bool value depending on whether the image was successfully written or not.

  if(outputFileName == "") {
    std::cout<<"New filename not provided"<<std::endl;
    return false;
  }

  char * opFileName = &outputFileName[0];

  std::vector<unsigned char> flippedPixels(image.getWidth()*image.getHeight()*image.getChannels());
  std::vector<unsigned char> tempPixels(image.getWidth()*image.getHeight()*image.getChannels());

  if (image.getChannels() == 3)
    //if the image format is .ppm
    glReadPixels(0, 0, image.getWidth(), image.getHeight(), GL_RGB, GL_UNSIGNED_BYTE , &tempPixels[0]);
  else
    glReadPixels(0, 0, image.getWidth(), image.getHeight(), GL_RGBA, GL_UNSIGNED_BYTE , &tempPixels[0]);
  
  // Flipping the image to write
  int i = 0;
  int temp = (image.getWidth()*image.getHeight()*image.getChannels()) - (image.getWidth()*image.getChannels());

  while(temp >= 0) {

    for(int j = 0; j < (image.getWidth()*image.getChannels()); ++j) {
      flippedPixels.at(i++) =tempPixels.at(j+temp);
    }

    temp = temp - (image.getWidth()*image.getChannels());
  }

  ImageOutput * out = ImageOutput::create(opFileName);
  if (!out) {
    std::cerr << "Could not create: " << geterror();
    return false;
  }

  ImageSpec spec(w, h, image.getChannels(), TypeDesc::UINT8);
  
  out->open(opFileName , spec);
  out->write_image(TypeDesc::UINT8, &flippedPixels[0]);
  out->close();
  delete out;

  return true;  
}



void displayImage(){
  
  // Displays the image current stored in pixels[]

  int viewportWidth = glutGet(GLUT_WINDOW_WIDTH);
  int viewportHeight = glutGet(GLUT_WINDOW_HEIGHT);

  glPixelZoom(1, -1);
  glRasterPos2i(0,(image.getHeight()-1));
  glViewport(0,0,viewportWidth,viewportHeight);
  glDrawPixels(image.getWidth(),image.getHeight(),GL_RGBA, GL_UNSIGNED_BYTE, image.getPixelData());

  glutSwapBuffers();
  glFlush();
}



void handleKey(unsigned char key , int x , int y) {

// handles input from keyboard

  switch(key) {

    case 'w':
    case 'W':
      if(image.getWriteImage()) {

        std::cout<<"New file named "<<image.getOutputFileName()<<" would be created."<<std::endl;

        if(image.writeImg()) {
          std::cout<<"Image written successfully."<<std::endl;
        }  
      }
      else {
        std::cout<<"Output file name was not specified"<<std::endl;
      }
      break;

    case 'f':
    case 'F':
      image.manipulateImage('f');
      break;

    case 'i':
    case 'I':
      image.manipulateImage('i');
      break;

    case 'r':
    case 'R':
      image.manipulateImage('r');
      break;

    case 'g':
    case 'G':
      image.manipulateImage('g');
      break;

    case 'b':
    case 'B':
      image.manipulateImage('b');
      break;

    case 'o':
    case 'O':
      if(image.readImage(filenames[currentFile])) {
        width = image.getWidth();
        height = image.getHeight();
        glutReshapeWindow(width , height);
      }
      glutPostRedisplay();
      break;

    case 'q':
    case 'Q':
    case 27:
      exit(0);
  }
}



void leftRightArrowKeys(int key , int x, int y) {

// handles left and write arrow keys to navigate if the user has specified more than one image.

  if(filenames.size() ==1) {
    std::cout<<"You need to specify more than one filename to navigate."<<std::endl;
    return;
  }
  switch(key) {
    case GLUT_KEY_LEFT:
      if(currentFile == 0) currentFile = filenames.size()-1;
      else currentFile--;
      break;
    case GLUT_KEY_RIGHT:
      if(currentFile == filenames.size()-1) currentFile = 0;
      else currentFile++;
      break;
  }
  if(!fileExists(filenames[currentFile])) {
    std::cout<<"This input file does not exists."<<std::endl;
    return;
  }
  if(image.readImage(filenames[currentFile])) {
    width = image.getWidth();
    height= image.getHeight();
    glutReshapeWindow(width , height);
  }
  glutPostRedisplay();
}



void handleMouse(int button, int value, int x, int y) {

//Handles mouse events

  if(button == GLUT_LEFT_BUTTON) {
    unsigned char pix[1*1*4];
    glReadPixels(x, glutGet(GLUT_WINDOW_HEIGHT)-y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, &pix[0]);
    std::cout<<"x co-ordinate: "<<abs(x)<<std::endl;
    std::cout<<"y co-ordinate: "<<abs(y)<<std::endl;
    std::cout<<"red channel: "<<(int)pix[0]<<std::endl;
    std::cout<<"green channel: "<<(int)pix[1]<<std::endl;
    std::cout<<"blue channel: "<<(int)pix[2]<<std::endl;
    std::cout<<"alpha channel: "<<(int)pix[3]<<std::endl;
  }
}



int main(int argc, char** argv) {

  bool writeImage = false;
  
  if(argc==1) {
    std::cout<< "Filename expected after ./oiioview "<<std::endl;
    return -1;
  } else if(argc==2) {
    writeImage = false;
    filenames.push_back(argv[1]);
  } else { 
    writeImage = true;
    for(int i = 1; i <= argc-1; i++) filenames.push_back(argv[i]); 
  }

  std::string inputFile = filenames[currentFile];
  std::string outputFile = "";

  if(writeImage) {
    outputFile = argv[argc-1];
    image.setOutputFileName(outputFile);
    image.setWriteImage(writeImage);
  }

  if(!fileExists(inputFile)) {
    std::cout<<std::endl<<"The input file specified does not exist."<<std::endl;
    return -1;
  }

  if(image.readImage(inputFile)) {

    width = image.getWidth(); 
    height = image.getHeight();

    // initialize glut
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_SINGLE | GLUT_RGBA);
    glutInitWindowSize(width, height);
    glutCreateWindow(inputFile.data());

    glutDisplayFunc(displayImage);
    glutKeyboardFunc(handleKey);
    glutMouseFunc(handleMouse);
    glutSpecialFunc(leftRightArrowKeys);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, width, 0, height);
    glClearColor(0, 0, 0, 255);
    glutMainLoop();
  }
  return 0;
}

