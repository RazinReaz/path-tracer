#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <unordered_set>

#include "math/vector3f.h"

int main() {
    std::string obj_folder = "assets/models/";
    std::string obj_file = "cube.obj";

    std::string obj_path = obj_folder + obj_file;

    std::fstream obj_file_stream(obj_path, std::ios::in);
    if (!obj_file_stream.is_open()) {
        std::cerr << "Failed to open the file: " << obj_path << std::endl;
        return 1;
    }
    std::cout << "Successfully opened the file: " << obj_path << std::endl;

    std::vector<vector3f> vecv, vecn, vectex;
    std::vector<std::vector<int>> vecf;

    std::string line;

    while (std::getline(obj_file_stream, line)) {
        if (line.empty() || line[0] == '#')
            continue;
        std::stringstream iss(line);
        std::string prefix;
        iss >> prefix;
        if (prefix == "v") {
            float x, y, z;
            iss >> x >> y >> z;
            vecv.push_back(vector3f(x, y, z));
        }
        else if (prefix == "vn") {
            float x, y, z;
            iss >> x >> y >> z;
            vecn.push_back(vector3f(x, y, z));
        }
        else if (prefix == "vt") {
            float s, t;
            iss >> s >> t;
            vectex.push_back(vector3f(s, t, 0));
        }
        else if (prefix == "f") {
            std::vector<int> face;
            std::string vertex;
            while (iss >> vertex) {
                std::istringstream vert_stream(vertex);
                std::string vind, vnind, vtind;
                std::getline(vert_stream, vind, '/');
                std::getline(vert_stream, vtind, '/');
                std::getline(vert_stream, vnind, '/');
                int v_index = std::stoi(vind) - 1;
                int vt_index = vtind.empty() ? -1 : std::stoi(vtind) - 1; // vt might be empty
                int vn_index = std::stoi(vnind) - 1;
                face.push_back(v_index);
                face.push_back(vt_index);
                face.push_back(vn_index);
            }
            vecf.push_back(face);
        }
    }
    obj_file_stream.close();


    int vert_size = 3;
    int norm_size = 3;
    int text_size = 2;
    int stride = (vert_size + text_size + norm_size);
    std::vector<float> vertices(vecf.size() * stride);

    for (size_t i = 0; i < vecf.size(); i++) {
        if (vecf[i].size() < 9)
        {
            std::cerr << "Face " << i << " does not have enough indices!" << std::endl;
            continue;
        }
        for (int j = 0, off = 0; j < 3; j++, off+=3) {

            int vert_index = vecf[i][off + 0];
            int text_index = vecf[i][off + 1];
            int norm_index = vecf[i][off + 2];

            if (vert_index < 0 || vert_index >= vecv.size())
            {
                std::cerr << "Invalid vertex index at face " << i << std::endl;
                continue;
            }
            if (text_index < 0 || text_index >= vectex.size())
            {
                std::cerr << "Invalid texture index at face " << i << std::endl;
                continue;
            }
            if (norm_index < 0 || norm_index >= vecn.size())
            {
                std::cerr << "Invalid normal index at face " << i << std::endl;
                continue;
            }

            int offset = stride * vert_index;
            if (offset + 7 >= vertices.size())
            {
                std::cerr << "Vertex buffer overflow at face " << i << std::endl;
                continue;
            }
            // vertex
            vertices[offset + 0] = vecv[vert_index].x;
            vertices[offset + 1] = vecv[vert_index].y;
            vertices[offset + 2] = vecv[vert_index].z;
            // texture
            vertices[offset + 3] = vectex[text_index].x;
            vertices[offset + 4] = vectex[text_index].y;
            // normal
            vertices[offset + 5] = vecn[norm_index].x;
            vertices[offset + 6] = vecn[norm_index].y;
            vertices[offset + 7] = vecn[norm_index].z;
        }
    }
    std::cout << "Number of vertices: " << vecv.size() << std::endl;
    std::cout << "Number of normals: " << vecn.size() << std::endl;
    std::cout << "Number of triangles: " << vecf.size() << std::endl;
    std::cout << "elements in vertices: " << vertices.size() << std::endl;
    std::cout << "number of vertices in vertices: " << vertices.size() / stride << std::endl;
    return 0;
}