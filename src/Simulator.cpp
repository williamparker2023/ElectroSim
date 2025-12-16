#include "Simulator.hpp"
#include <SFML/System/Vector2.hpp> // sf::Vector2f
#include <algorithm>
#include <cmath>
#include <memory>
#include <functional>
#include <vector>
#include <thread>

static inline sf::Vector2f clampMag(const sf::Vector2f& v, float maxMag) {
    float m2 = v.x*v.x + v.y*v.y;
    if (m2 <= maxMag*maxMag) return v;
    float inv = maxMag / std::sqrt(m2);
    return {v.x * inv, v.y * inv};
}

Simulator::Simulator(const Params& p) : P(p) {}

void Simulator::clear() { particles_.clear(); }

void Simulator::addParticle(const Particle& p) { particles_.push_back(p); }


// Reflect from rectangular bounds (meters)
void Simulator::applyBounds() {
    for (auto& p : particles_) {
        // Left/Right
        if (p.pos.x < p.radius) {
            p.pos.x = p.radius;
            p.vel.x = -p.vel.x * P.restitution;
        } else if (p.pos.x > P.boundsW - p.radius) {
            p.pos.x = P.boundsW - p.radius;
            p.vel.x = -p.vel.x * P.restitution;
        }
        // Top/Bottom
        if (p.pos.y < p.radius) {
            p.pos.y = p.radius;
            p.vel.y = -p.vel.y * P.restitution;
        } else if (p.pos.y > P.boundsH - p.radius) {
            p.pos.y = P.boundsH - p.radius;
            p.vel.y = -p.vel.y * P.restitution;
        }
    }
}   

void Simulator::computeElectricField(std::vector<sf::Vector2f>& Eout) const {
    const size_t n = particles_.size();
    Eout.assign(n, P.externalE); // start with uniform external field
    if (!electroOn_ || n == 0) return;

    const float k = P.k;
    const float soft2 = P.softening2;

    // --- Barnes-Hut quadtree (2D) ---
    struct Node {
        float xmin=0, ymin=0, xmax=0, ymax=0;
        float cx=0, cy=0;   // charge-weighted center
        float qtot=0;       // total charge in node
        int idx = -1;       // leaf particle index or -1
        std::unique_ptr<Node> child[4];
        float width() const { return xmax - xmin; }
        bool isLeaf() const { return child[0] == nullptr; }
        bool contains(const sf::Vector2f& p) const {
            return p.x >= xmin && p.x < xmax && p.y >= ymin && p.y < ymax;
        }
    };

    auto makeNode = [](float x0, float y0, float x1, float y1){
        auto n = std::make_unique<Node>();
        n->xmin = x0; n->ymin = y0; n->xmax = x1; n->ymax = y1;
        n->cx = 0; n->cy = 0; n->qtot = 0; n->idx = -1;
        return n;
    };

    // compute bounding square
    float xmin = particles_[0].pos.x, xmax = xmin;
    float ymin = particles_[0].pos.y, ymax = ymin;
    for (const auto &p : particles_) {
        xmin = std::min(xmin, p.pos.x);
        xmax = std::max(xmax, p.pos.x);
        ymin = std::min(ymin, p.pos.y);
        ymax = std::max(ymax, p.pos.y);
    }
    float cx = 0.5f*(xmin + xmax), cy = 0.5f*(ymin + ymax);
    float half = 0.5f * std::max(xmax - xmin, ymax - ymin);
    if (half < 1e-6f) half = 1e-3f;
    xmin = cx - half; xmax = cx + half;
    ymin = cy - half; ymax = cy + half;

    auto root = makeNode(xmin, ymin, xmax, ymax);

    // insert particle index into node (updates qtot, center-of-charge)
    std::function<void(Node*, size_t)> insert = [&](Node* node, size_t idx){
        const auto &p = particles_[idx];
        // update aggregate charge and center-of-charge
        float oldq = node->qtot;
        node->qtot += p.charge;
        if (std::fabs(node->qtot) > 1e-20f) {
            node->cx = (oldq * node->cx + p.charge * p.pos.x) / node->qtot;
            node->cy = (oldq * node->cy + p.charge * p.pos.y) / node->qtot;
        } else {
            node->cx = 0.5f * (node->xmin + node->xmax);
            node->cy = 0.5f * (node->ymin + node->ymax);
        }

        if (node->isLeaf()) {
            if (node->idx == -1) {
                node->idx = (int)idx;
                return;
            }
            // subdivide
            int existing = node->idx;
            node->idx = -1;
            float mx = 0.5f * (node->xmin + node->xmax);
            float my = 0.5f * (node->ymin + node->ymax);
            node->child[0] = makeNode(node->xmin, node->ymin, mx, my); // SW
            node->child[1] = makeNode(mx, node->ymin, node->xmax, my); // SE
            node->child[2] = makeNode(node->xmin, my, mx, node->ymax); // NW
            node->child[3] = makeNode(mx, my, node->xmax, node->ymax); // NE
            // re-insert existing particle
            for (int c = 0; c < 4; ++c) {
                if (node->child[c]->contains(particles_[existing].pos)) {
                    insert(node->child[c].get(), existing);
                    break;
                }
            }
        }
        // insert new particle into appropriate child
        if (!node->isLeaf()) {
            for (int c = 0; c < 4; ++c) {
                if (node->child[c]->contains(p.pos)) {
                    insert(node->child[c].get(), idx);
                    return;
                }
            }
            // numerical edge-case
            insert(node->child[0].get(), idx);
        }
    };

    // build tree (single-threaded)
    for (size_t i = 0; i < n; ++i) insert(root.get(), i);

    // accumulate field at position due to a node (skipIdx avoids self-contribution)
    const float theta = 0.5f; // opening angle
    std::function<void(const Node*, const sf::Vector2f&, sf::Vector2f&, size_t)> accField;
    accField = [&](const Node* node, const sf::Vector2f& pos, sf::Vector2f& outE, size_t skipIdx){
        if (!node) return;
        if (node->qtot == 0.f) {
            if (node->isLeaf() && node->idx == -1) return;
        }
        float rx = pos.x - node->cx;
        float ry = pos.y - node->cy;
        float r2 = rx*rx + ry*ry + soft2;
        float d = std::sqrt(r2);
        if (node->isLeaf()) {
            if (node->idx == -1) return;
            if ((size_t)node->idx == skipIdx) return;
            float invr3 = 1.0f / (r2 * d);
            float factor = k * node->qtot * invr3;
            outE.x += factor * rx;
            outE.y += factor * ry;
            return;
        }
        float s = node->width();
        if (s / d < theta) {
            float invr3 = 1.0f / (r2 * d);
            float factor = k * node->qtot * invr3;
            outE.x += factor * rx;
            outE.y += factor * ry;
        } else {
            for (int c = 0; c < 4; ++c) if (node->child[c]) accField(node->child[c].get(), pos, outE, skipIdx);
        }
    };

    // parallelize evaluation across particles (thread per chunk)
    unsigned hw = std::thread::hardware_concurrency();
    unsigned numThreads = (hw == 0 ? 1u : hw);
    if (numThreads > n) numThreads = static_cast<unsigned>(n);

    auto worker = [&](size_t istart, size_t iend){
        for (size_t i = istart; i < iend; ++i) {
            sf::Vector2f Ei = P.externalE;
            accField(root.get(), particles_[i].pos, Ei, i);
            Eout[i] = Ei;
        }
    };

    if (numThreads <= 1) {
        worker(0, n);
    } else {
        std::vector<std::thread> threads;
        threads.reserve(numThreads);
        size_t chunk = n / numThreads;
        size_t start = 0;
        for (unsigned t = 0; t < numThreads; ++t) {
            size_t end = (t + 1 == numThreads) ? n : (start + chunk);
            threads.emplace_back(worker, start, end);
            start = end;
        }
        for (auto &th : threads) th.join();
    }
}


void Simulator::integrateBoris(float dt, const std::vector<sf::Vector2f>& Efield) {
    const float Bz = P.externalBz;
    const size_t n = particles_.size();
    for (size_t i = 0; i < n; ++i) {
        Particle &p = particles_[i];
        double q = (double)p.charge;
        double m = (double)p.mass;
        double dt2 = 0.5 * (double)dt;

        // E half-kick
        double vx = p.vel.x + (q/m) * (double)Efield[i].x * dt2;
        double vy = p.vel.y + (q/m) * (double)Efield[i].y * dt2;

        // Magnetic rotation (B only z) -- standard Boris
        double tz = (q/m) * (double)Bz * dt2;
        double vpx = vx +  vy * tz;
        double vpy = vy -  vx * tz;
        double s = 2.0 * tz / (1.0 + tz*tz);
        double vfx = vx +  vpy * s;
        double vfy = vy -  vpx * s;

        // E half-kick
        vfx += (q/m) * (double)Efield[i].x * dt2;
        vfy += (q/m) * (double)Efield[i].y * dt2;

        // commit velocity
        p.vel.x = (float)vfx;
        p.vel.y = (float)vfy;

        // --- harmonic trap: a_trap = -k * (x - xc) / m  ---
        if (P.trapK > 0.0f) {
            sf::Vector2f disp = p.pos - P.trapCenter;
            double ax = - (double)P.trapK * (double)disp.x / m;
            double ay = - (double)P.trapK * (double)disp.y / m;
            p.vel.x += (float)(ax * (double)dt);
            p.vel.y += (float)(ay * (double)dt);
        }

        // --- viscous damping (multiplicative) ---
        if (P.damping > 0.0f) {
            double factor = std::exp(- (double)P.damping * (double)dt);
            p.vel.x = (float)((double)p.vel.x * factor);
            p.vel.y = (float)((double)p.vel.y * factor);
        }

        // position update (use final velocity)
        p.pos.x += p.vel.x * dt;
        p.pos.y += p.vel.y * dt;
    }
}

void Simulator::step(float dt) {
    std::vector<sf::Vector2f> Efield(particles_.size());
    computeElectricField(Efield);
    integrateBoris(dt, Efield);
    if (boundsOn_) applyBounds();
}