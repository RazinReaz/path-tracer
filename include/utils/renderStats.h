#pragma once
#include <chrono>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <string>
#include <ctime>

class RenderStats {
public:
    RenderStats(size_t triangles, int screenWidth, int screenHeight, int bounces, int spp)
        : triangleCount(triangles),
          screenWidth(screenWidth),
          screenHeight(screenHeight),
          bounces(bounces),
          spp(spp),
          totalFrames(0),
          accumulatedTime(0.0) 
    {
        startTime = std::chrono::high_resolution_clock::now();
    }

    void frameStart() {
        frameStartTime = std::chrono::high_resolution_clock::now();
    }

    void frameEnd() {
        auto frameEndTime = std::chrono::high_resolution_clock::now();
        double frameTime = std::chrono::duration<double>(frameEndTime - frameStartTime).count();
        accumulatedTime += frameTime;
        totalFrames++;

        double fps = totalFrames / (accumulatedTime + + 1e-9);

        std::cout << "\rRendering... "
                  << "Elapsed: " << std::fixed << std::setprecision(2) << accumulatedTime << "s | "
                  << "FPS: " << std::fixed << std::setprecision(1) << fps
                  << std::flush;
    }

    void saveLog(std::string folderPath)
    {
        auto endTime = std::chrono::high_resolution_clock::now();
        double totalRenderTime = std::chrono::duration<double>(endTime - startTime).count();
        double avgFPS = totalFrames / totalRenderTime;

        // Create timestamp
        std::time_t now = std::time(nullptr);
        std::tm localTime;
    #ifdef _WIN32
        localtime_s(&localTime, &now);
    #else
        localtime_r(&now, &localTime);
    #endif

        char buffer[64];
        std::strftime(buffer, sizeof(buffer), "%Y-%m-%d_%H-%M-%S", &localTime);

        std::string filePath = folderPath + "/render_stats_" + std::string(buffer) + ".log";
        std::ofstream logFile(filePath);

        // Write file
        logFile << "Render Summary\n";
        logFile << "==============\n";

        int labelWidth = 25; // Adjust spacing for alignment

        logFile << std::left << std::setw(labelWidth) << "Total Render Time:"   << totalRenderTime << " s\n";
        logFile << std::left << std::setw(labelWidth) << "Average FPS:"         << avgFPS << "\n";
        logFile << std::left << std::setw(labelWidth) << "Average render time:" << 1000.0f/avgFPS << " ms\n";
        logFile << std::left << std::setw(labelWidth) << "Total Frames:"        << totalFrames << "\n";
        logFile << std::left << std::setw(labelWidth) << "Screen Resolution:"   << screenWidth << "x" << screenHeight << "\n";
        logFile << std::left << std::setw(labelWidth) << "Number of Triangles:" << triangleCount << "\n";
        logFile << std::left << std::setw(labelWidth) << "Bounces:"             << bounces << "\n";
        logFile << std::left << std::setw(labelWidth) << "Samples per Pixel:"   << spp << "\n";

        logFile.close();

        std::cout << "\nRender complete! Summary saved to " << filePath << "\n";
    }

    

private:
    size_t triangleCount;
    int screenWidth, screenHeight;
    int bounces, spp;
    int totalFrames;
    double accumulatedTime;

    std::chrono::high_resolution_clock::time_point startTime;
    std::chrono::high_resolution_clock::time_point frameStartTime;
};
