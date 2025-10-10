#pragma once

#include <SFML/Graphics.hpp>
#include "MidiParser.hpp"
#include "bounce.hpp"
#include "square.hpp"

// World generattion
class World {
public:
    Square square;
    std::vector<Bounce> bounces;
    std::vector<sf::FloatRect> sigmaAreas;
    // Paramaters: Notes: Parsed MIDI notes. Seed: Randomness seed, use time(nullptr) for random seed. Winodw: Used for visualizing generation. ShowGeneration: Enable to visualize generation, good for debugging.
    void generateBounces(const std::vector<float>& notes, unsigned int seed, sf::RenderWindow& window, bool showGeneration);
};