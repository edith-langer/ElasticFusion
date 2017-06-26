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
 
#include "GPUTexture.h"

const std::string GPUTexture::RGB = "RGB";
const std::string GPUTexture::DEPTH_RAW = "DEPTH";
const std::string GPUTexture::DEPTH_FILTERED = "DEPTH_FILTERED";
const std::string GPUTexture::DEPTH_METRIC = "DEPTH_METRIC";
const std::string GPUTexture::DEPTH_METRIC_FILTERED = "DEPTH_METRIC_FILTERED";
const std::string GPUTexture::DEPTH_NORM = "DEPTH_NORM";

GPUTexture::GPUTexture(const int width,
                       const int height,
                       const GLenum internalFormat,
                       const GLenum format,
                       const GLenum dataType,
                       const bool draw,
                       const bool cuda)
 : texture(new pangolin::GlTexture(width, height, internalFormat, draw, 0, format, dataType)),
   draw(draw),
   width(width),
   height(height),
   internalFormat(internalFormat),
   format(format),
   dataType(dataType)
{
    if(cuda)
    {
        cudaGraphicsGLRegisterImage(&cudaRes, texture->tid, GL_TEXTURE_2D, cudaGraphicsRegisterFlagsReadOnly);
    }
    else
    {
        cudaRes = 0;
    }
}

GPUTexture::~GPUTexture()
{
    if(texture)
    {
        delete texture;
    }

    if(cudaRes)
    {
        cudaGraphicsUnregisterResource(cudaRes);
    }
}

float GPUTexture::computeAverage()
{
    texture->Bind();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
    glGenerateMipmap(GL_TEXTURE_2D);
    int max_level = std::floor(std::log2(std::max(width, height)));
    switch (internalFormat)
    {
    case GL_LUMINANCE8:
        {
            unsigned char average = 0;
            glGetTexImage(GL_TEXTURE_2D, max_level, format, dataType, &average);
            return average;
        }
    case GL_LUMINANCE16:
        {
            unsigned short average = 0;
            glGetTexImage(GL_TEXTURE_2D, max_level, format, dataType, &average);
            return average;
        }
    case GL_RED:
    case GL_R32F:
    case GL_LUMINANCE:
    case GL_LUMINANCE32F_ARB:
        {
            float average = 0;
            glGetTexImage(GL_TEXTURE_2D, max_level, format, dataType, &average);
            return average;
        }
    default:
        throw std::runtime_error(
            "GlTexture::computeAverage - Unsupported internal format (" +
            pangolin::Convert<std::string,GLint>::Do(internalFormat) +
            ")"
        );
    }
}

