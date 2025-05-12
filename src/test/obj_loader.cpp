#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include "../math/vector3f.h"
#include "../math/vector3f.h"

int main() {
    std::string obj_folder = "assets/models/";
    std::string obj_file = "suzanne.obj";

    std::string obj_path = obj_folder + obj_file;

    std::fstream obj_file_stream(obj_path, std::ios::in);
    if (!obj_file_stream.is_open()) {
        std::cerr << "Failed to open the file: " << obj_path << std::endl;
        return 1;
    }
    std::cout << "Successfully opened the file: " << obj_path << std::endl;

    std::vector<vector3f> vecv, vecn;
    std::vector<std::vector<int>> vecf;

    std::string line;

    while (std::getline(obj_file_stream, line)) {
        if (line.empty() || line[0] == '#')
            continue;
        std::stringstream iss(line);
        std::string prefix;
        iss >> prefix;
        if (prefix == "v") {
            double x, y, z;
            iss >> x >> y >> z;
            vecv.push_back(vector3f(x, y, z));
        }
        else if (prefix == "vn") {
            double x, y, z;
            iss >> x >> y >> z;
            vecn.push_back(vector3f(x, y, z));
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
    std::cout << "Number of vertices: " << vecv.size() << std::endl;
    std::cout << "Number of normals: " << vecn.size() << std::endl;
    std::cout << "Number of triangles: " << vecf.size() << std::endl;
    obj_file_stream.close();
    return 0;
}