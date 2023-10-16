#pragma once

#include "RayTracer.hpp"
#include "Strings.hpp"

void loadScene(tp::Scene& scene, const tp::String& scenePath, tp::RayTracer::RenderSettings& settings);
