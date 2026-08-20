target_sources(mcpi_core PRIVATE
    src/assets/FallbackAssetSource.cpp
    src/assets/OriginalPiAssetSource.cpp
    src/client/Camera.cpp
    src/client/ChunkMesh.cpp
    src/client/HudRenderer.cpp
    src/client/LevelRenderer.cpp
    src/client/Screen.cpp
    src/client/SoundEngine.cpp
)

if(TARGET mcpi_client)
    target_sources(mcpi_client PRIVATE
        src/client/SdlAudioMixer.cpp
    )
endif()
