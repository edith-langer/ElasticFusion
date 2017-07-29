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

#ifndef VERTEX_H_
#define VERTEX_H_

#include <Eigen/Core>

#include "../Defines.h"

struct Vertex
{
    EFUSION_API static const int SIZE;

    Vertex(){}

    float x;
    float y;
    float z;
    float confidence;
    unsigned int color;
    unsigned int id;
    unsigned int init_time;
    unsigned int timestamp;
    float nx;
    float ny;
    float nz;
    float radius;

};


#endif /* VERTEX_H_ */
