#include <Geode/Geode.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>

using namespace geode::prelude;

namespace {
    struct PState {
        int sinceClick = -1;   // steps since last press (-1 = never)
        int held = 0;          // steps the button has been held
        bool down = false;
    };
    PState g_p[2];
    int g_steps = 0;           // physics steps this attempt

    void resetState() {
        g_p[0] = PState{};
        g_p[1] = PState{};
        g_steps = 0;
    }

    const char* modeName(PlayerObject* p) {
        if (!p) return "?";
        if (p->m_isShip)   return "Ship";
        if (p->m_isBird)   return "UFO";
        if (p->m_isBall)   return "Ball";
        if (p->m_isDart)   return "Wave";
        if (p->m_isRobot)  return "Robot";
        if (p->m_isSpider) return "Spider";
        if (p->m_isSwing)  return "Swing";
        return "Cube";
    }
}

class $modify(FWCLayer, GJBaseGameLayer) {
    struct Fields {
        CCLabelBMFont* label = nullptr;
    };

    void ensureLabel() {
        if (m_fields->label) return;
        if (PlayLayer::get() != reinterpret_cast<PlayLayer*>(this)) return;
        auto l = CCLabelBMFont::create("", "chatFont.fnt");
        l->setAnchorPoint({0.f, 1.f});
        l->setAlignment(kCCTextAlignmentLeft);
        auto win = CCDirector::sharedDirector()->getWinSize();
        l->setPosition({6.f, win.height - 6.f});
        l->setZOrder(9999);
        if (auto ui = PlayLayer::get()->m_uiLayer) ui->addChild(l);
        else this->addChild(l);
        m_fields->label = l;
    }

    void handleButton(bool down, int button, bool isPlayer1) {
        GJBaseGameLayer::handleButton(down, button, isPlayer1);
        if (button != 1) return;
        auto& s = g_p[isPlayer1 ? 0 : 1];
        if (down && !s.down) { s.sinceClick = 0; s.held = 0; }
        s.down = down;
    }

    void processCommands(float dt) {
        GJBaseGameLayer::processCommands(dt);
        if (PlayLayer::get() != reinterpret_cast<PlayLayer*>(this)) return;

        g_steps++;
        for (auto& s : g_p) {
            if (s.sinceClick >= 0) s.sinceClick++;
            if (s.down) s.held++;
        }

        this->ensureLabel();
        if (!m_fields->label) return;

        auto mod = Mod::get();
        bool show = mod->getSettingValue<bool>("show-counter");
        m_fields->label->setVisible(show);
        if (!show) return;

        float tps = 240.f;
        int refFps = (int)mod->getSettingValue<int64_t>("ref-fps");
        float perFrame = tps / (float)refFps;

        auto fmt = [&](int steps) {
            return fmt::format("{} st / {:.1f} f", steps, steps / perFrame);
        };
        auto line = [&](int idx, PlayerObject* p) {
            auto& s = g_p[idx];
            return fmt::format("P{} {}: click {} | held {}",
                idx + 1, modeName(p),
                s.sinceClick < 0 ? std::string("-") : fmt(s.sinceClick),
                s.down ? fmt(s.held) : std::string("-"));
        };

        std::string text = fmt::format("Steps {} ({:.1f} f @{})\n", g_steps, g_steps / perFrame, refFps);
        text += line(0, m_player1);
        if (m_gameState.m_isDualMode) text += "\n" + line(1, m_player2);

        m_fields->label->setString(text.c_str());
        m_fields->label->setScale((float)mod->getSettingValue<double>("label-scale"));
        m_fields->label->setOpacity((GLubyte)mod->getSettingValue<int64_t>("label-opacity"));
    }
};

class $modify(FWCPlayLayer, PlayLayer) {
    void resetLevel() {
        PlayLayer::resetLevel();
        resetState();
    }
};
