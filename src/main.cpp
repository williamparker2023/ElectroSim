#include <SFML/Graphics.hpp>
#include "Simulator.hpp"
#include "Particle.hpp"
#include <iostream>
#include <cmath>
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
    window.setTitle("ElectroSim");
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
    float radius = 0.01f;   // m

    // --- UI state for inputs (SI units) ---
    float uiCharge = 8e-7f;   // Coulombs
    float uiMass   = 2e-3f;   // kg
    float uiBz     = 0.0f;    // Tesla (out-of-screen), placeholder (not applied yet)
    float uiEx = 0.0f;   // V/m  (N/C)
    float uiEy = 0.0f;   // V/m

    bool showEField = false; // << new: toggle for drawing sampled E-field
    auto eFieldRect = [&](){ return sf::FloatRect(12.f, 92.f, 120.f, 28.f); }; // button rect


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

    auto resetRect = [&](){
        return sf::FloatRect(12.f, 52.f, 80.f, 28.f); // x, y, w, h
    };

    auto drawButton = [&](const sf::FloatRect& r, const char* text, bool enabled){
        sf::RectangleShape box({r.width, r.height});
        box.setPosition({r.left, r.top});
        box.setFillColor(enabled ? sf::Color(60,60,60) : sf::Color(30,30,30));
        box.setOutlineThickness(1.f);
        box.setOutlineColor(sf::Color(120,120,120));
        window.draw(box);

        if (uiFont.getInfo().family != "") {
            sf::Text t(text, uiFont, 14);
            t.setFillColor(enabled ? sf::Color::White : sf::Color(160,160,160));
            auto b = t.getLocalBounds();
            // center text in box
            t.setPosition(r.left + (r.width  - b.width)/2.f  - b.left,
                        r.top  + (r.height - b.height)/2.f - b.top - 2.f);
            window.draw(t);
        }
    };

    auto drawRow = [&](int idx, const char* label, const std::string& valueText) {
        auto R = rowRect(idx);
        const float buttonW = 32.f;
        const float spacing = 4.f;
        const int numButtons = 6;
        const float totalButtonsW = numButtons * buttonW + (numButtons - 1) * spacing;
        const float totalW = totalButtonsW + 100.f; // 100px for value box
        const float startX = R.left + (R.width - totalW) / 2.f;

        // Draw label
        if (uiFont.getInfo().family != "") {
            sf::Text tLabel(label, uiFont, 14);
            tLabel.setFillColor(sf::Color(200,200,200));
            tLabel.setPosition(R.left, R.top - 16.f);
            window.draw(tLabel);
        }

        // Buttons text and multiplier mapping
        const char* btnLabels[numButtons] = {"-100", "-10", "-1", "+1", "+10", "+100"};
        for (int i = 0; i < numButtons; ++i) {
            sf::FloatRect bRect(startX + i * (buttonW + spacing), R.top, buttonW, R.height);
            sf::RectangleShape box({bRect.width, bRect.height});
            box.setPosition({bRect.left, bRect.top});
            box.setFillColor(sf::Color(70,70,70));
            box.setOutlineThickness(1.f);
            box.setOutlineColor(sf::Color(120,120,120));
            window.draw(box);

            if (uiFont.getInfo().family != "") {
                sf::Text t(btnLabels[i], uiFont, 13);
                t.setFillColor(sf::Color::White);
                auto bounds = t.getLocalBounds();
                t.setPosition(
                    bRect.left + (bRect.width - bounds.width)/2.f - bounds.left,
                    bRect.top + (bRect.height - bounds.height)/2.f - bounds.top - 2.f
                );
                window.draw(t);
            }
        }

        // Value display box (on the right)
        sf::FloatRect valRect(startX + totalButtonsW + spacing, R.top, 100.f, R.height);
        sf::RectangleShape valBx({valRect.width, valRect.height});
        valBx.setPosition({valRect.left, valRect.top});
        valBx.setFillColor(sf::Color(35,35,35));
        valBx.setOutlineThickness(1.f);
        valBx.setOutlineColor(sf::Color(120,120,120));
        window.draw(valBx);

        if (uiFont.getInfo().family != "") {
            sf::Text tVal(valueText, uiFont, 16);
            tVal.setFillColor(sf::Color(230,230,230));
            tVal.setPosition(valRect.left + 6.f, valRect.top + 4.f);
            window.draw(tVal);
        }
    };

    auto drawArrow = [&](const sf::Vector2f& tailPx, const sf::Vector2f& tipPx, sf::Color color){
        sf::Vertex line[] = {
            sf::Vertex(tailPx, color),
            sf::Vertex(tipPx, color)
        };
        window.draw(line, 2, sf::Lines);

        // arrow head
        sf::Vector2f dir = tipPx - tailPx;
        float len = std::sqrt(dir.x*dir.x + dir.y*dir.y);
        if (len < 1e-6f) return;
        sf::Vector2f u = dir / len;
        sf::Vector2f perp(-u.y, u.x);
        float headSize = std::min(6.f, std::max(3.f, len * 0.3f));
        sf::ConvexShape tri;
        tri.setPointCount(3);
        tri.setPoint(0, tipPx);
        tri.setPoint(1, tipPx - u * headSize + perp * (headSize * 0.5f));
        tri.setPoint(2, tipPx - u * headSize - perp * (headSize * 0.5f));
        tri.setFillColor(color);
        window.draw(tri);
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
                bool uiConsumed = false;

                // Reset button: only active when paused
                if (resetRect().contains(mousePx)) {
                    if (paused) sim.clear();
                    uiConsumed = true;
                }
                // E-field toggle button
                if (eFieldRect().contains(mousePx)) {
                    showEField = !showEField;
                    uiConsumed = true;
                }

                // Helper lambdas used below
                const float buttonW = 32.f, spacing = 4.f;
                const int   numButtons = 6;
                auto rowButtons = [&](const sf::FloatRect& R, int i)->sf::FloatRect {
                    const float totalButtonsW = numButtons * buttonW + (numButtons - 1) * spacing;
                    const float totalW = totalButtonsW + 100.f;
                    const float startX = R.left + (R.width - totalW) / 2.f;
                    return { startX + i * (buttonW + spacing), R.top, buttonW, R.height };
                };
                const float mult[numButtons] = {-100.f,-10.f,-1.f,+1.f,+10.f,+100.f};

                // Row 0: Charge (C)
                {
                    auto R = rowRect(0);
                    for (int i = 0; i < numButtons; ++i) {
                        if (rowButtons(R, i).contains(mousePx)) {
                            uiCharge += 2e-7f * mult[i];                 // base step × multiplier
                            uiCharge = std::clamp(uiCharge, 1e-8f, 5e-6f);
                            uiConsumed = true;
                            break;
                        }
                    }
                }

                // Row 1: Mass (kg)
                {
                    auto R = rowRect(1);
                    const float baseStep = 5e-4f, minMass = 1e-4f, maxMass = 5e-2f;
                    for (int i = 0; i < numButtons; ++i) {
                        if (rowButtons(R, i).contains(mousePx)) {
                            uiMass += baseStep * mult[i];
                            uiMass = std::clamp(uiMass, minMass, maxMass);
                            uiConsumed = true;
                            break;
                        }
                    }
                }

                // Row 2: Bz (T)
                {
                    auto R = rowRect(2);
                    const float baseStep = 1.0f, minBz = -10000.f, maxBz = +10000.f;
                    for (int i = 0; i < numButtons; ++i) {
                        if (rowButtons(R, i).contains(mousePx)) {
                            uiBz += baseStep * mult[i];
                            uiBz = std::clamp(uiBz, minBz, maxBz);
                            uiConsumed = true;
                            break;
                        }
                    }
                }

                // Row 3: Ex (V/m)
                {
                    auto R = rowRect(3);
                    const float baseStep = 10.f, minE = -1e4f, maxE = +1e4f;
                    for (int i = 0; i < numButtons; ++i) {
                        if (rowButtons(R, i).contains(mousePx)) {
                            uiEx += baseStep * mult[i];
                            uiEx = std::clamp(uiEx, minE, maxE);
                            uiConsumed = true;
                            break;
                        }
                    }
                }

                // Row 4: Ey (V/m)
                {
                    auto R = rowRect(4);
                    const float baseStep = 10.f, minE = -1e4f, maxE = +1e4f;
                    for (int i = 0; i < numButtons; ++i) {
                        if (rowButtons(R, i).contains(mousePx)) {
                            uiEy += baseStep * mult[i];
                            uiEy = std::clamp(uiEy, minE, maxE);
                            uiConsumed = true;
                            break;
                        }
                    }
                }

                // If any UI element handled the click, don't spawn a particle
                if (uiConsumed) continue;
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

            sim.params().externalE = { uiEx, uiEy };
            sim.params().externalBz = uiBz; 

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

        // Draw E-field sample grid arrows (pixels)
        if (showEField) {
            const float gridPx = 36.f;               // visual spacing in pixels (tune for density)
            const float arrowVisualScale = 2e-6f;    // visual scale factor -> adjust to taste
            const auto& params = sim.params();
            const float k = params.k;
            const float soft2 = params.softening2;

            for (float gx = gridPx * 0.5f; gx < (float)W; gx += gridPx) {
                for (float gy = gridPx * 0.5f; gy < (float)H; gy += gridPx) {
                    sf::Vector2f pPx(gx, gy);
                    sf::Vector2f pM = pPx / ppm; // to meters

                    // start with external uniform E-field
                    sf::Vector2f E = sim.params().externalE;

                    // sum contributions from particles: E += k * q * r / |r|^3
                    for (const auto& part : sim.particles()) {
                        sf::Vector2f r = pM - part.pos;
                        float r2 = r.x*r.x + r.y*r.y + soft2;
                        float rmag = std::sqrt(r2);
                        float invr3 = (rmag > 0.0f) ? 1.0f / (rmag * r2) : 0.0f;
                        E += (k * part.charge) * r * invr3;
                    }

                    // convert E (V/m) to pixels for drawing
                    float Emag = std::sqrt(E.x*E.x + E.y*E.y);
                    if (Emag < 1e-12f){
                        // draw a small neutral dot so the grid is visible even with no field
                        sf::CircleShape dot(2.f);
                        dot.setOrigin(2.f, 2.f);
                        dot.setPosition(pPx);
                        dot.setFillColor(sf::Color(70,70,110));
                        window.draw(dot);
                        continue;
                    }
                    sf::Vector2f dir = E / Emag;

                    float lenPx = std::clamp(Emag * ppm * arrowVisualScale, 4.f, gridPx * 0.9f);
                    sf::Vector2f tip = pPx + dir * lenPx;
                    // color by sign of dot with +x (or just white); using magnitude tint
                    sf::Uint8 c = static_cast<sf::Uint8>(std::min(255.f, 40.f + 215.f * std::min(1.f, Emag * 1e-3f)));
                    drawArrow(pPx, tip, sf::Color(c, c, 255));
                }
            }
        }

        //tiny cursor dot so you can see where you click
        sf::CircleShape cursor(3.0f);
        cursor.setOrigin(3.0f, 3.0f);
        cursor.setPosition(sf::Vector2f(sf::Mouse::getPosition(window)));
        cursor.setFillColor(sf::Color(200,200,200));
        window.draw(cursor);

        if (uiFont.getInfo().family != "") { // loaded ok
            drawButton(resetRect(), "Reset", paused);
            drawButton(eFieldRect(), "E Field", showEField);

            auto fmt = [](float v) {
                char buf[64];
                std::snprintf(buf, sizeof(buf), "%.3g", v);
                return std::string(buf);
            };

            // Rows
            drawRow(0, "Charge (C)", fmt(uiCharge));
            drawRow(1, "Mass (kg)",  fmt(uiMass));
            drawRow(2, "Bz (T)",     fmt(uiBz));
            drawRow(3, "Ex (V/m)",   fmt(uiEx));
            drawRow(4, "Ey (V/m)",   fmt(uiEy));
        }

        window.display();
    }

    return 0;
}
