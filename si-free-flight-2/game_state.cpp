#include "game_state.h"
#include "island.h"
#include "rlgl.h"
#include "write_debug.h"
#include <cstdlib>
#include <cstring>

GameState::GameState()
    : island_()
    , scene_()
    , renderer_()
    , cameraController_()
    , selection_()
{
    // Load scene graph from LZEXE-compressed executable
    const char* exe_paths[] = {
        "stunt.exe",
        "STUNT.EXE",
        nullptr
    };
    
    bool loaded = false;
    const char* used_exe = nullptr;
    
    for (int i = 0; exe_paths[i] != nullptr; i++) {
        if (scene_.loadFromExecutable(exe_paths[i], "data")) {
            used_exe = exe_paths[i];
            loaded = true;
            break;
        }
    }
    
    if (loaded) {
        // Extract anchor points from scene graph loader
        const auto& anchorPts = scene_.getAnchorPoints();

        if (!anchorPts.empty()) {
            island_.loadFromAnchorPoints(anchorPts);
        } else {
            write_debug("WARNING: No anchor points extracted from executable\n");
        }
    } else {
        write_debug("ERROR: Failed to load scene graph from any executable\n");
        write_debug("Cannot continue without scene graph data.\n");
    }

    renderer_.initMaterials();
}

GameState::~GameState()
{
    // Cleanup
    renderer_.cleanup();
    scene_.cleanup();
    island_.cleanup();
}
