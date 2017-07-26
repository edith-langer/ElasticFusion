/*
 * This file is part of ElasticFusion.
 *
 * Copyright (C) 2015 Imperial College London
 *
 * The use of the code within this file and all code within files that
 * make up the software that is ElasticFusion is permitted for
 * non-commercial purposes only.  The full terms and conditions that
 * apply to the code within this file are detailed within the LICENSE.txt
 * file and at <http://www.imperial.ac.uk/dyson-robotics-lab/downloads/elastic-fusion/elastic-fusion-license/>
 * unless explicitly stated.  By downloading this file you agree to
 * comply with these terms.
 *
 * If you wish to use any of this code for commercial purposes then
 * please email researchcontracts.engineering@imperial.ac.uk.
 *
 */

#include <cstdio>
#include <vector>
#include <fstream>
#include <iostream>

#include "CRC.h"

#include "RawLogReader.h"

#include "JpegCodec.h"
#include "ZlibCodec.h"

namespace
{

struct FrameInfo
{
  double timestamp;
  int32_t exposure;
  uint8_t auto_exposure;
  uint8_t auto_white_balance;
};

struct MetaInfo
{
  std::string camera_model = "";
  std::string serial_number = "";
  cv::Size depth_image_resolution = {0, 0};
  cv::Size color_image_resolution = {0, 0};
  unsigned long depth_image_size = 0;
  unsigned long color_image_size = 0;
  float depth_scale = 0.001f;
  int num_frames = -1;
  bool with_exposure = false;
  bool with_auto_exposure = false;
  bool with_auto_white_balance = false;
  std::streampos data_start_pos;
};

class File
{
public:
  File(const std::string& filename)
  : file_(filename)
  {
    if (!file_.is_open())
      throw "Failed to open file";
  }

  ~File()
  {
    file_.close();
  }

  void
  saveDataStart()
  {
    data_start_pos_ = file_.tellg();
  }

  void
  seekDataStart()
  {
    file_.seekg(data_start_pos_);
  }

  template<typename T> T
  read()
  {
    T v;
    file_.read((char*)&v, sizeof(T));
    return v;
  }

  template<typename T> void
  read(T* ptr, size_t size)
  {
    file_.read((char*)ptr, size);
  }

  std::string
  readLine()
  {
    std::string line;
    std::getline(file_, line);
    return line;
  }

  void
  skip(size_t offset)
  {
    file_.seekg(offset, std::ios_base::cur);
  }

  /** Read bytes from the beginning of the file until the current position and compute CRC32. */
  uint32_t
  computeCRC32()
  {
    auto current = file_.tellg();
    file_.seekg(0, std::ios_base::beg);
    auto size = current - file_.tellg();
    char buffer[size];
    file_.read(buffer, size);
    return crc32(0, buffer, size);
  }

private:
  std::ifstream file_;
  std::streampos data_start_pos_;
};

class FrameReader
{
public:
  virtual ~FrameReader() { }

  virtual void
  read(File& file, uint8_t* color, uint8_t* depth, FrameInfo& frame_info) = 0;

  virtual void
  skip(File& file) = 0;
};

class KLGFrameReader : public FrameReader
{
public:
  KLGFrameReader(MetaInfo& meta)
  {
    depth_buffer_.resize(meta.depth_image_size);
    color_buffer_.resize(meta.color_image_size);
  }

  virtual void
  read(File& file, uint8_t* color, uint8_t* depth, FrameInfo& frame_info) override
  {
    frame_info.timestamp = 0.000001 * file.read<int64_t>();
    auto depth_size = file.read<uint32_t>();
    auto color_size = file.read<uint32_t>();

    bool depth_compressed = (depth_size != depth_buffer_.size());
    uint8_t* depth_ptr = depth_compressed ? depth_buffer_.data() : depth;
    file.read((char*)depth_ptr, depth_size);
    if (depth_compressed)
      ZLIBCodec::decode(depth_buffer_.data(), depth_size, depth, depth_buffer_.size());

    if (color_size > 0)
    {
      bool color_compressed = (color_size != color_buffer_.size());
      uint8_t* color_ptr = color_compressed ? color_buffer_.data() : color;
      file.read((char*)color_ptr, color_size);
      if (color_compressed)
        JPEGCodec::decode(color_buffer_, color);
    }
  }

  virtual void
  skip(File& file) override
  {
    file.skip(8);
    auto depth_size = file.read<int32_t>();
    auto color_size = file.read<int32_t>();
    file.skip(depth_size + color_size);
  }

private:
  std::vector<uint8_t> depth_buffer_;
  std::vector<uint8_t> color_buffer_;
};

class RgbdFrameReader : public FrameReader
{
public:
  RgbdFrameReader(MetaInfo& meta)
  {
    depth_buffer_.resize(meta.depth_image_size);
    color_buffer_.resize(meta.color_image_size);
  }

  virtual void
  read(File& file, uint8_t* color, uint8_t* depth, FrameInfo& frame_info) override
  {
    frame_info.timestamp = 0.000001 * file.read<int64_t>();
    frame_info.exposure = file.read<int32_t>();
    frame_info.auto_exposure = file.read<uint8_t>();
    frame_info.auto_white_balance = file.read<uint8_t>();
    auto color_size = file.read<uint32_t>();
    auto depth_size = file.read<uint32_t>();
    file.read(color_buffer_.data(), color_size);
    JPEGCodec::decode(color_buffer_, color);
    file.read(depth_buffer_.data(), depth_size);
    ZLIBCodec::decode(depth_buffer_.data(), depth_size, depth, depth_buffer_.size());
  }

  virtual void
  skip(File& file) override
  {
    file.skip(8 + 4 + 1 + 1);
    auto depth_size = file.read<int32_t>();
    auto color_size = file.read<int32_t>();
    file.skip(depth_size + color_size);
  }
private:
  std::vector<uint8_t> depth_buffer_;
  std::vector<uint8_t> color_buffer_;
};

}

struct RawLogReader::Impl
{
  MetaInfo meta;
  const std::string filename;
  File file;
  std::unique_ptr<FrameReader> frame_reader;
  int32_t next_frame_index = 0;
  bool repeat = false;
  FrameInfo last_frame;

  Impl(const std::string& fn)
  : filename(fn)
  , file(fn)
  {
    auto ends_with = [](const std::string& s, const std::string& ending)
    {
        return ending.size() > s.size() ? false : std::equal(ending.rbegin(), ending.rend(), s.rbegin());
    };

    if (ends_with(filename, ".klg"))
    {
      readRawLogHeader();
      frame_reader.reset(new KLGFrameReader(meta));
    }
    else if (ends_with(filename, ".rgbd"))
    {
      readRgdbHeader();
      frame_reader.reset(new RgbdFrameReader(meta));
    }
    else
      throw "Unsupported dataset file extension";

    file.seekDataStart();
  }

  void
  readRawLogHeader()
  {
    meta.num_frames = file.read<int32_t>();
    file.saveDataStart();
    // In fact, the header is over, but we proceed to read the first frame to understand whether we have
    // color stream or not
    file.read<int64_t>();                        // timestamp
    file.read<int32_t>();                        // depth size
    auto have_color = file.read<int32_t>() > 0;  // color size
    // Update meta information
    meta.depth_image_resolution = {640, 480};
    meta.depth_image_size = meta.depth_image_resolution.area() * 2;
    if (have_color != 0)
    {
      meta.color_image_resolution = {640, 480};
      meta.color_image_size = meta.color_image_resolution.area() * 3;
    }
  }

  void
  readRgdbHeader()
  {
    std::string line = file.readLine();
    if (line != "# RGB-D V1")
      throw "Invalid header";
    for (;;)
    {
      line = file.readLine();
      char buffer[line.size()];
      if (sscanf(line.c_str(), "Camera model %s", buffer) == 1)
      {
        meta.camera_model = std::string(buffer);
        continue;
      }
      if (sscanf(line.c_str(), "Camera serial number %s", buffer) == 1)
      {
        meta.serial_number = std::string(buffer);
        continue;
      }
      if (sscanf(line.c_str(), "Stream color %d %d", &meta.color_image_resolution.width,
                 &meta.color_image_resolution.height) == 2)
      {
        continue;
      }
      if (sscanf(line.c_str(), "Stream depth %d %d", &meta.depth_image_resolution.width,
                 &meta.depth_image_resolution.height) == 2)
      {
        continue;
      }
      if (sscanf(line.c_str(), "Depth scale %f", &meta.depth_scale) == 1)
      {
        continue;
      }
      if (line == "With exposure")
      {
        meta.with_exposure = true;
        continue;
      }
      if (line == "With auto exposure")
      {
        meta.with_auto_exposure = true;
        continue;
      }
      if (line == "With auto white balance")
      {
        meta.with_auto_white_balance = true;
        continue;
      }
      if (sscanf(line.c_str(), "Number of frames %i", &meta.num_frames) == 1)
      {
        break;
      }
    }
    meta.depth_image_size = meta.depth_image_resolution.area() * 2;
    meta.color_image_size = meta.color_image_resolution.area() * 3;
    // Re-read header and compute CRC32
    auto crc_actual = file.computeCRC32();
    // Read stored checksum
    auto crc_stored = file.read<uint32_t>();
    if (crc_actual != crc_stored)
      throw "Invalid header (CRC mismatch)";
    auto padding = file.read<uint8_t>();
    file.skip(padding);
    file.saveDataStart();
  }

  double
  grabFrame(unsigned char* color, unsigned short* depth, bool flipColors)
  {
    if (next_frame_index >= meta.num_frames)
    {
      file.seekDataStart();
      next_frame_index = 0;
    }

    frame_reader->read(file, color, reinterpret_cast<uint8_t*>(depth), last_frame);

    if (flipColors)
    {
        for (size_t i = 0; i < meta.color_image_size; i += 3)
        {
            std::swap(color[i], color[i + 2]);
        }
    }

    ++next_frame_index;
    return last_frame.timestamp;
  }

  void
  skipFrame()
  {
    frame_reader->skip(file);
    ++next_frame_index;
  }
};


RawLogReader::RawLogReader(std::string file, bool flipColors)
: LogReader(file, flipColors)
, p(new Impl(file))
{
    rgb = new unsigned char[numPixels * 3];
    depth = new unsigned short[numPixels];
}

RawLogReader::~RawLogReader()
{
    delete rgb;
    delete depth;
}

inline bool
RawLogReader::hasMore()
{
    return p->repeat || p->next_frame_index < p->meta.num_frames; // TODO: or with -1?
}

void RawLogReader::getNext()
{
  timestamp = 1e6 * p->grabFrame(rgb, depth, flipColors);
  currentFrame = p->next_frame_index;;
}

void RawLogReader::rewind()
{
    p.reset(new Impl(file));
    currentFrame = p->next_frame_index;;
    timestamp = 0;
}

bool RawLogReader::rewound()
{
    std::cout << "Rewound (not implemented)" << std::endl;
    return true;
}

int RawLogReader::getNumFrames()
{
      return p->meta.num_frames;
}

const std::string RawLogReader::getFile()
{
    return file;
}

float RawLogReader::getExposureTime()
{
      return p->last_frame.exposure;
}

int RawLogReader::getGain()
{
      return 100;
}

void RawLogReader::setGain(int gain)
{

}

void RawLogReader::setAuto(bool value)
{

}

void RawLogReader::setExposureTime(float exposure_time)
{

}

void RawLogReader::getBack()
{
}

void RawLogReader::fastForward(int frame)
{
  auto i = static_cast<int>(frame);
  if (i >= p->meta.num_frames)
    throw "Attemped to seek beyond the valid frame index";
  while (p->next_frame_index < i)
    p->skipFrame();
  currentFrame = p->next_frame_index;;
}

