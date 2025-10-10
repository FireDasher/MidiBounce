#pragma once

#include <SFML/Graphics.hpp>

struct Square {
    float speed;
    sf::Vector2f position = {0.0f, 0.0f};
    sf::Vector2f direction = {1.0f, 1.0f};
    sf::Vector2f size = {32.0f, 32.0f}; // Display size, does not effect physics
    sf::Vector2f squishVel = {0.0f, 0.0f};

    inline sf::FloatRect getRect() {
		return {position - (size*0.5f), size};
	};
};