#ifndef SOUNDMANAGER_HPP
#define SOUNDMANAGER_HPP

#ifdef HAS_GUI

#include <SFML/Audio.hpp>
#include <string>
#include <array>
#include <memory>

class SoundManager {
public:
    enum Effect { Merge, Error, Click, Submit, Result, Count };

    SoundManager() = default;
    ~SoundManager() = default;

    bool load();
    void play(Effect e);

private:
    std::array<sf::SoundBuffer, Count> _buffers;
    std::array<std::unique_ptr<sf::Sound>, Count> _sounds;
    bool _loaded = false;
};

#endif // HAS_GUI
#endif // SOUNDMANAGER_HPP
