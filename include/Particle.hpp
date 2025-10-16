#pragma once
#include <SFML/Graphics.hpp>

struct Particle {
    sf::Vector2f pos;
    sf::Vector2f vel;

    float charge;
    float mass;
    float radius;

    sf::Color color = sf::Color::White;
};