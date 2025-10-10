#include <unordered_set>
#include "world.hpp"
#include "square.hpp"

sf::FloatRect rectUnion(const sf::FloatRect& rect1, const sf::FloatRect& rect2) {
    float minX = std::min(rect1.position.x, rect2.position.x);
    float minY = std::min(rect1.position.y, rect2.position.y);
    float maxX = std::max(rect1.position.x + rect1.size.x, rect2.position.x + rect2.size.x);
    float maxY = std::max(rect1.position.y + rect1.size.y, rect2.position.y + rect2.size.y);

    return sf::FloatRect({minX, minY}, {maxX - minX, maxY - minY});
}

bool intersects(const sf::FloatRect& rect1, const sf::FloatRect& rect2) {
    return
        rect1.position.x < rect2.position.x + rect2.size.x && rect1.position.x + rect1.size.x > rect2.position.x
        &&
        rect1.position.y < rect2.position.y + rect2.size.y && rect1.position.y + rect1.size.y > rect2.position.y
    ;
}

bool collideListBounces(const sf::FloatRect& rect1, const std::vector<Bounce>& list) {
    for (int i = 0; i < list.size(); ++i) if (
        intersects(rect1, list[i].getRect())
    ) return true;
    return false;
}

bool collideList(const sf::FloatRect& rect1, const std::vector<sf::FloatRect>& list, int ignore) {
    if (list.size() <= ignore) return false;
    for (int i = 0; i < list.size() - ignore; ++i) if (
        intersects(rect1, list[i])
    ) return true;
    return false;
}

// No recusion solution
struct Moment {
    sf::Vector2f position;
    sf::Vector2f direction;
    float time;
    int nextNote;
    bool vertical;
    int pathStart;
};
void World::generateBounces(const std::vector<float>& notes, unsigned int seed, sf::RenderWindow& window, bool showGeneration) {
    srand(seed);

    bounces = {};

    square.position = {0.0f, 0.0f};
    square.direction = {1.0f, 1.0f};
    bool vertical = false;

    int changedirchance = 30;

    float delta = 1.f / 60.f; // 60 TPS
    float time = 0.0f;

    int nextNote = 0;

    sf::FloatRect startRect = square.getRect();

    std::vector<Moment> moments{Moment{square.position, square.direction, time, nextNote, vertical, 0}};
    std::unordered_set<int> backtracedBounces;
    bool collided = false;

    std::vector<sf::FloatRect> path{square.getRect()};

    sf::Vector2f cameraPos{0.0f, 0.0f};

	bool cancel = false;

    while (nextNote < notes.size() && !cancel) {
        square.position += square.direction * square.speed * delta;
        path.push_back(square.getRect());
        if (collided || collideListBounces(square.getRect(), bounces)) {
            if (collided) {
				backtracedBounces.erase(bounces.size() - 1);
                moments.pop_back();
                bounces.pop_back();
                collided = false;
            }
            while (backtracedBounces.contains(bounces.size() - 1)) {
				backtracedBounces.erase(bounces.size() - 1);
                moments.pop_back();
                bounces.pop_back();
            }
            bounces.pop_back();

            Moment& moment = moments.back();
            while (path.size() != moment.pathStart) path.pop_back();
            nextNote = moment.nextNote;
            square.position = moment.position;
            square.direction = moment.direction;
            time = moment.time;
            
            vertical = !moment.vertical;
            
            moments.pop_back();
            
            backtracedBounces.insert(bounces.size());
        }
        if (time >= notes[nextNote]) {
			// Bounce
			if (!backtracedBounces.contains(bounces.size()) && rand() % 100 < changedirchance) vertical = !vertical;

			moments.emplace_back(Moment{square.position, square.direction, time, nextNote, vertical, (int)path.size()});

			if (vertical) square.direction.x *= -1.0f;
			else square.direction.y *= -1.0f;

			Bounce bounce{square.position, square.direction, vertical, time};

			sigmaAreas.push_back(rectUnion(startRect, square.getRect()));
			bounces.push_back(bounce);

			startRect = square.getRect();
			collided = collideList(bounce.getRect(), path, 8);

			///////////
            nextNote++;
        }
        time += delta;

        // render
        if (showGeneration) {
            window.clear({64, 64, 64});
            cameraPos = (square.position - (sf::Vector2f)window.getSize() * 0.5f) * 3.0f * delta + cameraPos - 3.0f * cameraPos * delta;
            while (const std::optional event = window.pollEvent()) {
                if (event->is<sf::Event::Closed>()) {
                    window.close();
					cancel = true;
                } else if (const auto* resized = event->getIf<sf::Event::Resized>()) {
                    window.setView(sf::View{sf::FloatRect{{0.0f, 0.0f},{(float)resized->size.x, (float)resized->size.y}}});
                }
            }
            for (const auto& area : sigmaAreas) {
                sf::RectangleShape rect(area.size);
                rect.setPosition(area.position - cameraPos);
                rect.setFillColor({128, 128, 128});
                window.draw(rect);
            }
            for (int i = 0; i < bounces.size(); ++i) {
                const auto& bounce = bounces[i];
                sf::FloatRect bounceRect = bounce.getRect();
                sf::RectangleShape rect(bounceRect.size);
                rect.setPosition(bounceRect.position - cameraPos);
                rect.setFillColor({0, 0, 255});
                if (backtracedBounces.contains(i)) rect.setFillColor(sf::Color::Red);
                window.draw(rect);
            }
            for (const auto& something : path) {
                sf::RectangleShape rect(something.size);
                rect.setPosition(something.position - cameraPos);
                rect.setFillColor({255, 255, 0});
                window.draw(rect);
            }
            sf::FloatRect squareRect = square.getRect();
            sf::RectangleShape rect(squareRect.size);
            rect.setPosition(squareRect.position - cameraPos);
            rect.setFillColor(sf::Color::Red);
            rect.setOutlineColor(sf::Color::Black);
            rect.setOutlineThickness(-4.0f);
            window.draw(rect);

            window.display();
        }
    }

    square.position = {0.0f, 0.0f};
    square.direction = {1.0f, 1.0f};
}