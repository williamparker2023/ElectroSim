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
    unsigned W = 800, H = 600;       // pixels
    float ppm = 100.0f;          // pixels per meter (100 px = 1 m)

    const float WORLD_W_M = W / ppm;
    const float WORLD_H_M = H / ppm;

    sf::RenderWindow window(sf::VideoMode(W, H), "ElectroSim (SI units)");
    window.setFramerateLimit(120);
    sf::View view(sf::FloatRect(0.f, 0.f, (float)W, (float)H));
    window.setView(view);

    sf::Font uiFont;
    if(!uiFont.loadFromFile("assets/fonts/RobotoRegular-3m4L.ttf")){
        std::cout << "Font Not Found";
    }

    // -------------------------------
    // Simulator params in SI units
    // -------------------------------
    Simulator::Params prm;
    prm.boundsW     = W / ppm;             // meters
    prm.boundsH     = H / ppm;             // meters
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
    float radius = 0.1f;   // m

    // --- UI state for inputs (SI units) ---
    float uiCharge = 8e-7f;   // Coulombs
    float uiMass   = 2e-3f;   // kg
    float uiBz     = 0.0f;    // Tesla (out-of-screen), placeholder (not applied yet)

    // Layout for three rows (top-right panel)
    const sf::Vector2f rowSize{ 208.f, 32.f };
    const float rowGap = 8.f;

    auto rowRect = [&](int i){ 
        float x = static_cast<float>(W) - 220.f;
        float y = 16.f + i * (rowSize.y + rowGap);
        return sf::FloatRect(x, y, rowSize.x, rowSize.y); 
    };

    auto minusRect = [&](const sf::FloatRect& r){ return sf::FloatRect(r.left, r.top, 32.f, r.height); };
    auto plusRect  = [&](const sf::FloatRect& r){ return sf::FloatRect(r.left + r.width - 32.f, r.top, 32.f, r.height); };
    auto valueRect = [&](const sf::FloatRect& r){ return sf::FloatRect(r.left + 36.f, r.top, r.width - 72.f, r.height); };

    auto makeRect = [](const sf::FloatRect& r, sf::Color fill){
        sf::RectangleShape s; s.setPosition({r.left, r.top}); s.setSize({r.width, r.height});
        s.setFillColor(fill); s.setOutlineThickness(1.f); s.setOutlineColor(sf::Color(120,120,120));
        return s;
    };

    auto drawRow = [&](int idx, const char* label, const std::string& valueText) {
        auto R     = rowRect(idx);
        auto minus = makeRect(minusRect(R), sf::Color(70,70,70));
        auto plus  = makeRect(plusRect(R),  sf::Color(70,70,70));
        auto valBx = makeRect(valueRect(R), sf::Color(35,35,35));

        window.draw(minus);
        window.draw(plus);
        window.draw(valBx);

        if (uiFont.getInfo().family != "") {
            // label (above the row)
            sf::Text tLabel(label, uiFont, 14);
            tLabel.setFillColor(sf::Color(200,200,200));
            tLabel.setPosition(R.left, R.top - 16.f);
            window.draw(tLabel);

            // glyphs on the +/- boxes
            sf::Text tMinus("-", uiFont, 18);
            tMinus.setFillColor(sf::Color::White);
            tMinus.setPosition(minus.getPosition().x + 10.f, minus.getPosition().y + 4.f);
            window.draw(tMinus);

            sf::Text tPlus("+", uiFont, 18);
            tPlus.setFillColor(sf::Color::White);
            tPlus.setPosition(plus.getPosition().x + 9.f, plus.getPosition().y + 4.f);
            window.draw(tPlus);

            // value text
            sf::Text tVal(valueText, uiFont, 16);
            tVal.setFillColor(sf::Color(230,230,230));
            tVal.setPosition(valBx.getPosition().x + 6.f, valBx.getPosition().y + 4.f);
            window.draw(tVal);
        }
    };
    
    sf::Clock clock;
    float accTime = 0.0f;
    const float dt = 1.0f / 240.0f; // seconds
    
    while (window.isOpen()) {
        // ---- events ----
        sf::Event e;
        while (window.pollEvent(e)) {
            if (e.type == sf::Event::Closed) window.close();

            if (e.type == sf::Event::Resized) {
                W = e.size.width;
                H = e.size.height;
                float ppmX = W / WORLD_W_M;
                float ppmY = H / WORLD_H_M;
                ppm = std::min(ppmX, ppmY);
                view.setSize((float)W, (float)H);
                view.setCenter(W / 2.f, H / 2.f);
                window.setView(view);
                sim.params().boundsW = W / ppm;
                sim.params().boundsH = H / ppm;
                if (paused && sim.boundsEnabled()) {
                    for (auto& p : sim.particles()) {
                        if (p.pos.x < p.radius)                    { p.pos.x = p.radius; }
                        if (p.pos.x > sim.params().boundsW - p.radius) { p.pos.x = sim.params().boundsW - p.radius; }
                        if (p.pos.y < p.radius)                    { p.pos.y = p.radius; }
                        if (p.pos.y > sim.params().boundsH - p.radius) { p.pos.y = sim.params().boundsH - p.radius; }
                    }
                }

            }


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

            auto contains = [](const sf::FloatRect& r, sf::Vector2f p){ return r.contains(p); };
            //buttons
            if (e.type == sf::Event::MouseButtonPressed) {
                sf::Vector2f mousePx = (sf::Vector2f)sf::Mouse::getPosition(window);

                // Row 0: Charge (C)
                {
                    auto R = rowRect(0);
                    if (contains(R, mousePx)) {
                        if (contains(minusRect(R), mousePx)) { 
                            uiCharge -= 2e-7f;                          // step down 0.2 µC
                            if (uiCharge < 1e-8f) uiCharge = 1e-8f;     // clamp
                            continue; // consume click; don't place a particle
                        }
                        if (contains(plusRect(R), mousePx))  { 
                            uiCharge += 2e-7f;                          // step up 0.2 µC
                            if (uiCharge > 5e-6f) uiCharge = 5e-6f;     // clamp
                            continue;
                        }
                    }
                }

                // Row 1: Mass (kg)
                {
                    auto R = rowRect(1);
                    if (contains(R, mousePx)) {
                        if (contains(minusRect(R), mousePx)) { 
                            uiMass -= 5e-4f;                            // step 0.0005 kg (0.5 g)
                            if (uiMass < 1e-4f) uiMass = 1e-4f;         // min 0.1 g
                            continue;
                        }
                        if (contains(plusRect(R), mousePx))  { 
                            uiMass += 5e-4f;
                            if (uiMass > 5e-2f) uiMass = 5e-2f;         // max 50 g
                            continue;
                        }
                    }
                }

                // Row 2: Bz (Tesla) — UI only for now
                {
                    auto R = rowRect(2);
                    if (contains(R, mousePx)) {
                        if (contains(minusRect(R), mousePx)) { 
                            uiBz -= 0.05f;                              // step 0.05 T
                            if (uiBz < -2.0f) uiBz = -2.0f;             // clamp [-2, +2] T
                            continue;
                        }
                        if (contains(plusRect(R), mousePx))  { 
                            uiBz += 0.05f;
                            if (uiBz >  2.0f) uiBz =  2.0f;
                            continue;
                        }
                    }
                }

                // If we didn't click the UI, fall through to other click handling...
            }

            // Click to spawn: Left = +q, Right = -q
            if (e.type == sf::Event::MouseButtonPressed && paused) {
                sf::Vector2f mousePx = sf::Vector2f(sf::Mouse::getPosition(window));
                sf::Vector2f mouseM  = mousePx / ppm; // px -> meters

                float q = (e.mouseButton.button == sf::Mouse::Left) ? +uiCharge : -uiCharge;
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
            sf::Vector2f posPx = p.pos * ppm;
            float rPx = std::max(2.0f, p.radius * ppm); // ensure visible

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

            auto fmt = [](float v) {
                char buf[64];
                std::snprintf(buf, sizeof(buf), "%.3g", v);
                return std::string(buf);
            };

            drawRow(0, "Charge (C)", fmt(uiCharge));
            drawRow(1, "Mass (kg)",  fmt(uiMass));
            drawRow(2, "Bz (T)",     fmt(uiBz));
        }

        


        window.display();
    }

    return 0;
}
