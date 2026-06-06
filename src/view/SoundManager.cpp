#ifdef HAS_GUI

#include "view/SoundManager.hpp"
#include <iostream>

static const char* kFileNames[] = {
    "assets/sounds/merge.wav",
    "assets/sounds/error.wav",
    "assets/sounds/click.wav",
    "assets/sounds/submit.wav",
    "assets/sounds/result.wav"
};

bool SoundManager::load()
{
    for (int i = 0; i < Count; ++i) {
        if (!_buffers[i].loadFromFile(kFileNames[i])) {
            std::cerr << "[SoundManager] Warning: could not load " << kFileNames[i] << std::endl;
            continue;
        }
        _sounds[i] = std::make_unique<sf::Sound>(_buffers[i]);
    }

    _loaded = true;
    return _loaded;
}

void SoundManager::play(Effect e)
{
    if (e < 0 || e >= Count) return;
    if (!_sounds[e]) return;
    _sounds[e]->stop();
    _sounds[e]->play();
}

#endif // HAS_GUI
