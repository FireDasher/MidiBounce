#pragma once

#include <SFML/Graphics.hpp>

struct Bounce {
    sf::Vector2f position;
    sf::Vector2f direction;
    bool vertical;
    float time;
    inline sf::FloatRect getRect() const {
		if (vertical) {
			// Left / Right
			if (direction.x < 0.0f) {
				// Right
				return {{position.x + 16.0f, position.y - 16.0f}, {8.0f, 32.0f}};
			} else {
				// Left
				return {{position.x - 24.0f, position.y - 16.0f}, {8.0f, 32.0f}};
			}
		} else {
			// Top / Bottom
			if (direction.y < 0.0f) {
				// Bottom
				return {{position.x - 16.0f, position.y + 16.0f}, {32.0f, 8.0f}};
			} else {
				// Top
				return {{position.x - 16.0f, position.y - 24.0f}, {32.0f, 8.0f}};
			}
		}
	};
};