#pragma once

#include "RayTracer.hpp"

#include <string>

void loadScene(tp::Scene& scene, const std::string& scenePath, tp::RayTracer::RenderSettings& settings);
