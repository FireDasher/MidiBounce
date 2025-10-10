#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <cmath>
#include <algorithm>
#include "world.hpp"

int main(int argc, const char** argv) {
	std::vector<std::string> args;
	std::unordered_set<std::string> flags;
	for (int i = 0; i < argc; ++i) {
		std::string arg(argv[i]);
		if (arg.starts_with("--")) {
			arg.erase(0, 2);
			flags.insert(arg);
		} else args.push_back(arg);
	}

    if (args.size() < 2) {
        std::cout << "Usage: " + std::string(args[0]) + " <midi-file> <music-file> [seed] [--fullscreen] [--visgen (Visualize Generation)]" << std::endl;
        return 0;
    }

	bool fullscreen = flags.contains("fullscreen");
    sf::RenderWindow window(fullscreen ? sf::VideoMode::getDesktopMode() : sf::VideoMode({800, 600}), "Midi Bounce", fullscreen ? sf::State::Fullscreen : sf::State::Windowed);
    window.setVerticalSyncEnabled(true);
	window.setMouseCursorVisible(false);
	
    srand(time(nullptr));
	
    World world{{600.0f}};
    Square& square = world.square;
	
    sf::Music music(args[2]);
	
    unsigned int seed = args.size() > 3 ? std::stoi(args[3]) : time(nullptr);
	std::cout << "Seed: " << seed << std::endl;
    
    std::cout << "Loading MidiFile..." << std::endl;
	Midi::MidiFile mid = Midi::loadMidi10(args[1]);

    std::cout << "Removing overlapping notes..." << std::endl;
	std::vector<float> data;
	float minSpacing = 0.03f;
	float lastNoteTime = 0.0f;
	for (const auto& note : mid.notes) if (note.time > lastNoteTime + minSpacing) {
		lastNoteTime = note.time;
		data.push_back(note.time);
	}

    std::cout << "Generating world..." << std::endl;
	world.generateBounces(data, seed, window, flags.contains("visgen"));

    std::cout << "Finished loading" << std::endl;
    
    float timewarp = 1.0f;
	
    sf::Vector2f camerapos = {0.0f, 0.0f};
    bool flyingmode = false;
    float easingRate = 3.0f;
	
    int nextBounce = 0;
    float time = 0.0f;
	
    sf::Vector2f targetSquareSize{32.0f, 32.0f};
	
    sf::Color colors[4] = {
		sf::Color{255, 0, 0},
        sf::Color{0, 255, 0},
        sf::Color{0, 0, 255},
        sf::Color{255, 255, 0},
    };
	
    bool started = false;

	sf::Clock deltaClock{};

    while (window.isOpen()) {
        float deltaTime = deltaClock.restart().asSeconds();

        square.squishVel += (targetSquareSize - square.size) * deltaTime * square.speed;
        square.squishVel *= std::expf(-6.0f * deltaTime);
        square.size += square.squishVel * deltaTime;

        if (!started) {
            time += deltaTime;
            if (time > 2.0f) {
                started = true;
                time = 0.0f;
                music.play();
            }
        }

        if (started && nextBounce < world.bounces.size()) {
            time += deltaTime * timewarp;
            square.position += square.direction * square.speed * deltaTime * timewarp;
            if (const Bounce& bounce = world.bounces[nextBounce]; time >= bounce.time) {
                square.position = bounce.position;
                square.direction = bounce.direction;

                if (bounce.vertical) {
                    square.size.x = 24.0f;
                    square.size.y = 40.0f;
                } else {
                    square.size.x = 40.0f;
                    square.size.y = 24.0f;
                }

                nextBounce++;
            }
        }

        while (const std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
            } else if (const auto* resized = event->getIf<sf::Event::Resized>()) {
                window.setView(sf::View{sf::FloatRect{{0.0f, 0.0f},{(float)resized->size.x, (float)resized->size.y}}});
            } else if (const auto* key = event->getIf<sf::Event::KeyPressed>()) {
                if (key->scancode == sf::Keyboard::Scan::Num0) {
                    flyingmode = !flyingmode;
                } else if (key->scancode == sf::Keyboard::Scan::R) {
                    started = false;
                    square.position = {0.0f, 0.0f};
                    square.direction = {1.0f, 1.0f};
                    time = 0.0f;
                    nextBounce = 0;
                    music.stop();
                }
            }
        }
        sf::Vector2f windowSize = (sf::Vector2f)window.getSize();

        window.clear({64, 64, 64});

        if (flyingmode) {
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Scan::Right)) {
                camerapos.x += 1200.0f * deltaTime;
            } if (sf::Keyboard::isKeyPressed(sf::Keyboard::Scan::Left)) {
                camerapos.x -= 1200.0f * deltaTime;
            } if (sf::Keyboard::isKeyPressed(sf::Keyboard::Scan::Up)) {
                camerapos.y -= 1200.0f * deltaTime;
            } if (sf::Keyboard::isKeyPressed(sf::Keyboard::Scan::Down)) {
                camerapos.y += 1200.0f * deltaTime;
            }
        } else {
            camerapos = (square.position - windowSize * 0.5f) * easingRate * deltaTime + camerapos - easingRate * camerapos * deltaTime;
        }

        for (const auto& area : world.sigmaAreas) {
            sf::RectangleShape rect(area.size);
            rect.setPosition(area.position - camerapos);
            rect.setFillColor({128, 128, 128});
            window.draw(rect);
        }


        for (const auto& bounce : world.bounces) {
            sf::FloatRect bounceRect = bounce.getRect();
            sf::RectangleShape rect(bounceRect.size);
            rect.setPosition(bounceRect.position - camerapos);
            rect.setFillColor({64, 64, 64});
            window.draw(rect);
        }

        sf::FloatRect squareRect = square.getRect();
        sf::RectangleShape rect(squareRect.size);
        rect.setPosition(squareRect.position - camerapos);
        rect.setFillColor(colors[nextBounce % 4]);
        rect.setOutlineColor(sf::Color::Black);
        rect.setOutlineThickness(-4.0f);
        window.draw(rect);
        
        window.display();
    }
}