#pragma once

#include <string>
#include <vector>
#include "ray-tracer/triangle.h"
#include "ray-tracer/materials.h"

class SceneLoader
{
    public:
    SceneLoader(const std::string &filename, const std::string &mtlBasePath);
    ~SceneLoader();

    virtual void loadTrianglesAndMaterials(std::vector<Triangle> &outTriangles, std::vector<Material> &outMaterials) = 0;
    
    std::string m_filename;
    std::string m_mtlBasePath;
    private:
};

SceneLoader::SceneLoader(const std::string &filename, const std::string &mtlBasePath)
{
    m_filename = filename;
    m_mtlBasePath = mtlBasePath;
}
