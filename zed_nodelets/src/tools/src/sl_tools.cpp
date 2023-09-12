///////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2023, STEREOLABS.
//
// All rights reserved.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
///////////////////////////////////////////////////////////////////////////

#include <sensor_msgs/image_encodings.h>
#include <sys/stat.h>

#include <boost/make_shared.hpp>
#include <experimental/filesystem>  // for std::experimental::filesystem::absolute
#include <sstream>
#include <vector>

#include "sl_tools.h"

namespace sl_tools
{
bool file_exist(const std::string& name)
{
  struct stat buffer;
  return (stat(name.c_str(), &buffer) == 0);
}

namespace fs = std::experimental::filesystem;
std::string resolveFilePath(std::string file_path)
{
  if(file_path.empty())
  {
    return file_path;
  }


  std::string abs_path = file_path;
  if (file_path[0] == '~')
  {
    std::string home = getenv("HOME");
    file_path.erase(0, 1);
    abs_path = home + file_path;
  }
  else if (file_path[0] == '.')
  {
    if (file_path[1] == '.' && file_path[2] == '/')
    {
      file_path.erase(0, 2);
      fs::path current_path = fs::current_path();
      fs::path parent_path = current_path.parent_path();
      abs_path = parent_path.string() + file_path;
    }
    else if (file_path[1] == '/')
    {
      file_path.erase(0, 1);
      fs::path current_path = fs::current_path();
      abs_path = current_path.string() + file_path;
    }
    else
    {
      std::cerr << "[sl_tools::resolveFilePath] Invalid file path '" << file_path << "' replaced with null string."
                << std::endl;
      return std::string();
    }
  }
  else if(file_path[0] != '/')
  {
    fs::path current_path = fs::current_path();
    abs_path = current_path.string() + "/" + file_path;
  }

  return abs_path;
}

std::string getSDKVersion(int& major, int& minor, int& sub_minor)
{
  std::string ver = sl::Camera::getSDKVersion().c_str();
  std::vector<std::string> strings;
  std::istringstream f(ver);
  std::string s;

  while (getline(f, s, '.'))
  {
    strings.push_back(s);
  }

  major = 0;
  minor = 0;
  sub_minor = 0;

  switch (strings.size())
  {
    case 3:
      sub_minor = std::stoi(strings[2]);

    case 2:
      minor = std::stoi(strings[1]);

    case 1:
      major = std::stoi(strings[0]);
  }

  return ver;
}

ros::Time slTime2Ros(sl::Timestamp t)
{
  uint32_t sec = static_cast<uint32_t>(t.getNanoseconds() / 1000000000);
  uint32_t nsec = static_cast<uint32_t>(t.getNanoseconds() % 1000000000);
  return ros::Time(sec, nsec);
}

bool isZED(sl::MODEL camModel)
{
  if (camModel == sl::MODEL::ZED) {
    return true;
  }
  return false;
}

bool isZEDM(sl::MODEL camModel)
{
  if (camModel == sl::MODEL::ZED_M) {
    return true;
  }
  return false;
}

bool isZED2OrZED2i(sl::MODEL camModel)
{
  if (camModel == sl::MODEL::ZED2) {
    return true;
  }
  if (camModel == sl::MODEL::ZED2i) {
    return true;
  }
  return false;
}

bool isZEDX(sl::MODEL camModel)
{
  if (camModel == sl::MODEL::ZED_X) {
    return true;
  }
  if (camModel == sl::MODEL::ZED_XM) {
    return true;
  }
  return false;
}

void imageToROSmsg(sensor_msgs::ImagePtr imgMsgPtr, sl::Mat img, std::string frameId, ros::Time t)
{
  if (!imgMsgPtr)
  {
    return;
  }

  imgMsgPtr->header.stamp = t;
  imgMsgPtr->header.frame_id = frameId;
  imgMsgPtr->height = img.getHeight();
  imgMsgPtr->width = img.getWidth();

  int num = 1;  // for endianness detection
  imgMsgPtr->is_bigendian = !(*(char*)&num == 1);

  imgMsgPtr->step = img.getStepBytes();

  size_t size = imgMsgPtr->step * imgMsgPtr->height;
  imgMsgPtr->data.resize(size);

  sl::MAT_TYPE dataType = img.getDataType();

  switch (dataType)
  {
    case sl::MAT_TYPE::F32_C1: /**< float 1 channel.*/
      imgMsgPtr->encoding = sensor_msgs::image_encodings::TYPE_32FC1;
      memcpy((char*)(&imgMsgPtr->data[0]), img.getPtr<sl::float1>(), size);
      break;

    case sl::MAT_TYPE::F32_C2: /**< float 2 channels.*/
      imgMsgPtr->encoding = sensor_msgs::image_encodings::TYPE_32FC2;
      memcpy((char*)(&imgMsgPtr->data[0]), img.getPtr<sl::float2>(), size);
      break;

    case sl::MAT_TYPE::F32_C3: /**< float 3 channels.*/
      imgMsgPtr->encoding = sensor_msgs::image_encodings::TYPE_32FC3;
      memcpy((char*)(&imgMsgPtr->data[0]), img.getPtr<sl::float3>(), size);
      break;

    case sl::MAT_TYPE::F32_C4: /**< float 4 channels.*/
      imgMsgPtr->encoding = sensor_msgs::image_encodings::TYPE_32FC4;
      memcpy((char*)(&imgMsgPtr->data[0]), img.getPtr<sl::float4>(), size);
      break;

    case sl::MAT_TYPE::U8_C1: /**< unsigned char 1 channel.*/
      imgMsgPtr->encoding = sensor_msgs::image_encodings::MONO8;
      memcpy((char*)(&imgMsgPtr->data[0]), img.getPtr<sl::uchar1>(), size);
      break;

    case sl::MAT_TYPE::U8_C2: /**< unsigned char 2 channels.*/
      imgMsgPtr->encoding = sensor_msgs::image_encodings::TYPE_8UC2;
      memcpy((char*)(&imgMsgPtr->data[0]), img.getPtr<sl::uchar2>(), size);
      break;

    case sl::MAT_TYPE::U8_C3: /**< unsigned char 3 channels.*/
      imgMsgPtr->encoding = sensor_msgs::image_encodings::BGR8;
      memcpy((char*)(&imgMsgPtr->data[0]), img.getPtr<sl::uchar3>(), size);
      break;

    case sl::MAT_TYPE::U8_C4: /**< unsigned char 4 channels.*/
      imgMsgPtr->encoding = sensor_msgs::image_encodings::BGRA8;
      memcpy((char*)(&imgMsgPtr->data[0]), img.getPtr<sl::uchar4>(), size);
      break;

    case sl::MAT_TYPE::U16_C1: /**< unsigned short 1 channel.*/
      imgMsgPtr->encoding = sensor_msgs::image_encodings::TYPE_16UC1;
      memcpy((uint16_t*)(&imgMsgPtr->data[0]), img.getPtr<sl::ushort1>(), size);
      break;
  }
}

void imagesToROSmsg(sensor_msgs::ImagePtr imgMsgPtr, sl::Mat left, sl::Mat right, std::string frameId, ros::Time t)
{
  if (left.getWidth() != right.getWidth() || left.getHeight() != right.getHeight() ||
      left.getChannels() != right.getChannels() || left.getDataType() != right.getDataType())
  {
    return;
  }

  if (!imgMsgPtr)
  {
    imgMsgPtr = boost::make_shared<sensor_msgs::Image>();
  }

  imgMsgPtr->header.stamp = t;
  imgMsgPtr->header.frame_id = frameId;
  imgMsgPtr->height = left.getHeight();
  imgMsgPtr->width = 2 * left.getWidth();

  int num = 1;  // for endianness detection
  imgMsgPtr->is_bigendian = !(*(char*)&num == 1);

  imgMsgPtr->step = 2 * left.getStepBytes();

  size_t size = imgMsgPtr->step * imgMsgPtr->height;
  imgMsgPtr->data.resize(size);

  sl::MAT_TYPE dataType = left.getDataType();

  int dataSize = 0;
  char* srcL;
  char* srcR;

  switch (dataType)
  {
    case sl::MAT_TYPE::F32_C1: /**< float 1 channel.*/
      imgMsgPtr->encoding = sensor_msgs::image_encodings::TYPE_32FC1;
      dataSize = sizeof(float);
      srcL = (char*)left.getPtr<sl::float1>();
      srcR = (char*)right.getPtr<sl::float1>();
      break;

    case sl::MAT_TYPE::F32_C2: /**< float 2 channels.*/
      imgMsgPtr->encoding = sensor_msgs::image_encodings::TYPE_32FC2;
      dataSize = 2 * sizeof(float);
      srcL = (char*)left.getPtr<sl::float2>();
      srcR = (char*)right.getPtr<sl::float2>();
      break;

    case sl::MAT_TYPE::F32_C3: /**< float 3 channels.*/
      imgMsgPtr->encoding = sensor_msgs::image_encodings::TYPE_32FC3;
      dataSize = 3 * sizeof(float);
      srcL = (char*)left.getPtr<sl::float3>();
      srcR = (char*)right.getPtr<sl::float3>();
      break;

    case sl::MAT_TYPE::F32_C4: /**< float 4 channels.*/
      imgMsgPtr->encoding = sensor_msgs::image_encodings::TYPE_32FC4;
      dataSize = 4 * sizeof(float);
      srcL = (char*)left.getPtr<sl::float4>();
      srcR = (char*)right.getPtr<sl::float4>();
      break;

    case sl::MAT_TYPE::U8_C1: /**< unsigned char 1 channel.*/
      imgMsgPtr->encoding = sensor_msgs::image_encodings::MONO8;
      dataSize = sizeof(char);
      srcL = (char*)left.getPtr<sl::uchar1>();
      srcR = (char*)right.getPtr<sl::uchar1>();
      break;

    case sl::MAT_TYPE::U8_C2: /**< unsigned char 2 channels.*/
      imgMsgPtr->encoding = sensor_msgs::image_encodings::TYPE_8UC2;
      dataSize = 2 * sizeof(char);
      srcL = (char*)left.getPtr<sl::uchar2>();
      srcR = (char*)right.getPtr<sl::uchar2>();
      break;

    case sl::MAT_TYPE::U8_C3: /**< unsigned char 3 channels.*/
      imgMsgPtr->encoding = sensor_msgs::image_encodings::BGR8;
      dataSize = 3 * sizeof(char);
      srcL = (char*)left.getPtr<sl::uchar3>();
      srcR = (char*)right.getPtr<sl::uchar3>();
      break;

    case sl::MAT_TYPE::U8_C4: /**< unsigned char 4 channels.*/
      imgMsgPtr->encoding = sensor_msgs::image_encodings::BGRA8;
      dataSize = 4 * sizeof(char);
      srcL = (char*)left.getPtr<sl::uchar4>();
      srcR = (char*)right.getPtr<sl::uchar4>();
      break;
  }

  char* dest = (char*)(&imgMsgPtr->data[0]);

  for (int i = 0; i < left.getHeight(); i++)
  {
    memcpy(dest, srcL, left.getStepBytes());
    dest += left.getStepBytes();
    memcpy(dest, srcR, right.getStepBytes());
    dest += right.getStepBytes();

    srcL += left.getStepBytes();
    srcR += right.getStepBytes();
  }
}

std::vector<std::string> split_string(const std::string& s, char seperator)
{
  std::vector<std::string> output;
  std::string::size_type prev_pos = 0, pos = 0;

  while ((pos = s.find(seperator, pos)) != std::string::npos)
  {
    std::string substring(s.substr(prev_pos, pos - prev_pos));
    output.push_back(substring);
    prev_pos = ++pos;
  }

  output.push_back(s.substr(prev_pos, pos - prev_pos));
  return output;
}

CSmartMean::CSmartMean(int winSize)
{
  mValCount = 0;

  mMeanCorr = 0.0;
  mMean = 0.0;
  mWinSize = winSize;

  mGamma = (static_cast<double>(mWinSize) - 1.) / static_cast<double>(mWinSize);
}

double CSmartMean::addValue(double val)
{
  mValCount++;

  mMeanCorr = mGamma * mMeanCorr + (1. - mGamma) * val;
  mMean = mMeanCorr / (1. - pow(mGamma, mValCount));

  return mMean;
}

}  // namespace sl_tools
