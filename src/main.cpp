#include <SFML/Graphics.hpp>

int main() {
    // Create a window
    sf::RenderWindow window(sf::VideoMode(800, 600), "ElectroSim Test");

    // Create a small white dot
    sf::CircleShape dot(5.f); // radius = 5 pixels
    dot.setFillColor(sf::Color::White);
    dot.setOrigin(5.f, 5.f);            // center the shape
    dot.setPosition(400.f, 300.f);      // place it in the middle of the window

    // Main loop
    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();
        }

        // Draw the dot
        window.clear(sf::Color::Black);
        window.draw(dot);
        window.display();
    }

    return 0;
}
