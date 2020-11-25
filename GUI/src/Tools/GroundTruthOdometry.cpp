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

#include "GroundTruthOdometry.h"
#include <cmath>
#include <iostream>

GroundTruthOdometry::GroundTruthOdometry(const std::string & filename)
 : last_utime(0)
{
    loadTrajectory(filename);
}

GroundTruthOdometry::~GroundTruthOdometry()
{

}

void GroundTruthOdometry::loadTrajectory(const std::string & filename)
{
    std::ifstream file;
    std::string line;
    file.open(filename.c_str());
    while (!file.eof())
    {
        unsigned long long int utime;
        double time;
        float x, y, z, qx, qy, qz, qw;
        std::getline(file, line);
        int n = sscanf(line.c_str(), "%lf %f %f %f %f %f %f %f", &time, &x, &y, &z, &qx, &qy, &qz, &qw);
        if(file.eof())
            break;

        assert(n == 8);

        char check_type [20];
        sscanf(line.c_str(), "%s", &check_type);
        std::string s(check_type);

        if (s.find('.') != std::string::npos) {
            printf("Multply timestamp");
            time = time * 1000000;
        }

        utime = time;
        printf("%lld %f %f %f %f %f %f %f\n", utime, x, y, z, qx, qy, qz, qw);


        Eigen::Quaternionf q(qw, qx, qy, qz);
        Eigen::Vector3f t(x, y, z);

        Eigen::Isometry3f T;
        T.setIdentity();
        T.pretranslate(t).rotate(q);
        camera_trajectory[utime] = T;
    }
}

Eigen::Matrix4f GroundTruthOdometry::getTransformation(uint64_t timestamp)
{
    Eigen::Matrix4f pose = Eigen::Matrix4f::Identity();

    std::cout << timestamp << " " << last_utime << std::endl;
    if(last_utime != 0)
    {

        /*for(std::map<uint64_t, Eigen::Isometry3f, std::less<int>, Eigen::aligned_allocator<std::pair<const uint64_t, Eigen::Isometry3f> >>::iterator it = camera_trajectory.begin(); it != camera_trajectory.end(); ++it) {
          std::cout << it->first << "\n";
        }*/
        std::map<uint64_t, Eigen::Isometry3f>::const_iterator it = camera_trajectory.find(last_utime);
        std::cout << std::distance(camera_trajectory.begin(),camera_trajectory.find(last_utime)) << std::endl;
        if (it == camera_trajectory.end())
        {
            last_utime = timestamp;
            std::cout << "GT pose " << pose << std::endl;
            return pose;
        }

        //Poses are stored in the file in iSAM basis, undo it
        Eigen::Matrix4f M;
        M <<  0,  0, 1, 0,
             -1,  0, 0, 0,
              0, -1, 0, 0,
              0,  0, 0, 1;

        //pose = M.inverse() * camera_trajectory[timestamp] * M;
        Eigen::Matrix4f I = Eigen::Matrix4f::Identity();
        pose = camera_trajectory[timestamp] * I;
    }
    else
    {
        std::map<uint64_t, Eigen::Isometry3f>::const_iterator it = camera_trajectory.find(timestamp);
        Eigen::Isometry3f ident = it->second;
        pose = Eigen::Matrix4f::Identity();
        camera_trajectory[last_utime] = ident;
    }

    last_utime = timestamp;

    return pose;
}

Eigen::MatrixXd GroundTruthOdometry::getCovariance()
{
    Eigen::MatrixXd cov(6, 6);
    cov.setIdentity();
    cov(0, 0) = 0.1;
    cov(1, 1) = 0.1;
    cov(2, 2) = 0.1;
    cov(3, 3) = 0.5;
    cov(4, 4) = 0.5;
    cov(5, 5) = 0.5;
    return cov;
}
