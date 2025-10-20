#include <SFML/Graphics.hpp>
#include "Simulator.hpp"
#include "Particle.hpp"
#include <iostream>
enum class Mode { Custom, ElectronGun };

//add magnetic field
int main() {
    // -------------------------------
    // Window + render scale
    // -------------------------------
    const unsigned W = 800, H = 600;       // pixels
    constexpr float PPM = 100.0f;          // pixels per meter (100 px = 1 m)

    sf::RenderWindow window(sf::VideoMode(W, H), "ElectroSim (SI units)");
    window.setFramerateLimit(120);

    sf::Font uiFont;
    if(!uiFont.loadFromFile("assets/fonts/RobotoRegular-3m4L.ttf")){
        std::cout << "Font Not Found";
    }

    // -------------------------------
    // Simulator params in SI units
    // -------------------------------
    Simulator::Params prm;
    prm.boundsW     = W / PPM;             // meters
    prm.boundsH     = H / PPM;             // meters
    prm.k           = 8.9875517923e9f;     // Coulomb constant (N·m^2/C^2)
    prm.softening2  = (0.05f * 0.05f);               // (0.01 m)^2
    prm.restitution = 0.9f;                // mirror walls
    prm.maxAccel    = 1.0e4f;              // m/s^2 clamp for safety


    Simulator sim(prm);
    sim.setBoundsEnabled(false);
    sim.setElectrostaticsEnabled(true);

    bool paused = true;
    

    float qMag   = 1e-6f;   // Coulombs
    float mass   = 1e-3f;   // kg
    float radius = 0.01f;   // m

    // --- UI state for inputs (SI units) ---
    float uiCharge = 8e-7f;   // Coulombs
    float uiMass   = 2e-3f;   // kg
    float uiBz     = 0.0f;    // Tesla (out-of-screen), placeholder (not applied yet)

    // Layout for three rows (top-right panel)
    const sf::Vector2f panelPos{ W - 220.f, 12.f };
    const sf::Vector2f rowSize{ 208.f, 32.f };
    const float rowGap = 8.f;

    auto rowRect = [&](int i){ return sf::FloatRect(panelPos.x, panelPos.y + i*(rowSize.y+rowGap), rowSize.x, rowSize.y); };

    auto minusRect = [&](const sf::FloatRect& r){ return sf::FloatRect(r.left, r.top, 32.f, r.height); };
    auto plusRect  = [&](const sf::FloatRect& r){ return sf::FloatRect(r.left + r.width - 32.f, r.top, 32.f, r.height); };
    auto valueRect = [&](const sf::FloatRect& r){ return sf::FloatRect(r.left + 36.f, r.top, r.width - 72.f, r.height); };

    auto makeRect = [](const sf::FloatRect& r, sf::Color fill){
        sf::RectangleShape s; s.setPosition({r.left, r.top}); s.setSize({r.width, r.height});
        s.setFillColor(fill); s.setOutlineThickness(1.f); s.setOutlineColor(sf::Color(120,120,120));
        return s;
    };
    
    sf::Clock clock;
    float accTime = 0.0f;
    const float dt = 1.0f / 240.0f; // seconds
    
    while (window.isOpen()) {
        // ---- events ----
        sf::Event e;
        while (window.pollEvent(e)) {
            if (e.type == sf::Event::Closed) window.close();

            if (e.type == sf::Event::KeyPressed) {
                if (e.key.code == sf::Keyboard::Space) paused = !paused;
                if (e.key.code == sf::Keyboard::C)     sim.clear(); // clear all particles
            }

            // Mouse wheel: adjust spawn charge magnitude (±0.2 µC steps, clamped)
            if (e.type == sf::Event::MouseWheelScrolled) {
                qMag += (e.mouseWheelScroll.delta > 0 ? +2e-7f : -2e-7f);
                if (qMag < 1e-7f) qMag = 1e-7f;
                if (qMag > 5e-6f) qMag = 5e-6f;
            }

            // Click to spawn: Left = +q, Right = -q
            if (e.type == sf::Event::MouseButtonPressed && paused) {
                sf::Vector2f mousePx = sf::Vector2f(sf::Mouse::getPosition(window));
                sf::Vector2f mouseM  = mousePx / PPM; // px -> meters

                float q = (e.mouseButton.button == sf::Mouse::Left) ? +qMag : -qMag;
                sf::Color col = (q > 0) ? sf::Color::Red : sf::Color::Blue;

                sim.addParticle({ mouseM, {0.0f, 0.0f}, q, mass, radius, col });
            }
        }


        // ---- update (fixed dt) ----
        float frame = clock.restart().asSeconds();

        if (paused) {
            // Do NOT accumulate time while paused
            accTime = 0.0f;
        } else {
            // Accumulate, but cap how much we try to catch up this frame
            accTime += std::min(frame, 0.25f); // don't accumulate more than 0.25s per frame

            // Also cap the number of physics steps per frame to avoid spiral-of-death
            int steps = 0, maxSteps = 240;     // at most ~0.5s of sim @ 1/480 dt, adjust as you like
            while (accTime >= dt && steps < maxSteps) {
                sim.step(dt);
                accTime -= dt;
                ++steps;
            }

            // (Optional) if we hit the cap, drop leftover time
            if (steps == maxSteps) accTime = 0.0f;
        }


        // ---- draw ----
        window.clear(sf::Color::Black);

        for (const auto& p : sim.particles()) {
            // meters -> pixels
            sf::Vector2f posPx = p.pos * PPM;
            float rPx = std::max(2.0f, p.radius * PPM); // ensure visible

            sf::CircleShape shape(rPx);
            shape.setOrigin(rPx, rPx);
            shape.setPosition(posPx);
            shape.setFillColor(p.color);
            window.draw(shape);
        }

        // (Optional) tiny cursor dot so you can see where you click
        sf::CircleShape cursor(3.0f);
        cursor.setOrigin(3.0f, 3.0f);
        cursor.setPosition(sf::Vector2f(sf::Mouse::getPosition(window)));
        cursor.setFillColor(sf::Color(200,200,200));
        window.draw(cursor);

        if (uiFont.getInfo().family != "") { // loaded ok
            sf::Text lbl("UI OK", uiFont, 16);
            lbl.setFillColor(sf::Color(200,200,200));
            lbl.setPosition(12.f, 12.f);
            window.draw(lbl);
        }


        window.display();
    }

    return 0;
}
