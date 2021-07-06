
#include <cassert>
#include <cmath>
#include <iostream>
#include <thread>

#include <SFML/Graphics.hpp>

#define WINDOW_W 800
#define WINDOW_H 800

constexpr double DegToRad(double aDeg) {
    return (aDeg / 180.0) * 3.1415926535;
}

int main(int argc, char* argv[]) {
    double angle = 0.0;

    sf::RenderWindow window(sf::VideoMode(WINDOW_W, WINDOW_H), "SFML works!", sf::Style::Fullscreen);
    window.setVerticalSyncEnabled(true);
    //window.setFramerateLimit(60);

    sf::CircleShape shape(32.f);
    shape.setFillColor(sf::Color::Green);
    
    // ===== Main loop =====
    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
                window.close();
        }

        {
            angle += 1.0;
            if (angle >= 360.0) {
                angle -= 360.0;
            }
            float x = WINDOW_W / 2.0 + std::cos(DegToRad(-angle)) * 300.0;
            float y = WINDOW_H / 2.0 - std::sin(DegToRad(-angle)) * 300.0;

            shape.setPosition({x - 16.f, y - 16.f});
        }

        window.clear();
        window.draw(shape);
        window.display();
    }

    return 0;
}