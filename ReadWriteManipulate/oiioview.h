/* Author: Ashfaq Sherwa
   E-mail: asherwa@clemson.edu
   Programming Assignment # 1 for CpSc 6040
   Submitted On: 09/08/2015

   Header file overview.h

   This program reads, writes, displays and manipulates image using OIIO and OpenGL concepts.
   It also handles input from keyboard and mouse.
*/

#include <iostream>
#include <fstream>
#include <cstring>

#include <GL/glut.h>

#include <OpenImageIO/imageio.h>
OIIO_NAMESPACE_USING

class Image {
public:
  Image();
  ~Image();
  
  bool readImage(std::string filename);
  bool writeImg();
  void manipulateImage(char c);

  int getHeight() const { return h; }
  int getWidth() const { return w; }
  int getChannels() const { return channels; }

  int setWriteImage(bool value) { writeImage = value; }
  int getWriteImage() const { return writeImage; }

  unsigned char* getPixelData() { return pixels; }

  void setOutputFileName(std::string filename) { outputFileName = filename.data(); }
  std::string getOutputFileName() { return outputFileName; }

private:
  unsigned char* pixels;
  int w,h,channels;
  bool writeImage;
  std::string outputFileName;
};

